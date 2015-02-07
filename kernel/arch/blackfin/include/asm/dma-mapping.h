
#ifndef _BLACKFIN_DMA_MAPPING_H
#define _BLACKFIN_DMA_MAPPING_H

#include <asm/scatterlist.h>

void dma_alloc_init(unsigned long start, unsigned long end);
void *dma_alloc_coherent(struct device *dev, size_t size,
			 dma_addr_t *dma_handle, gfp_t gfp);
void dma_free_coherent(struct device *dev, size_t size, void *vaddr,
		       dma_addr_t dma_handle);

#define dma_alloc_noncoherent(d, s, h, f) dma_alloc_coherent(d, s, h, f)
#define dma_free_noncoherent(d, s, v, h) dma_free_coherent(d, s, v, h)

static inline
int dma_mapping_error(struct device *dev, dma_addr_t dma_addr)
{
	return 0;
}

extern dma_addr_t dma_map_single(struct device *dev, void *ptr, size_t size,
				 enum dma_data_direction direction);

static inline dma_addr_t
dma_map_page(struct device *dev, struct page *page,
	     unsigned long offset, size_t size,
	     enum dma_data_direction dir)
{
	return dma_map_single(dev, page_address(page) + offset, size, dir);
}

extern void dma_unmap_single(struct device *dev, dma_addr_t dma_addr, size_t size,
			  enum dma_data_direction direction);

static inline void
dma_unmap_page(struct device *dev, dma_addr_t dma_addr, size_t size,
	       enum dma_data_direction dir)
{
	dma_unmap_single(dev, dma_addr, size, dir);
}

extern int dma_map_sg(struct device *dev, struct scatterlist *sg, int nents,
		      enum dma_data_direction direction);

extern void dma_unmap_sg(struct device *dev, struct scatterlist *sg,
		      int nhwentries, enum dma_data_direction direction);

static inline void dma_sync_single_for_cpu(struct device *dev,
					dma_addr_t handle, size_t size,
					enum dma_data_direction dir)
{
}

static inline void dma_sync_single_for_device(struct device *dev,
					dma_addr_t handle, size_t size,
					enum dma_data_direction dir)
{
}
#endif				/* _BLACKFIN_DMA_MAPPING_H */
