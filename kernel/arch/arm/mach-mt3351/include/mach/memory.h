
#ifndef __ASM_ARCH_MEMORY_H
#define __ASM_ARCH_MEMORY_H

#define PHYS_OFFSET	(0x00000000)
#define BUS_OFFSET	(0x00000000)

#define __virt_to_bus(x)	((x) - PAGE_OFFSET)
#define __bus_to_virt(x)	((x) + PAGE_OFFSET)
#define __virt_to_phys(x)	((x) - PAGE_OFFSET)
#define __phys_to_virt(x)	((x) + PAGE_OFFSET)

#define PCI0_BASE           0

#endif
