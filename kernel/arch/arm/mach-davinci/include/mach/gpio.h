

#ifndef	__DAVINCI_GPIO_H
#define	__DAVINCI_GPIO_H

#include <linux/io.h>
#include <asm-generic/gpio.h>
#include <mach/hardware.h>
#include <mach/irqs.h>

#define	GPIO(X)		(X)		/* 0 <= X <= 70 */
#define	GPIOV18(X)	(X)		/* 1.8V i/o; 0 <= X <= 53 */
#define	GPIOV33(X)	((X)+54)	/* 3.3V i/o; 0 <= X <= 17 */

struct gpio_controller {
	u32	dir;
	u32	out_data;
	u32	set_data;
	u32	clr_data;
	u32	in_data;
	u32	set_rising;
	u32	clr_rising;
	u32	set_falling;
	u32	clr_falling;
	u32	intstat;
};

static inline struct gpio_controller *__iomem
__gpio_to_controller(unsigned gpio)
{
	void *__iomem ptr;

	if (gpio < 32)
		ptr = IO_ADDRESS(DAVINCI_GPIO_BASE + 0x10);
	else if (gpio < 64)
		ptr = IO_ADDRESS(DAVINCI_GPIO_BASE + 0x38);
	else if (gpio < DAVINCI_N_GPIO)
		ptr = IO_ADDRESS(DAVINCI_GPIO_BASE + 0x60);
	else
		ptr = NULL;
	return ptr;
}

static inline u32 __gpio_mask(unsigned gpio)
{
	return 1 << (gpio % 32);
}

static inline void gpio_set_value(unsigned gpio, int value)
{
	if (__builtin_constant_p(value) && gpio < DAVINCI_N_GPIO) {
		struct gpio_controller	*__iomem g;
		u32			mask;

		g = __gpio_to_controller(gpio);
		mask = __gpio_mask(gpio);
		if (value)
			__raw_writel(mask, &g->set_data);
		else
			__raw_writel(mask, &g->clr_data);
		return;
	}

	__gpio_set_value(gpio, value);
}

static inline int gpio_get_value(unsigned gpio)
{
	struct gpio_controller	*__iomem g;

	if (!__builtin_constant_p(gpio) || gpio >= DAVINCI_N_GPIO)
		return __gpio_get_value(gpio);

	g = __gpio_to_controller(gpio);
	return __gpio_mask(gpio) & __raw_readl(&g->in_data);
}

static inline int gpio_cansleep(unsigned gpio)
{
	if (__builtin_constant_p(gpio) && gpio < DAVINCI_N_GPIO)
		return 0;
	else
		return __gpio_cansleep(gpio);
}

static inline int gpio_to_irq(unsigned gpio)
{
	if (gpio >= DAVINCI_N_GPIO)
		return -EINVAL;
	return DAVINCI_N_AINTC_IRQ + gpio;
}

static inline int irq_to_gpio(unsigned irq)
{
	/* caller guarantees gpio_to_irq() succeeded */
	return irq - DAVINCI_N_AINTC_IRQ;
}

#endif				/* __DAVINCI_GPIO_H */
