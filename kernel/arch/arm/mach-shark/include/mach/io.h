

#ifndef __ASM_ARM_ARCH_IO_H
#define __ASM_ARM_ARCH_IO_H

#define PCIO_BASE	0xe0000000
#define IO_SPACE_LIMIT	0xffffffff

#define __io(a)		((void __iomem *)(PCIO_BASE + (a)))
#define __mem_pci(addr)	(addr)

#endif
