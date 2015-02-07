

#include <asm/iommu.h>


static void *dma_iommu_alloc_coherent(struct device *dev, size_t size,
				      dma_addr_t *dma_handle, gfp_t flag)
{
	return iommu_alloc_coherent(dev, dev->archdata.dma_data, size,
				    dma_handle, device_to_mask(dev), flag,
				    dev_to_node(dev));
}

static void dma_iommu_free_coherent(struct device *dev, size_t size,
				    void *vaddr, dma_addr_t dma_handle)
{
	iommu_free_coherent(dev->archdata.dma_data, size, vaddr, dma_handle);
}

static dma_addr_t dma_iommu_map_page(struct device *dev, struct page *page,
				     unsigned long offset, size_t size,
				     enum dma_data_direction direction,
				     struct dma_attrs *attrs)
{
	return iommu_map_page(dev, dev->archdata.dma_data, page, offset, size,
			      device_to_mask(dev), direction, attrs);
}


static void dma_iommu_unmap_page(struct device *dev, dma_addr_t dma_handle,
				 size_t size, enum dma_data_direction direction,
				 struct dma_attrs *attrs)
{
	iommu_unmap_page(dev->archdata.dma_data, dma_handle, size, direction,
			 attrs);
}


static int dma_iommu_map_sg(struct device *dev, struct scatterlist *sglist,
			    int nelems, enum dma_data_direction direction,
			    struct dma_attrs *attrs)
{
	return iommu_map_sg(dev, dev->archdata.dma_data, sglist, nelems,
			    device_to_mask(dev), direction, attrs);
}

static void dma_iommu_unmap_sg(struct device *dev, struct scatterlist *sglist,
		int nelems, enum dma_data_direction direction,
		struct dma_attrs *attrs)
{
	iommu_unmap_sg(dev->archdata.dma_data, sglist, nelems, direction,
		       attrs);
}

/* We support DMA to/from any memory page via the iommu */
static int dma_iommu_dma_supported(struct device *dev, u64 mask)
{
	struct iommu_table *tbl = dev->archdata.dma_data;

	if (!tbl || tbl->it_offset > mask) {
		printk(KERN_INFO
		       "Warning: IOMMU offset too big for device mask\n");
		if (tbl)
			printk(KERN_INFO
			       "mask: 0x%08llx, table offset: 0x%08lx\n",
				mask, tbl->it_offset);
		else
			printk(KERN_INFO "mask: 0x%08llx, table unavailable\n",
				mask);
		return 0;
	} else
		return 1;
}

struct dma_mapping_ops dma_iommu_ops = {
	.alloc_coherent	= dma_iommu_alloc_coherent,
	.free_coherent	= dma_iommu_free_coherent,
	.map_sg		= dma_iommu_map_sg,
	.unmap_sg	= dma_iommu_unmap_sg,
	.dma_supported	= dma_iommu_dma_supported,
	.map_page	= dma_iommu_map_page,
	.unmap_page	= dma_iommu_unmap_page,
};
EXPORT_SYMBOL(dma_iommu_ops);