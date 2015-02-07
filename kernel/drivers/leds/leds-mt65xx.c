

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/leds.h>
#include <linux/workqueue.h>
#include <linux/wakelock.h>

#include <mach/mt6516_pll.h>
#include <mach/mt6516_pwm.h>
#include <mach/mt6516_gpio.h>
#include <mach/mt65xx_leds.h>

#include <cust_leds.h>

#ifdef MT65XX_LEDS_DEBUG
#define LEDS_DEBUG(format, ...) printk(format, __VA_ARGS__)
#define LEDS_DEBUG_FUNC printk("%s\n",__FUNCTION__)
#define LEDS_INFO(format, ...) printk(format, __VA_ARGS__)
#else
#define LEDS_DEBUG(format, ...)
#define LEDS_DEBUG_FUNC
#define LEDS_INFO(format, ...)
#endif

#if defined(CONFIG_MT6516_OPPO_BOARD)
static bool pwm2_vcam_on = false;
#define PWM2_VCAM_POWER_ON() \
	do { \
	if(!pwm2_vcam_on) { \
		pwm2_vcam_on=true; \
		hwPowerOn(MT6516_POWER_VCAM_A, VOL_2800,"_PWM2");\
	} } while(0)
#define PWM2_VCAM_POWER_OFF() \
	do { \
	if(pwm2_vcam_on) { \
		pwm2_vcam_on=false; \
		hwPowerDown(MT6516_POWER_VCAM_A,"_PWM2"); \
	} } while(0)
#else
#define PWM2_VCAM_POWER_ON()
#define PWM2_VCAM_POWER_OFF()
#endif

struct mt65xx_led_data {
	struct led_classdev cdev;
	struct cust_mt65xx_led cust;
	struct work_struct work;
	int level;
	int delay_on;
	int delay_off;
};


/* import functions */
extern void pmic_bl_dim_duty(kal_uint8 duty);
extern ssize_t  mt6326_bl_Enable(void);
extern ssize_t  mt6326_bl_Disable(void);
extern ssize_t mt6326_kpled_Enable(void);
extern ssize_t mt6326_kpled_Disable(void);
extern ssize_t mt6326_kpled_dim_duty_Full(void);
extern ssize_t mt6326_kpled_dim_duty_0(void);

/* export functions */
int mt65xx_leds_brightness_set(enum mt65xx_led_type type, enum led_brightness level);

/* internal functions */
static int brightness_set_pwm(int pwm_num, enum led_brightness level);
static int brightness_set_pmic(enum mt65xx_led_pmic pmic_type, enum led_brightness level);
static int brightness_set_gpio(int gpio_num, enum led_brightness level);
static int mt65xx_led_set_cust(struct cust_mt65xx_led *cust, int level);
static void mt65xx_led_work(struct work_struct *work);
static void mt65xx_led_set(struct led_classdev *led_cdev, enum led_brightness level);
static int  mt65xx_blink_set(struct led_classdev *led_cdev,
			     unsigned long *delay_on,
			     unsigned long *delay_off);

static struct mt65xx_led_data *g_leds_data[MT65XX_LED_TYPE_TOTAL];
struct wake_lock leds_suspend_lock;

static int brightness_mapto64(int level)
{
        if (level < 30)
                return (level >> 1) + 7;
        else if (level <= 120)
                return (level >> 2) + 14;
        else if (level <= 160)
                return level / 5 + 20;
        else
                return (level >> 3) + 33;
}

static int brightness_set_pwm(int pwm_num, enum led_brightness level)
{
	LEDS_INFO("LED PWM#%d:%d\n", pwm_num, level);
	if (pwm_num == 0) {
		LEDS_INFO("PWM#%d SendWaveNum:%d\n", pwm_num, INREG32(PWM0_SEND_WAVENUM));
		CLRREG32(PWM_ENABLE, PWM0_ENABLE);
		hwDisableClock(MT6516_PDN_PERI_PWM0,"PWM0");
		if (level) {
			hwEnableClock(MT6516_PDN_PERI_PWM0,"PWM0");
			OUTREG32(PWM0_CON, 0x8007);
			OUTREG32(PWM0_DATA_WIDTH, LED_FULL-1);
			OUTREG32(PWM0_THRESH, level-1);
			SETREG32(PWM_ENABLE, PWM0_ENABLE);
		}
		return 0;
	}
	else if (pwm_num == 1) {
		LEDS_INFO("PWM#%d SendWaveNum:%d\n", pwm_num, INREG32(PWM1_SEND_WAVENUM));
		CLRREG32(PWM_ENABLE, PWM1_ENABLE);
		hwDisableClock(MT6516_PDN_PERI_PWM1,"PWM1");
		if (level) {
			hwEnableClock(MT6516_PDN_PERI_PWM1,"PWM1");
			OUTREG32(PWM1_CON, 0x8007);
			OUTREG32(PWM1_DATA_WIDTH, LED_FULL-1);
			OUTREG32(PWM1_THRESH, level-1);
			SETREG32(PWM_ENABLE, PWM1_ENABLE);
		}
		return 0;
	}
	else if (pwm_num == 2) {
		LEDS_INFO("PWM#%d SendWaveNum:%d\n", pwm_num, INREG32(PWM2_SEND_WAVENUM));
		CLRREG32(PWM_ENABLE, PWM2_ENABLE);
		hwDisableClock(MT6516_PDN_PERI_PWM2,"PWM2");
		PWM2_VCAM_POWER_OFF();
		if (level) {
			hwEnableClock(MT6516_PDN_PERI_PWM2,"PWM2");
			PWM2_VCAM_POWER_ON();
			OUTREG32(PWM2_CON, 0x8007);
			OUTREG32(PWM2_DATA_WIDTH, LED_FULL-1);
			OUTREG32(PWM2_THRESH, level-1);
			SETREG32(PWM_ENABLE, PWM2_ENABLE);
		}
		return 0;
	}
	else if (pwm_num == 3) {
		LEDS_INFO("PWM#%d SendWaveNum:%d\n", pwm_num, INREG32(PWM3_SEND_WAVENUM));
		CLRREG32(PWM_ENABLE, PWM3_ENABLE);
		hwDisableClock(MT6516_PDN_PERI_PWM3,"PWM3");
		if (level) {
			hwEnableClock(MT6516_PDN_PERI_PWM3,"PWM3");
			OUTREG32(PWM3_CON, 0x8007);
			OUTREG32(PWM3_DATA_WIDTH, LED_FULL-1);
			OUTREG32(PWM3_THRESH, level-1);
			SETREG32(PWM_ENABLE, PWM3_ENABLE);
		}
		return 0;
	}
	else if (pwm_num == 4) {
		LEDS_INFO("PWM#%d SendWaveNum:%d\n", pwm_num, INREG32(PWM4_SEND_WAVENUM));
		CLRREG32(PWM_ENABLE, PWM4_ENABLE);
		hwDisableClock(MT6516_PDN_PERI_PWM,"PWM4");
		if (level) {
			hwEnableClock(MT6516_PDN_PERI_PWM,"PWM4");
			OUTREG32(PWM4_CON, 0x0207);
			if (level == LED_FULL)
				OUTREG32(PWM4_SEND_DATA0, 0x3);
			else
				OUTREG32(PWM4_SEND_DATA0, 0x2);
			OUTREG32(PWM4_HDURATION, level-1);
			OUTREG32(PWM4_LDURATION, LED_FULL-level-1);
			SETREG32(PWM_ENABLE, PWM4_ENABLE);
		}
		return 0;
	}
	else if (pwm_num == 5) {
		LEDS_INFO("PWM#%d SendWaveNum:%d\n", pwm_num, INREG32(PWM5_SEND_WAVENUM));
		CLRREG32(PWM_ENABLE, PWM5_ENABLE);
		hwDisableClock(MT6516_PDN_PERI_PWM,"PWM5");
		if (level) {
			hwEnableClock(MT6516_PDN_PERI_PWM,"PWM5");
			OUTREG32(PWM5_CON, 0x0207);
			if (level == LED_FULL)
				OUTREG32(PWM5_SEND_DATA0, 0x3);
			else
				OUTREG32(PWM5_SEND_DATA0, 0x2);
			OUTREG32(PWM5_HDURATION, level-1);
			OUTREG32(PWM5_LDURATION, LED_FULL-level-1);
			SETREG32(PWM_ENABLE, PWM5_ENABLE);
		}
		return 0;
	}
	else if (pwm_num == 6) {
		LEDS_INFO("PWM#%d SendWaveNum:%d\n", pwm_num, INREG32(PWM6_SEND_WAVENUM));
		if (level) {
			hwEnableClock(MT6516_PDN_PERI_PWM,"PWM6");
			OUTREG32(PWM6_CON, 0x7E08);
			OUTREG32(PWM6_HDURATION, 4);
			OUTREG32(PWM6_LDURATION, 4);
			level = brightness_mapto64(level);
			if (level < 32) {
				OUTREG32(PWM6_SEND_DATA0, (1 << level) - 1);
				OUTREG32(PWM6_SEND_DATA1, 0x0);
			}
			else if (level == 32) {
				OUTREG32(PWM6_SEND_DATA0, 0xFFFFFFFF);
				OUTREG32(PWM6_SEND_DATA1, 0x0);
			}
			else if (level < 64) {
				level -= 32;
				OUTREG32(PWM6_SEND_DATA0, 0xFFFFFFFF);
				OUTREG32(PWM6_SEND_DATA1, (1 << level) - 1);
			}
			else {
				OUTREG32(PWM6_SEND_DATA0, 0xFFFFFFFF);
				OUTREG32(PWM6_SEND_DATA1, 0xFFFFFFFF);
			}
			SETREG32(PWM_ENABLE, PWM6_ENABLE);
		}
		else {
			CLRREG32(PWM_ENABLE, PWM6_ENABLE);
			hwDisableClock(MT6516_PDN_PERI_PWM,"PWM6");
		}
		return 0;
	}

	return -1;
}

static int brightness_set_pmic(enum mt65xx_led_pmic pmic_type, enum led_brightness level)
{
	LEDS_INFO("LED PMIC#%d:%d\n", pmic_type, level);

	if (pmic_type == MT65XX_LED_PMIC_LCD) {
		if (level) {
			level = level<8?1:level/8;
			mt6326_bl_Enable();
			pmic_bl_dim_duty(level);
		}
		else {
			mt6326_bl_Disable();
		}
		return 0;
	}
	else if (pmic_type == MT65XX_LED_PMIC_BUTTON) {
		if (level) {
			mt6326_kpled_dim_duty_Full();
			mt6326_kpled_Enable();
		}
		else {
			mt6326_kpled_dim_duty_0();
			mt6326_kpled_Disable();
		}
		return 0;
	}

	return -1;
}

static int brightness_set_gpio(int gpio_num, enum led_brightness level)
{
	LEDS_INFO("LED GPIO#%d:%d\n", gpio_num, level);
	mt_set_gpio_mode(gpio_num, GPIO_MODE_GPIO);
	mt_set_gpio_dir(gpio_num, GPIO_DIR_OUT);

	if (level)
		mt_set_gpio_out(gpio_num, GPIO_OUT_ONE);
	else
		mt_set_gpio_out(gpio_num, GPIO_OUT_ZERO);

	return 0;
}

static int mt65xx_led_set_cust(struct cust_mt65xx_led *cust, int level)
{
	if (level > LED_FULL)
		level = LED_FULL;
	else if (level < 0)
		level = 0;

	switch (cust->mode) {
		case MT65XX_LED_MODE_PWM:
			return brightness_set_pwm(cust->data, level);
		case MT65XX_LED_MODE_GPIO:
			return brightness_set_gpio(cust->data, level);
		case MT65XX_LED_MODE_PMIC:
			return brightness_set_pmic(cust->data, level);
		case MT65XX_LED_MODE_CUST:
			return ((cust_brightness_set)(cust->data))(level);
		case MT65XX_LED_MODE_NONE:
		default:
			break;
	}
	return -1;
}

static void mt65xx_led_work(struct work_struct *work)
{
	struct mt65xx_led_data *led_data =
		container_of(work, struct mt65xx_led_data, work);

	LEDS_DEBUG("LED:%s:%d\n", led_data->cust.name, led_data->level);

	mt65xx_led_set_cust(&led_data->cust, led_data->level);
}

static void mt65xx_led_set(struct led_classdev *led_cdev, enum led_brightness level)
{
	struct mt65xx_led_data *led_data =
		container_of(led_cdev, struct mt65xx_led_data, cdev);

	// do something only when level is changed
	if (led_data->level != level) {
		led_data->level = level;
		schedule_work(&led_data->work);
	}
}

static int  mt65xx_blink_set(struct led_classdev *led_cdev,
			     unsigned long *delay_on,
			     unsigned long *delay_off)
{
	struct mt65xx_led_data *led_data =
		container_of(led_cdev, struct mt65xx_led_data, cdev);
	static int got_wake_lock = 0;

	// only allow software blink when delay_on or delay_off changed
	if (*delay_on != led_data->delay_on || *delay_off != led_data->delay_off) {
		led_data->delay_on = *delay_on;
		led_data->delay_off = *delay_off;
		if (led_data->delay_on && led_data->delay_off) { // enable blink
			if (!got_wake_lock) {
				wake_lock(&leds_suspend_lock);
				got_wake_lock = 1;
			}
		}
		else if (!led_data->delay_on && !led_data->delay_off) { // disable blink
			if (got_wake_lock) {
				wake_unlock(&leds_suspend_lock);
				got_wake_lock = 0;
			}
		}
		return -1;
	}

	// delay_on and delay_off are not changed
	return 0;
}

int mt65xx_leds_brightness_set(enum mt65xx_led_type type, enum led_brightness level)
{
	struct cust_mt65xx_led *cust_led_list = get_cust_led_list();

	LEDS_DEBUG("LED#%d:%d\n", type, level);

	if (type < 0 || type >= MT65XX_LED_TYPE_TOTAL)
		return -1;

	if (level > LED_FULL)
		level = LED_FULL;
	else if (level < 0)
		level = 0;

	return mt65xx_led_set_cust(&cust_led_list[type], level);

}
EXPORT_SYMBOL(mt65xx_leds_brightness_set);

static int __init mt65xx_leds_probe(struct platform_device *pdev)
{
	int i;
	int ret;
	struct cust_mt65xx_led *cust_led_list = get_cust_led_list();

	for (i = 0; i < MT65XX_LED_TYPE_TOTAL; i++) {
		if (cust_led_list[i].mode == MT65XX_LED_MODE_NONE) {
			g_leds_data[i] = NULL;
			continue;
		}

		g_leds_data[i] = kzalloc(sizeof(struct mt65xx_led_data), GFP_KERNEL);
		if (!g_leds_data[i]) {
			ret = -ENOMEM;
			goto err;
		}

		g_leds_data[i]->cust.mode = cust_led_list[i].mode;
		g_leds_data[i]->cust.data = cust_led_list[i].data;
		g_leds_data[i]->cust.name = cust_led_list[i].name;

		g_leds_data[i]->cdev.name = cust_led_list[i].name;

		g_leds_data[i]->cdev.brightness_set = mt65xx_led_set;
		g_leds_data[i]->cdev.blink_set = mt65xx_blink_set;

		INIT_WORK(&g_leds_data[i]->work, mt65xx_led_work);

		ret = led_classdev_register(&pdev->dev, &g_leds_data[i]->cdev);
		if (ret)
			goto err;
	}


	return 0;

err:
	if (i) {
		for (i = i-1; i >=0; i--) {
			if (!g_leds_data[i])
				continue;
			led_classdev_unregister(&g_leds_data[i]->cdev);
			cancel_work_sync(&g_leds_data[i]->work);
			kfree(g_leds_data[i]);
			g_leds_data[i] = NULL;
		}
	}

	return ret;
}

static int mt65xx_leds_remove(struct platform_device *pdev)
{
	int i;
	for (i = 0; i < MT65XX_LED_TYPE_TOTAL; i++) {
		if (!g_leds_data[i])
			continue;
		led_classdev_unregister(&g_leds_data[i]->cdev);
		cancel_work_sync(&g_leds_data[i]->work);
		kfree(g_leds_data[i]);
		g_leds_data[i] = NULL;
	}

	return 0;
}

static int mt65xx_leds_suspend(struct platform_device *dev, pm_message_t state)
{
//	mt65xx_leds_brightness_set(MT65XX_LED_TYPE_RED, LED_OFF);
//	mt65xx_leds_brightness_set(MT65XX_LED_TYPE_GREEN, LED_OFF);
//	mt65xx_leds_brightness_set(MT65XX_LED_TYPE_BLUE, LED_OFF);
	return 0;
}

static struct platform_driver mt65xx_leds_driver = {
	.driver		= {
		.name	= "leds-mt65xx",
		.owner	= THIS_MODULE,
	},
	.probe		= mt65xx_leds_probe,
	.remove		= mt65xx_leds_remove,
	.suspend	= mt65xx_leds_suspend,
};

static struct platform_device mt65xx_leds_device = {
	.name = "leds-mt65xx",
	.id = -1
};

static int __init mt65xx_leds_init(void)
{
	int ret;

	ret = platform_device_register(&mt65xx_leds_device);
	if (ret)
		printk("mt65xx_leds:dev:E%d\n", ret);

	ret = platform_driver_register(&mt65xx_leds_driver);

	if (ret)
	{
		printk("mt65xx_leds:drv:E%d\n", ret);
		platform_device_unregister(&mt65xx_leds_device);
		return ret;
	}

	wake_lock_init(&leds_suspend_lock, WAKE_LOCK_SUSPEND, "leds wakelock");

	return ret;
}

static void __exit mt65xx_leds_exit(void)
{
	platform_driver_unregister(&mt65xx_leds_driver);
	platform_device_unregister(&mt65xx_leds_device);
}

module_init(mt65xx_leds_init);
module_exit(mt65xx_leds_exit);

MODULE_AUTHOR("MediaTek Inc.");
MODULE_DESCRIPTION("LED driver for MediaTek MT65xx chip");
MODULE_LICENSE("GPL");
MODULE_ALIAS("leds-mt65xx");

