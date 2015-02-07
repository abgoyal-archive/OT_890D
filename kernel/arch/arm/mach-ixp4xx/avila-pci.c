

#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/delay.h>

#include <asm/mach/pci.h>
#include <asm/irq.h>
#include <mach/hardware.h>
#include <asm/mach-types.h>

void __init avila_pci_preinit(void)
{
	set_irq_type(IRQ_AVILA_PCI_INTA, IRQ_TYPE_LEVEL_LOW);
	set_irq_type(IRQ_AVILA_PCI_INTB, IRQ_TYPE_LEVEL_LOW);
	set_irq_type(IRQ_AVILA_PCI_INTC, IRQ_TYPE_LEVEL_LOW);
	set_irq_type(IRQ_AVILA_PCI_INTD, IRQ_TYPE_LEVEL_LOW);

	ixp4xx_pci_preinit();
}

static int __init avila_map_irq(struct pci_dev *dev, u8 slot, u8 pin)
{
	static int pci_irq_table[AVILA_PCI_IRQ_LINES] = {
		IRQ_AVILA_PCI_INTA,
		IRQ_AVILA_PCI_INTB,
		IRQ_AVILA_PCI_INTC,
		IRQ_AVILA_PCI_INTD
	};

	int irq = -1;

	if (slot >= 1 &&
	slot <= (machine_is_loft() ? LOFT_PCI_MAX_DEV : AVILA_PCI_MAX_DEV) &&
		pin >= 1 && pin <= AVILA_PCI_IRQ_LINES) {
		irq = pci_irq_table[(slot + pin - 2) % 4];
	}

	return irq;
}

struct hw_pci avila_pci __initdata = {
	.nr_controllers = 1,
	.preinit	= avila_pci_preinit,
	.swizzle	= pci_std_swizzle,
	.setup		= ixp4xx_setup,
	.scan		= ixp4xx_scan_bus,
	.map_irq	= avila_map_irq,
};

int __init avila_pci_init(void)
{
	if (machine_is_avila() || machine_is_loft())
		pci_common_init(&avila_pci);
	return 0;
}

subsys_initcall(avila_pci_init);

