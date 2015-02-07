

#include <linux/types.h>
#include <linux/kernel.h>

#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/init.h>
#include <linux/rio.h>
#include <linux/rio_drv.h>
#include <linux/rio_ids.h>
#include <linux/rio_regs.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/slab.h>

#include "rio.h"

LIST_HEAD(rio_devices);
static LIST_HEAD(rio_switches);

#define RIO_ENUM_CMPL_MAGIC	0xdeadbeef

static void rio_enum_timeout(unsigned long);

DEFINE_SPINLOCK(rio_global_list_lock);

static int next_destid = 0;
static int next_switchid = 0;
static int next_net = 0;

static struct timer_list rio_enum_timer =
TIMER_INITIALIZER(rio_enum_timeout, 0, 0);

static int rio_mport_phys_table[] = {
	RIO_EFB_PAR_EP_ID,
	RIO_EFB_PAR_EP_REC_ID,
	RIO_EFB_SER_EP_ID,
	RIO_EFB_SER_EP_REC_ID,
	-1,
};

static int rio_sport_phys_table[] = {
	RIO_EFB_PAR_EP_FREE_ID,
	RIO_EFB_SER_EP_FREE_ID,
	-1,
};

static u16 rio_get_device_id(struct rio_mport *port, u16 destid, u8 hopcount)
{
	u32 result;

	rio_mport_read_config_32(port, destid, hopcount, RIO_DID_CSR, &result);

	return RIO_GET_DID(port->sys_size, result);
}

static void rio_set_device_id(struct rio_mport *port, u16 destid, u8 hopcount, u16 did)
{
	rio_mport_write_config_32(port, destid, hopcount, RIO_DID_CSR,
				  RIO_SET_DID(port->sys_size, did));
}

static void rio_local_set_device_id(struct rio_mport *port, u16 did)
{
	rio_local_write_config_32(port, RIO_DID_CSR, RIO_SET_DID(port->sys_size,
				did));
}

static int rio_clear_locks(struct rio_mport *port)
{
	struct rio_dev *rdev;
	u32 result;
	int ret = 0;

	/* Write component tag CSR magic complete value */
	rio_local_write_config_32(port, RIO_COMPONENT_TAG_CSR,
				  RIO_ENUM_CMPL_MAGIC);
	list_for_each_entry(rdev, &rio_devices, global_list)
	    rio_write_config_32(rdev, RIO_COMPONENT_TAG_CSR,
				RIO_ENUM_CMPL_MAGIC);

	/* Release host device id locks */
	rio_local_write_config_32(port, RIO_HOST_DID_LOCK_CSR,
				  port->host_deviceid);
	rio_local_read_config_32(port, RIO_HOST_DID_LOCK_CSR, &result);
	if ((result & 0xffff) != 0xffff) {
		printk(KERN_INFO
		       "RIO: badness when releasing host lock on master port, result %8.8x\n",
		       result);
		ret = -EINVAL;
	}
	list_for_each_entry(rdev, &rio_devices, global_list) {
		rio_write_config_32(rdev, RIO_HOST_DID_LOCK_CSR,
				    port->host_deviceid);
		rio_read_config_32(rdev, RIO_HOST_DID_LOCK_CSR, &result);
		if ((result & 0xffff) != 0xffff) {
			printk(KERN_INFO
			       "RIO: badness when releasing host lock on vid %4.4x did %4.4x\n",
			       rdev->vid, rdev->did);
			ret = -EINVAL;
		}
	}

	return ret;
}

static int rio_enum_host(struct rio_mport *port)
{
	u32 result;

	/* Set master port host device id lock */
	rio_local_write_config_32(port, RIO_HOST_DID_LOCK_CSR,
				  port->host_deviceid);

	rio_local_read_config_32(port, RIO_HOST_DID_LOCK_CSR, &result);
	if ((result & 0xffff) != port->host_deviceid)
		return -1;

	/* Set master port destid and init destid ctr */
	rio_local_set_device_id(port, port->host_deviceid);

	if (next_destid == port->host_deviceid)
		next_destid++;

	return 0;
}

static int rio_device_has_destid(struct rio_mport *port, int src_ops,
				 int dst_ops)
{
	u32 mask = RIO_OPS_READ | RIO_OPS_WRITE | RIO_OPS_ATOMIC_TST_SWP | RIO_OPS_ATOMIC_INC | RIO_OPS_ATOMIC_DEC | RIO_OPS_ATOMIC_SET | RIO_OPS_ATOMIC_CLR;

	return !!((src_ops | dst_ops) & mask);
}

static void rio_release_dev(struct device *dev)
{
	struct rio_dev *rdev;

	rdev = to_rio_dev(dev);
	kfree(rdev);
}

static int rio_is_switch(struct rio_dev *rdev)
{
	if (rdev->pef & RIO_PEF_SWITCH)
		return 1;
	return 0;
}

static void rio_route_set_ops(struct rio_dev *rdev)
{
	struct rio_route_ops *cur = __start_rio_route_ops;
	struct rio_route_ops *end = __end_rio_route_ops;

	while (cur < end) {
		if ((cur->vid == rdev->vid) && (cur->did == rdev->did)) {
			pr_debug("RIO: adding routing ops for %s\n", rio_name(rdev));
			rdev->rswitch->add_entry = cur->add_hook;
			rdev->rswitch->get_entry = cur->get_hook;
		}
		cur++;
	}

	if (!rdev->rswitch->add_entry || !rdev->rswitch->get_entry)
		printk(KERN_ERR "RIO: missing routing ops for %s\n",
		       rio_name(rdev));
}

static void __devinit rio_add_device(struct rio_dev *rdev)
{
	device_add(&rdev->dev);

	spin_lock(&rio_global_list_lock);
	list_add_tail(&rdev->global_list, &rio_devices);
	spin_unlock(&rio_global_list_lock);

	rio_create_sysfs_dev_files(rdev);
}

static struct rio_dev *rio_setup_device(struct rio_net *net,
					struct rio_mport *port, u16 destid,
					u8 hopcount, int do_enum)
{
	struct rio_dev *rdev;
	struct rio_switch *rswitch;
	int result, rdid;

	rdev = kzalloc(sizeof(struct rio_dev), GFP_KERNEL);
	if (!rdev)
		goto out;

	rdev->net = net;
	rio_mport_read_config_32(port, destid, hopcount, RIO_DEV_ID_CAR,
				 &result);
	rdev->did = result >> 16;
	rdev->vid = result & 0xffff;
	rio_mport_read_config_32(port, destid, hopcount, RIO_DEV_INFO_CAR,
				 &rdev->device_rev);
	rio_mport_read_config_32(port, destid, hopcount, RIO_ASM_ID_CAR,
				 &result);
	rdev->asm_did = result >> 16;
	rdev->asm_vid = result & 0xffff;
	rio_mport_read_config_32(port, destid, hopcount, RIO_ASM_INFO_CAR,
				 &result);
	rdev->asm_rev = result >> 16;
	rio_mport_read_config_32(port, destid, hopcount, RIO_PEF_CAR,
				 &rdev->pef);
	if (rdev->pef & RIO_PEF_EXT_FEATURES)
		rdev->efptr = result & 0xffff;

	rio_mport_read_config_32(port, destid, hopcount, RIO_SRC_OPS_CAR,
				 &rdev->src_ops);
	rio_mport_read_config_32(port, destid, hopcount, RIO_DST_OPS_CAR,
				 &rdev->dst_ops);

	if (rio_device_has_destid(port, rdev->src_ops, rdev->dst_ops)) {
		if (do_enum) {
			rio_set_device_id(port, destid, hopcount, next_destid);
			rdev->destid = next_destid++;
			if (next_destid == port->host_deviceid)
				next_destid++;
		} else
			rdev->destid = rio_get_device_id(port, destid, hopcount);
	} else
		/* Switch device has an associated destID */
		rdev->destid = RIO_INVALID_DESTID;

	/* If a PE has both switch and other functions, show it as a switch */
	if (rio_is_switch(rdev)) {
		rio_mport_read_config_32(port, destid, hopcount,
					 RIO_SWP_INFO_CAR, &rdev->swpinfo);
		rswitch = kmalloc(sizeof(struct rio_switch), GFP_KERNEL);
		if (!rswitch) {
			kfree(rdev);
			rdev = NULL;
			goto out;
		}
		rswitch->switchid = next_switchid;
		rswitch->hopcount = hopcount;
		rswitch->destid = destid;
		rswitch->route_table = kzalloc(sizeof(u8)*
					RIO_MAX_ROUTE_ENTRIES(port->sys_size),
					GFP_KERNEL);
		if (!rswitch->route_table) {
			kfree(rdev);
			rdev = NULL;
			kfree(rswitch);
			goto out;
		}
		/* Initialize switch route table */
		for (rdid = 0; rdid < RIO_MAX_ROUTE_ENTRIES(port->sys_size);
				rdid++)
			rswitch->route_table[rdid] = RIO_INVALID_ROUTE;
		rdev->rswitch = rswitch;
		dev_set_name(&rdev->dev, "%02x:s:%04x", rdev->net->id,
			     rdev->rswitch->switchid);
		rio_route_set_ops(rdev);

		list_add_tail(&rswitch->node, &rio_switches);

	} else
		dev_set_name(&rdev->dev, "%02x:e:%04x", rdev->net->id,
			     rdev->destid);

	rdev->dev.bus = &rio_bus_type;

	device_initialize(&rdev->dev);
	rdev->dev.release = rio_release_dev;
	rio_dev_get(rdev);

	rdev->dma_mask = DMA_32BIT_MASK;
	rdev->dev.dma_mask = &rdev->dma_mask;
	rdev->dev.coherent_dma_mask = DMA_32BIT_MASK;

	if ((rdev->pef & RIO_PEF_INB_DOORBELL) &&
	    (rdev->dst_ops & RIO_DST_OPS_DOORBELL))
		rio_init_dbell_res(&rdev->riores[RIO_DOORBELL_RESOURCE],
				   0, 0xffff);

	rio_add_device(rdev);

      out:
	return rdev;
}

static int
rio_sport_is_active(struct rio_mport *port, u16 destid, u8 hopcount, int sport)
{
	u32 result;
	u32 ext_ftr_ptr;

	int *entry = rio_sport_phys_table;

	do {
		if ((ext_ftr_ptr =
		     rio_mport_get_feature(port, 0, destid, hopcount, *entry)))

			break;
	} while (*++entry >= 0);

	if (ext_ftr_ptr)
		rio_mport_read_config_32(port, destid, hopcount,
					 ext_ftr_ptr +
					 RIO_PORT_N_ERR_STS_CSR(sport),
					 &result);

	return (result & PORT_N_ERR_STS_PORT_OK);
}

static int rio_route_add_entry(struct rio_mport *mport, struct rio_switch *rswitch,
			       u16 table, u16 route_destid, u8 route_port)
{
	return rswitch->add_entry(mport, rswitch->destid,
					rswitch->hopcount, table,
					route_destid, route_port);
}

static int
rio_route_get_entry(struct rio_mport *mport, struct rio_switch *rswitch, u16 table,
		    u16 route_destid, u8 * route_port)
{
	return rswitch->get_entry(mport, rswitch->destid,
					rswitch->hopcount, table,
					route_destid, route_port);
}

static u16 rio_get_host_deviceid_lock(struct rio_mport *port, u8 hopcount)
{
	u32 result;

	rio_mport_read_config_32(port, RIO_ANY_DESTID(port->sys_size), hopcount,
				 RIO_HOST_DID_LOCK_CSR, &result);

	return (u16) (result & 0xffff);
}

static u8
rio_get_swpinfo_inport(struct rio_mport *mport, u16 destid, u8 hopcount)
{
	u32 result;

	rio_mport_read_config_32(mport, destid, hopcount, RIO_SWP_INFO_CAR,
				 &result);

	return (u8) (result & 0xff);
}

static u8 rio_get_swpinfo_tports(struct rio_mport *mport, u16 destid,
				 u8 hopcount)
{
	u32 result;

	rio_mport_read_config_32(mport, destid, hopcount, RIO_SWP_INFO_CAR,
				 &result);

	return RIO_GET_TOTAL_PORTS(result);
}

static void rio_net_add_mport(struct rio_net *net, struct rio_mport *port)
{
	spin_lock(&rio_global_list_lock);
	list_add_tail(&port->nnode, &net->mports);
	spin_unlock(&rio_global_list_lock);
}

static int rio_enum_peer(struct rio_net *net, struct rio_mport *port,
			 u8 hopcount)
{
	int port_num;
	int num_ports;
	int cur_destid;
	int sw_destid;
	int sw_inport;
	struct rio_dev *rdev;
	u16 destid;
	int tmp;

	if (rio_get_host_deviceid_lock(port, hopcount) == port->host_deviceid) {
		pr_debug("RIO: PE already discovered by this host\n");
		/*
		 * Already discovered by this host. Add it as another
		 * master port for the current network.
		 */
		rio_net_add_mport(net, port);
		return 0;
	}

	/* Attempt to acquire device lock */
	rio_mport_write_config_32(port, RIO_ANY_DESTID(port->sys_size),
				  hopcount,
				  RIO_HOST_DID_LOCK_CSR, port->host_deviceid);
	while ((tmp = rio_get_host_deviceid_lock(port, hopcount))
	       < port->host_deviceid) {
		/* Delay a bit */
		mdelay(1);
		/* Attempt to acquire device lock again */
		rio_mport_write_config_32(port, RIO_ANY_DESTID(port->sys_size),
					  hopcount,
					  RIO_HOST_DID_LOCK_CSR,
					  port->host_deviceid);
	}

	if (rio_get_host_deviceid_lock(port, hopcount) > port->host_deviceid) {
		pr_debug(
		    "RIO: PE locked by a higher priority host...retreating\n");
		return -1;
	}

	/* Setup new RIO device */
	rdev = rio_setup_device(net, port, RIO_ANY_DESTID(port->sys_size),
					hopcount, 1);
	if (rdev) {
		/* Add device to the global and bus/net specific list. */
		list_add_tail(&rdev->net_list, &net->devices);
	} else
		return -1;

	if (rio_is_switch(rdev)) {
		next_switchid++;
		sw_inport = rio_get_swpinfo_inport(port,
				RIO_ANY_DESTID(port->sys_size), hopcount);
		rio_route_add_entry(port, rdev->rswitch, RIO_GLOBAL_TABLE,
				    port->host_deviceid, sw_inport);
		rdev->rswitch->route_table[port->host_deviceid] = sw_inport;

		for (destid = 0; destid < next_destid; destid++) {
			if (destid == port->host_deviceid)
				continue;
			rio_route_add_entry(port, rdev->rswitch, RIO_GLOBAL_TABLE,
					    destid, sw_inport);
			rdev->rswitch->route_table[destid] = sw_inport;
		}

		num_ports =
		    rio_get_swpinfo_tports(port, RIO_ANY_DESTID(port->sys_size),
						hopcount);
		pr_debug(
		    "RIO: found %s (vid %4.4x did %4.4x) with %d ports\n",
		    rio_name(rdev), rdev->vid, rdev->did, num_ports);
		sw_destid = next_destid;
		for (port_num = 0; port_num < num_ports; port_num++) {
			if (sw_inport == port_num)
				continue;

			cur_destid = next_destid;

			if (rio_sport_is_active
			    (port, RIO_ANY_DESTID(port->sys_size), hopcount,
			     port_num)) {
				pr_debug(
				    "RIO: scanning device on port %d\n",
				    port_num);
				rio_route_add_entry(port, rdev->rswitch,
						RIO_GLOBAL_TABLE,
						RIO_ANY_DESTID(port->sys_size),
						port_num);

				if (rio_enum_peer(net, port, hopcount + 1) < 0)
					return -1;

				/* Update routing tables */
				if (next_destid > cur_destid) {
					for (destid = cur_destid;
					     destid < next_destid; destid++) {
						if (destid == port->host_deviceid)
							continue;
						rio_route_add_entry(port, rdev->rswitch,
								    RIO_GLOBAL_TABLE,
								    destid,
								    port_num);
						rdev->rswitch->
						    route_table[destid] =
						    port_num;
					}
				}
			}
		}

		/* Check for empty switch */
		if (next_destid == sw_destid) {
			next_destid++;
			if (next_destid == port->host_deviceid)
				next_destid++;
		}

		rdev->rswitch->destid = sw_destid;
	} else
		pr_debug("RIO: found %s (vid %4.4x did %4.4x)\n",
		    rio_name(rdev), rdev->vid, rdev->did);

	return 0;
}

static int rio_enum_complete(struct rio_mport *port)
{
	u32 tag_csr;
	int ret = 0;

	rio_local_read_config_32(port, RIO_COMPONENT_TAG_CSR, &tag_csr);

	if (tag_csr == RIO_ENUM_CMPL_MAGIC)
		ret = 1;

	return ret;
}

static int
rio_disc_peer(struct rio_net *net, struct rio_mport *port, u16 destid,
	      u8 hopcount)
{
	u8 port_num, route_port;
	int num_ports;
	struct rio_dev *rdev;
	u16 ndestid;

	/* Setup new RIO device */
	if ((rdev = rio_setup_device(net, port, destid, hopcount, 0))) {
		/* Add device to the global and bus/net specific list. */
		list_add_tail(&rdev->net_list, &net->devices);
	} else
		return -1;

	if (rio_is_switch(rdev)) {
		next_switchid++;

		/* Associated destid is how we accessed this switch */
		rdev->rswitch->destid = destid;

		num_ports = rio_get_swpinfo_tports(port, destid, hopcount);
		pr_debug(
		    "RIO: found %s (vid %4.4x did %4.4x) with %d ports\n",
		    rio_name(rdev), rdev->vid, rdev->did, num_ports);
		for (port_num = 0; port_num < num_ports; port_num++) {
			if (rio_get_swpinfo_inport(port, destid, hopcount) ==
			    port_num)
				continue;

			if (rio_sport_is_active
			    (port, destid, hopcount, port_num)) {
				pr_debug(
				    "RIO: scanning device on port %d\n",
				    port_num);
				for (ndestid = 0;
				     ndestid < RIO_ANY_DESTID(port->sys_size);
				     ndestid++) {
					rio_route_get_entry(port, rdev->rswitch,
							    RIO_GLOBAL_TABLE,
							    ndestid,
							    &route_port);
					if (route_port == port_num)
						break;
				}

				if (rio_disc_peer
				    (net, port, ndestid, hopcount + 1) < 0)
					return -1;
			}
		}
	} else
		pr_debug("RIO: found %s (vid %4.4x did %4.4x)\n",
		    rio_name(rdev), rdev->vid, rdev->did);

	return 0;
}

static int rio_mport_is_active(struct rio_mport *port)
{
	u32 result = 0;
	u32 ext_ftr_ptr;
	int *entry = rio_mport_phys_table;

	do {
		if ((ext_ftr_ptr =
		     rio_mport_get_feature(port, 1, 0, 0, *entry)))
			break;
	} while (*++entry >= 0);

	if (ext_ftr_ptr)
		rio_local_read_config_32(port,
					 ext_ftr_ptr +
					 RIO_PORT_N_ERR_STS_CSR(port->index),
					 &result);

	return (result & PORT_N_ERR_STS_PORT_OK);
}

static struct rio_net __devinit *rio_alloc_net(struct rio_mport *port)
{
	struct rio_net *net;

	net = kzalloc(sizeof(struct rio_net), GFP_KERNEL);
	if (net) {
		INIT_LIST_HEAD(&net->node);
		INIT_LIST_HEAD(&net->devices);
		INIT_LIST_HEAD(&net->mports);
		list_add_tail(&port->nnode, &net->mports);
		net->hport = port;
		net->id = next_net++;
	}
	return net;
}

static void rio_update_route_tables(struct rio_mport *port)
{
	struct rio_dev *rdev;
	struct rio_switch *rswitch;
	u8 sport;
	u16 destid;

	list_for_each_entry(rdev, &rio_devices, global_list) {

		destid = (rio_is_switch(rdev))?rdev->rswitch->destid:rdev->destid;

		list_for_each_entry(rswitch, &rio_switches, node) {

			if (rio_is_switch(rdev)	&& (rdev->rswitch == rswitch))
				continue;

			if (RIO_INVALID_ROUTE == rswitch->route_table[destid]) {

				sport = rio_get_swpinfo_inport(port,
						rswitch->destid, rswitch->hopcount);

				if (rswitch->add_entry)	{
					rio_route_add_entry(port, rswitch, RIO_GLOBAL_TABLE, destid, sport);
					rswitch->route_table[destid] = sport;
				}
			}
		}
	}
}

int __devinit rio_enum_mport(struct rio_mport *mport)
{
	struct rio_net *net = NULL;
	int rc = 0;

	printk(KERN_INFO "RIO: enumerate master port %d, %s\n", mport->id,
	       mport->name);
	/* If somebody else enumerated our master port device, bail. */
	if (rio_enum_host(mport) < 0) {
		printk(KERN_INFO
		       "RIO: master port %d device has been enumerated by a remote host\n",
		       mport->id);
		rc = -EBUSY;
		goto out;
	}

	/* If master port has an active link, allocate net and enum peers */
	if (rio_mport_is_active(mport)) {
		if (!(net = rio_alloc_net(mport))) {
			printk(KERN_ERR "RIO: failed to allocate new net\n");
			rc = -ENOMEM;
			goto out;
		}
		if (rio_enum_peer(net, mport, 0) < 0) {
			/* A higher priority host won enumeration, bail. */
			printk(KERN_INFO
			       "RIO: master port %d device has lost enumeration to a remote host\n",
			       mport->id);
			rio_clear_locks(mport);
			rc = -EBUSY;
			goto out;
		}
		rio_update_route_tables(mport);
		rio_clear_locks(mport);
	} else {
		printk(KERN_INFO "RIO: master port %d link inactive\n",
		       mport->id);
		rc = -EINVAL;
	}

      out:
	return rc;
}

static void rio_build_route_tables(void)
{
	struct rio_dev *rdev;
	int i;
	u8 sport;

	list_for_each_entry(rdev, &rio_devices, global_list)
	    if (rio_is_switch(rdev))
		for (i = 0;
		     i < RIO_MAX_ROUTE_ENTRIES(rdev->net->hport->sys_size);
		     i++) {
			if (rio_route_get_entry
			    (rdev->net->hport, rdev->rswitch, RIO_GLOBAL_TABLE,
			     i, &sport) < 0)
				continue;
			rdev->rswitch->route_table[i] = sport;
		}
}

static void rio_enum_timeout(unsigned long data)
{
	/* Enumeration timed out, set flag */
	*(int *)data = 1;
}

int __devinit rio_disc_mport(struct rio_mport *mport)
{
	struct rio_net *net = NULL;
	int enum_timeout_flag = 0;

	printk(KERN_INFO "RIO: discover master port %d, %s\n", mport->id,
	       mport->name);

	/* If master port has an active link, allocate net and discover peers */
	if (rio_mport_is_active(mport)) {
		if (!(net = rio_alloc_net(mport))) {
			printk(KERN_ERR "RIO: Failed to allocate new net\n");
			goto bail;
		}

		pr_debug("RIO: wait for enumeration complete...");

		rio_enum_timer.expires =
		    jiffies + CONFIG_RAPIDIO_DISC_TIMEOUT * HZ;
		rio_enum_timer.data = (unsigned long)&enum_timeout_flag;
		add_timer(&rio_enum_timer);
		while (!rio_enum_complete(mport)) {
			mdelay(1);
			if (enum_timeout_flag) {
				del_timer_sync(&rio_enum_timer);
				goto timeout;
			}
		}
		del_timer_sync(&rio_enum_timer);

		pr_debug("done\n");
		if (rio_disc_peer(net, mport, RIO_ANY_DESTID(mport->sys_size),
					0) < 0) {
			printk(KERN_INFO
			       "RIO: master port %d device has failed discovery\n",
			       mport->id);
			goto bail;
		}

		rio_build_route_tables();
	}

	return 0;

      timeout:
	pr_debug("timeout\n");
      bail:
	return -EBUSY;
}
