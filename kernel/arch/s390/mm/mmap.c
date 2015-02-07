

#include <linux/personality.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <asm/pgalloc.h>

#define MIN_GAP (128*1024*1024)
#define MAX_GAP (STACK_TOP/6*5)

static inline unsigned long mmap_base(void)
{
	unsigned long gap = current->signal->rlim[RLIMIT_STACK].rlim_cur;

	if (gap < MIN_GAP)
		gap = MIN_GAP;
	else if (gap > MAX_GAP)
		gap = MAX_GAP;

	return STACK_TOP - (gap & PAGE_MASK);
}

static inline int mmap_is_legacy(void)
{
#ifdef CONFIG_64BIT
	/*
	 * Force standard allocation for 64 bit programs.
	 */
	if (!test_thread_flag(TIF_31BIT))
		return 1;
#endif
	return sysctl_legacy_va_layout ||
	    (current->personality & ADDR_COMPAT_LAYOUT) ||
	    current->signal->rlim[RLIMIT_STACK].rlim_cur == RLIM_INFINITY;
}

#ifndef CONFIG_64BIT

void arch_pick_mmap_layout(struct mm_struct *mm)
{
	/*
	 * Fall back to the standard layout if the personality
	 * bit is set, or if the expected stack growth is unlimited:
	 */
	if (mmap_is_legacy()) {
		mm->mmap_base = TASK_UNMAPPED_BASE;
		mm->get_unmapped_area = arch_get_unmapped_area;
		mm->unmap_area = arch_unmap_area;
	} else {
		mm->mmap_base = mmap_base();
		mm->get_unmapped_area = arch_get_unmapped_area_topdown;
		mm->unmap_area = arch_unmap_area_topdown;
	}
}
EXPORT_SYMBOL_GPL(arch_pick_mmap_layout);

#else

int s390_mmap_check(unsigned long addr, unsigned long len)
{
	if (!test_thread_flag(TIF_31BIT) &&
	    len >= TASK_SIZE && TASK_SIZE < (1UL << 53))
		return crst_table_upgrade(current->mm, 1UL << 53);
	return 0;
}

static unsigned long
s390_get_unmapped_area(struct file *filp, unsigned long addr,
		unsigned long len, unsigned long pgoff, unsigned long flags)
{
	struct mm_struct *mm = current->mm;
	unsigned long area;
	int rc;

	area = arch_get_unmapped_area(filp, addr, len, pgoff, flags);
	if (!(area & ~PAGE_MASK))
		return area;
	if (area == -ENOMEM &&
	    !test_thread_flag(TIF_31BIT) && TASK_SIZE < (1UL << 53)) {
		/* Upgrade the page table to 4 levels and retry. */
		rc = crst_table_upgrade(mm, 1UL << 53);
		if (rc)
			return (unsigned long) rc;
		area = arch_get_unmapped_area(filp, addr, len, pgoff, flags);
	}
	return area;
}

static unsigned long
s390_get_unmapped_area_topdown(struct file *filp, const unsigned long addr,
			  const unsigned long len, const unsigned long pgoff,
			  const unsigned long flags)
{
	struct mm_struct *mm = current->mm;
	unsigned long area;
	int rc;

	area = arch_get_unmapped_area_topdown(filp, addr, len, pgoff, flags);
	if (!(area & ~PAGE_MASK))
		return area;
	if (area == -ENOMEM &&
	    !test_thread_flag(TIF_31BIT) && TASK_SIZE < (1UL << 53)) {
		/* Upgrade the page table to 4 levels and retry. */
		rc = crst_table_upgrade(mm, 1UL << 53);
		if (rc)
			return (unsigned long) rc;
		area = arch_get_unmapped_area_topdown(filp, addr, len,
						      pgoff, flags);
	}
	return area;
}
void arch_pick_mmap_layout(struct mm_struct *mm)
{
	/*
	 * Fall back to the standard layout if the personality
	 * bit is set, or if the expected stack growth is unlimited:
	 */
	if (mmap_is_legacy()) {
		mm->mmap_base = TASK_UNMAPPED_BASE;
		mm->get_unmapped_area = s390_get_unmapped_area;
		mm->unmap_area = arch_unmap_area;
	} else {
		mm->mmap_base = mmap_base();
		mm->get_unmapped_area = s390_get_unmapped_area_topdown;
		mm->unmap_area = arch_unmap_area_topdown;
	}
}
EXPORT_SYMBOL_GPL(arch_pick_mmap_layout);

#endif
