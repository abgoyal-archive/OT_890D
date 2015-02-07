
#ifndef FC_TRANSPORT_FCOE_H
#define FC_TRANSPORT_FCOE_H

#include <linux/device.h>
#include <linux/netdevice.h>
#include <scsi/scsi_host.h>
#include <scsi/libfc.h>

struct fcoe_transport {
	char *name;
	unsigned short vendor;
	unsigned short device;
	struct bus_type *bus;
	struct device_driver *driver;
	int (*create)(struct net_device *device);
	int (*destroy)(struct net_device *device);
	bool (*match)(struct net_device *device);
	struct list_head list;
	struct list_head devlist;
	struct mutex devlock;
};

#define MODULE_ALIAS_FCOE_PCI(vendor, device) \
	MODULE_ALIAS("fcoe-pci-" __stringify(vendor) "-" __stringify(device))

/* exported funcs */
int fcoe_transport_attach(struct net_device *netdev);
int fcoe_transport_release(struct net_device *netdev);
int fcoe_transport_register(struct fcoe_transport *t);
int fcoe_transport_unregister(struct fcoe_transport *t);
int fcoe_load_transport_driver(struct net_device *netdev);
int __init fcoe_transport_init(void);
int __exit fcoe_transport_exit(void);

/* fcow_sw is the default transport */
extern struct fcoe_transport fcoe_sw_transport;
#endif /* FC_TRANSPORT_FCOE_H */
