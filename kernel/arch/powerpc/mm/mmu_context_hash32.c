

#include <linux/mm.h>
#include <linux/init.h>

#include <asm/mmu_context.h>
#include <asm/tlbflush.h>

#define NO_CONTEXT      	((unsigned long) -1)
#define LAST_CONTEXT    	32767
#define FIRST_CONTEXT    	1


static unsigned long next_mmu_context;
static unsigned long context_map[LAST_CONTEXT / BITS_PER_LONG + 1];


int init_new_context(struct task_struct *t, struct mm_struct *mm)
{
	unsigned long ctx = next_mmu_context;

	while (test_and_set_bit(ctx, context_map)) {
		ctx = find_next_zero_bit(context_map, LAST_CONTEXT+1, ctx);
		if (ctx > LAST_CONTEXT)
			ctx = 0;
	}
	next_mmu_context = (ctx + 1) & LAST_CONTEXT;
	mm->context.id = ctx;

	return 0;
}

void destroy_context(struct mm_struct *mm)
{
	preempt_disable();
	if (mm->context.id != NO_CONTEXT) {
		clear_bit(mm->context.id, context_map);
		mm->context.id = NO_CONTEXT;
	}
	preempt_enable();
}

void __init mmu_context_init(void)
{
	/* Reserve context 0 for kernel use */
	context_map[0] = (1 << FIRST_CONTEXT) - 1;
	next_mmu_context = FIRST_CONTEXT;
}
