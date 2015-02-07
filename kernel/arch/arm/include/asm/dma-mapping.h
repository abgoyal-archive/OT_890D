
#ifndef ASMARM_DMA_MAPPING_H
#define ASMARM_DMA_MAPPING_H

#ifdef __KERNEL__

#include <linux/mm_types.h>
#include <linux/scatterlist.h>

#include <asm-generic/dma-coherent.h>
#include <asm/memory.h>

#ifndef __arch_page_to_dma
static inline dma_addr_t page_to_dma(struct device *dev, struct page *page)
{
	return (dma_addr_t)__virt_to_bus((unsigned long)page_address(page));
}

static inline void *dma_to_virt(struct device *dev, dma_addr_t addr)
{
	return (void *)__bus_to_virt(addr);
}

static inline dma_addr_t virt_to_dma(struct device *dev, void *addr)
{
	return (dma_addr_t)__virt_to_bus((unsigned long)(addr));
}
#else
static inline dma_addr_t page_to_dma(struct device *dev, struct page *page)
{
	return __arch_page_to_dma(dev, page);
}

static inline void *dma_to_virt(struct device *dev, dma_addr_t addr)
{
	return __arch_dma_to_virt(dev, addr);
}

static inline dma_addr_t virt_to_dma(struct device *dev, void *addr)
{
	return __arch_virt_to_dma(dev, addr);
}
#endif

extern void dma_cache_maint(const void *kaddr, size_t size, int rw);

static inline int dma_supported(struct device *dev, u64 mask)
{
	if (mask < ISA_DMA_THRESHOLD)
		return 0;
	return 1;
}

static inline int dma_set_mask(struct device *dev, u64 dma_mask)
{
	if (!dev->dma_mask || !dma_supported(dev, dma_mask))
		return -EIO;

	*dev->dma_mask = dma_mask;

	return 0;
}

static inline int dma_get_cache_alignment(void)
{
	return 32;
}

static inline int dma_is_consistent(struct device *dev, dma_addr_t handle)
{
	return !!arch_is_coherent();
}

static inline int dma_mapping_error(struct device *dev, dma_addr_t dma_addr)
{
	return dma_addr == ~0;
}

static inline void *dma_alloc_noncoherent(struct device *dev, size_t size,
		dma_addr_t *handle, gfp_t gfp)
{
	return NULL;
}

static inline void dma_free_noncoherent(struct device *dev, size_t size,
		void *cpu_addr, dma_addr_t handle)
{
}

extern void *dma_alloc_coherent(struct device *, size_t, dma_addr_t *, gfp_t);

extern void dma_free_coherent(struct device *, size_t, void *, dma_addr_t);

int dma_mmap_coherent(struct device *, struct vm_area_struct *,
		void *, dma_addr_t, size_t);


extern void *dma_alloc_writecombine(struct device *, size_t, dma_addr_t *,
		gfp_t);

#define dma_free_writecombine(dev,size,cpu_addr,handle) \
	dma_free_coherent(dev,size,cpu_addr,handle)

int dma_mmap_writecombine(struct device *, struct vm_area_struct *,
		void *, dma_addr_t, size_t);


#ifdef CONFIG_DMABOUNCE

extern int dmabounce_register_dev(struct device *, unsigned long,
		unsigned long);

extern void dmabounce_unregister_dev(struct device *);

extern int dma_needs_bounce(struct device*, dma_addr_t, size_t);

extern dma_addr_t dma_map_single(struct device *, void *, size_t,
		enum dma_data_direction);
extern dma_addr_t dma_map_page(struct device *, struct page *,
		unsigned long, size_t, enum dma_data_direction);
extern void dma_unmap_single(struct device *, dma_addr_t, size_t,
		enum dma_data_direction);

int dmabounce_sync_for_cpu(struct device *, dma_addr_t, unsigned long,
		size_t, enum dma_data_direction);
int dmabounce_sync_for_device(struct device *, dma_addr_t, unsigned long,
		size_t, enum dma_data_direction);
#else
static inline int dmabounce_sync_for_cpu(struct device *d, dma_addr_t addr,
	unsigned long offset, size_t size, enum dma_data_direction dir)
{
	return 1;
}

static inline int dmabounce_sync_for_device(struct device *d, dma_addr_t addr,
	unsigned long offset, size_t size, enum dma_data_direction dir)
{
	return 1;
}


static inline dma_addr_t dma_map_single(struct device *dev, void *cpu_addr,
		size_t size, enum dma_data_direction dir)
{
	BUG_ON(!valid_dma_direction(dir));

	if (!arch_is_coherent())
		dma_cache_maint(cpu_addr, size, dir);

	return virt_to_dma(dev, cpu_addr);
}

static inline dma_addr_t dma_map_page(struct device *dev, struct page *page,
	     unsigned long offset, size_t size, enum dma_data_direction dir)
{
	BUG_ON(!valid_dma_direction(dir));

	if (!arch_is_coherent())
		dma_cache_maint(page_address(page) + offset, size, dir);

	return page_to_dma(dev, page) + offset;
}

static inline void dma_unmap_single(struct device *dev, dma_addr_t handle,
		size_t size, enum dma_data_direction dir)
{
	/* nothing to do */
}
#endif /* CONFIG_DMABOUNCE */

static inline void dma_unmap_page(struct device *dev, dma_addr_t handle,
		size_t size, enum dma_data_direction dir)
{
	dma_unmap_single(dev, handle, size, dir);
}

static inline void dma_sync_single_range_for_cpu(struct device *dev,
		dma_addr_t handle, unsigned long offset, size_t size,
		enum dma_data_direction dir)
{
	BUG_ON(!valid_dma_direction(dir));

	dmabounce_sync_for_cpu(dev, handle, offset, size, dir);
}

static inline void dma_sync_single_range_for_device(struct device *dev,
		dma_addr_t handle, unsigned long offset, size_t size,
		enum dma_data_direction dir)
{
	BUG_ON(!valid_dma_direction(dir));

	if (!dmabounce_sync_for_device(dev, handle, offset, size, dir))
		return;

	if (!arch_is_coherent())
		dma_cache_maint(dma_to_virt(dev, handle) + offset, size, dir);
}

static inline void dma_sync_single_for_cpu(struct device *dev,
		dma_addr_t handle, size_t size, enum dma_data_direction dir)
{
	dma_sync_single_range_for_cpu(dev, handle, 0, size, dir);
}

static inline void dma_sync_single_for_device(struct device *dev,
		dma_addr_t handle, size_t size, enum dma_data_direction dir)
{
	dma_sync_single_range_for_device(dev, handle, 0, size, dir);
}

extern int dma_map_sg(struct device *, struct scatterlist *, int,
		enum dma_data_direction);
extern void dma_unmap_sg(struct device *, struct scatterlist *, int,
		enum dma_data_direction);
extern void dma_sync_sg_for_cpu(struct device *, struct scatterlist *, int,
		enum dma_data_direction);
extern void dma_sync_sg_for_device(struct device *, struct scatterlist *, int,
		enum dma_data_direction);


#endif /* __KERNEL__ */
#endif
