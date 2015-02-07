

#ifndef __ASM_ARCH_MEMORY_H
#define __ASM_ARCH_MEMORY_H

#define PHYS_OFFSET	UL(0x00000000)

#include <mach/ixp2000-regs.h>

#define __virt_to_bus(v) \
	(((__virt_to_phys(v) - 0x0) + (*IXP2000_PCI_SDRAM_BAR & 0xfffffff0)))

#define __bus_to_virt(b) \
	__phys_to_virt((((b - (*IXP2000_PCI_SDRAM_BAR & 0xfffffff0)) + 0x0)))

#endif

