

#include <linux/pci.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/irq.h>

#include <asm/mach-types.h>
#include <mach/hardware.h>
#include <mach/gtwx5715.h>
#include <asm/mach/pci.h>

void __init gtwx5715_pci_preinit(void)
{
	set_irq_type(GTWX5715_PCI_SLOT0_INTA_IRQ, IRQ_TYPE_LEVEL_LOW);
	set_irq_type(GTWX5715_PCI_SLOT0_INTB_IRQ, IRQ_TYPE_LEVEL_LOW);
	set_irq_type(GTWX5715_PCI_SLOT1_INTA_IRQ, IRQ_TYPE_LEVEL_LOW);
	set_irq_type(GTWX5715_PCI_SLOT1_INTB_IRQ, IRQ_TYPE_LEVEL_LOW);

	ixp4xx_pci_preinit();
}


static int __init gtwx5715_map_irq(struct pci_dev *dev, u8 slot, u8 pin)
{
	int rc;
	static int gtwx5715_irqmap
			[GTWX5715_PCI_SLOT_COUNT]
			[GTWX5715_PCI_INT_PIN_COUNT] = {
	{GTWX5715_PCI_SLOT0_INTA_IRQ, GTWX5715_PCI_SLOT0_INTB_IRQ},
	{GTWX5715_PCI_SLOT1_INTA_IRQ, GTWX5715_PCI_SLOT1_INTB_IRQ},
};

	if (slot >= GTWX5715_PCI_SLOT_COUNT ||
			pin >= GTWX5715_PCI_INT_PIN_COUNT) rc = -1;
	else
		rc = gtwx5715_irqmap[slot][pin-1];

	printk("%s: Mapped slot %d pin %d to IRQ %d\n", __func__, slot, pin, rc);
	return(rc);
}

struct hw_pci gtwx5715_pci __initdata = {
	.nr_controllers = 1,
	.preinit =        gtwx5715_pci_preinit,
	.swizzle =        pci_std_swizzle,
	.setup =          ixp4xx_setup,
	.scan =           ixp4xx_scan_bus,
	.map_irq =        gtwx5715_map_irq,
};

int __init gtwx5715_pci_init(void)
{
	if (machine_is_gtwx5715())
	{
		pci_common_init(&gtwx5715_pci);
	}

	return 0;
}

subsys_initcall(gtwx5715_pci_init);
