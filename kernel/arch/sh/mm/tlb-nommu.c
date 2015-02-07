
#include <linux/kernel.h>
#include <linux/mm.h>
#include <asm/pgtable.h>
#include <asm/tlbflush.h>

void local_flush_tlb_all(void)
{
	BUG();
}

void local_flush_tlb_mm(struct mm_struct *mm)
{
	BUG();
}

void local_flush_tlb_range(struct vm_area_struct *vma, unsigned long start,
			    unsigned long end)
{
	BUG();
}

void local_flush_tlb_page(struct vm_area_struct *vma, unsigned long page)
{
	BUG();
}

void local_flush_tlb_one(unsigned long asid, unsigned long page)
{
	BUG();
}

void local_flush_tlb_kernel_range(unsigned long start, unsigned long end)
{
	BUG();
}

void update_mmu_cache(struct vm_area_struct * vma,
		      unsigned long address, pte_t pte)
{
	BUG();
}

void __init page_table_range_init(unsigned long start, unsigned long end,
				  pgd_t *pgd_base)
{
}

void __set_fixmap(enum fixed_addresses idx, unsigned long phys, pgprot_t prot)
{
}
