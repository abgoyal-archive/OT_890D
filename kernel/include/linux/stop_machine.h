
#ifndef _LINUX_STOP_MACHINE
#define _LINUX_STOP_MACHINE
#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <asm/system.h>

#if defined(CONFIG_STOP_MACHINE) && defined(CONFIG_SMP)

int stop_machine(int (*fn)(void *), void *data, const struct cpumask *cpus);

int __stop_machine(int (*fn)(void *), void *data, const struct cpumask *cpus);

int stop_machine_create(void);

void stop_machine_destroy(void);

#else

static inline int stop_machine(int (*fn)(void *), void *data,
			       const struct cpumask *cpus)
{
	int ret;
	local_irq_disable();
	ret = fn(data);
	local_irq_enable();
	return ret;
}

static inline int stop_machine_create(void) { return 0; }
static inline void stop_machine_destroy(void) { }

#endif /* CONFIG_SMP */
#endif /* _LINUX_STOP_MACHINE */
