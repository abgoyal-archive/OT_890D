


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/workqueue.h>
#include <mach/mt6516_typedefs.h>
#include <linux/switch.h>
#include "yusu_audio_stream.h"

#ifndef _YUSU_ANDROID_HEADSET_H_
#define _YUSU_ANDROID_HEADSET_H_


void Sound_Headset_Set_Gpio(void);
void Sound_Headset_Unset_Gpio(void);

#endif


