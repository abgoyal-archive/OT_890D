

#include <linux/pci.h>
#include <linux/init.h>
#include <linux/irq.h>

#include <asm/mach/pci.h>
#include <asm/mach-types.h>

void __init dsmg600_pci_preinit(void)
{
	set_irq_type(IRQ_DSMG600_PCI_INTA, IRQ_TYPE_LEVEL_LOW);
	set_irq_type(IRQ_DSMG600_PCI_INTB, IRQ_TYPE_LEVEL_LOW);
	set_irq_type(IRQ_DSMG600_PCI_INTC, IRQ_TYPE_LEVEL_LOW);
	set_irq_type(IRQ_DSMG600_PCI_INTD, IRQ_TYPE_LEVEL_LOW);
	set_irq_type(IRQ_DSMG600_PCI_INTE, IRQ_TYPE_LEVEL_LOW);
	set_irq_type(IRQ_DSMG600_PCI_INTF, IRQ_TYPE_LEVEL_LOW);

	ixp4xx_pci_preinit();
}

static int __init dsmg600_map_irq(struct pci_dev *dev, u8 slot, u8 pin)
{
	static int pci_irq_table[DSMG600_PCI_MAX_DEV][DSMG600_PCI_IRQ_LINES] =
	{
		{ IRQ_DSMG600_PCI_INTE, -1, -1 },
		{ IRQ_DSMG600_PCI_INTA, -1, -1 },
		{ IRQ_DSMG600_PCI_INTB, IRQ_DSMG600_PCI_INTC, IRQ_DSMG600_PCI_INTD },
		{ IRQ_DSMG600_PCI_INTF, -1, -1 },
	};

	int irq = -1;

	if (slot >= 1 && slot <= DSMG600_PCI_MAX_DEV &&
		pin >= 1 && pin <= DSMG600_PCI_IRQ_LINES)
		irq = pci_irq_table[slot-1][pin-1];

	return irq;
}

struct hw_pci __initdata dsmg600_pci = {
	.nr_controllers = 1,
	.preinit	= dsmg600_pci_preinit,
	.swizzle	= pci_std_swizzle,
	.setup		= ixp4xx_setup,
	.scan		= ixp4xx_scan_bus,
	.map_irq	= dsmg600_map_irq,
};

int __init dsmg600_pci_init(void)
{
	if (machine_is_dsmg600())
		pci_common_init(&dsmg600_pci);

	return 0;
}

subsys_initcall(dsmg600_pci_init);
