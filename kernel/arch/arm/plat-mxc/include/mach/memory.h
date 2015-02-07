


#ifndef __ASM_ARCH_MXC_MEMORY_H__
#define __ASM_ARCH_MXC_MEMORY_H__

#if defined CONFIG_ARCH_MX1
#define PHYS_OFFSET		UL(0x08000000)
#elif defined CONFIG_ARCH_MX2
#define PHYS_OFFSET		UL(0xA0000000)
#elif defined CONFIG_ARCH_MX3
#define PHYS_OFFSET		UL(0x80000000)
#endif

#endif /* __ASM_ARCH_MXC_MEMORY_H__ */
