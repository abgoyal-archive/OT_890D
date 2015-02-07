
#include <linux/kernel.h>
#include <linux/sched.h>

void up_send_ipi_single(int cpu, unsigned int action)
{
	panic(KERN_ERR "%s called", __func__);
}

static inline void up_send_ipi_mask(cpumask_t mask, unsigned int action)
{
	panic(KERN_ERR "%s called", __func__);
}

void __cpuinit up_init_secondary(void)
{
}

void __cpuinit up_smp_finish(void)
{
}

/* Hook for after all CPUs are online */
void up_cpus_done(void)
{
}

void __cpuinit up_boot_secondary(int cpu, struct task_struct *idle)
{
}

void __init up_smp_setup(void)
{
}

void __init up_prepare_cpus(unsigned int max_cpus)
{
}

struct plat_smp_ops up_smp_ops = {
	.send_ipi_single	= up_send_ipi_single,
	.send_ipi_mask		= up_send_ipi_mask,
	.init_secondary		= up_init_secondary,
	.smp_finish		= up_smp_finish,
	.cpus_done		= up_cpus_done,
	.boot_secondary		= up_boot_secondary,
	.smp_setup		= up_smp_setup,
	.prepare_cpus		= up_prepare_cpus,
};
