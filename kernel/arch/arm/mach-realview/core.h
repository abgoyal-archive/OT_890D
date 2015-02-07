

#ifndef __ASM_ARCH_REALVIEW_H
#define __ASM_ARCH_REALVIEW_H

#include <linux/amba/bus.h>
#include <linux/io.h>

#include <asm/leds.h>

#define AMBA_DEVICE(name,busid,base,plat)			\
static struct amba_device name##_device = {			\
	.dev		= {					\
		.coherent_dma_mask = ~0,			\
		.init_name = busid,				\
		.platform_data = plat,				\
	},							\
	.res		= {					\
		.start	= REALVIEW_##base##_BASE,		\
		.end	= (REALVIEW_##base##_BASE) + SZ_4K - 1,	\
		.flags	= IORESOURCE_MEM,			\
	},							\
	.dma_mask	= ~0,					\
	.irq		= base##_IRQ,				\
	/* .dma		= base##_DMA,*/				\
}

extern struct platform_device realview_flash_device;
extern struct platform_device realview_i2c_device;
extern struct mmc_platform_data realview_mmc0_plat_data;
extern struct mmc_platform_data realview_mmc1_plat_data;
extern struct clcd_board clcd_plat_data;
extern void __iomem *gic_cpu_base_addr;
#ifdef CONFIG_LOCAL_TIMERS
extern void __iomem *twd_base;
#endif
extern void __iomem *timer0_va_base;
extern void __iomem *timer1_va_base;
extern void __iomem *timer2_va_base;
extern void __iomem *timer3_va_base;

extern void realview_leds_event(led_event_t ledevt);
extern void realview_timer_init(unsigned int timer_irq);
extern int realview_flash_register(struct resource *res, u32 num);
extern int realview_eth_register(const char *name, struct resource *res);

#endif
