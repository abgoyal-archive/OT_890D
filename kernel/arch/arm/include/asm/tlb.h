
#ifndef __ASMARM_TLB_H
#define __ASMARM_TLB_H

#include <asm/cacheflush.h>
#include <asm/tlbflush.h>

#ifndef CONFIG_MMU

#include <linux/pagemap.h>
#include <asm-generic/tlb.h>

#else /* !CONFIG_MMU */

#include <asm/pgalloc.h>

struct mmu_gather {
	struct mm_struct	*mm;
	unsigned int		fullmm;
};

DECLARE_PER_CPU(struct mmu_gather, mmu_gathers);

static inline struct mmu_gather *
tlb_gather_mmu(struct mm_struct *mm, unsigned int full_mm_flush)
{
	struct mmu_gather *tlb = &get_cpu_var(mmu_gathers);

	tlb->mm = mm;
	tlb->fullmm = full_mm_flush;

	return tlb;
}

static inline void
tlb_finish_mmu(struct mmu_gather *tlb, unsigned long start, unsigned long end)
{
	if (tlb->fullmm)
		flush_tlb_mm(tlb->mm);

	/* keep the page table cache within bounds */
	check_pgt_cache();

	put_cpu_var(mmu_gathers);
}

#define tlb_remove_tlb_entry(tlb,ptep,address)	do { } while (0)

static inline void
tlb_start_vma(struct mmu_gather *tlb, struct vm_area_struct *vma)
{
	if (!tlb->fullmm)
		flush_cache_range(vma, vma->vm_start, vma->vm_end);
}

static inline void
tlb_end_vma(struct mmu_gather *tlb, struct vm_area_struct *vma)
{
	if (!tlb->fullmm)
		flush_tlb_range(vma, vma->vm_start, vma->vm_end);
}

#define tlb_remove_page(tlb,page)	free_page_and_swap_cache(page)
#define pte_free_tlb(tlb, ptep)		pte_free((tlb)->mm, ptep)
#define pmd_free_tlb(tlb, pmdp)		pmd_free((tlb)->mm, pmdp)

#define tlb_migrate_finish(mm)		do { } while (0)

#endif /* CONFIG_MMU */
#endif
