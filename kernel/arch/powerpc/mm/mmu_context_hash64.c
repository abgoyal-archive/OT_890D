

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/spinlock.h>
#include <linux/idr.h>

#include <asm/mmu_context.h>

static DEFINE_SPINLOCK(mmu_context_lock);
static DEFINE_IDR(mmu_context_idr);

#define NO_CONTEXT	0
#define MAX_CONTEXT	((1UL << 19) - 1)

int init_new_context(struct task_struct *tsk, struct mm_struct *mm)
{
	int index;
	int err;

again:
	if (!idr_pre_get(&mmu_context_idr, GFP_KERNEL))
		return -ENOMEM;

	spin_lock(&mmu_context_lock);
	err = idr_get_new_above(&mmu_context_idr, NULL, 1, &index);
	spin_unlock(&mmu_context_lock);

	if (err == -EAGAIN)
		goto again;
	else if (err)
		return err;

	if (index > MAX_CONTEXT) {
		spin_lock(&mmu_context_lock);
		idr_remove(&mmu_context_idr, index);
		spin_unlock(&mmu_context_lock);
		return -ENOMEM;
	}

	/* The old code would re-promote on fork, we don't do that
	 * when using slices as it could cause problem promoting slices
	 * that have been forced down to 4K
	 */
	if (slice_mm_new_context(mm))
		slice_set_user_psize(mm, mmu_virtual_psize);
	mm->context.id = index;

	return 0;
}

void destroy_context(struct mm_struct *mm)
{
	spin_lock(&mmu_context_lock);
	idr_remove(&mmu_context_idr, mm->context.id);
	spin_unlock(&mmu_context_lock);

	mm->context.id = NO_CONTEXT;
}
