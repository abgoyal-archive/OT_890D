
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/smp.h>
#include <linux/interrupt.h>
#include <linux/kernel_stat.h>
#include <linux/sched.h>
#include <linux/module.h>

#include <asm/mmu_context.h>
#include <asm/system.h>
#include <asm/time.h>

#include <asm/octeon/octeon.h>

volatile unsigned long octeon_processor_boot = 0xff;
volatile unsigned long octeon_processor_sp;
volatile unsigned long octeon_processor_gp;

static irqreturn_t mailbox_interrupt(int irq, void *dev_id)
{
	const int coreid = cvmx_get_core_num();
	uint64_t action;

	/* Load the mailbox register to figure out what we're supposed to do */
	action = cvmx_read_csr(CVMX_CIU_MBOX_CLRX(coreid));

	/* Clear the mailbox to clear the interrupt */
	cvmx_write_csr(CVMX_CIU_MBOX_CLRX(coreid), action);

	if (action & SMP_CALL_FUNCTION)
		smp_call_function_interrupt();

	/* Check if we've been told to flush the icache */
	if (action & SMP_ICACHE_FLUSH)
		asm volatile ("synci 0($0)\n");
	return IRQ_HANDLED;
}

void octeon_send_ipi_single(int cpu, unsigned int action)
{
	int coreid = cpu_logical_map(cpu);
	/*
	pr_info("SMP: Mailbox send cpu=%d, coreid=%d, action=%u\n", cpu,
	       coreid, action);
	*/
	cvmx_write_csr(CVMX_CIU_MBOX_SETX(coreid), action);
}

static inline void octeon_send_ipi_mask(cpumask_t mask, unsigned int action)
{
	unsigned int i;

	for_each_cpu_mask(i, mask)
		octeon_send_ipi_single(i, action);
}

static void octeon_smp_setup(void)
{
	const int coreid = cvmx_get_core_num();
	int cpus;
	int id;

	int core_mask = octeon_get_boot_coremask();

	cpus_clear(cpu_possible_map);
	__cpu_number_map[coreid] = 0;
	__cpu_logical_map[0] = coreid;
	cpu_set(0, cpu_possible_map);

	cpus = 1;
	for (id = 0; id < 16; id++) {
		if ((id != coreid) && (core_mask & (1 << id))) {
			cpu_set(cpus, cpu_possible_map);
			__cpu_number_map[id] = cpus;
			__cpu_logical_map[cpus] = id;
			cpus++;
		}
	}
}

static void octeon_boot_secondary(int cpu, struct task_struct *idle)
{
	int count;

	pr_info("SMP: Booting CPU%02d (CoreId %2d)...\n", cpu,
		cpu_logical_map(cpu));

	octeon_processor_sp = __KSTK_TOS(idle);
	octeon_processor_gp = (unsigned long)(task_thread_info(idle));
	octeon_processor_boot = cpu_logical_map(cpu);
	mb();

	count = 10000;
	while (octeon_processor_sp && count) {
		/* Waiting for processor to get the SP and GP */
		udelay(1);
		count--;
	}
	if (count == 0)
		pr_err("Secondary boot timeout\n");
}

static void octeon_init_secondary(void)
{
	const int coreid = cvmx_get_core_num();
	union cvmx_ciu_intx_sum0 interrupt_enable;

	octeon_check_cpu_bist();
	octeon_init_cvmcount();
	/*
	pr_info("SMP: CPU%d (CoreId %lu) started\n", cpu, coreid);
	*/
	/* Enable Mailbox interrupts to this core. These are the only
	   interrupts allowed on line 3 */
	cvmx_write_csr(CVMX_CIU_MBOX_CLRX(coreid), 0xffffffff);
	interrupt_enable.u64 = 0;
	interrupt_enable.s.mbox = 0x3;
	cvmx_write_csr(CVMX_CIU_INTX_EN0((coreid * 2)), interrupt_enable.u64);
	cvmx_write_csr(CVMX_CIU_INTX_EN0((coreid * 2 + 1)), 0);
	cvmx_write_csr(CVMX_CIU_INTX_EN1((coreid * 2)), 0);
	cvmx_write_csr(CVMX_CIU_INTX_EN1((coreid * 2 + 1)), 0);
	/* Enable core interrupt processing for 2,3 and 7 */
	set_c0_status(0x8c01);
}

void octeon_prepare_cpus(unsigned int max_cpus)
{
	cvmx_write_csr(CVMX_CIU_MBOX_CLRX(cvmx_get_core_num()), 0xffffffff);
	if (request_irq(OCTEON_IRQ_MBOX0, mailbox_interrupt, IRQF_SHARED,
			"mailbox0", mailbox_interrupt)) {
		panic("Cannot request_irq(OCTEON_IRQ_MBOX0)\n");
	}
	if (request_irq(OCTEON_IRQ_MBOX1, mailbox_interrupt, IRQF_SHARED,
			"mailbox1", mailbox_interrupt)) {
		panic("Cannot request_irq(OCTEON_IRQ_MBOX1)\n");
	}
}

static void octeon_smp_finish(void)
{
#ifdef CONFIG_CAVIUM_GDB
	unsigned long tmp;
	/* Pulse MCD0 signal on Ctrl-C to stop all the cores. Also set the MCD0
	   to be not masked by this core so we know the signal is received by
	   someone */
	asm volatile ("dmfc0 %0, $22\n"
		      "ori   %0, %0, 0x9100\n" "dmtc0 %0, $22\n" : "=r" (tmp));
#endif

	octeon_user_io_init();

	/* to generate the first CPU timer interrupt */
	write_c0_compare(read_c0_count() + mips_hpt_frequency / HZ);
}

static void octeon_cpus_done(void)
{
#ifdef CONFIG_CAVIUM_GDB
	unsigned long tmp;
	/* Pulse MCD0 signal on Ctrl-C to stop all the cores. Also set the MCD0
	   to be not masked by this core so we know the signal is received by
	   someone */
	asm volatile ("dmfc0 %0, $22\n"
		      "ori   %0, %0, 0x9100\n" "dmtc0 %0, $22\n" : "=r" (tmp));
#endif
}

struct plat_smp_ops octeon_smp_ops = {
	.send_ipi_single	= octeon_send_ipi_single,
	.send_ipi_mask		= octeon_send_ipi_mask,
	.init_secondary		= octeon_init_secondary,
	.smp_finish		= octeon_smp_finish,
	.cpus_done		= octeon_cpus_done,
	.boot_secondary		= octeon_boot_secondary,
	.smp_setup		= octeon_smp_setup,
	.prepare_cpus		= octeon_prepare_cpus,
};
