

#ifndef __ASM_ARCH_HARDWARE_H
#define __ASM_ARCH_HARDWARE_H

#ifndef __ASSEMBLY__



extern void s3c2410_gpio_cfgpin(unsigned int pin, unsigned int function);

extern unsigned int s3c2410_gpio_getcfg(unsigned int pin);


extern int s3c2410_gpio_getirq(unsigned int pin);


extern int s3c2410_gpio_irq2pin(unsigned int irq);

#ifdef CONFIG_CPU_S3C2400

extern int s3c2400_gpio_getirq(unsigned int pin);

#endif /* CONFIG_CPU_S3C2400 */


extern int s3c2410_gpio_irqfilter(unsigned int pin, unsigned int on,
				  unsigned int config);


extern void s3c2410_gpio_pullup(unsigned int pin, unsigned int to);


extern int s3c2410_gpio_getpull(unsigned int pin);

extern void s3c2410_gpio_setpin(unsigned int pin, unsigned int to);

extern unsigned int s3c2410_gpio_getpin(unsigned int pin);

extern unsigned int s3c2410_modify_misccr(unsigned int clr, unsigned int chg);

#ifdef CONFIG_CPU_S3C2440

extern int s3c2440_set_dsc(unsigned int pin, unsigned int value);

#endif /* CONFIG_CPU_S3C2440 */

#ifdef CONFIG_CPU_S3C2412

extern int s3c2412_gpio_set_sleepcfg(unsigned int pin, unsigned int state);

#endif /* CONFIG_CPU_S3C2412 */

#endif /* __ASSEMBLY__ */

#include <asm/sizes.h>
#include <mach/map.h>

/* machine specific hardware definitions should go after this */

/* currently here until moved into config (todo) */
#define CONFIG_NO_MULTIWORD_IO

#endif /* __ASM_ARCH_HARDWARE_H */
