

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/leds.h>

#include <asm/gpio.h>

/* LEDS */
static struct gpio_led default_leds[] = {
	{ .name = "rdc:dmz", .gpio = 1, },
};

static struct gpio_led_platform_data rdc321x_led_data = {
	.num_leds = ARRAY_SIZE(default_leds),
	.leds = default_leds,
};

static struct platform_device rdc321x_leds = {
	.name = "leds-gpio",
	.id = -1,
	.dev = {
		.platform_data = &rdc321x_led_data,
	}
};

/* Watchdog */
static struct platform_device rdc321x_wdt = {
	.name = "rdc321x-wdt",
	.id = -1,
	.num_resources = 0,
};

static struct platform_device *rdc321x_devs[] = {
	&rdc321x_leds,
	&rdc321x_wdt
};

static int __init rdc_board_setup(void)
{
	rdc321x_gpio_setup();

	return platform_add_devices(rdc321x_devs, ARRAY_SIZE(rdc321x_devs));
}

arch_initcall(rdc_board_setup);
