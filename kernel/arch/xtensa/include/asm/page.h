

#ifndef _XTENSA_PAGE_H
#define _XTENSA_PAGE_H

#include <asm/processor.h>
#include <asm/types.h>
#include <asm/cache.h>


#define XCHAL_KSEG_CACHED_VADDR 0xd0000000
#define XCHAL_KSEG_BYPASS_VADDR 0xd8000000
#define XCHAL_KSEG_PADDR        0x00000000
#define XCHAL_KSEG_SIZE         0x08000000


#define PAGE_SHIFT		12
#define PAGE_SIZE		(__XTENSA_UL_CONST(1) << PAGE_SHIFT)
#define PAGE_MASK		(~(PAGE_SIZE-1))

#define PAGE_OFFSET		XCHAL_KSEG_CACHED_VADDR
#define MAX_MEM_PFN		XCHAL_KSEG_SIZE
#define PGTABLE_START		0x80000000


#if DCACHE_WAY_SIZE > PAGE_SIZE
# define DCACHE_ALIAS_ORDER	(DCACHE_WAY_SHIFT - PAGE_SHIFT)
# define DCACHE_ALIAS_MASK	(PAGE_MASK & (DCACHE_WAY_SIZE - 1))
# define DCACHE_ALIAS(a)	(((a) & DCACHE_ALIAS_MASK) >> PAGE_SHIFT)
# define DCACHE_ALIAS_EQ(a,b)	((((a) ^ (b)) & DCACHE_ALIAS_MASK) == 0)
#else
# define DCACHE_ALIAS_ORDER	0
#endif

#if ICACHE_WAY_SIZE > PAGE_SIZE
# define ICACHE_ALIAS_ORDER	(ICACHE_WAY_SHIFT - PAGE_SHIFT)
# define ICACHE_ALIAS_MASK	(PAGE_MASK & (ICACHE_WAY_SIZE - 1))
# define ICACHE_ALIAS(a)	(((a) & ICACHE_ALIAS_MASK) >> PAGE_SHIFT)
# define ICACHE_ALIAS_EQ(a,b)	((((a) ^ (b)) & ICACHE_ALIAS_MASK) == 0)
#else
# define ICACHE_ALIAS_ORDER	0
#endif


#ifdef __ASSEMBLY__

#define __pgprot(x)	(x)

#else


typedef struct { unsigned long pte; } pte_t;		/* page table entry */
typedef struct { unsigned long pgd; } pgd_t;		/* PGD table entry */
typedef struct { unsigned long pgprot; } pgprot_t;
typedef struct page *pgtable_t;

#define pte_val(x)	((x).pte)
#define pgd_val(x)	((x).pgd)
#define pgprot_val(x)	((x).pgprot)

#define __pte(x)	((pte_t) { (x) } )
#define __pgd(x)	((pgd_t) { (x) } )
#define __pgprot(x)	((pgprot_t) { (x) } )


#if XCHAL_HAVE_NSA

static inline __attribute_const__ int get_order(unsigned long size)
{
	int lz;
	asm ("nsau %0, %1" : "=r" (lz) : "r" ((size - 1) >> PAGE_SHIFT));
	return 32 - lz;
}

#else

# include <asm-generic/page.h>

#endif

struct page;
extern void clear_page(void *page);
extern void copy_page(void *to, void *from);


#if DCACHE_WAY_SIZE > PAGE_SIZE
extern void clear_user_page(void*, unsigned long, struct page*);
extern void copy_user_page(void*, void*, unsigned long, struct page*);
#else
# define clear_user_page(page, vaddr, pg)	clear_page(page)
# define copy_user_page(to, from, vaddr, pg)	copy_page(to, from)
#endif


#define __pa(x)			((unsigned long) (x) - PAGE_OFFSET)
#define __va(x)			((void *)((unsigned long) (x) + PAGE_OFFSET))
#define pfn_valid(pfn)		((unsigned long)pfn < max_mapnr)
#ifdef CONFIG_DISCONTIGMEM
# error CONFIG_DISCONTIGMEM not supported
#endif

#define virt_to_page(kaddr)	pfn_to_page(__pa(kaddr) >> PAGE_SHIFT)
#define page_to_virt(page)	__va(page_to_pfn(page) << PAGE_SHIFT)
#define virt_addr_valid(kaddr)	pfn_valid(__pa(kaddr) >> PAGE_SHIFT)
#define page_to_phys(page)	(page_to_pfn(page) << PAGE_SHIFT)

#define WANT_PAGE_VIRTUAL


#endif /* __ASSEMBLY__ */

#define VM_DATA_DEFAULT_FLAGS	(VM_READ | VM_WRITE | VM_EXEC | \
				 VM_MAYREAD | VM_MAYWRITE | VM_MAYEXEC)

#include <asm-generic/memory_model.h>
#endif /* _XTENSA_PAGE_H */