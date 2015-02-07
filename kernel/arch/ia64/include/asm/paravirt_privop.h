

#ifndef _ASM_IA64_PARAVIRT_PRIVOP_H
#define _ASM_IA64_PARAVIRT_PRIVOP_H

#ifdef CONFIG_PARAVIRT

#ifndef __ASSEMBLY__

#include <linux/types.h>
#include <asm/kregs.h> /* for IA64_PSR_I */


struct pv_cpu_ops {
	void (*fc)(unsigned long addr);
	unsigned long (*thash)(unsigned long addr);
	unsigned long (*get_cpuid)(int index);
	unsigned long (*get_pmd)(int index);
	unsigned long (*getreg)(int reg);
	void (*setreg)(int reg, unsigned long val);
	void (*ptcga)(unsigned long addr, unsigned long size);
	unsigned long (*get_rr)(unsigned long index);
	void (*set_rr)(unsigned long index, unsigned long val);
	void (*set_rr0_to_rr4)(unsigned long val0, unsigned long val1,
			       unsigned long val2, unsigned long val3,
			       unsigned long val4);
	void (*ssm_i)(void);
	void (*rsm_i)(void);
	unsigned long (*get_psr_i)(void);
	void (*intrin_local_irq_restore)(unsigned long flags);
};

extern struct pv_cpu_ops pv_cpu_ops;

extern void ia64_native_setreg_func(int regnum, unsigned long val);
extern unsigned long ia64_native_getreg_func(int regnum);

/************************************************/
/* Instructions paravirtualized for performance */
/************************************************/

#define paravirt_ssm(mask)			\
	do {					\
		if ((mask) == IA64_PSR_I)	\
			pv_cpu_ops.ssm_i();	\
		else				\
			ia64_native_ssm(mask);	\
	} while (0)

#define paravirt_rsm(mask)			\
	do {					\
		if ((mask) == IA64_PSR_I)	\
			pv_cpu_ops.rsm_i();	\
		else				\
			ia64_native_rsm(mask);	\
	} while (0)

#define paravirt_getreg(reg)					\
	({							\
		unsigned long res;				\
		if ((reg) == _IA64_REG_IP)			\
			res = ia64_native_getreg(_IA64_REG_IP); \
		else						\
			res = pv_cpu_ops.getreg(reg);		\
		res;						\
	})

struct pv_cpu_asm_switch {
	unsigned long switch_to;
	unsigned long leave_syscall;
	unsigned long work_processed_syscall;
	unsigned long leave_kernel;
};
void paravirt_cpu_asm_init(const struct pv_cpu_asm_switch *cpu_asm_switch);

#endif /* __ASSEMBLY__ */

#define IA64_PARAVIRT_ASM_FUNC(name)	paravirt_ ## name

#else

/* fallback for native case */
#define IA64_PARAVIRT_ASM_FUNC(name)	ia64_native_ ## name

#endif /* CONFIG_PARAVIRT */

#define ia64_switch_to			IA64_PARAVIRT_ASM_FUNC(switch_to)
#define ia64_leave_syscall		IA64_PARAVIRT_ASM_FUNC(leave_syscall)
#define ia64_work_processed_syscall	\
	IA64_PARAVIRT_ASM_FUNC(work_processed_syscall)
#define ia64_leave_kernel		IA64_PARAVIRT_ASM_FUNC(leave_kernel)

#endif /* _ASM_IA64_PARAVIRT_PRIVOP_H */
