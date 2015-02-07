

#include <linux/module.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/kdev_t.h>
#include <linux/idr.h>
#include <linux/hwmon.h>
#include <linux/gfp.h>
#include <linux/spinlock.h>

#define HWMON_ID_PREFIX "hwmon"
#define HWMON_ID_FORMAT HWMON_ID_PREFIX "%d"

static struct class *hwmon_class;

static DEFINE_IDR(hwmon_idr);
static DEFINE_SPINLOCK(idr_lock);

struct device *hwmon_device_register(struct device *dev)
{
	struct device *hwdev;
	int id, err;

again:
	if (unlikely(idr_pre_get(&hwmon_idr, GFP_KERNEL) == 0))
		return ERR_PTR(-ENOMEM);

	spin_lock(&idr_lock);
	err = idr_get_new(&hwmon_idr, NULL, &id);
	spin_unlock(&idr_lock);

	if (unlikely(err == -EAGAIN))
		goto again;
	else if (unlikely(err))
		return ERR_PTR(err);

	id = id & MAX_ID_MASK;
	hwdev = device_create(hwmon_class, dev, MKDEV(0, 0), NULL,
			      HWMON_ID_FORMAT, id);

	if (IS_ERR(hwdev)) {
		spin_lock(&idr_lock);
		idr_remove(&hwmon_idr, id);
		spin_unlock(&idr_lock);
	}

	return hwdev;
}

void hwmon_device_unregister(struct device *dev)
{
	int id;

	if (likely(sscanf(dev_name(dev), HWMON_ID_FORMAT, &id) == 1)) {
		device_unregister(dev);
		spin_lock(&idr_lock);
		idr_remove(&hwmon_idr, id);
		spin_unlock(&idr_lock);
	} else
		dev_dbg(dev->parent,
			"hwmon_device_unregister() failed: bad class ID!\n");
}

static int __init hwmon_init(void)
{
	hwmon_class = class_create(THIS_MODULE, "hwmon");
	if (IS_ERR(hwmon_class)) {
		printk(KERN_ERR "hwmon.c: couldn't create sysfs class\n");
		return PTR_ERR(hwmon_class);
	}
	return 0;
}

static void __exit hwmon_exit(void)
{
	class_destroy(hwmon_class);
}

subsys_initcall(hwmon_init);
module_exit(hwmon_exit);

EXPORT_SYMBOL_GPL(hwmon_device_register);
EXPORT_SYMBOL_GPL(hwmon_device_unregister);

MODULE_AUTHOR("Mark M. Hoffman <mhoffman@lightlink.com>");
MODULE_DESCRIPTION("hardware monitoring sysfs/class support");
MODULE_LICENSE("GPL");

