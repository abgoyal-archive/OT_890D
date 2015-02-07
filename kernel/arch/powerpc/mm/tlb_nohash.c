

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/highmem.h>
#include <linux/pagemap.h>
#include <linux/preempt.h>
#include <linux/spinlock.h>

#include <asm/tlbflush.h>
#include <asm/tlb.h>

#include "mmu_decl.h"


void local_flush_tlb_mm(struct mm_struct *mm)
{
	unsigned int pid;

	preempt_disable();
	pid = mm->context.id;
	if (pid != MMU_NO_CONTEXT)
		_tlbil_pid(pid);
	preempt_enable();
}
EXPORT_SYMBOL(local_flush_tlb_mm);

void local_flush_tlb_page(struct vm_area_struct *vma, unsigned long vmaddr)
{
	unsigned int pid;

	preempt_disable();
	pid = vma ? vma->vm_mm->context.id : 0;
	if (pid != MMU_NO_CONTEXT)
		_tlbil_va(vmaddr, pid);
	preempt_enable();
}
EXPORT_SYMBOL(local_flush_tlb_page);


#ifdef CONFIG_SMP

static DEFINE_SPINLOCK(tlbivax_lock);

struct tlb_flush_param {
	unsigned long addr;
	unsigned int pid;
};

static void do_flush_tlb_mm_ipi(void *param)
{
	struct tlb_flush_param *p = param;

	_tlbil_pid(p ? p->pid : 0);
}

static void do_flush_tlb_page_ipi(void *param)
{
	struct tlb_flush_param *p = param;

	_tlbil_va(p->addr, p->pid);
}



void flush_tlb_mm(struct mm_struct *mm)
{
	cpumask_t cpu_mask;
	unsigned int pid;

	preempt_disable();
	pid = mm->context.id;
	if (unlikely(pid == MMU_NO_CONTEXT))
		goto no_context;
	cpu_mask = mm->cpu_vm_mask;
	cpu_clear(smp_processor_id(), cpu_mask);
	if (!cpus_empty(cpu_mask)) {
		struct tlb_flush_param p = { .pid = pid };
		smp_call_function_mask(cpu_mask, do_flush_tlb_mm_ipi, &p, 1);
	}
	_tlbil_pid(pid);
 no_context:
	preempt_enable();
}
EXPORT_SYMBOL(flush_tlb_mm);

void flush_tlb_page(struct vm_area_struct *vma, unsigned long vmaddr)
{
	cpumask_t cpu_mask;
	unsigned int pid;

	preempt_disable();
	pid = vma ? vma->vm_mm->context.id : 0;
	if (unlikely(pid == MMU_NO_CONTEXT))
		goto bail;
	cpu_mask = vma->vm_mm->cpu_vm_mask;
	cpu_clear(smp_processor_id(), cpu_mask);
	if (!cpus_empty(cpu_mask)) {
		/* If broadcast tlbivax is supported, use it */
		if (mmu_has_feature(MMU_FTR_USE_TLBIVAX_BCAST)) {
			int lock = mmu_has_feature(MMU_FTR_LOCK_BCAST_INVAL);
			if (lock)
				spin_lock(&tlbivax_lock);
			_tlbivax_bcast(vmaddr, pid);
			if (lock)
				spin_unlock(&tlbivax_lock);
			goto bail;
		} else {
			struct tlb_flush_param p = { .pid = pid, .addr = vmaddr };
			smp_call_function_mask(cpu_mask,
					       do_flush_tlb_page_ipi, &p, 1);
		}
	}
	_tlbil_va(vmaddr, pid);
 bail:
	preempt_enable();
}
EXPORT_SYMBOL(flush_tlb_page);

#endif /* CONFIG_SMP */

void flush_tlb_kernel_range(unsigned long start, unsigned long end)
{
#ifdef CONFIG_SMP
	preempt_disable();
	smp_call_function(do_flush_tlb_mm_ipi, NULL, 1);
	_tlbil_pid(0);
	preempt_enable();
#else
	_tlbil_pid(0);
#endif
}
EXPORT_SYMBOL(flush_tlb_kernel_range);

void flush_tlb_range(struct vm_area_struct *vma, unsigned long start,
		     unsigned long end)

{
	flush_tlb_mm(vma->vm_mm);
}
EXPORT_SYMBOL(flush_tlb_range);
