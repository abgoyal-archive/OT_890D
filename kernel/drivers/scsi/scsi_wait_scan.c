

#include <linux/module.h>
#include "scsi_priv.h"

static int __init wait_scan_init(void)
{
	scsi_complete_async_scans();
	return 0;
}

static void __exit wait_scan_exit(void)
{
}

MODULE_DESCRIPTION("SCSI wait for scans");
MODULE_AUTHOR("James Bottomley");
MODULE_LICENSE("GPL");

late_initcall(wait_scan_init);
module_exit(wait_scan_exit);
