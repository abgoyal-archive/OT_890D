

#include <linux/init.h>

#include "lock_dlm.h"

static int __init init_lock_dlm(void)
{
	int error;

	error = gfs2_register_lockproto(&gdlm_ops);
	if (error) {
		printk(KERN_WARNING "lock_dlm:  can't register protocol: %d\n",
		       error);
		return error;
	}

	error = gdlm_sysfs_init();
	if (error) {
		gfs2_unregister_lockproto(&gdlm_ops);
		return error;
	}

	printk(KERN_INFO
	       "Lock_DLM (built %s %s) installed\n", __DATE__, __TIME__);
	return 0;
}

static void __exit exit_lock_dlm(void)
{
	gdlm_sysfs_exit();
	gfs2_unregister_lockproto(&gdlm_ops);
}

module_init(init_lock_dlm);
module_exit(exit_lock_dlm);

MODULE_DESCRIPTION("GFS DLM Locking Module");
MODULE_AUTHOR("Red Hat, Inc.");
MODULE_LICENSE("GPL");

