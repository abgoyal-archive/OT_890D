

#ifndef ASM_PCI_H
#define	ASM_PCI_H

#include <linux/mm.h>
#include <asm/scatterlist.h>
#include <asm-generic/pci-dma-compat.h>
#include <asm-generic/pci.h>

struct pci_dev;

#define pcibios_assign_all_busses()	0

extern void pcibios_set_master(struct pci_dev *dev);

extern void pcibios_penalize_isa_irq(int irq);

#ifdef CONFIG_MMU
extern void *consistent_alloc(gfp_t gfp, size_t size, dma_addr_t *dma_handle);
extern void consistent_free(void *vaddr);
extern void consistent_sync(void *vaddr, size_t size, int direction);
extern void consistent_sync_page(struct page *page, unsigned long offset,
				 size_t size, int direction);
#endif

extern void *pci_alloc_consistent(struct pci_dev *hwdev, size_t size,
				  dma_addr_t *dma_handle);

extern void pci_free_consistent(struct pci_dev *hwdev, size_t size,
				void *vaddr, dma_addr_t dma_handle);

/* Return the index of the PCI controller for device PDEV. */
#define pci_controller_num(PDEV)	(0)

#define PCI_DMA_BUS_IS_PHYS	(1)

/* pci_unmap_{page,single} is a nop so... */
#define DECLARE_PCI_UNMAP_ADDR(ADDR_NAME)
#define DECLARE_PCI_UNMAP_LEN(LEN_NAME)
#define pci_unmap_addr(PTR, ADDR_NAME)		(0)
#define pci_unmap_addr_set(PTR, ADDR_NAME, VAL)	do { } while (0)
#define pci_unmap_len(PTR, LEN_NAME)		(0)
#define pci_unmap_len_set(PTR, LEN_NAME, VAL)	do { } while (0)

#ifdef CONFIG_PCI
static inline void pci_dma_burst_advice(struct pci_dev *pdev,
					enum pci_dma_burst_strategy *strat,
					unsigned long *strategy_parameter)
{
	*strat = PCI_DMA_BURST_INFINITY;
	*strategy_parameter = ~0UL;
}
#endif

#define PCIBIOS_MIN_IO		0x100
#define PCIBIOS_MIN_MEM		0x00010000

static inline void pci_dma_sync_single(struct pci_dev *hwdev,
				       dma_addr_t dma_handle,
				       size_t size, int direction)
{
	if (direction == PCI_DMA_NONE)
                BUG();

	frv_cache_wback_inv((unsigned long)bus_to_virt(dma_handle),
			    (unsigned long)bus_to_virt(dma_handle) + size);
}

static inline void pci_dma_sync_sg(struct pci_dev *hwdev,
				   struct scatterlist *sg,
				   int nelems, int direction)
{
	int i;

	if (direction == PCI_DMA_NONE)
                BUG();

	for (i = 0; i < nelems; i++)
		frv_cache_wback_inv(sg_dma_address(&sg[i]),
				    sg_dma_address(&sg[i])+sg_dma_len(&sg[i]));
}


#endif
