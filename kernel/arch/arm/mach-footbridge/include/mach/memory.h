
#ifndef __ASM_ARCH_MEMORY_H
#define __ASM_ARCH_MEMORY_H


#if defined(CONFIG_FOOTBRIDGE_ADDIN)
#ifndef __ASSEMBLY__
extern unsigned long __virt_to_bus(unsigned long);
extern unsigned long __bus_to_virt(unsigned long);
#endif
#define __virt_to_bus	__virt_to_bus
#define __bus_to_virt	__bus_to_virt

#elif defined(CONFIG_FOOTBRIDGE_HOST)

#define __virt_to_bus(x)	((x) - 0xe0000000)
#define __bus_to_virt(x)	((x) + 0xe0000000)

#else

#error "Undefined footbridge mode"

#endif

#define FLUSH_BASE		0xf9000000

#define PHYS_OFFSET		UL(0x00000000)

#define FLUSH_BASE_PHYS		0x50000000

#endif
