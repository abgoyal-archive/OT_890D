

#define gpio_get_value	__gpio_get_value
#define gpio_set_value	__gpio_set_value
#define gpio_cansleep	__gpio_cansleep
#define gpio_to_irq	__gpio_to_irq


#define ARCH_NR_GPIOS	(256 + CONFIG_S3C24XX_GPIO_EXTRA)

#include <asm-generic/gpio.h>
