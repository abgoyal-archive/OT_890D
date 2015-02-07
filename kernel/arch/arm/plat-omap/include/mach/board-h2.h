

#ifndef __ASM_ARCH_OMAP_H2_H
#define __ASM_ARCH_OMAP_H2_H

/* At OMAP1610 Innovator the Ethernet is directly connected to CS1 */
#define OMAP1610_ETHR_START		0x04000300

#define H2_TPS_GPIO_BASE		(OMAP_MAX_GPIO_LINES + 16 /* MPUIO */)
#	define H2_TPS_GPIO_MMC_PWR_EN	(H2_TPS_GPIO_BASE + 3)

extern void h2_mmc_init(void);

#endif /*  __ASM_ARCH_OMAP_H2_H */

