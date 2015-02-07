

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/sysdev.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/gpio_keys.h>
#include <linux/pwm_backlight.h>
#include <linux/rtc.h>
#include <linux/leds.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/pda_power.h>
#include <linux/power_supply.h>
#include <linux/wm97xx_batt.h>
#include <linux/mtd/physmap.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <mach/mfp-pxa27x.h>
#include <mach/pxa27x_keypad.h>
#include <mach/pxafb.h>
#include <mach/pxa2xx-regs.h>
#include <mach/mmc.h>
#include <mach/udc.h>
#include <mach/pxa27x-udc.h>
#include <mach/i2c.h>
#include <mach/camera.h>
#include <media/soc_camera.h>

#include <mach/mioa701.h>

#include "generic.h"
#include "devices.h"

static unsigned long mioa701_pin_config[] = {
	/* Mio global */
	MIO_CFG_OUT(GPIO9_CHARGE_EN, AF0, DRIVE_LOW),
	MIO_CFG_OUT(GPIO18_POWEROFF, AF0, DRIVE_LOW),
	MFP_CFG_OUT(GPIO3, AF0, DRIVE_HIGH),
	MFP_CFG_OUT(GPIO4, AF0, DRIVE_HIGH),
	MIO_CFG_IN(GPIO80_MAYBE_CHARGE_VDROP, AF0),

	/* Backlight PWM 0 */
	GPIO16_PWM0_OUT,

	/* MMC */
	GPIO32_MMC_CLK,
	GPIO92_MMC_DAT_0,
	GPIO109_MMC_DAT_1,
	GPIO110_MMC_DAT_2,
	GPIO111_MMC_DAT_3,
	GPIO112_MMC_CMD,
	MIO_CFG_IN(GPIO78_SDIO_RO, AF0),
	MIO_CFG_IN(GPIO15_SDIO_INSERT, AF0),
	MIO_CFG_OUT(GPIO91_SDIO_EN, AF0, DRIVE_LOW),

	/* USB */
	MIO_CFG_IN(GPIO13_nUSB_DETECT, AF0),
	MIO_CFG_OUT(GPIO22_USB_ENABLE, AF0, DRIVE_LOW),

	/* LCD */
	GPIO58_LCD_LDD_0,
	GPIO59_LCD_LDD_1,
	GPIO60_LCD_LDD_2,
	GPIO61_LCD_LDD_3,
	GPIO62_LCD_LDD_4,
	GPIO63_LCD_LDD_5,
	GPIO64_LCD_LDD_6,
	GPIO65_LCD_LDD_7,
	GPIO66_LCD_LDD_8,
	GPIO67_LCD_LDD_9,
	GPIO68_LCD_LDD_10,
	GPIO69_LCD_LDD_11,
	GPIO70_LCD_LDD_12,
	GPIO71_LCD_LDD_13,
	GPIO72_LCD_LDD_14,
	GPIO73_LCD_LDD_15,
	GPIO74_LCD_FCLK,
	GPIO75_LCD_LCLK,
	GPIO76_LCD_PCLK,

	/* QCI */
	GPIO12_CIF_DD_7,
	GPIO17_CIF_DD_6,
	GPIO50_CIF_DD_3,
	GPIO51_CIF_DD_2,
	GPIO52_CIF_DD_4,
	GPIO53_CIF_MCLK,
	GPIO54_CIF_PCLK,
	GPIO55_CIF_DD_1,
	GPIO81_CIF_DD_0,
	GPIO82_CIF_DD_5,
	GPIO84_CIF_FV,
	GPIO85_CIF_LV,

	/* Bluetooth */
	MIO_CFG_IN(GPIO14_BT_nACTIVITY, AF0),
	GPIO44_BTUART_CTS,
	GPIO42_BTUART_RXD,
	GPIO45_BTUART_RTS,
	GPIO43_BTUART_TXD,
	MIO_CFG_OUT(GPIO83_BT_ON, AF0, DRIVE_LOW),
	MIO_CFG_OUT(GPIO77_BT_UNKNOWN1, AF0, DRIVE_HIGH),
	MIO_CFG_OUT(GPIO86_BT_MAYBE_nRESET, AF0, DRIVE_HIGH),

	/* GPS */
	MIO_CFG_OUT(GPIO23_GPS_UNKNOWN1, AF0, DRIVE_LOW),
	MIO_CFG_OUT(GPIO26_GPS_ON, AF0, DRIVE_LOW),
	MIO_CFG_OUT(GPIO27_GPS_RESET, AF0, DRIVE_LOW),
	MIO_CFG_OUT(GPIO106_GPS_UNKNOWN2, AF0, DRIVE_LOW),
	MIO_CFG_OUT(GPIO107_GPS_UNKNOWN3, AF0, DRIVE_LOW),
	GPIO46_STUART_RXD,
	GPIO47_STUART_TXD,

	/* GSM */
	MIO_CFG_OUT(GPIO24_GSM_MOD_RESET_CMD, AF0, DRIVE_LOW),
	MIO_CFG_OUT(GPIO88_GSM_nMOD_ON_CMD, AF0, DRIVE_HIGH),
	MIO_CFG_OUT(GPIO90_GSM_nMOD_OFF_CMD, AF0, DRIVE_HIGH),
	MIO_CFG_OUT(GPIO114_GSM_nMOD_DTE_UART_STATE, AF0, DRIVE_HIGH),
	MIO_CFG_IN(GPIO25_GSM_MOD_ON_STATE, AF0),
	MIO_CFG_IN(GPIO113_GSM_EVENT, AF0) | WAKEUP_ON_EDGE_BOTH,
	GPIO34_FFUART_RXD,
	GPIO35_FFUART_CTS,
	GPIO36_FFUART_DCD,
	GPIO37_FFUART_DSR,
	GPIO39_FFUART_TXD,
	GPIO40_FFUART_DTR,
	GPIO41_FFUART_RTS,

	/* Sound */
	GPIO89_AC97_SYSCLK,
	MIO_CFG_IN(GPIO12_HPJACK_INSERT, AF0),

	/* Leds */
	MIO_CFG_OUT(GPIO10_LED_nCharging, AF0, DRIVE_HIGH),
	MIO_CFG_OUT(GPIO97_LED_nBlue, AF0, DRIVE_HIGH),
	MIO_CFG_OUT(GPIO98_LED_nOrange, AF0, DRIVE_HIGH),
	MIO_CFG_OUT(GPIO82_LED_nVibra, AF0, DRIVE_HIGH),
	MIO_CFG_OUT(GPIO115_LED_nKeyboard, AF0, DRIVE_HIGH),

	/* Keyboard */
	MIO_CFG_IN(GPIO0_KEY_POWER, AF0) | WAKEUP_ON_EDGE_BOTH,
	MIO_CFG_IN(GPIO93_KEY_VOLUME_UP, AF0),
	MIO_CFG_IN(GPIO94_KEY_VOLUME_DOWN, AF0),
	GPIO100_KP_MKIN_0,
	GPIO101_KP_MKIN_1,
	GPIO102_KP_MKIN_2,
	GPIO103_KP_MKOUT_0,
	GPIO104_KP_MKOUT_1,
	GPIO105_KP_MKOUT_2,

	/* I2C */
	GPIO117_I2C_SCL,
	GPIO118_I2C_SDA,

	/* Unknown */
	MFP_CFG_IN(GPIO20, AF0),
	MFP_CFG_IN(GPIO21, AF0),
	MFP_CFG_IN(GPIO33, AF0),
	MFP_CFG_OUT(GPIO49, AF0, DRIVE_HIGH),
	MFP_CFG_OUT(GPIO57, AF0, DRIVE_HIGH),
	MFP_CFG_IN(GPIO96, AF0),
	MFP_CFG_OUT(GPIO116, AF0, DRIVE_HIGH),
};

#define MIO_GPIO_IN(num, _desc) \
	{ .gpio = (num), .dir = 0, .desc = (_desc) }
#define MIO_GPIO_OUT(num, _init, _desc) \
	{ .gpio = (num), .dir = 1, .init = (_init), .desc = (_desc) }
struct gpio_ress {
	unsigned gpio : 8;
	unsigned dir : 1;
	unsigned init : 1;
	char *desc;
};

static int mio_gpio_request(struct gpio_ress *gpios, int size)
{
	int i, rc = 0;
	int gpio;
	int dir;

	for (i = 0; (!rc) && (i < size); i++) {
		gpio = gpios[i].gpio;
		dir = gpios[i].dir;
		rc = gpio_request(gpio, gpios[i].desc);
		if (rc) {
			printk(KERN_ERR "Error requesting GPIO %d(%s) : %d\n",
			       gpio, gpios[i].desc, rc);
			continue;
		}
		if (dir)
			gpio_direction_output(gpio, gpios[i].init);
		else
			gpio_direction_input(gpio);
	}
	while ((rc) && (--i >= 0))
		gpio_free(gpios[i].gpio);
	return rc;
}

static void mio_gpio_free(struct gpio_ress *gpios, int size)
{
	int i;

	for (i = 0; i < size; i++)
		gpio_free(gpios[i].gpio);
}

/* LCD Screen and Backlight */
static struct platform_pwm_backlight_data mioa701_backlight_data = {
	.pwm_id		= 0,
	.max_brightness	= 100,
	.dft_brightness	= 50,
	.pwm_period_ns	= 4000 * 1024,	/* Fl = 250kHz */
};

static struct pxafb_mode_info mioa701_ltm0305a776c = {
	.pixclock		= 220000,	/* CLK=4.545 MHz */
	.xres			= 240,
	.yres			= 320,
	.bpp			= 16,
	.hsync_len		= 4,
	.vsync_len		= 2,
	.left_margin		= 6,
	.right_margin		= 4,
	.upper_margin		= 5,
	.lower_margin		= 3,
};

static void mioa701_lcd_power(int on, struct fb_var_screeninfo *si)
{
	gpio_set_value(GPIO87_LCD_POWER, on);
}

static struct pxafb_mach_info mioa701_pxafb_info = {
	.modes			= &mioa701_ltm0305a776c,
	.num_modes		= 1,
	.lcd_conn		= LCD_COLOR_TFT_16BPP | LCD_PCLK_EDGE_FALL,
	.pxafb_lcd_power	= mioa701_lcd_power,
};

static unsigned int mioa701_matrix_keys[] = {
	KEY(0, 0, KEY_UP),
	KEY(0, 1, KEY_RIGHT),
	KEY(0, 2, KEY_MEDIA),
	KEY(1, 0, KEY_DOWN),
	KEY(1, 1, KEY_ENTER),
	KEY(1, 2, KEY_CONNECT),	/* GPS key */
	KEY(2, 0, KEY_LEFT),
	KEY(2, 1, KEY_PHONE),	/* Phone Green key */
	KEY(2, 2, KEY_CAMERA)	/* Camera key */
};
static struct pxa27x_keypad_platform_data mioa701_keypad_info = {
	.matrix_key_rows = 3,
	.matrix_key_cols = 3,
	.matrix_key_map = mioa701_matrix_keys,
	.matrix_key_map_size = ARRAY_SIZE(mioa701_matrix_keys),
};

#define MIO_KEY(key, _gpio, _desc, _wakeup) \
	{ .code = (key), .gpio = (_gpio), .active_low = 0, \
	.desc = (_desc), .type = EV_KEY, .wakeup = (_wakeup) }
static struct gpio_keys_button mioa701_button_table[] = {
	MIO_KEY(KEY_EXIT, GPIO0_KEY_POWER, "Power button", 1),
	MIO_KEY(KEY_VOLUMEUP, GPIO93_KEY_VOLUME_UP, "Volume up", 0),
	MIO_KEY(KEY_VOLUMEDOWN, GPIO94_KEY_VOLUME_DOWN, "Volume down", 0),
	MIO_KEY(KEY_HP, GPIO12_HPJACK_INSERT, "HP jack detect", 0)
};

static struct gpio_keys_platform_data mioa701_gpio_keys_data = {
	.buttons  = mioa701_button_table,
	.nbuttons = ARRAY_SIZE(mioa701_button_table),
};

#define ONE_LED(_gpio, _name) \
{ .gpio = (_gpio), .name = (_name), .active_low = true }
static struct gpio_led gpio_leds[] = {
	ONE_LED(GPIO10_LED_nCharging, "mioa701:charging"),
	ONE_LED(GPIO97_LED_nBlue, "mioa701:blue"),
	ONE_LED(GPIO98_LED_nOrange, "mioa701:orange"),
	ONE_LED(GPIO82_LED_nVibra, "mioa701:vibra"),
	ONE_LED(GPIO115_LED_nKeyboard, "mioa701:keyboard")
};

static struct gpio_led_platform_data gpio_led_info = {
	.leds = gpio_leds,
	.num_leds = ARRAY_SIZE(gpio_leds),
};

static int is_gsm_on(void)
{
	int is_on;

	is_on = !!gpio_get_value(GPIO25_GSM_MOD_ON_STATE);
	return is_on;
}

irqreturn_t gsm_on_irq(int irq, void *p)
{
	printk(KERN_DEBUG "Mioa701: GSM status changed to %s\n",
	       is_gsm_on() ? "on" : "off");
	return IRQ_HANDLED;
}

struct gpio_ress gsm_gpios[] = {
	MIO_GPIO_IN(GPIO25_GSM_MOD_ON_STATE, "GSM state"),
	MIO_GPIO_IN(GPIO113_GSM_EVENT, "GSM event"),
};

static int __init gsm_init(void)
{
	int rc;

	rc = mio_gpio_request(ARRAY_AND_SIZE(gsm_gpios));
	if (rc)
		goto err_gpio;
	rc = request_irq(gpio_to_irq(GPIO25_GSM_MOD_ON_STATE), gsm_on_irq,
			 IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
			 "GSM XS200 Power Irq", NULL);
	if (rc)
		goto err_irq;

	gpio_set_wake(GPIO113_GSM_EVENT, 1);
	return 0;

err_irq:
	printk(KERN_ERR "Mioa701: Can't request GSM_ON irq\n");
	mio_gpio_free(ARRAY_AND_SIZE(gsm_gpios));
err_gpio:
	printk(KERN_ERR "Mioa701: gsm not available\n");
	return rc;
}

static void gsm_exit(void)
{
	free_irq(gpio_to_irq(GPIO25_GSM_MOD_ON_STATE), NULL);
	mio_gpio_free(ARRAY_AND_SIZE(gsm_gpios));
}



static void udc_power_command(int cmd)
{
	switch (cmd) {
	case PXA2XX_UDC_CMD_DISCONNECT:
		gpio_set_value(GPIO22_USB_ENABLE, 0);
		break;
	case PXA2XX_UDC_CMD_CONNECT:
		gpio_set_value(GPIO22_USB_ENABLE, 1);
		break;
	default:
		printk(KERN_INFO "udc_control: unknown command (0x%x)!\n", cmd);
		break;
	}
}

static int is_usb_connected(void)
{
	return !gpio_get_value(GPIO13_nUSB_DETECT);
}

static struct pxa2xx_udc_mach_info mioa701_udc_info = {
	.udc_is_connected = is_usb_connected,
	.udc_command	  = udc_power_command,
};

struct gpio_ress udc_gpios[] = {
	MIO_GPIO_OUT(GPIO22_USB_ENABLE, 0, "USB Vbus enable")
};

static int __init udc_init(void)
{
	pxa_set_udc_info(&mioa701_udc_info);
	return mio_gpio_request(ARRAY_AND_SIZE(udc_gpios));
}

static void udc_exit(void)
{
	mio_gpio_free(ARRAY_AND_SIZE(udc_gpios));
}

static void mci_setpower(struct device *dev, unsigned int vdd)
{
	struct pxamci_platform_data *p_d = dev->platform_data;

	if ((1 << vdd) & p_d->ocr_mask)
		gpio_set_value(GPIO91_SDIO_EN, 1);	/* enable SDIO power */
	else
		gpio_set_value(GPIO91_SDIO_EN, 0);	/* disable SDIO power */
}

static int mci_get_ro(struct device *dev)
{
	return gpio_get_value(GPIO78_SDIO_RO);
}

struct gpio_ress mci_gpios[] = {
	MIO_GPIO_IN(GPIO78_SDIO_RO, 	"SDIO readonly detect"),
	MIO_GPIO_IN(GPIO15_SDIO_INSERT,	"SDIO insertion detect"),
	MIO_GPIO_OUT(GPIO91_SDIO_EN, 0,	"SDIO power enable")
};

static void mci_exit(struct device *dev, void *data)
{
	mio_gpio_free(ARRAY_AND_SIZE(mci_gpios));
	free_irq(gpio_to_irq(GPIO15_SDIO_INSERT), data);
}

static struct pxamci_platform_data mioa701_mci_info;

static int mci_init(struct device *dev, irq_handler_t detect_int, void *data)
{
	int rc;
	int irq = gpio_to_irq(GPIO15_SDIO_INSERT);

	rc = mio_gpio_request(ARRAY_AND_SIZE(mci_gpios));
	if (rc)
		goto err_gpio;
	/* enable RE/FE interrupt on card insertion and removal */
	rc = request_irq(irq, detect_int,
			 IRQF_DISABLED | IRQF_TRIGGER_RISING |
			 IRQF_TRIGGER_FALLING,
			 "MMC card detect", data);
	if (rc)
		goto err_irq;

	mioa701_mci_info.detect_delay = msecs_to_jiffies(250);
	return 0;

err_irq:
	dev_err(dev, "mioa701_mci_init: MMC/SD:"
		" can't request MMC card detect IRQ\n");
	mio_gpio_free(ARRAY_AND_SIZE(mci_gpios));
err_gpio:
	return rc;
}

static struct pxamci_platform_data mioa701_mci_info = {
	.ocr_mask = MMC_VDD_32_33 | MMC_VDD_33_34,
	.init	  = mci_init,
	.get_ro	  = mci_get_ro,
	.setpower = mci_setpower,
	.exit	  = mci_exit,
};

/* FlashRAM */
static struct resource strataflash_resource = {
	.start = PXA_CS0_PHYS,
	.end   = PXA_CS0_PHYS + SZ_64M - 1,
	.flags = IORESOURCE_MEM,
};

static struct physmap_flash_data strataflash_data = {
	.width = 2,
	/* .set_vpp = mioa701_set_vpp, */
};

static struct platform_device strataflash = {
	.name	       = "physmap-flash",
	.id	       = -1,
	.resource      = &strataflash_resource,
	.num_resources = 1,
	.dev = {
		.platform_data = &strataflash_data,
	},
};

#define RESUME_ENABLE_ADDR	0xa020b000
#define RESUME_ENABLE_VAL	0x0f0f0f0f
#define RESUME_BT_ADDR		0xa020b020
#define RESUME_UNKNOWN_ADDR	0xa020b024
#define RESUME_VECTOR_ADDR	0xa0100000
#define BOOTSTRAP_WORDS		mioa701_bootstrap_lg/4

static u32 *save_buffer;

static void install_bootstrap(void)
{
	int i;
	u32 *rom_bootstrap  = phys_to_virt(RESUME_VECTOR_ADDR);
	u32 *src = &mioa701_bootstrap;

	for (i = 0; i < BOOTSTRAP_WORDS; i++)
		rom_bootstrap[i] = src[i];
}


static int mioa701_sys_suspend(struct sys_device *sysdev, pm_message_t state)
{
	int i = 0, is_bt_on;
	u32 *mem_resume_vector	= phys_to_virt(RESUME_VECTOR_ADDR);
	u32 *mem_resume_enabler = phys_to_virt(RESUME_ENABLE_ADDR);
	u32 *mem_resume_bt	= phys_to_virt(RESUME_BT_ADDR);
	u32 *mem_resume_unknown	= phys_to_virt(RESUME_UNKNOWN_ADDR);

	/* Devices prepare suspend */
	is_bt_on = !!gpio_get_value(GPIO83_BT_ON);
	pxa2xx_mfp_set_lpm(GPIO83_BT_ON,
			   is_bt_on ? MFP_LPM_DRIVE_HIGH : MFP_LPM_DRIVE_LOW);

	for (i = 0; i < BOOTSTRAP_WORDS; i++)
		save_buffer[i] = mem_resume_vector[i];
	save_buffer[i++] = *mem_resume_enabler;
	save_buffer[i++] = *mem_resume_bt;
	save_buffer[i++] = *mem_resume_unknown;

	*mem_resume_enabler = RESUME_ENABLE_VAL;
	*mem_resume_bt	    = is_bt_on;

	install_bootstrap();
	return 0;
}

static int mioa701_sys_resume(struct sys_device *sysdev)
{
	int i = 0;
	u32 *mem_resume_vector	= phys_to_virt(RESUME_VECTOR_ADDR);
	u32 *mem_resume_enabler = phys_to_virt(RESUME_ENABLE_ADDR);
	u32 *mem_resume_bt	= phys_to_virt(RESUME_BT_ADDR);
	u32 *mem_resume_unknown	= phys_to_virt(RESUME_UNKNOWN_ADDR);

	for (i = 0; i < BOOTSTRAP_WORDS; i++)
		mem_resume_vector[i] = save_buffer[i];
	*mem_resume_enabler = save_buffer[i++];
	*mem_resume_bt	    = save_buffer[i++];
	*mem_resume_unknown = save_buffer[i++];

	return 0;
}

static struct sysdev_class mioa701_sysclass = {
	.name = "mioa701",
};

static struct sys_device sysdev_bootstrap = {
	.cls		= &mioa701_sysclass,
};

static struct sysdev_driver driver_bootstrap = {
	.suspend	= &mioa701_sys_suspend,
	.resume		= &mioa701_sys_resume,
};

static int __init bootstrap_init(void)
{
	int rc;
	int save_size = mioa701_bootstrap_lg + (sizeof(u32) * 3);

	rc = sysdev_class_register(&mioa701_sysclass);
	if (rc) {
		printk(KERN_ERR "Failed registering mioa701 sys class\n");
		return -ENODEV;
	}
	rc = sysdev_register(&sysdev_bootstrap);
	if (rc) {
		printk(KERN_ERR "Failed registering mioa701 sys device\n");
		return -ENODEV;
	}
	rc = sysdev_driver_register(&mioa701_sysclass, &driver_bootstrap);
	if (rc) {
		printk(KERN_ERR "Failed registering PMU sys driver\n");
		return -ENODEV;
	}

	save_buffer = kmalloc(save_size, GFP_KERNEL);
	if (!save_buffer)
		return -ENOMEM;
	printk(KERN_INFO "MioA701: allocated %d bytes for bootstrap\n",
	       save_size);
	return 0;
}

static void bootstrap_exit(void)
{
	kfree(save_buffer);
	sysdev_driver_unregister(&mioa701_sysclass, &driver_bootstrap);
	sysdev_unregister(&sysdev_bootstrap);
	sysdev_class_unregister(&mioa701_sysclass);

	printk(KERN_CRIT "Unregistering mioa701 suspend will hang next"
	       "resume !!!\n");
}

static char *supplicants[] = {
	"mioa701_battery"
};

static int is_ac_connected(void)
{
	return gpio_get_value(GPIO96_AC_DETECT);
}

static void mioa701_set_charge(int flags)
{
	gpio_set_value(GPIO9_CHARGE_EN, (flags == PDA_POWER_CHARGE_USB));
}

static struct pda_power_pdata power_pdata = {
	.is_ac_online	= is_ac_connected,
	.is_usb_online	= is_usb_connected,
	.set_charge = mioa701_set_charge,
	.supplied_to = supplicants,
	.num_supplicants = ARRAY_SIZE(supplicants),
};

static struct resource power_resources[] = {
	[0] = {
		.name	= "ac",
		.start	= gpio_to_irq(GPIO96_AC_DETECT),
		.end	= gpio_to_irq(GPIO96_AC_DETECT),
		.flags	= IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHEDGE |
		IORESOURCE_IRQ_LOWEDGE,
	},
	[1] = {
		.name	= "usb",
		.start	= gpio_to_irq(GPIO13_nUSB_DETECT),
		.end	= gpio_to_irq(GPIO13_nUSB_DETECT),
		.flags	= IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHEDGE |
		IORESOURCE_IRQ_LOWEDGE,
	},
};

static struct platform_device power_dev = {
	.name		= "pda-power",
	.id		= -1,
	.resource	= power_resources,
	.num_resources	= ARRAY_SIZE(power_resources),
	.dev = {
		.platform_data	= &power_pdata,
	},
};

static struct wm97xx_batt_info mioa701_battery_data = {
	.batt_aux	= WM97XX_AUX_ID1,
	.temp_aux	= -1,
	.charge_gpio	= -1,
	.min_voltage	= 0xc00,
	.max_voltage	= 0xfc0,
	.batt_tech	= POWER_SUPPLY_TECHNOLOGY_LION,
	.batt_div	= 1,
	.batt_mult	= 1,
	.batt_name	= "mioa701_battery",
};

struct pxacamera_platform_data mioa701_pxacamera_platform_data = {
	.flags  = PXA_CAMERA_MASTER | PXA_CAMERA_DATAWIDTH_8 |
		PXA_CAMERA_PCLK_EN | PXA_CAMERA_MCLK_EN,
	.mclk_10khz = 5000,
};

static struct soc_camera_link iclink = {
	.bus_id	= 0, /* Must match id in pxa27x_device_camera in device.c */
};

/* Board I2C devices. */
static struct i2c_board_info __initdata mioa701_i2c_devices[] = {
	{
		/* Must initialize before the camera(s) */
		I2C_BOARD_INFO("mt9m111", 0x5d),
		.platform_data = &iclink,
	},
};

struct i2c_pxa_platform_data i2c_pdata = {
	.fast_mode = 1,
};


/* Devices */
#define MIO_PARENT_DEV(var, strname, tparent, pdata)	\
static struct platform_device var = {			\
	.name		= strname,			\
	.id		= -1,				\
	.dev		= {				\
		.platform_data = pdata,			\
		.parent	= tparent,			\
	},						\
};
#define MIO_SIMPLE_DEV(var, strname, pdata)	\
	MIO_PARENT_DEV(var, strname, NULL, pdata)

MIO_SIMPLE_DEV(mioa701_gpio_keys, "gpio-keys",	    &mioa701_gpio_keys_data)
MIO_PARENT_DEV(mioa701_backlight, "pwm-backlight",  &pxa27x_device_pwm0.dev,
		&mioa701_backlight_data);
MIO_SIMPLE_DEV(mioa701_led,	  "leds-gpio",	    &gpio_led_info)
MIO_SIMPLE_DEV(pxa2xx_pcm,	  "pxa2xx-pcm",	    NULL)
MIO_SIMPLE_DEV(pxa2xx_ac97,	  "pxa2xx-ac97",    NULL)
MIO_PARENT_DEV(mio_wm9713_codec,  "wm9713-codec",   &pxa2xx_ac97.dev, NULL)
MIO_SIMPLE_DEV(mioa701_sound,	  "mioa701-wm9713", NULL)
MIO_SIMPLE_DEV(mioa701_board,	  "mioa701-board",  NULL)

static struct platform_device *devices[] __initdata = {
	&mioa701_gpio_keys,
	&mioa701_backlight,
	&mioa701_led,
	&pxa2xx_pcm,
	&pxa2xx_ac97,
	&mio_wm9713_codec,
	&mioa701_sound,
	&power_dev,
	&strataflash,
	&mioa701_board
};

static void mioa701_machine_exit(void);

static void mioa701_poweroff(void)
{
	mioa701_machine_exit();
	arm_machine_restart('s');
}

static void mioa701_restart(char c)
{
	mioa701_machine_exit();
	arm_machine_restart('s');
}

struct gpio_ress global_gpios[] = {
	MIO_GPIO_OUT(GPIO9_CHARGE_EN, 1, "Charger enable"),
	MIO_GPIO_OUT(GPIO18_POWEROFF, 0, "Power Off"),
	MIO_GPIO_OUT(GPIO87_LCD_POWER, 0, "LCD Power")
};

static void __init mioa701_machine_init(void)
{
	PSLR  = 0xff100000; /* SYSDEL=125ms, PWRDEL=125ms, PSLR_SL_ROD=1 */
	PCFR = PCFR_DC_EN | PCFR_GPR_EN | PCFR_OPDE;
	RTTR = 32768 - 1; /* Reset crazy WinCE value */
	UP2OCR = UP2OCR_HXOE;

	pxa2xx_mfp_config(ARRAY_AND_SIZE(mioa701_pin_config));
	mio_gpio_request(ARRAY_AND_SIZE(global_gpios));
	bootstrap_init();
	set_pxa_fb_info(&mioa701_pxafb_info);
	pxa_set_mci_info(&mioa701_mci_info);
	pxa_set_keypad_info(&mioa701_keypad_info);
	wm97xx_bat_set_pdata(&mioa701_battery_data);
	udc_init();
	pm_power_off = mioa701_poweroff;
	arm_pm_restart = mioa701_restart;
	platform_add_devices(devices, ARRAY_SIZE(devices));
	gsm_init();

	pxa_set_i2c_info(&i2c_pdata);
	pxa_set_camera_info(&mioa701_pxacamera_platform_data);
	i2c_register_board_info(0, ARRAY_AND_SIZE(mioa701_i2c_devices));
}

static void mioa701_machine_exit(void)
{
	udc_exit();
	bootstrap_exit();
	gsm_exit();
}

MACHINE_START(MIOA701, "MIO A701")
	.phys_io	= 0x40000000,
	.io_pg_offst	= (io_p2v(0x40000000) >> 18) & 0xfffc,
	.boot_params	= 0xa0000100,
	.map_io		= &pxa_map_io,
	.init_irq	= &pxa27x_init_irq,
	.init_machine	= mioa701_machine_init,
	.timer		= &pxa_timer,
MACHINE_END
