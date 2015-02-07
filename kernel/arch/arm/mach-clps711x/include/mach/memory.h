
#ifndef __ASM_ARCH_MEMORY_H
#define __ASM_ARCH_MEMORY_H


#define PHYS_OFFSET	UL(0xc0000000)

#if !defined(CONFIG_ARCH_CDB89712) && !defined (CONFIG_ARCH_AUTCPU12)

#define __virt_to_bus(x)	((x) - PAGE_OFFSET)
#define __bus_to_virt(x)	((x) + PAGE_OFFSET)

#endif




#define NODE_MEM_SIZE_BITS	24
#define SECTION_SIZE_BITS	24
#define MAX_PHYSMEM_BITS	32

#endif

