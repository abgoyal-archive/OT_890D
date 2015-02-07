
#ifndef M68KNOMMU_PCI_H
#define	M68KNOMMU_PCI_H

#include <asm/pci_mm.h>

#ifdef CONFIG_COMEMPCI
#define PCIBIOS_MIN_IO		0x100
#define PCIBIOS_MIN_MEM		0x00010000

#define pcibios_scan_all_fns(a, b)	0

static inline int pci_dma_supported(struct pci_dev *hwdev, u64 mask)
{
	return 1;
}

#endif /* CONFIG_COMEMPCI */

#endif /* M68KNOMMU_PCI_H */
