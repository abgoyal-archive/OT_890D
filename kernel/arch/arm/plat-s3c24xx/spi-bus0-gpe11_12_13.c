

#include <linux/kernel.h>

#include <mach/hardware.h>

#include <mach/spi.h>
#include <mach/regs-gpio.h>

void s3c24xx_spi_gpiocfg_bus0_gpe11_12_13(struct s3c2410_spi_info *spi,
					  int enable)
{
	if (enable) {
		s3c2410_gpio_cfgpin(S3C2410_GPE13, S3C2410_GPE13_SPICLK0);
		s3c2410_gpio_cfgpin(S3C2410_GPE12, S3C2410_GPE12_SPIMOSI0);
		s3c2410_gpio_cfgpin(S3C2410_GPE11, S3C2410_GPE11_SPIMISO0);
		s3c2410_gpio_pullup(S3C2410_GPE11, 0);
		s3c2410_gpio_pullup(S3C2410_GPE13, 0);
	} else {
		s3c2410_gpio_cfgpin(S3C2410_GPE13, S3C2410_GPIO_INPUT);
		s3c2410_gpio_cfgpin(S3C2410_GPE11, S3C2410_GPIO_INPUT);
		s3c2410_gpio_pullup(S3C2410_GPE11, 1);
		s3c2410_gpio_pullup(S3C2410_GPE12, 1);
		s3c2410_gpio_pullup(S3C2410_GPE13, 1);
	}
}
