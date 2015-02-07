

#ifndef __ASM_ARCH_MEMORY_H
#define __ASM_ARCH_MEMORY_H

#include <mach/hardware.h>

#define PHYS_OFFSET		(0x00000000)

#define __virt_to_bus(v)						\
	({ unsigned int ret;						\
	ret = ((__virt_to_phys(v) - 0x00000000) +			\
	 (*((volatile int *)IXP23XX_PCI_SDRAM_BAR) & 0xfffffff0)); 	\
	ret; })

#define __bus_to_virt(b)						\
	({ unsigned int data;						\
	data = *((volatile int *)IXP23XX_PCI_SDRAM_BAR);		\
	 __phys_to_virt((((b - (data & 0xfffffff0)) + 0x00000000))); })

#define arch_is_coherent()	1

#endif
