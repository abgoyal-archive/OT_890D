

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/earlysuspend.h>

#include <asm/atomic.h>
#include <asm/uaccess.h>
#include <asm/tcm.h>

#include <mach/mt6516_typedefs.h>
#include <mach/mt6516_reg_base.h>
#include <mach/mt6516_gpio.h>
#include <mt6516_kpd.h>
#include <mach/irqs.h>

#define KPD_AUTOTEST	KPD_YES
#define KPD_DEBUG	KPD_NO

#define KPD_NAME	"mt6516-kpd"

/* Keypad registers */
#define KP_STA		(KP_BASE + 0x0000)
#define KP_MEM1		(KP_BASE + 0x0004)
#define KP_MEM2		(KP_BASE + 0x0008)
#define KP_MEM3		(KP_BASE + 0x000c)
#define KP_MEM4		(KP_BASE + 0x0010)
#define KP_MEM5		(KP_BASE + 0x0014)
#define KP_DEBOUNCE	(KP_BASE + 0x0018)

#define KPD_NUM_MEMS	5
#define KPD_MEM5_BITS	8

#define KPD_NUM_KEYS	72	/* 4 * 16 + KPD_MEM5_BITS */

#define KPD_DEBOUNCE_MASK	((1U << 14) - 1)
#define kpd_debounce_time(val)	((val) >> 5)		/* ms */

#define KPD_SAY		"kpd: "
#if KPD_DEBUG
#define kpd_print(fmt, arg...)	printk(KPD_SAY fmt, ##arg)
#else
#define kpd_print(fmt, arg...)	do {} while (0)
#endif

typedef enum {
#if KPD_AUTOTEST
	PRESS_OK_KEY		= 1,
	RELEASE_OK_KEY		= 2,
	PRESS_MENU_KEY		= 3,
	RELEASE_MENU_KEY	= 4,
	PRESS_UP_KEY		= 5,
	RELEASE_UP_KEY		= 6,
	PRESS_DOWN_KEY		= 7,
	RELEASE_DOWN_KEY	= 8,
	PRESS_LEFT_KEY		= 9,
	RELEASE_LEFT_KEY	= 10,
	PRESS_RIGHT_KEY		= 11,
	RELEASE_RIGHT_KEY	= 12,
	PRESS_HOME_KEY		= 13,
	RELEASE_HOME_KEY	= 14,
	PRESS_BACK_KEY		= 15,
	RELEASE_BACK_KEY	= 16,
	PRESS_CALL_KEY		= 17,
	RELEASE_CALL_KEY	= 18,
	PRESS_ENDCALL_KEY	= 19,
	RELEASE_ENDCALL_KEY	= 20,
	PRESS_VLUP_KEY		= 21,
	RELEASE_VLUP_KEY	= 22,
	PRESS_VLDOWN_KEY	= 23,
	RELEASE_VLDOWN_KEY	= 24,
	PRESS_FOCUS_KEY		= 25,
	RELEASE_FOCUS_KEY	= 26,
	PRESS_CAMERA_KEY	= 27,
	RELEASE_CAMERA_KEY	= 28,
#endif
	SET_KPD_BACKLIGHT	= 29,
} kpd_ioctl_cmd;

struct kpd_ledctl {
	u8 onoff;
	u8 div;		/* 0 ~ 15 */
	u8 duty;	/* 0 ~ 31 */
};

extern void MT6516_EINTIRQUnmask(unsigned int line);
extern void MT6516_EINT_Set_HW_Debounce(kal_uint8 eintno, kal_uint32 ms);
extern void MT6516_EINT_Set_Polarity(kal_uint8 eintno, kal_bool ACT_Polarity);
extern kal_uint32 MT6516_EINT_Set_Sensitivity(kal_uint8 eintno, kal_bool sens);
extern void MT6516_EINT_Registration(kal_uint8 eintno,
                                     kal_bool Dbounce_En,
                                     kal_bool ACT_Polarity,
                                     void (EINT_FUNC_PTR)(void),
                                     kal_bool auto_umask);

static struct input_dev *kpd_input_dev;
static bool kpd_suspend = false;
static int kpd_show_hw_keycode = 0;
static int kpd_show_register = 0;

/* for backlight control */
#if KPD_DRV_CTRL_BACKLIGHT
static void kpd_switch_backlight(struct work_struct *work);
static void kpd_backlight_timeout(unsigned long data);
static DECLARE_WORK(kpd_backlight_work, kpd_switch_backlight);
static DEFINE_TIMER(kpd_backlight_timer, kpd_backlight_timeout, 0, 0);

static unsigned long kpd_wake_keybit[BITS_TO_LONGS(KEY_CNT)];
static u16 kpd_wake_key[] __initdata = KPD_BACKLIGHT_WAKE_KEY;

static volatile bool kpd_backlight_on;
static atomic_t kpd_key_pressed = ATOMIC_INIT(0);
#endif

/* for slide QWERTY */
#if KPD_HAS_SLIDE_QWERTY
static void kpd_slide_handler(unsigned long data);
static DECLARE_TASKLET(kpd_slide_tasklet, kpd_slide_handler, 0);

static u8 kpd_slide_state = !KPD_SLIDE_POLARITY;
#endif

/* for Power key using EINT */
#if KPD_PWRKEY_USE_EINT
static void kpd_pwrkey_handler(unsigned long data);
static DECLARE_TASKLET(kpd_pwrkey_tasklet, kpd_pwrkey_handler, 0);

static u8 kpd_pwrkey_state = !KPD_PWRKEY_POLARITY;
#endif

/* for keymap handling */
static void kpd_keymap_handler(unsigned long data);
static DECLARE_TASKLET(kpd_keymap_tasklet, kpd_keymap_handler, 0);

static u16 kpd_keymap[KPD_NUM_KEYS] = KPD_INIT_KEYMAP();
static u16 kpd_keymap_state[KPD_NUM_MEMS] = {
	0xffff, 0xffff, 0xffff, 0xffff, 0x00ff
};

/* for autotest */
#if KPD_AUTOTEST
static const u16 kpd_auto_keymap[] = {
	KEY_OK, KEY_MENU,
	KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,
	KEY_HOME, KEY_BACK,
	KEY_CALL, KEY_ENDCALL,
	KEY_VOLUMEUP, KEY_VOLUMEDOWN,
	KEY_FOCUS, KEY_CAMERA,
};
#endif

static inline void kpd_get_keymap_state(u16 state[])
{
	state[0] = *(volatile u16 *)KP_MEM1;
	state[1] = *(volatile u16 *)KP_MEM2;
	state[2] = *(volatile u16 *)KP_MEM3;
	state[3] = *(volatile u16 *)KP_MEM4;
	state[4] = *(volatile u16 *)KP_MEM5;
	if (kpd_show_register) {
		printk(KPD_SAY "register = %x %x %x %x %x\n",
		       state[0], state[1], state[2], state[3], state[4]);
	}
}

static inline void kpd_set_debounce(u16 val)
{
	*(volatile u16 *)KP_DEBOUNCE = (u16)(val & KPD_DEBOUNCE_MASK);
}

#if KPD_DRV_CTRL_BACKLIGHT
static void kpd_switch_backlight(struct work_struct *work)
{
	if (kpd_backlight_on) {
		kpd_enable_backlight();
		kpd_print("backlight is on\n");
	} else {
		kpd_disable_backlight();
		kpd_print("backlight is off\n");
	}
}

static void kpd_backlight_timeout(unsigned long data)
{
	if (!atomic_read(&kpd_key_pressed)) {
		kpd_backlight_on = !!atomic_read(&kpd_key_pressed);
		schedule_work(&kpd_backlight_work);
		data = 1;
	}
	kpd_print("backlight timeout%s\n", 
	          data ? ": schedule backlight work" : "");
}

void kpd_backlight_handler(bool pressed, u16 linux_keycode)
{
	if (kpd_suspend && !test_bit(linux_keycode, kpd_wake_keybit)) {
		kpd_print("Linux keycode %u is not WAKE key\n", linux_keycode);
		return;
	}

	/* not in suspend or the key pressed is WAKE key */
	if (pressed) {
		atomic_inc(&kpd_key_pressed);
		kpd_backlight_on = !!atomic_read(&kpd_key_pressed);
		schedule_work(&kpd_backlight_work);
		kpd_print("switch backlight on\n");
	} else {
		atomic_dec(&kpd_key_pressed);
		mod_timer(&kpd_backlight_timer,
		          jiffies + KPD_BACKLIGHT_TIME * HZ);
		kpd_print("activate backlight timer\n");
	}
}
#endif

static void kpd_gpio_state_setting(int pin,int mode,int dir,int state)
{
	int i;
	mt_set_gpio_mode(pin,mode);
	mt_set_gpio_dir(pin,dir);
	udelay(100);

	if(state==true)
		mt_set_gpio_out(pin,1);

	i = mt_get_gpio_dir(pin);
	if(i==0)//direction is in.
		printk(KERN_ERR"******%d,%d\n",i,mt_get_gpio_in(pin));
	else//i=1,direction is out.
		printk(KERN_ERR"******%d,%d\n",i,mt_get_gpio_out(pin));
}

#if KPD_HAS_SLIDE_QWERTY
static void kpd_slide_handler(unsigned long data)
{
	bool slid;
	u8 old_state = kpd_slide_state;

	kpd_slide_state = !kpd_slide_state;
	slid = (kpd_slide_state == !!KPD_SLIDE_POLARITY);
	/* for SW_LID, 0: lid open => slid, 1: lid shut => closed */
	input_report_switch(kpd_input_dev, SW_LID, !slid);
	kpd_print("report QWERTY = %s\n", slid ? "slid" : "closed");

	/* for detecting the return to old_state */
	MT6516_EINT_Set_Polarity(KPD_SLIDE_EINT, old_state);
	MT6516_EINTIRQUnmask(KPD_SLIDE_EINT);
}

static void kpd_slide_eint_handler(void)
{
	tasklet_schedule(&kpd_slide_tasklet);
}
#endif

#if KPD_PWRKEY_USE_EINT
static void kpd_pwrkey_handler(unsigned long data)
{
	bool pressed;
	u8 old_state = kpd_pwrkey_state;

	kpd_pwrkey_state = !kpd_pwrkey_state;
	pressed = (kpd_pwrkey_state == !!KPD_PWRKEY_POLARITY);
	if (kpd_show_hw_keycode) {
		printk(KPD_SAY "(%s) HW keycode = using EINT\n",
		       pressed ? "pressed" : "released");
	}
	kpd_backlight_handler(pressed, KPD_PWRKEY_MAP);
	input_report_key(kpd_input_dev, KPD_PWRKEY_MAP, pressed);
	kpd_print("report Linux keycode = %u\n", KPD_PWRKEY_MAP);

	/* for detecting the return to old_state */
	MT6516_EINT_Set_Polarity(KPD_PWRKEY_EINT, old_state);
	MT6516_EINTIRQUnmask(KPD_PWRKEY_EINT);
}

static void kpd_pwrkey_eint_handler(void)
{
	tasklet_schedule(&kpd_pwrkey_tasklet);
}
#endif

static void kpd_keymap_handler(unsigned long data)
{
	int i, j;
	bool pressed;
	u16 new_state[KPD_NUM_MEMS], change, mask;
	u16 hw_keycode, linux_keycode;

	kpd_get_keymap_state(new_state);

	for (i = 0; i < KPD_NUM_MEMS; i++) {
		change = new_state[i] ^ kpd_keymap_state[i];
		if (!change)
			continue;

		for (j = 0; j < 16; j++) {
			mask = 1U << j;
			if (!(change & mask))
				continue;

			hw_keycode = (i << 4) + j;
			/* bit is 1: not pressed, 0: pressed */
			pressed = !(new_state[i] & mask);
			if (kpd_show_hw_keycode) {
				printk(KPD_SAY "(%s) HW keycode = %u\n",
				       pressed ? "pressed" : "released",
				       hw_keycode);
			}
			BUG_ON(hw_keycode >= KPD_NUM_KEYS);
			linux_keycode = kpd_keymap[hw_keycode];
			if (unlikely(linux_keycode == 0)) {
				kpd_print("Linux keycode = 0\n");
				continue;
			}

			kpd_backlight_handler(pressed, linux_keycode);
			input_report_key(kpd_input_dev, linux_keycode, pressed);
			kpd_print("report Linux keycode = %u\n", linux_keycode);
		}
	}

	memcpy(kpd_keymap_state, new_state, sizeof(new_state));
	kpd_print("save new keymap state\n");
	enable_irq(MT6516_KP_IRQ_LINE);
}

static irqreturn_t __tcmfunc kpd_irq_handler(int irq, void *dev_id)
{
	/* use _nosync to avoid deadlock */
	disable_irq_nosync(MT6516_KP_IRQ_LINE);

	tasklet_schedule(&kpd_keymap_tasklet);
	return IRQ_HANDLED;
}

static long kpd_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *uarg = (void __user *)arg;
	struct kpd_ledctl ledctl;

	switch (cmd) {
#if KPD_AUTOTEST
	case PRESS_OK_KEY:
		printk("[AUTOTEST] PRESS OK KEY!!\n");
		input_report_key(kpd_input_dev, KEY_OK, 1);
		break;
	case RELEASE_OK_KEY:
		printk("[AUTOTEST] RELEASE OK KEY!!\n");
		input_report_key(kpd_input_dev, KEY_OK, 0);
		break;

	case PRESS_MENU_KEY:
		printk("[AUTOTEST] PRESS MENU KEY!!\n");
		input_report_key(kpd_input_dev, KEY_MENU, 1);
		break;
	case RELEASE_MENU_KEY:
		printk("[AUTOTEST] RELEASE MENU KEY!!\n");
		input_report_key(kpd_input_dev, KEY_MENU, 0);
		break;

	case PRESS_UP_KEY:
		printk("[AUTOTEST] PRESS UP KEY!!\n");
		input_report_key(kpd_input_dev, KEY_UP, 1);
		break;
	case RELEASE_UP_KEY:
		printk("[AUTOTEST] RELEASE UP KEY!!\n");
		input_report_key(kpd_input_dev, KEY_UP, 0);
		break;

	case PRESS_DOWN_KEY:
		printk("[AUTOTEST] PRESS DOWN KEY!!\n");
		input_report_key(kpd_input_dev, KEY_DOWN, 1);
		break;
	case RELEASE_DOWN_KEY:
		printk("[AUTOTEST] RELEASE DOWN KEY!!\n");
		input_report_key(kpd_input_dev, KEY_DOWN, 0);
		break;

	case PRESS_LEFT_KEY:
		printk("[AUTOTEST] PRESS LEFT KEY!!\n");
		input_report_key(kpd_input_dev, KEY_LEFT, 1);
		break;
	case RELEASE_LEFT_KEY:
		printk("[AUTOTEST] RELEASE LEFT KEY!!\n");
		input_report_key(kpd_input_dev, KEY_LEFT, 0);
		break;

	case PRESS_RIGHT_KEY:
		printk("[AUTOTEST] PRESS RIGHT KEY!!\n");
		input_report_key(kpd_input_dev, KEY_RIGHT, 1);
		break;
	case RELEASE_RIGHT_KEY:
		printk("[AUTOTEST] RELEASE RIGHT KEY!!\n");
		input_report_key(kpd_input_dev, KEY_RIGHT, 0);
		break;

	case PRESS_HOME_KEY:
		printk("[AUTOTEST] PRESS HOME KEY!!\n");
		input_report_key(kpd_input_dev, KEY_HOME, 1);
		break;
	case RELEASE_HOME_KEY:
		printk("[AUTOTEST] RELEASE HOME KEY!!\n");
		input_report_key(kpd_input_dev, KEY_HOME, 0);
		break;

	case PRESS_BACK_KEY:
		printk("[AUTOTEST] PRESS BACK KEY!!\n");
		input_report_key(kpd_input_dev, KEY_BACK, 1);
		break;
	case RELEASE_BACK_KEY:
		printk("[AUTOTEST] RELEASE BACK KEY!!\n");
		input_report_key(kpd_input_dev, KEY_BACK, 0);
		break;

	case PRESS_CALL_KEY:
		printk("[AUTOTEST] PRESS CALL KEY!!\n");
		input_report_key(kpd_input_dev, KEY_CALL, 1);
		break;
	case RELEASE_CALL_KEY:
		printk("[AUTOTEST] RELEASE CALL KEY!!\n");
		input_report_key(kpd_input_dev, KEY_CALL, 0);
		break;

	case PRESS_ENDCALL_KEY:
		printk("[AUTOTEST] PRESS ENDCALL KEY!!\n");
		input_report_key(kpd_input_dev, KEY_ENDCALL, 1);
		break;
	case RELEASE_ENDCALL_KEY:
		printk("[AUTOTEST] RELEASE ENDCALL KEY!!\n");
		input_report_key(kpd_input_dev, KEY_ENDCALL, 0);
		break;

	case PRESS_VLUP_KEY:
		printk("[AUTOTEST] PRESS VOLUMEUP KEY!!\n");
		input_report_key(kpd_input_dev, KEY_VOLUMEUP, 1);
		break;
	case RELEASE_VLUP_KEY:
		printk("[AUTOTEST] RELEASE VOLUMEUP KEY!!\n");
		input_report_key(kpd_input_dev, KEY_VOLUMEUP, 0);
		break;

	case PRESS_VLDOWN_KEY:
		printk("[AUTOTEST] PRESS VOLUMEDOWN KEY!!\n");
		input_report_key(kpd_input_dev, KEY_VOLUMEDOWN, 1);
		break;
	case RELEASE_VLDOWN_KEY:
		printk("[AUTOTEST] RELEASE VOLUMEDOWN KEY!!\n");
		input_report_key(kpd_input_dev, KEY_VOLUMEDOWN, 0);
		break;

	case PRESS_FOCUS_KEY:
		printk("[AUTOTEST] PRESS FOCUS KEY!!\n");
		input_report_key(kpd_input_dev, KEY_FOCUS, 1);
		break;
	case RELEASE_FOCUS_KEY:
		printk("[AUTOTEST] RELEASE FOCUS KEY!!\n");
		input_report_key(kpd_input_dev, KEY_FOCUS, 0);
		break;

	case PRESS_CAMERA_KEY:
		printk("[AUTOTEST] PRESS CAMERA KEY!!\n");
		input_report_key(kpd_input_dev, KEY_CAMERA, 1);
		break;
	case RELEASE_CAMERA_KEY:
		printk("[AUTOTEST] RELEASE CAMERA KEY!!\n");
		input_report_key(kpd_input_dev, KEY_CAMERA, 0);
		break;
#endif

	case SET_KPD_BACKLIGHT:
		if (copy_from_user(&ledctl, uarg, sizeof(struct kpd_ledctl)))
			return -EFAULT;

		kpd_set_backlight(ledctl.onoff, &ledctl.div, &ledctl.duty);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int kpd_dev_open(struct inode *inode, struct file *file)
{
	return 0;
}

static struct file_operations kpd_dev_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl	= kpd_dev_ioctl,
	.open		= kpd_dev_open,
};

static struct miscdevice kpd_dev = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= KPD_NAME,
	.fops	= &kpd_dev_fops,
};

static int kpd_pdrv_probe(struct platform_device *pdev)
{
	int i, r;

	/* initialize and register input device (/dev/input/eventX) */
	kpd_input_dev = input_allocate_device();
	if (!kpd_input_dev)
		return -ENOMEM;

	kpd_input_dev->name = KPD_NAME;
	kpd_input_dev->id.bustype = BUS_HOST;
	kpd_input_dev->id.vendor = 0x2454;
	kpd_input_dev->id.product = 0x6516;
	kpd_input_dev->id.version = 0x0010;

	__set_bit(EV_KEY, kpd_input_dev->evbit);

#if KPD_PWRKEY_USE_EINT
	__set_bit(KPD_PWRKEY_MAP, kpd_input_dev->keybit);
	kpd_keymap[8] = 0;
#endif
	for (i = 17; i < KPD_NUM_KEYS; i += 9)	/* only [8] works for Power key */
		kpd_keymap[i] = 0;

	for (i = 0; i < KPD_NUM_KEYS; i++) {
		if (kpd_keymap[i] != 0)
			__set_bit(kpd_keymap[i], kpd_input_dev->keybit);
	}

#if KPD_AUTOTEST
	for (i = 0; i < ARRAY_SIZE(kpd_auto_keymap); i++)
		__set_bit(kpd_auto_keymap[i], kpd_input_dev->keybit);
#endif

#if KPD_HAS_SLIDE_QWERTY
	__set_bit(EV_SW, kpd_input_dev->evbit);
	__set_bit(SW_LID, kpd_input_dev->swbit);
	__set_bit(SW_LID, kpd_input_dev->sw);	/* 1: lid shut => closed */
#endif

	kpd_input_dev->dev.parent = &pdev->dev;
	r = input_register_device(kpd_input_dev);
	if (r) {
		printk(KPD_SAY "register input device failed (%d)\n", r);
		input_free_device(kpd_input_dev);
		return r;
	}

	/* register device (/dev/mt6516-kpd) */
	kpd_dev.parent = &pdev->dev;
	r = misc_register(&kpd_dev);
	if (r) {
		printk(KPD_SAY "register device failed (%d)\n", r);
		input_unregister_device(kpd_input_dev);
		return r;
	}

	/* register IRQ and EINT */
	kpd_set_debounce(KPD_KEY_DEBOUNCE);
	r = request_irq(MT6516_KP_IRQ_LINE, kpd_irq_handler, 0, KPD_NAME, NULL);
	if (r) {
		printk(KPD_SAY "register IRQ failed (%d)\n", r);
		misc_deregister(&kpd_dev);
		input_unregister_device(kpd_input_dev);
		return r;
	}

#if KPD_PWRKEY_USE_EINT
	MT6516_EINT_Set_Sensitivity(KPD_PWRKEY_EINT, KPD_PWRKEY_SENSITIVE);
	MT6516_EINT_Set_HW_Debounce(KPD_PWRKEY_EINT,
	                            kpd_debounce_time(KPD_PWRKEY_DEBOUNCE));
	MT6516_EINT_Registration(KPD_PWRKEY_EINT, true, KPD_PWRKEY_POLARITY,
	                         kpd_pwrkey_eint_handler, false);
#endif

#if KPD_HAS_SLIDE_QWERTY
	MT6516_EINT_Set_Sensitivity(KPD_SLIDE_EINT, KPD_SLIDE_SENSITIVE);
	MT6516_EINT_Set_HW_Debounce(KPD_SLIDE_EINT,
	                            kpd_debounce_time(KPD_SLIDE_DEBOUNCE));
	MT6516_EINT_Registration(KPD_SLIDE_EINT, true, KPD_SLIDE_POLARITY,
	                         kpd_slide_eint_handler, false);
#endif

	/* switch backlight on */
	kpd_backlight_handler(true, 0);
	kpd_backlight_handler(false, 0);

	return 0;
}

/* should never be called */
static int kpd_pdrv_remove(struct platform_device *pdev)
{
	return 0;
}

#ifndef CONFIG_HAS_EARLYSUSPEND
static int kpd_pdrv_suspend(struct platform_device *pdev, pm_message_t state)
{
	kpd_suspend = true;
	kpd_disable_backlight();
	kpd_print("suspend!! (%d)\n", kpd_suspend);
	return 0;
}

static int kpd_pdrv_resume(struct platform_device *pdev)
{
	kpd_suspend = false;
	//kpd_enable_backlight();
	kpd_print("resume!! (%d)\n", kpd_suspend);
	return 0;
}
#else
//#define kpd_pdrv_suspend	NULL
//#define kpd_pdrv_resume		NULL
static int kpd_pdrv_suspend(struct platform_device *pdev, pm_message_t state)
{
	kpd_gpio_state_setting(111,GPIO_MODE_GPIO,1,true);
	kpd_gpio_state_setting(112,GPIO_MODE_GPIO,1,true);
	printk(KERN_ERR"kpd enter suspend state.\n");

	return 0;
}

static int kpd_pdrv_resume(struct platform_device *pdev)
{
	kpd_gpio_state_setting(111,1,0,false);
	kpd_gpio_state_setting(112,1,0,false);
	printk(KERN_ERR"kpd device resume.\n");

	return 0;
}
#endif

static struct platform_driver kpd_pdrv = {
	.probe		= kpd_pdrv_probe,
	.remove		= kpd_pdrv_remove,
	.suspend		= kpd_pdrv_suspend,
	.resume		= kpd_pdrv_resume,
	.driver		= {
		.name	= KPD_NAME,
		.owner	= THIS_MODULE,
	},
};

#ifdef CONFIG_HAS_EARLYSUSPEND
static void kpd_early_suspend(struct early_suspend *h)
{
	kpd_suspend = true;
	kpd_disable_backlight();
	//kpd_gpio_state_setting(111,GPIO_MODE_GPIO,1,kpd_suspend);
	//kpd_gpio_state_setting(112,GPIO_MODE_GPIO,1,kpd_suspend);
	kpd_print("early suspend!! (%d)\n", kpd_suspend);
}

static void kpd_early_resume(struct early_suspend *h)
{
	kpd_suspend = false;
	//kpd_enable_backlight();
	//kpd_gpio_state_setting(111,1,0,kpd_suspend);
	//kpd_gpio_state_setting(112,1,0,kpd_suspend);
	kpd_print("early resume!! (%d)\n", kpd_suspend);
}

static struct early_suspend kpd_early_suspend_desc = {
	.level		= EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1,
	.suspend	= kpd_early_suspend,
	.resume		= kpd_early_resume,
};
#endif

static int __init kpd_mod_init(void)
{
	int r;

#if KPD_DRV_CTRL_BACKLIGHT
	for (r = 0; r < ARRAY_SIZE(kpd_wake_key); r++)
		__set_bit(kpd_wake_key[r], kpd_wake_keybit);
#endif

	r = platform_driver_register(&kpd_pdrv);
	if (r) {
		printk(KPD_SAY "register driver failed (%d)\n", r);
		return r;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	register_early_suspend(&kpd_early_suspend_desc);
#endif
	return 0;
}

/* should never be called */
static void __exit kpd_mod_exit(void)
{
}

module_init(kpd_mod_init);
module_exit(kpd_mod_exit);

module_param(kpd_show_hw_keycode, bool, 0644);
module_param(kpd_show_register, bool, 0644);

MODULE_AUTHOR("Terry Chang <terry.chang@mediatek.com>");
MODULE_DESCRIPTION("MT6516 Keypad (KPD) Driver v3.4");
MODULE_LICENSE("GPL");
