
#include "i2400m.h"
#include <linux/wimax/i2400m.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

#define D_SUBMODULE driver
#include "debug-levels.h"


int i2400m_idle_mode_disabled;	/* 0 (idle mode enabled) by default */
module_param_named(idle_mode_disabled, i2400m_idle_mode_disabled, int, 0644);
MODULE_PARM_DESC(idle_mode_disabled,
		 "If true, the device will not enable idle mode negotiation "
		 "with the base station (when connected) to save power.");

int i2400m_queue_work(struct i2400m *i2400m,
		      void (*fn)(struct work_struct *), gfp_t gfp_flags,
		      const void *pl, size_t pl_size)
{
	int result;
	struct i2400m_work *iw;

	BUG_ON(i2400m->work_queue == NULL);
	result = -ENOMEM;
	iw = kzalloc(sizeof(*iw) + pl_size, gfp_flags);
	if (iw == NULL)
		goto error_kzalloc;
	iw->i2400m = i2400m_get(i2400m);
	memcpy(iw->pl, pl, pl_size);
	INIT_WORK(&iw->ws, fn);
	result = queue_work(i2400m->work_queue, &iw->ws);
error_kzalloc:
	return result;
}
EXPORT_SYMBOL_GPL(i2400m_queue_work);


int i2400m_schedule_work(struct i2400m *i2400m,
			 void (*fn)(struct work_struct *), gfp_t gfp_flags)
{
	int result;
	struct i2400m_work *iw;

	BUG_ON(i2400m->work_queue == NULL);
	result = -ENOMEM;
	iw = kzalloc(sizeof(*iw), gfp_flags);
	if (iw == NULL)
		goto error_kzalloc;
	iw->i2400m = i2400m_get(i2400m);
	INIT_WORK(&iw->ws, fn);
	result = schedule_work(&iw->ws);
	if (result == 0)
		result = -ENXIO;
error_kzalloc:
	return result;
}


static
int i2400m_op_msg_from_user(struct wimax_dev *wimax_dev,
			    const char *pipe_name,
			    const void *msg_buf, size_t msg_len,
			    const struct genl_info *genl_info)
{
	int result;
	struct i2400m *i2400m = wimax_dev_to_i2400m(wimax_dev);
	struct device *dev = i2400m_dev(i2400m);
	struct sk_buff *ack_skb;

	d_fnstart(4, dev, "(wimax_dev %p [i2400m %p] msg_buf %p "
		  "msg_len %zu genl_info %p)\n", wimax_dev, i2400m,
		  msg_buf, msg_len, genl_info);
	ack_skb = i2400m_msg_to_dev(i2400m, msg_buf, msg_len);
	result = PTR_ERR(ack_skb);
	if (IS_ERR(ack_skb))
		goto error_msg_to_dev;
	if (unlikely(i2400m->trace_msg_from_user))
		wimax_msg(&i2400m->wimax_dev, "trace",
			  msg_buf, msg_len, GFP_KERNEL);
	result = wimax_msg_send(&i2400m->wimax_dev, ack_skb);
error_msg_to_dev:
	d_fnend(4, dev, "(wimax_dev %p [i2400m %p] msg_buf %p msg_len %zu "
		"genl_info %p) = %d\n", wimax_dev, i2400m, msg_buf, msg_len,
		genl_info, result);
	return result;
}


struct i2400m_reset_ctx {
	struct completion completion;
	int result;
};


static
int i2400m_op_reset(struct wimax_dev *wimax_dev)
{
	int result;
	struct i2400m *i2400m = wimax_dev_to_i2400m(wimax_dev);
	struct device *dev = i2400m_dev(i2400m);
	struct i2400m_reset_ctx ctx = {
		.completion = COMPLETION_INITIALIZER_ONSTACK(ctx.completion),
		.result = 0,
	};

	d_fnstart(4, dev, "(wimax_dev %p)\n", wimax_dev);
	mutex_lock(&i2400m->init_mutex);
	i2400m->reset_ctx = &ctx;
	mutex_unlock(&i2400m->init_mutex);
	result = i2400m->bus_reset(i2400m, I2400M_RT_WARM);
	if (result < 0)
		goto out;
	result = wait_for_completion_timeout(&ctx.completion, 4*HZ);
	if (result == 0)
		result = -ETIMEDOUT;
	else if (result > 0)
		result = ctx.result;
	/* if result < 0, pass it on */
	mutex_lock(&i2400m->init_mutex);
	i2400m->reset_ctx = NULL;
	mutex_unlock(&i2400m->init_mutex);
out:
	d_fnend(4, dev, "(wimax_dev %p) = %d\n", wimax_dev, result);
	return result;
}


static
int i2400m_check_mac_addr(struct i2400m *i2400m)
{
	int result;
	struct device *dev = i2400m_dev(i2400m);
	struct sk_buff *skb;
	const struct i2400m_tlv_detailed_device_info *ddi;
	struct net_device *net_dev = i2400m->wimax_dev.net_dev;
	const unsigned char zeromac[ETH_ALEN] = { 0 };

	d_fnstart(3, dev, "(i2400m %p)\n", i2400m);
	skb = i2400m_get_device_info(i2400m);
	if (IS_ERR(skb)) {
		result = PTR_ERR(skb);
		dev_err(dev, "Cannot verify MAC address, error reading: %d\n",
			result);
		goto error;
	}
	/* Extract MAC addresss */
	ddi = (void *) skb->data;
	BUILD_BUG_ON(ETH_ALEN != sizeof(ddi->mac_address));
	d_printf(2, dev, "GET DEVICE INFO: mac addr "
		 "%02x:%02x:%02x:%02x:%02x:%02x\n",
		 ddi->mac_address[0], ddi->mac_address[1],
		 ddi->mac_address[2], ddi->mac_address[3],
		 ddi->mac_address[4], ddi->mac_address[5]);
	if (!memcmp(net_dev->perm_addr, ddi->mac_address,
		   sizeof(ddi->mac_address)))
		goto ok;
	dev_warn(dev, "warning: device reports a different MAC address "
		 "to that of boot mode's\n");
	dev_warn(dev, "device reports     %02x:%02x:%02x:%02x:%02x:%02x\n",
		 ddi->mac_address[0], ddi->mac_address[1],
		 ddi->mac_address[2], ddi->mac_address[3],
		 ddi->mac_address[4], ddi->mac_address[5]);
	dev_warn(dev, "boot mode reported %02x:%02x:%02x:%02x:%02x:%02x\n",
		 net_dev->perm_addr[0], net_dev->perm_addr[1],
		 net_dev->perm_addr[2], net_dev->perm_addr[3],
		 net_dev->perm_addr[4], net_dev->perm_addr[5]);
	if (!memcmp(zeromac, ddi->mac_address, sizeof(zeromac)))
		dev_err(dev, "device reports an invalid MAC address, "
			"not updating\n");
	else {
		dev_warn(dev, "updating MAC address\n");
		net_dev->addr_len = ETH_ALEN;
		memcpy(net_dev->perm_addr, ddi->mac_address, ETH_ALEN);
		memcpy(net_dev->dev_addr, ddi->mac_address, ETH_ALEN);
	}
ok:
	result = 0;
	kfree_skb(skb);
error:
	d_fnend(3, dev, "(i2400m %p) = %d\n", i2400m, result);
	return result;
}


static
int __i2400m_dev_start(struct i2400m *i2400m, enum i2400m_bri flags)
{
	int result;
	struct wimax_dev *wimax_dev = &i2400m->wimax_dev;
	struct net_device *net_dev = wimax_dev->net_dev;
	struct device *dev = i2400m_dev(i2400m);
	int times = 3;

	d_fnstart(3, dev, "(i2400m %p)\n", i2400m);
retry:
	result = i2400m_dev_bootstrap(i2400m, flags);
	if (result < 0) {
		dev_err(dev, "cannot bootstrap device: %d\n", result);
		goto error_bootstrap;
	}
	result = i2400m_tx_setup(i2400m);
	if (result < 0)
		goto error_tx_setup;
	result = i2400m->bus_dev_start(i2400m);
	if (result < 0)
		goto error_bus_dev_start;
	i2400m->work_queue = create_singlethread_workqueue(wimax_dev->name);
	if (i2400m->work_queue == NULL) {
		result = -ENOMEM;
		dev_err(dev, "cannot create workqueue\n");
		goto error_create_workqueue;
	}
	/* At this point is ok to send commands to the device */
	result = i2400m_check_mac_addr(i2400m);
	if (result < 0)
		goto error_check_mac_addr;
	i2400m->ready = 1;
	wimax_state_change(wimax_dev, WIMAX_ST_UNINITIALIZED);
	result = i2400m_dev_initialize(i2400m);
	if (result < 0)
		goto error_dev_initialize;
	/* At this point, reports will come for the device and set it
	 * to the right state if it is different than UNINITIALIZED */
	d_fnend(3, dev, "(net_dev %p [i2400m %p]) = %d\n",
		net_dev, i2400m, result);
	return result;

error_dev_initialize:
error_check_mac_addr:
	destroy_workqueue(i2400m->work_queue);
error_create_workqueue:
	i2400m->bus_dev_stop(i2400m);
error_bus_dev_start:
	i2400m_tx_release(i2400m);
error_tx_setup:
error_bootstrap:
	if (result == -ERESTARTSYS && times-- > 0) {
		flags = I2400M_BRI_SOFT;
		goto retry;
	}
	d_fnend(3, dev, "(net_dev %p [i2400m %p]) = %d\n",
		net_dev, i2400m, result);
	return result;
}


static
int i2400m_dev_start(struct i2400m *i2400m, enum i2400m_bri bm_flags)
{
	int result;
	mutex_lock(&i2400m->init_mutex);	/* Well, start the device */
	result = __i2400m_dev_start(i2400m, bm_flags);
	if (result >= 0)
		i2400m->updown = 1;
	mutex_unlock(&i2400m->init_mutex);
	return result;
}


static
void __i2400m_dev_stop(struct i2400m *i2400m)
{
	struct wimax_dev *wimax_dev = &i2400m->wimax_dev;
	struct device *dev = i2400m_dev(i2400m);

	d_fnstart(3, dev, "(i2400m %p)\n", i2400m);
	wimax_state_change(wimax_dev, __WIMAX_ST_QUIESCING);
	i2400m_dev_shutdown(i2400m);
	i2400m->ready = 0;
	destroy_workqueue(i2400m->work_queue);
	i2400m->bus_dev_stop(i2400m);
	i2400m_tx_release(i2400m);
	wimax_state_change(wimax_dev, WIMAX_ST_DOWN);
	d_fnend(3, dev, "(i2400m %p) = 0\n", i2400m);
}


static
void i2400m_dev_stop(struct i2400m *i2400m)
{
	mutex_lock(&i2400m->init_mutex);
	if (i2400m->updown) {
		__i2400m_dev_stop(i2400m);
		i2400m->updown = 0;
	}
	mutex_unlock(&i2400m->init_mutex);
}


static
void __i2400m_dev_reset_handle(struct work_struct *ws)
{
	int result;
	struct i2400m_work *iw = container_of(ws, struct i2400m_work, ws);
	struct i2400m *i2400m = iw->i2400m;
	struct device *dev = i2400m_dev(i2400m);
	enum wimax_st wimax_state;
	struct i2400m_reset_ctx *ctx = i2400m->reset_ctx;

	d_fnstart(3, dev, "(ws %p i2400m %p)\n", ws, i2400m);
	result = 0;
	if (mutex_trylock(&i2400m->init_mutex) == 0) {
		/* We are still in i2400m_dev_start() [let it fail] or
		 * i2400m_dev_stop() [we are shutting down anyway, so
		 * ignore it] or we are resetting somewhere else. */
		dev_err(dev, "device rebooted\n");
		i2400m_msg_to_dev_cancel_wait(i2400m, -ERESTARTSYS);
		complete(&i2400m->msg_completion);
		goto out;
	}
	wimax_state = wimax_state_get(&i2400m->wimax_dev);
	if (wimax_state < WIMAX_ST_UNINITIALIZED) {
		dev_info(dev, "device rebooted: it is down, ignoring\n");
		goto out_unlock;	/* ifconfig up/down wasn't called */
	}
	dev_err(dev, "device rebooted: reinitializing driver\n");
	__i2400m_dev_stop(i2400m);
	i2400m->updown = 0;
	result = __i2400m_dev_start(i2400m,
				    I2400M_BRI_SOFT | I2400M_BRI_MAC_REINIT);
	if (result < 0) {
		dev_err(dev, "device reboot: cannot start the device: %d\n",
			result);
		result = i2400m->bus_reset(i2400m, I2400M_RT_BUS);
		if (result >= 0)
			result = -ENODEV;
	} else
		i2400m->updown = 1;
out_unlock:
	if (i2400m->reset_ctx) {
		ctx->result = result;
		complete(&ctx->completion);
	}
	mutex_unlock(&i2400m->init_mutex);
out:
	i2400m_put(i2400m);
	kfree(iw);
	d_fnend(3, dev, "(ws %p i2400m %p) = void\n", ws, i2400m);
	return;
}


int i2400m_dev_reset_handle(struct i2400m *i2400m)
{
	return i2400m_schedule_work(i2400m, __i2400m_dev_reset_handle,
				    GFP_ATOMIC);
}
EXPORT_SYMBOL_GPL(i2400m_dev_reset_handle);


int i2400m_setup(struct i2400m *i2400m, enum i2400m_bri bm_flags)
{
	int result = -ENODEV;
	struct device *dev = i2400m_dev(i2400m);
	struct wimax_dev *wimax_dev = &i2400m->wimax_dev;
	struct net_device *net_dev = i2400m->wimax_dev.net_dev;

	d_fnstart(3, dev, "(i2400m %p)\n", i2400m);

	snprintf(wimax_dev->name, sizeof(wimax_dev->name),
		 "i2400m-%s:%s", dev->bus->name, dev->bus_id);

	i2400m->bm_cmd_buf = kzalloc(I2400M_BM_CMD_BUF_SIZE, GFP_KERNEL);
	if (i2400m->bm_cmd_buf == NULL) {
		dev_err(dev, "cannot allocate USB command buffer\n");
		goto error_bm_cmd_kzalloc;
	}
	i2400m->bm_ack_buf = kzalloc(I2400M_BM_ACK_BUF_SIZE, GFP_KERNEL);
	if (i2400m->bm_ack_buf == NULL) {
		dev_err(dev, "cannot allocate USB ack buffer\n");
		goto error_bm_ack_buf_kzalloc;
	}
	result = i2400m_bootrom_init(i2400m, bm_flags);
	if (result < 0) {
		dev_err(dev, "read mac addr: bootrom init "
			"failed: %d\n", result);
		goto error_bootrom_init;
	}
	result = i2400m_read_mac_addr(i2400m);
	if (result < 0)
		goto error_read_mac_addr;

	result = register_netdev(net_dev);	/* Okey dokey, bring it up */
	if (result < 0) {
		dev_err(dev, "cannot register i2400m network device: %d\n",
			result);
		goto error_register_netdev;
	}
	netif_carrier_off(net_dev);

	result = i2400m_dev_start(i2400m, bm_flags);
	if (result < 0)
		goto error_dev_start;

	i2400m->wimax_dev.op_msg_from_user = i2400m_op_msg_from_user;
	i2400m->wimax_dev.op_rfkill_sw_toggle = i2400m_op_rfkill_sw_toggle;
	i2400m->wimax_dev.op_reset = i2400m_op_reset;
	result = wimax_dev_add(&i2400m->wimax_dev, net_dev);
	if (result < 0)
		goto error_wimax_dev_add;
	/* User space needs to do some init stuff */
	wimax_state_change(wimax_dev, WIMAX_ST_UNINITIALIZED);

	/* Now setup all that requires a registered net and wimax device. */
	result = i2400m_debugfs_add(i2400m);
	if (result < 0) {
		dev_err(dev, "cannot setup i2400m's debugfs: %d\n", result);
		goto error_debugfs_setup;
	}
	d_fnend(3, dev, "(i2400m %p) = %d\n", i2400m, result);
	return result;

error_debugfs_setup:
	wimax_dev_rm(&i2400m->wimax_dev);
error_wimax_dev_add:
	i2400m_dev_stop(i2400m);
error_dev_start:
	unregister_netdev(net_dev);
error_register_netdev:
error_read_mac_addr:
error_bootrom_init:
	kfree(i2400m->bm_ack_buf);
error_bm_ack_buf_kzalloc:
	kfree(i2400m->bm_cmd_buf);
error_bm_cmd_kzalloc:
	d_fnend(3, dev, "(i2400m %p) = %d\n", i2400m, result);
	return result;
}
EXPORT_SYMBOL_GPL(i2400m_setup);


void i2400m_release(struct i2400m *i2400m)
{
	struct device *dev = i2400m_dev(i2400m);

	d_fnstart(3, dev, "(i2400m %p)\n", i2400m);
	netif_stop_queue(i2400m->wimax_dev.net_dev);

	i2400m_debugfs_rm(i2400m);
	wimax_dev_rm(&i2400m->wimax_dev);
	i2400m_dev_stop(i2400m);
	unregister_netdev(i2400m->wimax_dev.net_dev);
	kfree(i2400m->bm_ack_buf);
	kfree(i2400m->bm_cmd_buf);
	d_fnend(3, dev, "(i2400m %p) = void\n", i2400m);
}
EXPORT_SYMBOL_GPL(i2400m_release);


struct d_level D_LEVEL[] = {
	D_SUBMODULE_DEFINE(control),
	D_SUBMODULE_DEFINE(driver),
	D_SUBMODULE_DEFINE(debugfs),
	D_SUBMODULE_DEFINE(fw),
	D_SUBMODULE_DEFINE(netdev),
	D_SUBMODULE_DEFINE(rfkill),
	D_SUBMODULE_DEFINE(rx),
	D_SUBMODULE_DEFINE(tx),
};
size_t D_LEVEL_SIZE = ARRAY_SIZE(D_LEVEL);


static
int __init i2400m_driver_init(void)
{
	return 0;
}
module_init(i2400m_driver_init);

static
void __exit i2400m_driver_exit(void)
{
	/* for scheds i2400m_dev_reset_handle() */
	flush_scheduled_work();
	return;
}
module_exit(i2400m_driver_exit);

MODULE_AUTHOR("Intel Corporation <linux-wimax@intel.com>");
MODULE_DESCRIPTION("Intel 2400M WiMAX networking bus-generic driver");
MODULE_LICENSE("GPL");
