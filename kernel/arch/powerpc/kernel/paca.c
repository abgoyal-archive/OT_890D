

#include <linux/threads.h>
#include <linux/module.h>

#include <asm/lppaca.h>
#include <asm/paca.h>
#include <asm/sections.h>

extern unsigned long __toc_start;

struct lppaca lppaca[] = {
	[0 ... (NR_CPUS-1)] = {
		.desc = 0xd397d781,	/* "LpPa" */
		.size = sizeof(struct lppaca),
		.dyn_proc_status = 2,
		.decr_val = 0x00ff0000,
		.fpregs_in_use = 1,
		.end_of_quantum = 0xfffffffffffffffful,
		.slb_count = 64,
		.vmxregs_in_use = 0,
		.page_ins = 0,
	},
};

struct slb_shadow slb_shadow[] __cacheline_aligned = {
	[0 ... (NR_CPUS-1)] = {
		.persistent = SLB_NUM_BOLTED,
		.buffer_length = sizeof(struct slb_shadow),
	},
};

struct paca_struct paca[NR_CPUS];
EXPORT_SYMBOL(paca);

void __init initialise_pacas(void)
{
	int cpu;

	/* The TOC register (GPR2) points 32kB into the TOC, so that 64kB
	 * of the TOC can be addressed using a single machine instruction.
	 */
	unsigned long kernel_toc = (unsigned long)(&__toc_start) + 0x8000UL;

	/* Can't use for_each_*_cpu, as they aren't functional yet */
	for (cpu = 0; cpu < NR_CPUS; cpu++) {
		struct paca_struct *new_paca = &paca[cpu];

		new_paca->lppaca_ptr = &lppaca[cpu];
		new_paca->lock_token = 0x8000;
		new_paca->paca_index = cpu;
		new_paca->kernel_toc = kernel_toc;
		new_paca->kernelbase = (unsigned long) _stext;
		new_paca->kernel_msr = MSR_KERNEL;
		new_paca->hw_cpu_id = 0xffff;
		new_paca->slb_shadow_ptr = &slb_shadow[cpu];
		new_paca->__current = &init_task;

	}
}
