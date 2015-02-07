
#ifndef __ASM_ARCH_IO_H
#define __ASM_ARCH_IO_H

#define IO_SPACE_LIMIT 0xffffffff

#define IO_PHYS		0x01c00000
#define IO_OFFSET	0xfd000000 /* Virtual IO = 0xfec00000 */
#define IO_SIZE		0x00400000
#define IO_VIRT		(IO_PHYS + IO_OFFSET)
#define io_v2p(va)	((va) - IO_OFFSET)
#define __IO_ADDRESS(x)	((x) + IO_OFFSET)

#define __io(a)			__typesafe_io(a)
#define __mem_pci(a)		(a)
#define __mem_isa(a)		(a)

#define IO_ADDRESS(pa)          IOMEM(__IO_ADDRESS(pa))

#ifdef __ASSEMBLER__
#define IOMEM(x)                x
#else
#define IOMEM(x)                ((void __force __iomem *)(x))

#define davinci_readb(a)	__raw_readb(IO_ADDRESS(a))
#define davinci_readw(a)	__raw_readw(IO_ADDRESS(a))
#define davinci_readl(a)	__raw_readl(IO_ADDRESS(a))

#define davinci_writeb(v, a)	__raw_writeb(v, IO_ADDRESS(a))
#define davinci_writew(v, a)	__raw_writew(v, IO_ADDRESS(a))
#define davinci_writel(v, a)	__raw_writel(v, IO_ADDRESS(a))

#endif /* __ASSEMBLER__ */
#endif /* __ASM_ARCH_IO_H */
