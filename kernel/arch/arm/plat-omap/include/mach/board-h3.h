
#ifndef __ASM_ARCH_OMAP_H3_H
#define __ASM_ARCH_OMAP_H3_H

/* In OMAP1710 H3 the Ethernet is directly connected to CS1 */
#define OMAP1710_ETHR_START		0x04000300

#define H3_TPS_GPIO_BASE		(OMAP_MAX_GPIO_LINES + 16 /* MPUIO */)
#	define H3_TPS_GPIO_MMC_PWR_EN	(H3_TPS_GPIO_BASE + 4)

extern void h3_mmc_init(void);

#endif /*  __ASM_ARCH_OMAP_H3_H */
