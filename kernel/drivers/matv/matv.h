
#ifndef __MATV_H__
#define __MATV_H__

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/mm_types.h>
#include <linux/mm.h>
#include <linux/jiffies.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <asm/page.h>
#include <linux/vmalloc.h>
#include <mach/mt6516_pll.h>
#include <mach/mt6516_typedefs.h> 
#include <mach/mt6516_reg_base.h> 
#include <linux/interrupt.h>
#include <mach/irqs.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <mach/mt6516_gpio.h>


#define MATV_DEVNAME "MT6516_MATV"

typedef enum 
{
    TEST_MATV_PRINT = 0,
    MATV_READ = 1,
    MATV_WRITE = 2,
    MATV_SET_PWR = 3,
    MATV_SET_RST = 4,
    MAX_MATV_CMD = 0xFFFFFFFF,
} MATV_CMD;


#endif //__MT6516_MM_QUEUE_H__
