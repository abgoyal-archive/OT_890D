

#include <linux/kernel.h>

struct platform_device;

#include <plat/iic.h>
#include <mach/hardware.h>
#include <mach/regs-gpio.h>

void s3c_i2c0_cfg_gpio(struct platform_device *dev)
{
	s3c2410_gpio_cfgpin(S3C2410_GPE15, S3C2410_GPE15_IICSDA);
	s3c2410_gpio_cfgpin(S3C2410_GPE14, S3C2410_GPE14_IICSCL);
}
