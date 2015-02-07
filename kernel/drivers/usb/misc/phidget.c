

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/device.h>

struct class *phidget_class;

static int __init init_phidget(void)
{
	phidget_class = class_create(THIS_MODULE, "phidget");

	if (IS_ERR(phidget_class))
		return PTR_ERR(phidget_class);

	return 0;
}

static void __exit cleanup_phidget(void)
{
	class_destroy(phidget_class);
}

EXPORT_SYMBOL_GPL(phidget_class);

module_init(init_phidget);
module_exit(cleanup_phidget);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sean Young <sean@mess.org>");
MODULE_DESCRIPTION("Container module for phidget class");

