

#include <linux/pci.h>
#include <linux/init.h>
#include <linux/irq.h>

#include <asm/mach/pci.h>
#include <asm/mach-types.h>

void __init fsg_pci_preinit(void)
{
	set_irq_type(IRQ_FSG_PCI_INTA, IRQ_TYPE_LEVEL_LOW);
	set_irq_type(IRQ_FSG_PCI_INTB, IRQ_TYPE_LEVEL_LOW);
	set_irq_type(IRQ_FSG_PCI_INTC, IRQ_TYPE_LEVEL_LOW);

	ixp4xx_pci_preinit();
}

static int __init fsg_map_irq(struct pci_dev *dev, u8 slot, u8 pin)
{
	static int pci_irq_table[FSG_PCI_IRQ_LINES] = {
		IRQ_FSG_PCI_INTC,
		IRQ_FSG_PCI_INTB,
		IRQ_FSG_PCI_INTA,
	};

	int irq = -1;
	slot = slot - 11;

	if (slot >= 1 && slot <= FSG_PCI_MAX_DEV &&
	    pin >= 1 && pin <= FSG_PCI_IRQ_LINES)
		irq = pci_irq_table[(slot - 1)];
	printk(KERN_INFO "%s: Mapped slot %d pin %d to IRQ %d\n",
	       __func__, slot, pin, irq);

	return irq;
}

struct hw_pci fsg_pci __initdata = {
	.nr_controllers = 1,
	.preinit =	  fsg_pci_preinit,
	.swizzle =	  pci_std_swizzle,
	.setup =	  ixp4xx_setup,
	.scan =		  ixp4xx_scan_bus,
	.map_irq =	  fsg_map_irq,
};

int __init fsg_pci_init(void)
{
	if (machine_is_fsg())
		pci_common_init(&fsg_pci);
	return 0;
}

subsys_initcall(fsg_pci_init);