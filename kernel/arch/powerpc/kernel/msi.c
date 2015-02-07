

#include <linux/kernel.h>
#include <linux/msi.h>

#include <asm/machdep.h>

int arch_msi_check_device(struct pci_dev* dev, int nvec, int type)
{
	if (!ppc_md.setup_msi_irqs || !ppc_md.teardown_msi_irqs) {
		pr_debug("msi: Platform doesn't provide MSI callbacks.\n");
		return -ENOSYS;
	}

	if (ppc_md.msi_check_device) {
		pr_debug("msi: Using platform check routine.\n");
		return ppc_md.msi_check_device(dev, nvec, type);
	}

        return 0;
}

int arch_setup_msi_irqs(struct pci_dev *dev, int nvec, int type)
{
	return ppc_md.setup_msi_irqs(dev, nvec, type);
}

void arch_teardown_msi_irqs(struct pci_dev *dev)
{
	ppc_md.teardown_msi_irqs(dev);
}
