

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/serial_core.h>
#include <linux/sysdev.h>

#include <plat/s3c2442.h>
#include <plat/cpu.h>

static struct sys_device s3c2442_sysdev = {
	.cls		= &s3c2442_sysclass,
};

int __init s3c2442_init(void)
{
	printk("S3C2442: Initialising architecture\n");

	return sysdev_register(&s3c2442_sysdev);
}
