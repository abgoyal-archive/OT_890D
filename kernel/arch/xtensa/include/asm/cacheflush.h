

#ifndef _XTENSA_CACHEFLUSH_H
#define _XTENSA_CACHEFLUSH_H

#ifdef __KERNEL__

#include <linux/mm.h>
#include <asm/processor.h>
#include <asm/page.h>


extern void __invalidate_dcache_all(void);
extern void __invalidate_icache_all(void);
extern void __invalidate_dcache_page(unsigned long);
extern void __invalidate_icache_page(unsigned long);
extern void __invalidate_icache_range(unsigned long, unsigned long);
extern void __invalidate_dcache_range(unsigned long, unsigned long);


#if XCHAL_DCACHE_IS_WRITEBACK
extern void __flush_invalidate_dcache_all(void);
extern void __flush_dcache_page(unsigned long);
extern void __flush_dcache_range(unsigned long, unsigned long);
extern void __flush_invalidate_dcache_page(unsigned long);
extern void __flush_invalidate_dcache_range(unsigned long, unsigned long);
#else
# define __flush_dcache_range(p,s)		do { } while(0)
# define __flush_dcache_page(p)			do { } while(0)
# define __flush_invalidate_dcache_page(p) 	__invalidate_dcache_page(p)
# define __flush_invalidate_dcache_range(p,s)	__invalidate_dcache_range(p,s)
#endif

#if (DCACHE_WAY_SIZE > PAGE_SIZE)
extern void __flush_invalidate_dcache_page_alias(unsigned long, unsigned long);
#endif
#if (ICACHE_WAY_SIZE > PAGE_SIZE)
extern void __invalidate_icache_page_alias(unsigned long, unsigned long);
#else
# define __invalidate_icache_page_alias(v,p)	do { } while(0)
#endif


#if (DCACHE_WAY_SIZE > PAGE_SIZE)

#define flush_cache_all()						\
	do {								\
		__flush_invalidate_dcache_all();			\
		__invalidate_icache_all();				\
	} while (0)

#define flush_cache_mm(mm)		flush_cache_all()
#define flush_cache_dup_mm(mm)		flush_cache_mm(mm)

#define flush_cache_vmap(start,end)	flush_cache_all()
#define flush_cache_vunmap(start,end)	flush_cache_all()

extern void flush_dcache_page(struct page*);
extern void flush_cache_range(struct vm_area_struct*, ulong, ulong);
extern void flush_cache_page(struct vm_area_struct*, unsigned long, unsigned long);

#else

#define flush_cache_all()				do { } while (0)
#define flush_cache_mm(mm)				do { } while (0)
#define flush_cache_dup_mm(mm)				do { } while (0)

#define flush_cache_vmap(start,end)			do { } while (0)
#define flush_cache_vunmap(start,end)			do { } while (0)

#define flush_dcache_page(page)				do { } while (0)

#define flush_cache_page(vma,addr,pfn)			do { } while (0)
#define flush_cache_range(vma,start,end)		do { } while (0)

#endif

/* Ensure consistency between data and instruction cache. */
#define flush_icache_range(start,end) 					\
	do {								\
		__flush_dcache_range(start, (end) - (start));		\
		__invalidate_icache_range(start,(end) - (start));	\
	} while (0)

/* This is not required, see Documentation/cachetlb.txt */
#define	flush_icache_page(vma,page)			do { } while (0)

#define flush_dcache_mmap_lock(mapping)			do { } while (0)
#define flush_dcache_mmap_unlock(mapping)		do { } while (0)

#if (DCACHE_WAY_SIZE > PAGE_SIZE)

extern void copy_to_user_page(struct vm_area_struct*, struct page*,
		unsigned long, void*, const void*, unsigned long);
extern void copy_from_user_page(struct vm_area_struct*, struct page*,
		unsigned long, void*, const void*, unsigned long);

#else

#define copy_to_user_page(vma, page, vaddr, dst, src, len)		\
	do {								\
		memcpy(dst, src, len);					\
		__flush_dcache_range((unsigned long) dst, len);		\
		__invalidate_icache_range((unsigned long) dst, len);	\
	} while (0)

#define copy_from_user_page(vma, page, vaddr, dst, src, len) \
	memcpy(dst, src, len)

#endif

#endif /* __KERNEL__ */
#endif /* _XTENSA_CACHEFLUSH_H */
