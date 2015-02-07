
#ifndef _ASM_PGALLOC_H
#define _ASM_PGALLOC_H

#include <asm/setup.h>
#include <asm/virtconvert.h>

#ifdef CONFIG_MMU

#define pmd_populate_kernel(mm, pmd, pte) __set_pmd(pmd, __pa(pte) | _PAGE_TABLE)
#define pmd_populate(MM, PMD, PAGE)						\
do {										\
	__set_pmd((PMD), page_to_pfn(PAGE) << PAGE_SHIFT | _PAGE_TABLE);	\
} while(0)
#define pmd_pgtable(pmd) pmd_page(pmd)


extern pgd_t *pgd_alloc(struct mm_struct *);
extern void pgd_free(struct mm_struct *mm, pgd_t *);

extern pte_t *pte_alloc_one_kernel(struct mm_struct *, unsigned long);

extern pgtable_t pte_alloc_one(struct mm_struct *, unsigned long);

static inline void pte_free_kernel(struct mm_struct *mm, pte_t *pte)
{
	free_page((unsigned long)pte);
}

static inline void pte_free(struct mm_struct *mm, pgtable_t pte)
{
	pgtable_page_dtor(pte);
	__free_page(pte);
}

#define __pte_free_tlb(tlb,pte)				\
do {							\
	pgtable_page_dtor(pte);				\
	tlb_remove_page((tlb),(pte));			\
} while (0)

#define pmd_alloc_one(mm, addr)		({ BUG(); ((pmd_t *) 2); })
#define pmd_free(mm, x)			do { } while (0)
#define __pmd_free_tlb(tlb,x)		do { } while (0)

#endif /* CONFIG_MMU */

#endif /* _ASM_PGALLOC_H */
