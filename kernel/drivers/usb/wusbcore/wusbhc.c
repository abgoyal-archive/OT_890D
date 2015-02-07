
#include <linux/device.h>
#include <linux/module.h>
#include "wusbhc.h"

static struct wusbhc *usbhc_dev_to_wusbhc(struct device *dev)
{
	struct usb_bus *usb_bus = dev_get_drvdata(dev);
	struct usb_hcd *usb_hcd = bus_to_hcd(usb_bus);
	return usb_hcd_to_wusbhc(usb_hcd);
}

static ssize_t wusb_trust_timeout_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct wusbhc *wusbhc = usbhc_dev_to_wusbhc(dev);

	return scnprintf(buf, PAGE_SIZE, "%u\n", wusbhc->trust_timeout);
}

static ssize_t wusb_trust_timeout_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	struct wusbhc *wusbhc = usbhc_dev_to_wusbhc(dev);
	ssize_t result = -ENOSYS;
	unsigned trust_timeout;

	result = sscanf(buf, "%u", &trust_timeout);
	if (result != 1) {
		result = -EINVAL;
		goto out;
	}
	/* FIXME: maybe we should check for range validity? */
	wusbhc->trust_timeout = trust_timeout;
	cancel_delayed_work(&wusbhc->keep_alive_timer);
	flush_workqueue(wusbd);
	queue_delayed_work(wusbd, &wusbhc->keep_alive_timer,
			   (trust_timeout * CONFIG_HZ)/1000/2);
out:
	return result < 0 ? result : size;
}
static DEVICE_ATTR(wusb_trust_timeout, 0644, wusb_trust_timeout_show,
					     wusb_trust_timeout_store);

static ssize_t wusb_chid_show(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	struct wusbhc *wusbhc = usbhc_dev_to_wusbhc(dev);
	ssize_t result = 0;

	if (wusbhc->wuie_host_info != NULL)
		result += ckhdid_printf(buf, PAGE_SIZE,
					&wusbhc->wuie_host_info->CHID);
	return result;
}

static ssize_t wusb_chid_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t size)
{
	struct wusbhc *wusbhc = usbhc_dev_to_wusbhc(dev);
	struct wusb_ckhdid chid;
	ssize_t result;

	result = sscanf(buf,
			"%02hhx %02hhx %02hhx %02hhx "
			"%02hhx %02hhx %02hhx %02hhx "
			"%02hhx %02hhx %02hhx %02hhx "
			"%02hhx %02hhx %02hhx %02hhx\n",
			&chid.data[0] , &chid.data[1] ,
			&chid.data[2] , &chid.data[3] ,
			&chid.data[4] , &chid.data[5] ,
			&chid.data[6] , &chid.data[7] ,
			&chid.data[8] , &chid.data[9] ,
			&chid.data[10], &chid.data[11],
			&chid.data[12], &chid.data[13],
			&chid.data[14], &chid.data[15]);
	if (result != 16) {
		dev_err(dev, "Unrecognized CHID (need 16 8-bit hex digits): "
			"%d\n", (int)result);
		return -EINVAL;
	}
	result = wusbhc_chid_set(wusbhc, &chid);
	return result < 0 ? result : size;
}
static DEVICE_ATTR(wusb_chid, 0644, wusb_chid_show, wusb_chid_store);

/* Group all the WUSBHC attributes */
static struct attribute *wusbhc_attrs[] = {
		&dev_attr_wusb_trust_timeout.attr,
		&dev_attr_wusb_chid.attr,
		NULL,
};

static struct attribute_group wusbhc_attr_group = {
	.name = NULL,	/* we want them in the same directory */
	.attrs = wusbhc_attrs,
};

int wusbhc_create(struct wusbhc *wusbhc)
{
	int result = 0;

	wusbhc->trust_timeout = WUSB_TRUST_TIMEOUT_MS;
	mutex_init(&wusbhc->mutex);
	result = wusbhc_mmcie_create(wusbhc);
	if (result < 0)
		goto error_mmcie_create;
	result = wusbhc_devconnect_create(wusbhc);
	if (result < 0)
		goto error_devconnect_create;
	result = wusbhc_rh_create(wusbhc);
	if (result < 0)
		goto error_rh_create;
	result = wusbhc_sec_create(wusbhc);
	if (result < 0)
		goto error_sec_create;
	return 0;

error_sec_create:
	wusbhc_rh_destroy(wusbhc);
error_rh_create:
	wusbhc_devconnect_destroy(wusbhc);
error_devconnect_create:
	wusbhc_mmcie_destroy(wusbhc);
error_mmcie_create:
	return result;
}
EXPORT_SYMBOL_GPL(wusbhc_create);

static inline struct kobject *wusbhc_kobj(struct wusbhc *wusbhc)
{
	return &wusbhc->usb_hcd.self.controller->kobj;
}

int wusbhc_b_create(struct wusbhc *wusbhc)
{
	int result = 0;
	struct device *dev = wusbhc->usb_hcd.self.controller;

	result = sysfs_create_group(wusbhc_kobj(wusbhc), &wusbhc_attr_group);
	if (result < 0) {
		dev_err(dev, "Cannot register WUSBHC attributes: %d\n", result);
		goto error_create_attr_group;
	}

	result = wusbhc_pal_register(wusbhc);
	if (result < 0)
		goto error_pal_register;
	return 0;

error_pal_register:
	sysfs_remove_group(wusbhc_kobj(wusbhc), &wusbhc_attr_group);
error_create_attr_group:
	return result;
}
EXPORT_SYMBOL_GPL(wusbhc_b_create);

void wusbhc_b_destroy(struct wusbhc *wusbhc)
{
	wusbhc_pal_unregister(wusbhc);
	sysfs_remove_group(wusbhc_kobj(wusbhc), &wusbhc_attr_group);
}
EXPORT_SYMBOL_GPL(wusbhc_b_destroy);

void wusbhc_destroy(struct wusbhc *wusbhc)
{
	wusbhc_sec_destroy(wusbhc);
	wusbhc_rh_destroy(wusbhc);
	wusbhc_devconnect_destroy(wusbhc);
	wusbhc_mmcie_destroy(wusbhc);
}
EXPORT_SYMBOL_GPL(wusbhc_destroy);

struct workqueue_struct *wusbd;
EXPORT_SYMBOL_GPL(wusbd);

#define CLUSTER_IDS 32
static DECLARE_BITMAP(wusb_cluster_id_table, CLUSTER_IDS);
static DEFINE_SPINLOCK(wusb_cluster_ids_lock);

/* FIXME: coordinate with the choose_addres() from the USB stack */
u8 wusb_cluster_id_get(void)
{
	u8 id;
	spin_lock(&wusb_cluster_ids_lock);
	id = find_first_zero_bit(wusb_cluster_id_table, CLUSTER_IDS);
	if (id > CLUSTER_IDS) {
		id = 0;
		goto out;
	}
	set_bit(id, wusb_cluster_id_table);
	id = (u8) 0xff - id;
out:
	spin_unlock(&wusb_cluster_ids_lock);
	return id;

}
EXPORT_SYMBOL_GPL(wusb_cluster_id_get);

void wusb_cluster_id_put(u8 id)
{
	id = 0xff - id;
	BUG_ON(id >= CLUSTER_IDS);
	spin_lock(&wusb_cluster_ids_lock);
	WARN_ON(!test_bit(id, wusb_cluster_id_table));
	clear_bit(id, wusb_cluster_id_table);
	spin_unlock(&wusb_cluster_ids_lock);
}
EXPORT_SYMBOL_GPL(wusb_cluster_id_put);

void wusbhc_giveback_urb(struct wusbhc *wusbhc, struct urb *urb, int status)
{
	struct wusb_dev *wusb_dev = __wusb_dev_get_by_usb_dev(wusbhc, urb->dev);

	if (status == 0) {
		wusb_dev->entry_ts = jiffies;

		/* wusbhc_devconnect_acked() can't be called from from
		   atomic context so defer it to a work queue. */
		if (!list_empty(&wusb_dev->cack_node))
			queue_work(wusbd, &wusb_dev->devconnect_acked_work);
	}

	usb_hcd_giveback_urb(&wusbhc->usb_hcd, urb, status);
}
EXPORT_SYMBOL_GPL(wusbhc_giveback_urb);

void wusbhc_reset_all(struct wusbhc *wusbhc)
{
	uwb_rc_reset_all(wusbhc->uwb_rc);
}
EXPORT_SYMBOL_GPL(wusbhc_reset_all);

static struct notifier_block wusb_usb_notifier = {
	.notifier_call = wusb_usb_ncb,
	.priority = INT_MAX	/* Need to be called first of all */
};

static int __init wusbcore_init(void)
{
	int result;
	result = wusb_crypto_init();
	if (result < 0)
		goto error_crypto_init;
	/* WQ is singlethread because we need to serialize notifications */
	wusbd = create_singlethread_workqueue("wusbd");
	if (wusbd == NULL) {
		result = -ENOMEM;
		printk(KERN_ERR "WUSB-core: Cannot create wusbd workqueue\n");
		goto error_wusbd_create;
	}
	usb_register_notify(&wusb_usb_notifier);
	bitmap_zero(wusb_cluster_id_table, CLUSTER_IDS);
	set_bit(0, wusb_cluster_id_table);	/* reserve Cluster ID 0xff */
	return 0;

error_wusbd_create:
	wusb_crypto_exit();
error_crypto_init:
	return result;

}
module_init(wusbcore_init);

static void __exit wusbcore_exit(void)
{
	clear_bit(0, wusb_cluster_id_table);
	if (!bitmap_empty(wusb_cluster_id_table, CLUSTER_IDS)) {
		char buf[256];
		bitmap_scnprintf(buf, sizeof(buf), wusb_cluster_id_table,
				 CLUSTER_IDS);
		printk(KERN_ERR "BUG: WUSB Cluster IDs not released "
		       "on exit: %s\n", buf);
		WARN_ON(1);
	}
	usb_unregister_notify(&wusb_usb_notifier);
	destroy_workqueue(wusbd);
	wusb_crypto_exit();
}
module_exit(wusbcore_exit);

MODULE_AUTHOR("Inaky Perez-Gonzalez <inaky.perez-gonzalez@intel.com>");
MODULE_DESCRIPTION("Wireless USB core");
MODULE_LICENSE("GPL");
