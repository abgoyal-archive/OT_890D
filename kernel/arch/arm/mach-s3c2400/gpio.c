

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/io.h>

#include <mach/hardware.h>
#include <asm/irq.h>

#include <mach/regs-gpio.h>

int s3c2400_gpio_getirq(unsigned int pin)
{
	if (pin < S3C2410_GPE0 || pin > S3C2400_GPE7_EINT7)
		return -1;  /* not valid interrupts */

	return (pin - S3C2410_GPE0) + IRQ_EINT0;
}

EXPORT_SYMBOL(s3c2400_gpio_getirq);
