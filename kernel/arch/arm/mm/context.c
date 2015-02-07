
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/mm.h>

#include <asm/mmu_context.h>
#include <asm/tlbflush.h>

static DEFINE_SPINLOCK(cpu_asid_lock);
unsigned int cpu_last_asid = ASID_FIRST_VERSION;

void __init_new_context(struct task_struct *tsk, struct mm_struct *mm)
{
	mm->context.id = 0;
}

void __new_context(struct mm_struct *mm)
{
	unsigned int asid;

	spin_lock(&cpu_asid_lock);
	asid = ++cpu_last_asid;
	if (asid == 0)
		asid = cpu_last_asid = ASID_FIRST_VERSION;

	/*
	 * If we've used up all our ASIDs, we need
	 * to start a new version and flush the TLB.
	 */
	if (unlikely((asid & ~ASID_MASK) == 0)) {
		asid = ++cpu_last_asid;
		/* set the reserved ASID before flushing the TLB */
		asm("mcr	p15, 0, %0, c13, c0, 1	@ set reserved context ID\n"
		    :
		    : "r" (0));
		isb();
		flush_tlb_all();
		if (icache_is_vivt_asid_tagged()) {
			asm("mcr	p15, 0, %0, c7, c5, 0	@ invalidate I-cache\n"
			    "mcr	p15, 0, %0, c7, c5, 6	@ flush BTAC/BTB\n"
			    :
			    : "r" (0));
			dsb();
		}
	}
	spin_unlock(&cpu_asid_lock);

	mm->cpu_vm_mask = cpumask_of_cpu(smp_processor_id());
	mm->context.id = asid;
}
