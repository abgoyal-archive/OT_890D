
#ifndef __LINUX_SMP_H
#define __LINUX_SMP_H


#include <linux/errno.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/cpumask.h>

extern void cpu_idle(void);

struct call_single_data {
	struct list_head list;
	void (*func) (void *info);
	void *info;
	u16 flags;
	u16 priv;
};

/* total number of cpus in this system (may exceed NR_CPUS) */
extern unsigned int total_cpus;

int smp_call_function_single(int cpuid, void (*func) (void *info), void *info,
				int wait);

#ifdef CONFIG_SMP

#include <linux/preempt.h>
#include <linux/kernel.h>
#include <linux/compiler.h>
#include <linux/thread_info.h>
#include <asm/smp.h>


extern void smp_send_stop(void);

extern void smp_send_reschedule(int cpu);


extern void smp_prepare_cpus(unsigned int max_cpus);

extern int __cpu_up(unsigned int cpunum);

extern void smp_cpus_done(unsigned int max_cpus);

int smp_call_function(void(*func)(void *info), void *info, int wait);
void smp_call_function_many(const struct cpumask *mask,
			    void (*func)(void *info), void *info, bool wait);

/* Deprecated: Use smp_call_function_many which takes a pointer to the mask. */
static inline int
smp_call_function_mask(cpumask_t mask, void(*func)(void *info), void *info,
		       int wait)
{
	smp_call_function_many(&mask, func, info, wait);
	return 0;
}

void __smp_call_function_single(int cpuid, struct call_single_data *data);

#ifdef CONFIG_USE_GENERIC_SMP_HELPERS
void generic_smp_call_function_single_interrupt(void);
void generic_smp_call_function_interrupt(void);
void ipi_call_lock(void);
void ipi_call_unlock(void);
void ipi_call_lock_irq(void);
void ipi_call_unlock_irq(void);
#endif

int on_each_cpu(void (*func) (void *info), void *info, int wait);

#define MSG_ALL_BUT_SELF	0x8000	/* Assume <32768 CPU's */
#define MSG_ALL			0x8001

#define MSG_INVALIDATE_TLB	0x0001	/* Remote processor TLB invalidate */
#define MSG_STOP_CPU		0x0002	/* Sent to shut down slave CPU's
					 * when rebooting
					 */
#define MSG_RESCHEDULE		0x0003	/* Reschedule request from master CPU*/
#define MSG_CALL_FUNCTION       0x0004  /* Call function on all other CPUs */

void smp_prepare_boot_cpu(void);

extern unsigned int setup_max_cpus;

#else /* !SMP */

#define raw_smp_processor_id()			0
static inline int up_smp_call_function(void (*func)(void *), void *info)
{
	return 0;
}
#define smp_call_function(func, info, wait) \
			(up_smp_call_function(func, info))
#define on_each_cpu(func,info,wait)		\
	({					\
		local_irq_disable();		\
		func(info);			\
		local_irq_enable();		\
		0;				\
	})
static inline void smp_send_reschedule(int cpu) { }
#define num_booting_cpus()			1
#define smp_prepare_boot_cpu()			do {} while (0)
#define smp_call_function_mask(mask, func, info, wait) \
			(up_smp_call_function(func, info))
#define smp_call_function_many(mask, func, info, wait) \
			(up_smp_call_function(func, info))
static inline void init_call_single_data(void)
{
}
#endif /* !SMP */

#ifdef CONFIG_DEBUG_PREEMPT
  extern unsigned int debug_smp_processor_id(void);
# define smp_processor_id() debug_smp_processor_id()
#else
# define smp_processor_id() raw_smp_processor_id()
#endif

#define get_cpu()		({ preempt_disable(); smp_processor_id(); })
#define put_cpu()		preempt_enable()
#define put_cpu_no_resched()	preempt_enable_no_resched()

void smp_setup_processor_id(void);

#endif /* __LINUX_SMP_H */
