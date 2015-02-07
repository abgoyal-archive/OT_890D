
#include <linux/init.h>

#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/smp.h>
#include <linux/kernel_stat.h>
#include <linux/mc146818rtc.h>
#include <linux/interrupt.h>

#include <asm/mtrr.h>
#include <asm/pgalloc.h>
#include <asm/tlbflush.h>
#include <asm/mmu_context.h>
#include <asm/proto.h>
#include <asm/apicdef.h>
#include <asm/idle.h>
#include <asm/uv/uv_hub.h>
#include <asm/uv/uv_bau.h>

#include <mach_ipi.h>

union smp_flush_state {
	struct {
		cpumask_t flush_cpumask;
		struct mm_struct *flush_mm;
		unsigned long flush_va;
		spinlock_t tlbstate_lock;
	};
	char pad[SMP_CACHE_BYTES];
} ____cacheline_aligned;

static DEFINE_PER_CPU(union smp_flush_state, flush_state);

void leave_mm(int cpu)
{
	if (read_pda(mmu_state) == TLBSTATE_OK)
		BUG();
	cpu_clear(cpu, read_pda(active_mm)->cpu_vm_mask);
	load_cr3(swapper_pg_dir);
}
EXPORT_SYMBOL_GPL(leave_mm);



asmlinkage void smp_invalidate_interrupt(struct pt_regs *regs)
{
	int cpu;
	int sender;
	union smp_flush_state *f;

	cpu = smp_processor_id();
	/*
	 * orig_rax contains the negated interrupt vector.
	 * Use that to determine where the sender put the data.
	 */
	sender = ~regs->orig_ax - INVALIDATE_TLB_VECTOR_START;
	f = &per_cpu(flush_state, sender);

	if (!cpu_isset(cpu, f->flush_cpumask))
		goto out;
		/*
		 * This was a BUG() but until someone can quote me the
		 * line from the intel manual that guarantees an IPI to
		 * multiple CPUs is retried _only_ on the erroring CPUs
		 * its staying as a return
		 *
		 * BUG();
		 */

	if (f->flush_mm == read_pda(active_mm)) {
		if (read_pda(mmu_state) == TLBSTATE_OK) {
			if (f->flush_va == TLB_FLUSH_ALL)
				local_flush_tlb();
			else
				__flush_tlb_one(f->flush_va);
		} else
			leave_mm(cpu);
	}
out:
	ack_APIC_irq();
	cpu_clear(cpu, f->flush_cpumask);
	inc_irq_stat(irq_tlb_count);
}

void native_flush_tlb_others(const cpumask_t *cpumaskp, struct mm_struct *mm,
			     unsigned long va)
{
	int sender;
	union smp_flush_state *f;
	cpumask_t cpumask = *cpumaskp;

	if (is_uv_system() && uv_flush_tlb_others(&cpumask, mm, va))
		return;

	/* Caller has disabled preemption */
	sender = smp_processor_id() % NUM_INVALIDATE_TLB_VECTORS;
	f = &per_cpu(flush_state, sender);

	/*
	 * Could avoid this lock when
	 * num_online_cpus() <= NUM_INVALIDATE_TLB_VECTORS, but it is
	 * probably not worth checking this for a cache-hot lock.
	 */
	spin_lock(&f->tlbstate_lock);

	f->flush_mm = mm;
	f->flush_va = va;
	cpus_or(f->flush_cpumask, cpumask, f->flush_cpumask);

	/*
	 * Make the above memory operations globally visible before
	 * sending the IPI.
	 */
	smp_mb();
	/*
	 * We have to send the IPI only to
	 * CPUs affected.
	 */
	send_IPI_mask(&cpumask, INVALIDATE_TLB_VECTOR_START + sender);

	while (!cpus_empty(f->flush_cpumask))
		cpu_relax();

	f->flush_mm = NULL;
	f->flush_va = 0;
	spin_unlock(&f->tlbstate_lock);
}

static int __cpuinit init_smp_flush(void)
{
	int i;

	for_each_possible_cpu(i)
		spin_lock_init(&per_cpu(flush_state, i).tlbstate_lock);

	return 0;
}
core_initcall(init_smp_flush);

void flush_tlb_current_task(void)
{
	struct mm_struct *mm = current->mm;
	cpumask_t cpu_mask;

	preempt_disable();
	cpu_mask = mm->cpu_vm_mask;
	cpu_clear(smp_processor_id(), cpu_mask);

	local_flush_tlb();
	if (!cpus_empty(cpu_mask))
		flush_tlb_others(cpu_mask, mm, TLB_FLUSH_ALL);
	preempt_enable();
}

void flush_tlb_mm(struct mm_struct *mm)
{
	cpumask_t cpu_mask;

	preempt_disable();
	cpu_mask = mm->cpu_vm_mask;
	cpu_clear(smp_processor_id(), cpu_mask);

	if (current->active_mm == mm) {
		if (current->mm)
			local_flush_tlb();
		else
			leave_mm(smp_processor_id());
	}
	if (!cpus_empty(cpu_mask))
		flush_tlb_others(cpu_mask, mm, TLB_FLUSH_ALL);

	preempt_enable();
}

void flush_tlb_page(struct vm_area_struct *vma, unsigned long va)
{
	struct mm_struct *mm = vma->vm_mm;
	cpumask_t cpu_mask;

	preempt_disable();
	cpu_mask = mm->cpu_vm_mask;
	cpu_clear(smp_processor_id(), cpu_mask);

	if (current->active_mm == mm) {
		if (current->mm)
			__flush_tlb_one(va);
		else
			leave_mm(smp_processor_id());
	}

	if (!cpus_empty(cpu_mask))
		flush_tlb_others(cpu_mask, mm, va);

	preempt_enable();
}

static void do_flush_tlb_all(void *info)
{
	unsigned long cpu = smp_processor_id();

	__flush_tlb_all();
	if (read_pda(mmu_state) == TLBSTATE_LAZY)
		leave_mm(cpu);
}

void flush_tlb_all(void)
{
	on_each_cpu(do_flush_tlb_all, NULL, 1);
}
