
#ifndef __ALPHA_PCI_H
#define __ALPHA_PCI_H

#ifdef __KERNEL__

#include <linux/spinlock.h>
#include <linux/dma-mapping.h>
#include <asm/scatterlist.h>
#include <asm/machvec.h>


struct pci_dev;
struct pci_bus;
struct resource;
struct pci_iommu_arena;
struct page;

/* A controller.  Used to manage multiple PCI busses.  */

struct pci_controller {
	struct pci_controller *next;
        struct pci_bus *bus;
	struct resource *io_space;
	struct resource *mem_space;

	/* The following are for reporting to userland.  The invariant is
	   that if we report a BWX-capable dense memory, we do not report
	   a sparse memory at all, even if it exists.  */
	unsigned long sparse_mem_base;
	unsigned long dense_mem_base;
	unsigned long sparse_io_base;
	unsigned long dense_io_base;

	/* This one's for the kernel only.  It's in KSEG somewhere.  */
	unsigned long config_space_base;

	unsigned int index;
	/* For compatibility with current (as of July 2003) pciutils
	   and XFree86. Eventually will be removed. */
	unsigned int need_domain_info;

	struct pci_iommu_arena *sg_pci;
	struct pci_iommu_arena *sg_isa;

	void *sysdata;
};


#define pcibios_assign_all_busses()	1
#define pcibios_scan_all_fns(a, b)	0

#define PCIBIOS_MIN_IO		alpha_mv.min_io_address
#define PCIBIOS_MIN_MEM		alpha_mv.min_mem_address

extern void pcibios_set_master(struct pci_dev *dev);

extern inline void pcibios_penalize_isa_irq(int irq, int active)
{
	/* We don't do dynamic PCI IRQ allocation */
}

/* IOMMU controls.  */

#define PCI_DMA_BUS_IS_PHYS  0


extern void *__pci_alloc_consistent(struct pci_dev *, size_t,
				    dma_addr_t *, gfp_t);
static inline void *
pci_alloc_consistent(struct pci_dev *dev, size_t size, dma_addr_t *dma)
{
	return __pci_alloc_consistent(dev, size, dma, GFP_ATOMIC);
}


extern void pci_free_consistent(struct pci_dev *, size_t, void *, dma_addr_t);


extern dma_addr_t pci_map_single(struct pci_dev *, void *, size_t, int);

/* Likewise, but for a page instead of an address.  */
extern dma_addr_t pci_map_page(struct pci_dev *, struct page *,
			       unsigned long, size_t, int);

/* Test for pci_map_single or pci_map_page having generated an error.  */

static inline int
pci_dma_mapping_error(struct pci_dev *pdev, dma_addr_t dma_addr)
{
	return dma_addr == 0;
}


extern void pci_unmap_single(struct pci_dev *, dma_addr_t, size_t, int);
extern void pci_unmap_page(struct pci_dev *, dma_addr_t, size_t, int);

/* pci_unmap_{single,page} is not a nop, thus... */
#define DECLARE_PCI_UNMAP_ADDR(ADDR_NAME)	\
	dma_addr_t ADDR_NAME;
#define DECLARE_PCI_UNMAP_LEN(LEN_NAME)		\
	__u32 LEN_NAME;
#define pci_unmap_addr(PTR, ADDR_NAME)			\
	((PTR)->ADDR_NAME)
#define pci_unmap_addr_set(PTR, ADDR_NAME, VAL)		\
	(((PTR)->ADDR_NAME) = (VAL))
#define pci_unmap_len(PTR, LEN_NAME)			\
	((PTR)->LEN_NAME)
#define pci_unmap_len_set(PTR, LEN_NAME, VAL)		\
	(((PTR)->LEN_NAME) = (VAL))


extern int pci_map_sg(struct pci_dev *, struct scatterlist *, int, int);


extern void pci_unmap_sg(struct pci_dev *, struct scatterlist *, int, int);


static inline void
pci_dma_sync_single_for_cpu(struct pci_dev *dev, dma_addr_t dma_addr,
			    long size, int direction)
{
	/* Nothing to do.  */
}

static inline void
pci_dma_sync_single_for_device(struct pci_dev *dev, dma_addr_t dma_addr,
			       size_t size, int direction)
{
	/* Nothing to do.  */
}


static inline void
pci_dma_sync_sg_for_cpu(struct pci_dev *dev, struct scatterlist *sg,
			int nents, int direction)
{
	/* Nothing to do.  */
}

static inline void
pci_dma_sync_sg_for_device(struct pci_dev *dev, struct scatterlist *sg,
			   int nents, int direction)
{
	/* Nothing to do.  */
}


extern int pci_dma_supported(struct pci_dev *hwdev, u64 mask);

#ifdef CONFIG_PCI
static inline void pci_dma_burst_advice(struct pci_dev *pdev,
					enum pci_dma_burst_strategy *strat,
					unsigned long *strategy_parameter)
{
	unsigned long cacheline_size;
	u8 byte;

	pci_read_config_byte(pdev, PCI_CACHE_LINE_SIZE, &byte);
	if (byte == 0)
		cacheline_size = 1024;
	else
		cacheline_size = (int) byte * 4;

	*strat = PCI_DMA_BURST_BOUNDARY;
	*strategy_parameter = cacheline_size;
}
#endif

/* TODO: integrate with include/asm-generic/pci.h ? */
static inline int pci_get_legacy_ide_irq(struct pci_dev *dev, int channel)
{
	return channel ? 15 : 14;
}

extern void pcibios_resource_to_bus(struct pci_dev *, struct pci_bus_region *,
				    struct resource *);

extern void pcibios_bus_to_resource(struct pci_dev *dev, struct resource *res,
				    struct pci_bus_region *region);

static inline struct resource *
pcibios_select_root(struct pci_dev *pdev, struct resource *res)
{
	struct resource *root = NULL;

	if (res->flags & IORESOURCE_IO)
		root = &ioport_resource;
	if (res->flags & IORESOURCE_MEM)
		root = &iomem_resource;

	return root;
}

#define pci_domain_nr(bus) ((struct pci_controller *)(bus)->sysdata)->index

static inline int pci_proc_domain(struct pci_bus *bus)
{
	struct pci_controller *hose = bus->sysdata;
	return hose->need_domain_info;
}

struct pci_dev *alpha_gendev_to_pci(struct device *dev);

#endif /* __KERNEL__ */

/* Values for the `which' argument to sys_pciconfig_iobase.  */
#define IOBASE_HOSE		0
#define IOBASE_SPARSE_MEM	1
#define IOBASE_DENSE_MEM	2
#define IOBASE_SPARSE_IO	3
#define IOBASE_DENSE_IO		4
#define IOBASE_ROOT_BUS		5
#define IOBASE_FROM_HOSE	0x10000

extern struct pci_dev *isa_bridge;

#endif /* __ALPHA_PCI_H */
