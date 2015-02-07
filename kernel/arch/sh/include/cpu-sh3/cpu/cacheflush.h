
#ifndef __ASM_CPU_SH3_CACHEFLUSH_H
#define __ASM_CPU_SH3_CACHEFLUSH_H

#if defined(CONFIG_SH7705_CACHE_32KB)
 /* 32KB cache, 4kb PAGE sizes need to check bit 12 */
#define CACHE_ALIAS 0x00001000

#define PG_mapped	PG_arch_1

void flush_cache_all(void);
void flush_cache_mm(struct mm_struct *mm);
#define flush_cache_dup_mm(mm) flush_cache_mm(mm)
void flush_cache_range(struct vm_area_struct *vma, unsigned long start,
                              unsigned long end);
void flush_cache_page(struct vm_area_struct *vma, unsigned long addr, unsigned long pfn);
void flush_dcache_page(struct page *pg);
void flush_icache_range(unsigned long start, unsigned long end);
void flush_icache_page(struct vm_area_struct *vma, struct page *page);

#define flush_dcache_mmap_lock(mapping)		do { } while (0)
#define flush_dcache_mmap_unlock(mapping)	do { } while (0)

/* SH3 has unified cache so no special action needed here */
#define flush_cache_sigtramp(vaddr)		do { } while (0)
#define flush_icache_user_range(vma,pg,adr,len)	do { } while (0)

#define p3_cache_init()				do { } while (0)

#else
#include <cpu-common/cpu/cacheflush.h>
#endif

#endif /* __ASM_CPU_SH3_CACHEFLUSH_H */
