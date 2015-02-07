

#ifndef __ASM_ARCH_OMAP_GPIOEXPANDER_H
#define __ASM_ARCH_OMAP_GPIOEXPANDER_H

/* Function Prototypes for GPIO Expander functions */

#ifdef CONFIG_GPIOEXPANDER_OMAP
int read_gpio_expa(u8 *, int);
int write_gpio_expa(u8 , int);
#else
static inline int read_gpio_expa(u8 *val, int addr)
{
	return 0;
}
static inline int write_gpio_expa(u8 val, int addr)
{
	return 0;
}
#endif

#endif /* __ASM_ARCH_OMAP_GPIOEXPANDER_H */
