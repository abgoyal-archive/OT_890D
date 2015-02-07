
#ifndef __ASM_SMP_H
#define __ASM_SMP_H

#include <linux/threads.h>
#include <linux/cpumask.h>
#include <linux/bitops.h>

#if defined(__KERNEL__) && defined(CONFIG_SMP) && !defined(__ASSEMBLY__)

#include <asm/lowcore.h>
#include <asm/sigp.h>
#include <asm/ptrace.h>
#include <asm/system.h>

typedef struct
{
	int        intresting;
	sigp_ccode ccode; 
	__u32      status;
	__u16      cpu;
} sigp_info;

extern void machine_restart_smp(char *);
extern void machine_halt_smp(void);
extern void machine_power_off_smp(void);

#define NO_PROC_ID		0xFF		/* No processor magic marker */

 
#define PROC_CHANGE_PENALTY	20		/* Schedule penalty */

#define raw_smp_processor_id()	(S390_lowcore.cpu_data.cpu_nr)

static inline __u16 hard_smp_processor_id(void)
{
	return stap();
}

static inline int
smp_cpu_not_running(int cpu)
{
	__u32 status;

	switch (signal_processor_ps(&status, 0, cpu, sigp_sense)) {
	case sigp_order_code_accepted:
	case sigp_status_stored:
		/* Check for stopped and check stop state */
		if (status & 0x50)
			return 1;
		break;
	case sigp_not_operational:
		return 1;
	default:
		break;
	}
	return 0;
}

#define cpu_logical_map(cpu) (cpu)

extern int __cpu_disable (void);
extern void __cpu_die (unsigned int cpu);
extern void cpu_die (void) __attribute__ ((noreturn));
extern int __cpu_up (unsigned int cpu);

extern struct mutex smp_cpu_state_mutex;
extern int smp_cpu_polarization[];

extern void arch_send_call_function_single_ipi(int cpu);
extern void arch_send_call_function_ipi(cpumask_t mask);

#endif

#ifndef CONFIG_SMP
static inline void smp_send_stop(void)
{
	/* Disable all interrupts/machine checks */
	__load_psw_mask(psw_kernel_bits & ~PSW_MASK_MCHECK);
}

#define hard_smp_processor_id()		0
#define smp_cpu_not_running(cpu)	1
#endif

#ifdef CONFIG_HOTPLUG_CPU
extern int smp_rescan_cpus(void);
#else
static inline int smp_rescan_cpus(void) { return 0; }
#endif

extern union save_area *zfcpdump_save_areas[NR_CPUS + 1];
#endif
