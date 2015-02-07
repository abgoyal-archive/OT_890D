
#ifndef __ASM_ARCH_MEMORY_H
#define __ASM_ARCH_MEMORY_H

#include <asm/page.h>
#include <asm/sizes.h>

#define DAVINCI_DDR_BASE    0x80000000
#define DAVINCI_IRAM_BASE   0x00008000 /* ARM Internal RAM */

#define PHYS_OFFSET DAVINCI_DDR_BASE

#define CONSISTENT_DMA_SIZE (14<<20)

#ifndef __ASSEMBLY__
static inline void
__arch_adjust_zones(int node, unsigned long *size, unsigned long *holes)
{
	unsigned int sz = (128<<20) >> PAGE_SHIFT;

	if (node != 0)
		sz = 0;

	size[1] = size[0] - sz;
	size[0] = sz;
}

#define arch_adjust_zones(node, zone_size, holes) \
        if ((meminfo.bank[0].size >> 20) > 128) __arch_adjust_zones(node, zone_size, holes)

#define ISA_DMA_THRESHOLD	(PHYS_OFFSET + (128<<20) - 1)
#define MAX_DMA_ADDRESS		(PAGE_OFFSET + (128<<20))

#endif

#endif /* __ASM_ARCH_MEMORY_H */
