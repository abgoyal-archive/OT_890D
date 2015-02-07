

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/workqueue.h>

//#include <linux/timed_output.h>
#include "timed_output.h"
#include <linux/hrtimer.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>

#include <linux/jiffies.h>
#include <linux/timer.h>

#include <mach/mt6516_typedefs.h>

#define VERSION					        "v 0.1"
#define VIB_DEVICE	        			"mt6516_vibrator"

#define PMIC_VIBRATOR_LEVEL     		(3000)
#define COUNT_DOWN_TIME					50

#define RSUCCESS        0


/* Debug message event */
#define DBG_EVT_NONE		0x00000000	/* No event */
#define DBG_EVT_INT			0x00000001	/* Interrupt related event */
#define DBG_EVT_TASKLET		0x00000002	/* Tasklet related event */

#define DBG_EVT_ALL			0xffffffff
 
#define DBG_EVT_MASK      	(DBG_EVT_TASKLET)

#if 1
#define MSG(evt, fmt, args...) \
do {	\
	if ((DBG_EVT_##evt) & DBG_EVT_MASK) { \
		printk(fmt, ##args); \
	} \
} while(0)

#define MSG_FUNC_ENTRY(f)	MSG(FUC, "<FUN_ENT>: %s\n", __FUNCTION__)
#else
#define MSG(evt, fmt, args...) do{}while(0)
#define MSG_FUNC_ENTRY(f)	   do{}while(0)
#endif

extern size_t mt6326_vibr_Enable(void);
extern size_t mt6326_vibr_Disable(void);


static struct work_struct vibrator_work;
static struct hrtimer vibe_timer;
static spinlock_t vibe_lock;
static int vibe_state;

static void update_vibrator(struct work_struct *work)
{
	if(!vibe_state)
		mt6326_vibr_Disable();
	else
		mt6326_vibr_Enable();
}

static int vibrator_get_time(struct timed_output_dev *dev)
{
	if (hrtimer_active(&vibe_timer)) 
	{
		ktime_t r = hrtimer_get_remaining(&vibe_timer);
		return r.tv.sec * 1000 + r.tv.nsec / 1000000;
	} 
	else
		return 0;
}

static void vibrator_enable(struct timed_output_dev *dev, int value)
{
		unsigned long   flags;

		spin_lock_irqsave(&vibe_lock, flags);
		hrtimer_cancel(&vibe_timer);

		if (value == 0)
			vibe_state = 0;
		else 
		{
			value = (value > 15000 ? 15000 : value);
			vibe_state = 1;
			hrtimer_start(&vibe_timer, 
							ktime_set(value / 1000, (value % 1000) * 1000000),
							HRTIMER_MODE_REL);
		}
		spin_unlock_irqrestore(&vibe_lock, flags);

		schedule_work(&vibrator_work);
}

static enum hrtimer_restart vibrator_timer_func(struct hrtimer *timer)
{
 		vibe_state = 0;
 		schedule_work(&vibrator_work);
 		return HRTIMER_NORESTART;
}

static struct timed_output_dev mt6516_vibrator = 
{
	.name = "vibrator",
	.get_time = vibrator_get_time,
	.enable = vibrator_enable,
};

static int vib_probe(struct platform_device *pdev)
{
	return 0;
}

static int vib_remove(struct platform_device *pdev)
{
	return 0;
}

static void vib_shutdown(struct platform_device *pdev)
{
	if(vibe_state) {
		vibe_state = 0;
		mt6326_vibr_Disable();
	}
}

static struct platform_driver vibrator_driver = 
{
    .probe    = vib_probe,
    .remove   = vib_remove,
    .shutdown = vib_shutdown,
    .driver   = {
    .name = VIB_DEVICE,
    },
};



static s32 __devinit vib_mod_init(void)
{	
	s32 ret;

	printk("MediaTek MT6516 vibrator driver register, version %s\n", VERSION);

	INIT_WORK(&vibrator_work, update_vibrator);

	spin_lock_init(&vibe_lock);
	vibe_state = 0;
	hrtimer_init(&vibe_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	vibe_timer.function = vibrator_timer_func;

	timed_output_dev_register(&mt6516_vibrator);

    ret = platform_driver_register(&vibrator_driver);

    if(ret) 
    {
		printk("Unable to register vibrator driver (%d)\n", ret);
		return ret;
    }	

	printk("Done\n");
 
    return RSUCCESS;
}

 
static void __exit vib_mod_exit(void)
{
	printk("MediaTek MT6516 vibrator driver unregister, version %s\n", VERSION);
	printk("Done\n");
}

module_init(vib_mod_init);
module_exit(vib_mod_exit);
MODULE_AUTHOR("Koshi, Chiu <koshi.chiu@mediatek.com>");
MODULE_DESCRIPTION("MT6516 Vibrator Driver (VIB)");
MODULE_LICENSE("GPL");
