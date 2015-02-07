

#include <linux/smp.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <asm/acpi.h>
#include <asm/arch_hooks.h>
#include <asm/e820.h>
#include <asm/setup.h>

#include <mach_ipi.h>

#ifdef CONFIG_HOTPLUG_CPU
#define DEFAULT_SEND_IPI	(1)
#else
#define DEFAULT_SEND_IPI	(0)
#endif

int no_broadcast = DEFAULT_SEND_IPI;

void __init pre_intr_init_hook(void)
{
	if (x86_quirks->arch_pre_intr_init) {
		if (x86_quirks->arch_pre_intr_init())
			return;
	}
	init_ISA_irqs();
}

static struct irqaction irq2 = {
	.handler = no_action,
	.mask = CPU_MASK_NONE,
	.name = "cascade",
};

void __init intr_init_hook(void)
{
	if (x86_quirks->arch_intr_init) {
		if (x86_quirks->arch_intr_init())
			return;
	}
	if (!acpi_ioapic)
		setup_irq(2, &irq2);

}

void __init pre_setup_arch_hook(void)
{
}

void __init trap_init_hook(void)
{
	if (x86_quirks->arch_trap_init) {
		if (x86_quirks->arch_trap_init())
			return;
	}
}

static struct irqaction irq0  = {
	.handler = timer_interrupt,
	.flags = IRQF_DISABLED | IRQF_NOBALANCING | IRQF_IRQPOLL | IRQF_TIMER,
	.mask = CPU_MASK_NONE,
	.name = "timer"
};

void __init pre_time_init_hook(void)
{
	if (x86_quirks->arch_pre_time_init)
		x86_quirks->arch_pre_time_init();
}

void __init time_init_hook(void)
{
	if (x86_quirks->arch_time_init) {
		/*
		 * A nonzero return code does not mean failure, it means
		 * that the architecture quirk does not want any
		 * generic (timer) setup to be performed after this:
		 */
		if (x86_quirks->arch_time_init())
			return;
	}

	irq0.mask = cpumask_of_cpu(0);
	setup_irq(0, &irq0);
}

#ifdef CONFIG_MCA
void mca_nmi_hook(void)
{
	/*
	 * If I recall correctly, there's a whole bunch of other things that
	 * we can do to check for NMI problems, but that's all I know about
	 * at the moment.
	 */
	pr_warning("NMI generated from unknown source!\n");
}
#endif

static __init int no_ipi_broadcast(char *str)
{
	get_option(&str, &no_broadcast);
	pr_info("Using %s mode\n",
		no_broadcast ? "No IPI Broadcast" : "IPI Broadcast");
	return 1;
}
__setup("no_ipi_broadcast=", no_ipi_broadcast);

static int __init print_ipi_mode(void)
{
	pr_info("Using IPI %s mode\n",
		no_broadcast ? "No-Shortcut" : "Shortcut");
	return 0;
}

late_initcall(print_ipi_mode);

