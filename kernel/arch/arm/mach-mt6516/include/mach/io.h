

#ifndef __ASM_ARM_ARCH_IO_H
#define __ASM_ARM_ARCH_IO_H

#include <mach/memory.h>

#define IO_SPACE_LIMIT 0xffffffff

#define __io(a)         ((void __iomem *)(PCI0_BASE + (a)))
#define __mem_pci(a)    (a)
#define __mem_isa(a)    (a)

#endif
