

#ifndef __ASM_ARCH_GPIO_CORE_H
#define __ASM_ARCH_GPIO_CORE_H __FILE__

#include <plat/gpio-core.h>
#include <mach/regs-gpio.h>

extern struct s3c_gpio_chip s3c24xx_gpios[];

static inline struct s3c_gpio_chip *s3c_gpiolib_getchip(unsigned int pin)
{
	struct s3c_gpio_chip *chip;

	if (pin > S3C2410_GPG10)
		return NULL;

	chip = &s3c24xx_gpios[pin/32];
	return (S3C2410_GPIO_OFFSET(pin) > chip->chip.ngpio) ? chip : NULL;
}

#endif /* __ASM_ARCH_GPIO_CORE_H */
