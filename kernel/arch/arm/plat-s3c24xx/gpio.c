


#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/io.h>

#include <mach/hardware.h>
#include <asm/irq.h>

#include <mach/regs-gpio.h>

void s3c2410_gpio_cfgpin(unsigned int pin, unsigned int function)
{
	void __iomem *base = S3C24XX_GPIO_BASE(pin);
	unsigned long mask;
	unsigned long con;
	unsigned long flags;

	if (pin < S3C2410_GPIO_BANKB) {
		mask = 1 << S3C2410_GPIO_OFFSET(pin);
	} else {
		mask = 3 << S3C2410_GPIO_OFFSET(pin)*2;
	}

	switch (function) {
	case S3C2410_GPIO_LEAVE:
		mask = 0;
		function = 0;
		break;

	case S3C2410_GPIO_INPUT:
	case S3C2410_GPIO_OUTPUT:
	case S3C2410_GPIO_SFN2:
	case S3C2410_GPIO_SFN3:
		if (pin < S3C2410_GPIO_BANKB) {
			function -= 1;
			function &= 1;
			function <<= S3C2410_GPIO_OFFSET(pin);
		} else {
			function &= 3;
			function <<= S3C2410_GPIO_OFFSET(pin)*2;
		}
	}

	/* modify the specified register wwith IRQs off */

	local_irq_save(flags);

	con  = __raw_readl(base + 0x00);
	con &= ~mask;
	con |= function;

	__raw_writel(con, base + 0x00);

	local_irq_restore(flags);
}

EXPORT_SYMBOL(s3c2410_gpio_cfgpin);

unsigned int s3c2410_gpio_getcfg(unsigned int pin)
{
	void __iomem *base = S3C24XX_GPIO_BASE(pin);
	unsigned long val = __raw_readl(base);

	if (pin < S3C2410_GPIO_BANKB) {
		val >>= S3C2410_GPIO_OFFSET(pin);
		val &= 1;
		val += 1;
	} else {
		val >>= S3C2410_GPIO_OFFSET(pin)*2;
		val &= 3;
	}

	return val | S3C2410_GPIO_INPUT;
}

EXPORT_SYMBOL(s3c2410_gpio_getcfg);

void s3c2410_gpio_pullup(unsigned int pin, unsigned int to)
{
	void __iomem *base = S3C24XX_GPIO_BASE(pin);
	unsigned long offs = S3C2410_GPIO_OFFSET(pin);
	unsigned long flags;
	unsigned long up;

	if (pin < S3C2410_GPIO_BANKB)
		return;

	local_irq_save(flags);

	up = __raw_readl(base + 0x08);
	up &= ~(1L << offs);
	up |= to << offs;
	__raw_writel(up, base + 0x08);

	local_irq_restore(flags);
}

EXPORT_SYMBOL(s3c2410_gpio_pullup);

int s3c2410_gpio_getpull(unsigned int pin)
{
	void __iomem *base = S3C24XX_GPIO_BASE(pin);
	unsigned long offs = S3C2410_GPIO_OFFSET(pin);

	if (pin < S3C2410_GPIO_BANKB)
		return -EINVAL;

	return (__raw_readl(base + 0x08) & (1L << offs)) ? 1 : 0;
}

EXPORT_SYMBOL(s3c2410_gpio_getpull);

void s3c2410_gpio_setpin(unsigned int pin, unsigned int to)
{
	void __iomem *base = S3C24XX_GPIO_BASE(pin);
	unsigned long offs = S3C2410_GPIO_OFFSET(pin);
	unsigned long flags;
	unsigned long dat;

	local_irq_save(flags);

	dat = __raw_readl(base + 0x04);
	dat &= ~(1 << offs);
	dat |= to << offs;
	__raw_writel(dat, base + 0x04);

	local_irq_restore(flags);
}

EXPORT_SYMBOL(s3c2410_gpio_setpin);

unsigned int s3c2410_gpio_getpin(unsigned int pin)
{
	void __iomem *base = S3C24XX_GPIO_BASE(pin);
	unsigned long offs = S3C2410_GPIO_OFFSET(pin);

	return __raw_readl(base + 0x04) & (1<< offs);
}

EXPORT_SYMBOL(s3c2410_gpio_getpin);

unsigned int s3c2410_modify_misccr(unsigned int clear, unsigned int change)
{
	unsigned long flags;
	unsigned long misccr;

	local_irq_save(flags);
	misccr = __raw_readl(S3C24XX_MISCCR);
	misccr &= ~clear;
	misccr ^= change;
	__raw_writel(misccr, S3C24XX_MISCCR);
	local_irq_restore(flags);

	return misccr;
}

EXPORT_SYMBOL(s3c2410_modify_misccr);

int s3c2410_gpio_getirq(unsigned int pin)
{
	if (pin < S3C2410_GPF0 || pin > S3C2410_GPG15)
		return -1;	/* not valid interrupts */

	if (pin < S3C2410_GPG0 && pin > S3C2410_GPF7)
		return -1;	/* not valid pin */

	if (pin < S3C2410_GPF4)
		return (pin - S3C2410_GPF0) + IRQ_EINT0;

	if (pin < S3C2410_GPG0)
		return (pin - S3C2410_GPF4) + IRQ_EINT4;

	return (pin - S3C2410_GPG0) + IRQ_EINT8;
}

EXPORT_SYMBOL(s3c2410_gpio_getirq);

int s3c2410_gpio_irq2pin(unsigned int irq)
{
	if (irq >= IRQ_EINT0 && irq <= IRQ_EINT3)
		return S3C2410_GPF0 + (irq - IRQ_EINT0);

	if (irq >= IRQ_EINT4 && irq <= IRQ_EINT7)
		return S3C2410_GPF4 + (irq - IRQ_EINT4);

	if (irq >= IRQ_EINT8 && irq <= IRQ_EINT23)
		return S3C2410_GPG0 + (irq - IRQ_EINT8);

	return -EINVAL;
}

EXPORT_SYMBOL(s3c2410_gpio_irq2pin);
