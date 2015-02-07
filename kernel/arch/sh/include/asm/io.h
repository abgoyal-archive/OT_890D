
#ifndef __ASM_SH_IO_H
#define __ASM_SH_IO_H
#include <asm/cache.h>
#include <asm/system.h>
#include <asm/addrspace.h>
#include <asm/machvec.h>
#include <asm/pgtable.h>
#include <asm-generic/iomap.h>

#ifdef __KERNEL__
#define __IO_PREFIX	generic
#include <asm/io_generic.h>
#include <asm/io_trapped.h>

#define inb(p)			sh_mv.mv_inb((p))
#define inw(p)			sh_mv.mv_inw((p))
#define inl(p)			sh_mv.mv_inl((p))
#define outb(x,p)		sh_mv.mv_outb((x),(p))
#define outw(x,p)		sh_mv.mv_outw((x),(p))
#define outl(x,p)		sh_mv.mv_outl((x),(p))

#define inb_p(p)		sh_mv.mv_inb_p((p))
#define inw_p(p)		sh_mv.mv_inw_p((p))
#define inl_p(p)		sh_mv.mv_inl_p((p))
#define outb_p(x,p)		sh_mv.mv_outb_p((x),(p))
#define outw_p(x,p)		sh_mv.mv_outw_p((x),(p))
#define outl_p(x,p)		sh_mv.mv_outl_p((x),(p))

#define insb(p,b,c)		sh_mv.mv_insb((p), (b), (c))
#define insw(p,b,c)		sh_mv.mv_insw((p), (b), (c))
#define insl(p,b,c)		sh_mv.mv_insl((p), (b), (c))
#define outsb(p,b,c)		sh_mv.mv_outsb((p), (b), (c))
#define outsw(p,b,c)		sh_mv.mv_outsw((p), (b), (c))
#define outsl(p,b,c)		sh_mv.mv_outsl((p), (b), (c))

#define __raw_writeb(v,a)	(__chk_io_ptr(a), *(volatile u8  __force *)(a) = (v))
#define __raw_writew(v,a)	(__chk_io_ptr(a), *(volatile u16 __force *)(a) = (v))
#define __raw_writel(v,a)	(__chk_io_ptr(a), *(volatile u32 __force *)(a) = (v))
#define __raw_writeq(v,a)	(__chk_io_ptr(a), *(volatile u64 __force *)(a) = (v))

#define __raw_readb(a)		(__chk_io_ptr(a), *(volatile u8  __force *)(a))
#define __raw_readw(a)		(__chk_io_ptr(a), *(volatile u16 __force *)(a))
#define __raw_readl(a)		(__chk_io_ptr(a), *(volatile u32 __force *)(a))
#define __raw_readq(a)		(__chk_io_ptr(a), *(volatile u64 __force *)(a))

#define readb(a)		({ u8  r_ = __raw_readb(a); mb(); r_; })
#define readw(a)		({ u16 r_ = __raw_readw(a); mb(); r_; })
#define readl(a)		({ u32 r_ = __raw_readl(a); mb(); r_; })
#define readq(a)		({ u64 r_ = __raw_readq(a); mb(); r_; })

#define writeb(v,a)		({ __raw_writeb((v),(a)); mb(); })
#define writew(v,a)		({ __raw_writew((v),(a)); mb(); })
#define writel(v,a)		({ __raw_writel((v),(a)); mb(); })
#define writeq(v,a)		({ __raw_writeq((v),(a)); mb(); })

/* SuperH on-chip I/O functions */
#define ctrl_inb		__raw_readb
#define ctrl_inw		__raw_readw
#define ctrl_inl		__raw_readl
#define ctrl_inq		__raw_readq

#define ctrl_outb		__raw_writeb
#define ctrl_outw		__raw_writew
#define ctrl_outl		__raw_writel
#define ctrl_outq		__raw_writeq

static inline void ctrl_delay(void)
{
#ifdef P2SEG
	__raw_readw(P2SEG);
#endif
}

#define __BUILD_MEMORY_STRING(bwlq, type)				\
									\
static inline void __raw_writes##bwlq(volatile void __iomem *mem,	\
				const void *addr, unsigned int count)	\
{									\
	const volatile type *__addr = addr;				\
									\
	while (count--) {						\
		__raw_write##bwlq(*__addr, mem);			\
		__addr++;						\
	}								\
}									\
									\
static inline void __raw_reads##bwlq(volatile void __iomem *mem,	\
			       void *addr, unsigned int count)		\
{									\
	volatile type *__addr = addr;					\
									\
	while (count--) {						\
		*__addr = __raw_read##bwlq(mem);			\
		__addr++;						\
	}								\
}

__BUILD_MEMORY_STRING(b, u8)
__BUILD_MEMORY_STRING(w, u16)
__BUILD_MEMORY_STRING(q, u64)

void __raw_writesl(void __iomem *addr, const void *data, int longlen);
void __raw_readsl(const void __iomem *addr, void *data, int longlen);

#define writesb			__raw_writesb
#define writesw			__raw_writesw
#define writesl			__raw_writesl

#define readsb			__raw_readsb
#define readsw			__raw_readsw
#define readsl			__raw_readsl

#define readb_relaxed(a)	readb(a)
#define readw_relaxed(a)	readw(a)
#define readl_relaxed(a)	readl(a)
#define readq_relaxed(a)	readq(a)

/* Simple MMIO */
#define ioread8(a)		__raw_readb(a)
#define ioread16(a)		__raw_readw(a)
#define ioread16be(a)		be16_to_cpu(__raw_readw((a)))
#define ioread32(a)		__raw_readl(a)
#define ioread32be(a)		be32_to_cpu(__raw_readl((a)))

#define iowrite8(v,a)		__raw_writeb((v),(a))
#define iowrite16(v,a)		__raw_writew((v),(a))
#define iowrite16be(v,a)	__raw_writew(cpu_to_be16((v)),(a))
#define iowrite32(v,a)		__raw_writel((v),(a))
#define iowrite32be(v,a)	__raw_writel(cpu_to_be32((v)),(a))

#define ioread8_rep(a, d, c)	__raw_readsb((a), (d), (c))
#define ioread16_rep(a, d, c)	__raw_readsw((a), (d), (c))
#define ioread32_rep(a, d, c)	__raw_readsl((a), (d), (c))

#define iowrite8_rep(a, s, c)	__raw_writesb((a), (s), (c))
#define iowrite16_rep(a, s, c)	__raw_writesw((a), (s), (c))
#define iowrite32_rep(a, s, c)	__raw_writesl((a), (s), (c))

/* synco on SH-4A, otherwise a nop */
#define mmiowb()		wmb()

#define IO_SPACE_LIMIT 0xffffffff

extern unsigned long generic_io_base;

static inline void __set_io_port_base(unsigned long pbase)
{
	generic_io_base = pbase;
}

#define __ioport_map(p, n) sh_mv.mv_ioport_map((p), (n))

/* We really want to try and get these to memcpy etc */
void memcpy_fromio(void *, const volatile void __iomem *, unsigned long);
void memcpy_toio(volatile void __iomem *, const void *, unsigned long);
void memset_io(volatile void __iomem *, int, unsigned long);

/* Quad-word real-mode I/O, don't ask.. */
unsigned long long peek_real_address_q(unsigned long long addr);
unsigned long long poke_real_address_q(unsigned long long addr,
				       unsigned long long val);

#if !defined(CONFIG_MMU)
#define virt_to_phys(address)	((unsigned long)(address))
#define phys_to_virt(address)	((void *)(address))
#else
#define virt_to_phys(address)	(__pa(address))
#define phys_to_virt(address)	(__va(address))
#endif

#ifdef CONFIG_MMU
void __iomem *__ioremap(unsigned long offset, unsigned long size,
			unsigned long flags);
void __iounmap(void __iomem *addr);

/* arch/sh/mm/ioremap_64.c */
unsigned long onchip_remap(unsigned long addr, unsigned long size,
			   const char *name);
extern void onchip_unmap(unsigned long vaddr);
#else
#define __ioremap(offset, size, flags)	((void __iomem *)(offset))
#define __iounmap(addr)			do { } while (0)
#define onchip_remap(addr, size, name)	(addr)
#define onchip_unmap(addr)		do { } while (0)
#endif /* CONFIG_MMU */

static inline void __iomem *
__ioremap_mode(unsigned long offset, unsigned long size, unsigned long flags)
{
#ifdef CONFIG_SUPERH32
	unsigned long last_addr = offset + size - 1;
#endif
	void __iomem *ret;

	ret = __ioremap_trapped(offset, size);
	if (ret)
		return ret;

#ifdef CONFIG_SUPERH32
	/*
	 * For P1 and P2 space this is trivial, as everything is already
	 * mapped. Uncached access for P1 addresses are done through P2.
	 * In the P3 case or for addresses outside of the 29-bit space,
	 * mapping must be done by the PMB or by using page tables.
	 */
	if (likely(PXSEG(offset) < P3SEG && PXSEG(last_addr) < P3SEG)) {
		if (unlikely(flags & _PAGE_CACHABLE))
			return (void __iomem *)P1SEGADDR(offset);

		return (void __iomem *)P2SEGADDR(offset);
	}

	/* P4 above the store queues are always mapped. */
	if (unlikely(offset >= P3_ADDR_MAX))
		return (void __iomem *)P4SEGADDR(offset);
#endif

	return __ioremap(offset, size, flags);
}

#define ioremap(offset, size)				\
	__ioremap_mode((offset), (size), 0)
#define ioremap_nocache(offset, size)			\
	__ioremap_mode((offset), (size), 0)
#define ioremap_cache(offset, size)			\
	__ioremap_mode((offset), (size), _PAGE_CACHABLE)
#define p3_ioremap(offset, size, flags)			\
	__ioremap((offset), (size), (flags))
#define ioremap_prot(offset, size, flags)		\
	__ioremap_mode((offset), (size), (flags))
#define iounmap(addr)					\
	__iounmap((addr))

#define maybebadio(port) \
	printk(KERN_ERR "bad PC-like io %s:%u for port 0x%lx at 0x%08x\n", \
	       __func__, __LINE__, (port), (u32)__builtin_return_address(0))

#define xlate_dev_mem_ptr(p)	__va(p)

#define xlate_dev_kmem_ptr(p)	p

#define ARCH_HAS_VALID_PHYS_ADDR_RANGE
int valid_phys_addr_range(unsigned long addr, size_t size);
int valid_mmap_phys_addr_range(unsigned long pfn, size_t size);

#endif /* __KERNEL__ */

#endif /* __ASM_SH_IO_H */
