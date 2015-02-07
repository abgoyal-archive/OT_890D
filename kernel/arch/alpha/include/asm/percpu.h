
#ifndef __ALPHA_PERCPU_H
#define __ALPHA_PERCPU_H
#include <linux/compiler.h>
#include <linux/threads.h>

#define per_cpu_var(var) per_cpu__##var

#ifdef CONFIG_SMP

extern unsigned long __per_cpu_offset[NR_CPUS];

#define per_cpu_offset(x) (__per_cpu_offset[x])

#define __my_cpu_offset per_cpu_offset(raw_smp_processor_id())
#ifdef CONFIG_DEBUG_PREEMPT
#define my_cpu_offset per_cpu_offset(smp_processor_id())
#else
#define my_cpu_offset __my_cpu_offset
#endif

#ifndef MODULE
#define SHIFT_PERCPU_PTR(var, offset) RELOC_HIDE(&per_cpu_var(var), (offset))
#define PER_CPU_ATTRIBUTES
#else
#define SHIFT_PERCPU_PTR(var, offset) ({		\
	extern int simple_identifier_##var(void);	\
	unsigned long __ptr, tmp_gp;			\
	asm (  "br	%1, 1f		  	      \n\
	1:	ldgp	%1, 0(%1)	    	      \n\
		ldq %0, per_cpu__" #var"(%1)\t!literal"		\
		: "=&r"(__ptr), "=&r"(tmp_gp));		\
	(typeof(&per_cpu_var(var)))(__ptr + (offset)); })

#define PER_CPU_ATTRIBUTES	__used

#endif /* MODULE */

#define per_cpu(var, cpu) \
	(*SHIFT_PERCPU_PTR(var, per_cpu_offset(cpu)))
#define __get_cpu_var(var) \
	(*SHIFT_PERCPU_PTR(var, my_cpu_offset))
#define __raw_get_cpu_var(var) \
	(*SHIFT_PERCPU_PTR(var, __my_cpu_offset))

#else /* ! SMP */

#define per_cpu(var, cpu)		(*((void)(cpu), &per_cpu_var(var)))
#define __get_cpu_var(var)		per_cpu_var(var)
#define __raw_get_cpu_var(var)		per_cpu_var(var)

#define PER_CPU_ATTRIBUTES

#endif /* SMP */

#define DECLARE_PER_CPU(type, name) extern __typeof__(type) per_cpu_var(name)

#endif /* __ALPHA_PERCPU_H */
