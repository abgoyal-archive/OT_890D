

#ifndef __ASM_ARCH_OMAP_APOLLON_H
#define __ASM_ARCH_OMAP_APOLLON_H

#include <mach/cpu.h>

extern void apollon_mmc_init(void);

static inline int apollon_plus(void)
{
	/* The apollon plus has IDCODE revision 5 */
	return omap_rev() & 0xc0;
}

/* Placeholder for APOLLON specific defines */
#define APOLLON_ETHR_GPIO_IRQ		74

#endif /*  __ASM_ARCH_OMAP_APOLLON_H */

