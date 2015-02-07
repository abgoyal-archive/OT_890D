
#ifndef __ASM_CPU_SH4_CACHEFLUSH_H
#define __ASM_CPU_SH4_CACHEFLUSH_H

void flush_cache_all(void);
void flush_dcache_all(void);
void flush_cache_mm(struct mm_struct *mm);
#define flush_cache_dup_mm(mm) flush_cache_mm(mm)
void flush_cache_range(struct vm_area_struct *vma, unsigned long start,
		       unsigned long end);
void flush_cache_page(struct vm_area_struct *vma, unsigned long addr,
		      unsigned long pfn);
void flush_dcache_page(struct page *pg);

#define flush_dcache_mmap_lock(mapping)		do { } while (0)
#define flush_dcache_mmap_unlock(mapping)	do { } while (0)

void flush_icache_range(unsigned long start, unsigned long end);
void flush_icache_user_range(struct vm_area_struct *vma, struct page *page,
			     unsigned long addr, int len);

#define flush_icache_page(vma,pg)		do { } while (0)

/* Initialization of P3 area for copy_user_page */
void p3_cache_init(void);

#define PG_mapped	PG_arch_1

#endif /* __ASM_CPU_SH4_CACHEFLUSH_H */
