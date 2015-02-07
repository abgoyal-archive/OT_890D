
#ifndef _ASM_X86_TIMER_H
#define _ASM_X86_TIMER_H
#include <linux/init.h>
#include <linux/pm.h>
#include <linux/percpu.h>

#define TICK_SIZE (tick_nsec / 1000)

unsigned long long native_sched_clock(void);
unsigned long native_calibrate_tsc(void);

#ifdef CONFIG_X86_32
extern int timer_ack;
extern int recalibrate_cpu_khz(void);
#endif /* CONFIG_X86_32 */

extern int no_timer_check;

#ifndef CONFIG_PARAVIRT
#define calibrate_tsc() native_calibrate_tsc()
#endif


DECLARE_PER_CPU(unsigned long, cyc2ns);

#define CYC2NS_SCALE_FACTOR 10 /* 2^10, carefully chosen */

static inline unsigned long long __cycles_2_ns(unsigned long long cyc)
{
	return cyc * per_cpu(cyc2ns, smp_processor_id()) >> CYC2NS_SCALE_FACTOR;
}

static inline unsigned long long cycles_2_ns(unsigned long long cyc)
{
	unsigned long long ns;
	unsigned long flags;

	local_irq_save(flags);
	ns = __cycles_2_ns(cyc);
	local_irq_restore(flags);

	return ns;
}

#endif /* _ASM_X86_TIMER_H */
