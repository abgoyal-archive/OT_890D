

#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/irq.h>

#include <asm/mach-types.h>
#include <mach/hardware.h>
#include <asm/irq.h>

#include <asm/mach/pci.h>

void __init coyote_pci_preinit(void)
{
	set_irq_type(IRQ_COYOTE_PCI_SLOT0, IRQ_TYPE_LEVEL_LOW);
	set_irq_type(IRQ_COYOTE_PCI_SLOT1, IRQ_TYPE_LEVEL_LOW);

	ixp4xx_pci_preinit();
}

static int __init coyote_map_irq(struct pci_dev *dev, u8 slot, u8 pin)
{
	if (slot == COYOTE_PCI_SLOT0_DEVID)
		return IRQ_COYOTE_PCI_SLOT0;
	else if (slot == COYOTE_PCI_SLOT1_DEVID)
		return IRQ_COYOTE_PCI_SLOT1;
	else return -1;
}

struct hw_pci coyote_pci __initdata = {
	.nr_controllers = 1,
	.preinit =        coyote_pci_preinit,
	.swizzle =        pci_std_swizzle,
	.setup =          ixp4xx_setup,
	.scan =           ixp4xx_scan_bus,
	.map_irq =        coyote_map_irq,
};

int __init coyote_pci_init(void)
{
	if (machine_is_adi_coyote())
		pci_common_init(&coyote_pci);
	return 0;
}

subsys_initcall(coyote_pci_init);
