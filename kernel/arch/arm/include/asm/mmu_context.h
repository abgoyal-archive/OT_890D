
#ifndef __ASM_ARM_MMU_CONTEXT_H
#define __ASM_ARM_MMU_CONTEXT_H

#include <linux/compiler.h>
#include <linux/sched.h>
#include <asm/cacheflush.h>
#include <asm/cachetype.h>
#include <asm/proc-fns.h>
#include <asm-generic/mm_hooks.h>

void __check_kvm_seq(struct mm_struct *mm);

#ifdef CONFIG_CPU_HAS_ASID

#define ASID_BITS		8
#define ASID_MASK		((~0) << ASID_BITS)
#define ASID_FIRST_VERSION	(1 << ASID_BITS)

extern unsigned int cpu_last_asid;

void __init_new_context(struct task_struct *tsk, struct mm_struct *mm);
void __new_context(struct mm_struct *mm);

static inline void check_context(struct mm_struct *mm)
{
	if (unlikely((mm->context.id ^ cpu_last_asid) >> ASID_BITS))
		__new_context(mm);

	if (unlikely(mm->context.kvm_seq != init_mm.context.kvm_seq))
		__check_kvm_seq(mm);
}

#define init_new_context(tsk,mm)	(__init_new_context(tsk,mm),0)

#else

static inline void check_context(struct mm_struct *mm)
{
	if (unlikely(mm->context.kvm_seq != init_mm.context.kvm_seq))
		__check_kvm_seq(mm);
}

#define init_new_context(tsk,mm)	0

#endif

#define destroy_context(mm)		do { } while(0)

static inline void
enter_lazy_tlb(struct mm_struct *mm, struct task_struct *tsk)
{
}

static inline void
switch_mm(struct mm_struct *prev, struct mm_struct *next,
	  struct task_struct *tsk)
{
#ifdef CONFIG_MMU
	unsigned int cpu = smp_processor_id();

#ifdef CONFIG_SMP
	/* check for possible thread migration */
	if (!cpus_empty(next->cpu_vm_mask) && !cpu_isset(cpu, next->cpu_vm_mask))
		__flush_icache_all();
#endif
	if (!cpu_test_and_set(cpu, next->cpu_vm_mask) || prev != next) {
		check_context(next);
		cpu_switch_mm(next->pgd, next);
		if (cache_is_vivt())
			cpu_clear(cpu, prev->cpu_vm_mask);
	}
#endif
}

#define deactivate_mm(tsk,mm)	do { } while (0)
#define activate_mm(prev,next)	switch_mm(prev, next, NULL)

#endif
