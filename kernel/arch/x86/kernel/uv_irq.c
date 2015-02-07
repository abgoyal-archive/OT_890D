

#include <linux/module.h>
#include <linux/irq.h>

#include <asm/apic.h>
#include <asm/uv/uv_irq.h>

static void uv_noop(unsigned int irq)
{
}

static unsigned int uv_noop_ret(unsigned int irq)
{
	return 0;
}

static void uv_ack_apic(unsigned int irq)
{
	ack_APIC_irq();
}

struct irq_chip uv_irq_chip = {
	.name		= "UV-CORE",
	.startup	= uv_noop_ret,
	.shutdown	= uv_noop,
	.enable		= uv_noop,
	.disable	= uv_noop,
	.ack		= uv_noop,
	.mask		= uv_noop,
	.unmask		= uv_noop,
	.eoi		= uv_ack_apic,
	.end		= uv_noop,
};

int uv_setup_irq(char *irq_name, int cpu, int mmr_blade,
		 unsigned long mmr_offset)
{
	int irq;
	int ret;

	irq = create_irq();
	if (irq <= 0)
		return -EBUSY;

	ret = arch_enable_uv_irq(irq_name, irq, cpu, mmr_blade, mmr_offset);
	if (ret != irq)
		destroy_irq(irq);

	return ret;
}
EXPORT_SYMBOL_GPL(uv_setup_irq);

void uv_teardown_irq(unsigned int irq, int mmr_blade, unsigned long mmr_offset)
{
	arch_disable_uv_irq(mmr_blade, mmr_offset);
	destroy_irq(irq);
}
EXPORT_SYMBOL_GPL(uv_teardown_irq);
