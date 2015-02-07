

#include <linux/module.h>

#include <asm/mach-au1x00/au1000.h>
#include <asm/gpio.h>

#define gpio1 sys
#if !defined(CONFIG_SOC_AU1000)

static struct au1x00_gpio2 *const gpio2 = (struct au1x00_gpio2 *) GPIO2_BASE;
#define GPIO2_OUTPUT_ENABLE_MASK 	0x00010000

static int au1xxx_gpio2_read(unsigned gpio)
{
	gpio -= AU1XXX_GPIO_BASE;
	return ((gpio2->pinstate >> gpio) & 0x01);
}

static void au1xxx_gpio2_write(unsigned gpio, int value)
{
	gpio -= AU1XXX_GPIO_BASE;

	gpio2->output = (GPIO2_OUTPUT_ENABLE_MASK << gpio) | ((!!value) << gpio);
}

static int au1xxx_gpio2_direction_input(unsigned gpio)
{
	gpio -= AU1XXX_GPIO_BASE;
	gpio2->dir &= ~(0x01 << gpio);
	return 0;
}

static int au1xxx_gpio2_direction_output(unsigned gpio, int value)
{
	gpio -= AU1XXX_GPIO_BASE;
	gpio2->dir |= 0x01 << gpio;
	gpio2->output = (GPIO2_OUTPUT_ENABLE_MASK << gpio) | ((!!value) << gpio);
	return 0;
}

#endif /* !defined(CONFIG_SOC_AU1000) */

static int au1xxx_gpio1_read(unsigned gpio)
{
	return (gpio1->pinstaterd >> gpio) & 0x01;
}

static void au1xxx_gpio1_write(unsigned gpio, int value)
{
	if (value)
		gpio1->outputset = (0x01 << gpio);
	else
		/* Output a zero */
		gpio1->outputclr = (0x01 << gpio);
}

static int au1xxx_gpio1_direction_input(unsigned gpio)
{
	gpio1->pininputen = (0x01 << gpio);
	return 0;
}

static int au1xxx_gpio1_direction_output(unsigned gpio, int value)
{
	gpio1->trioutclr = (0x01 & gpio);
	au1xxx_gpio1_write(gpio, value);
	return 0;
}

int au1xxx_gpio_get_value(unsigned gpio)
{
	if (gpio >= AU1XXX_GPIO_BASE)
#if defined(CONFIG_SOC_AU1000)
		return 0;
#else
		return au1xxx_gpio2_read(gpio);
#endif
	else
		return au1xxx_gpio1_read(gpio);
}
EXPORT_SYMBOL(au1xxx_gpio_get_value);

void au1xxx_gpio_set_value(unsigned gpio, int value)
{
	if (gpio >= AU1XXX_GPIO_BASE)
#if defined(CONFIG_SOC_AU1000)
		;
#else
		au1xxx_gpio2_write(gpio, value);
#endif
	else
		au1xxx_gpio1_write(gpio, value);
}
EXPORT_SYMBOL(au1xxx_gpio_set_value);

int au1xxx_gpio_direction_input(unsigned gpio)
{
	if (gpio >= AU1XXX_GPIO_BASE)
#if defined(CONFIG_SOC_AU1000)
		return -ENODEV;
#else
		return au1xxx_gpio2_direction_input(gpio);
#endif

	return au1xxx_gpio1_direction_input(gpio);
}
EXPORT_SYMBOL(au1xxx_gpio_direction_input);

int au1xxx_gpio_direction_output(unsigned gpio, int value)
{
	if (gpio >= AU1XXX_GPIO_BASE)
#if defined(CONFIG_SOC_AU1000)
		return -ENODEV;
#else
		return au1xxx_gpio2_direction_output(gpio, value);
#endif

	return au1xxx_gpio1_direction_output(gpio, value);
}
EXPORT_SYMBOL(au1xxx_gpio_direction_output);
