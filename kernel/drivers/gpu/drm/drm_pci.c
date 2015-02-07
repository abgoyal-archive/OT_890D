
/* drm_pci.h -- PCI DMA memory management wrappers for DRM -*- linux-c -*- */


#include <linux/pci.h>
#include <linux/dma-mapping.h>
#include "drmP.h"

/**********************************************************************/
/** \name PCI memory */
/*@{*/

drm_dma_handle_t *drm_pci_alloc(struct drm_device * dev, size_t size, size_t align,
				dma_addr_t maxaddr)
{
	drm_dma_handle_t *dmah;
#if 1
	unsigned long addr;
	size_t sz;
#endif
#ifdef DRM_DEBUG_MEMORY
	int area = DRM_MEM_DMA;

	spin_lock(&drm_mem_lock);
	if ((drm_ram_used >> PAGE_SHIFT)
	    > (DRM_RAM_PERCENT * drm_ram_available) / 100) {
		spin_unlock(&drm_mem_lock);
		return 0;
	}
	spin_unlock(&drm_mem_lock);
#endif

	/* pci_alloc_consistent only guarantees alignment to the smallest
	 * PAGE_SIZE order which is greater than or equal to the requested size.
	 * Return NULL here for now to make sure nobody tries for larger alignment
	 */
	if (align > size)
		return NULL;

	if (pci_set_dma_mask(dev->pdev, maxaddr) != 0) {
		DRM_ERROR("Setting pci dma mask failed\n");
		return NULL;
	}

	dmah = kmalloc(sizeof(drm_dma_handle_t), GFP_KERNEL);
	if (!dmah)
		return NULL;

	dmah->size = size;
	dmah->vaddr = dma_alloc_coherent(&dev->pdev->dev, size, &dmah->busaddr, GFP_KERNEL | __GFP_COMP);

#ifdef DRM_DEBUG_MEMORY
	if (dmah->vaddr == NULL) {
		spin_lock(&drm_mem_lock);
		++drm_mem_stats[area].fail_count;
		spin_unlock(&drm_mem_lock);
		kfree(dmah);
		return NULL;
	}

	spin_lock(&drm_mem_lock);
	++drm_mem_stats[area].succeed_count;
	drm_mem_stats[area].bytes_allocated += size;
	drm_ram_used += size;
	spin_unlock(&drm_mem_lock);
#else
	if (dmah->vaddr == NULL) {
		kfree(dmah);
		return NULL;
	}
#endif

	memset(dmah->vaddr, 0, size);

	/* XXX - Is virt_to_page() legal for consistent mem? */
	/* Reserve */
	for (addr = (unsigned long)dmah->vaddr, sz = size;
	     sz > 0; addr += PAGE_SIZE, sz -= PAGE_SIZE) {
		SetPageReserved(virt_to_page(addr));
	}

	return dmah;
}

EXPORT_SYMBOL(drm_pci_alloc);

void __drm_pci_free(struct drm_device * dev, drm_dma_handle_t * dmah)
{
#if 1
	unsigned long addr;
	size_t sz;
#endif
#ifdef DRM_DEBUG_MEMORY
	int area = DRM_MEM_DMA;
	int alloc_count;
	int free_count;
#endif

	if (!dmah->vaddr) {
#ifdef DRM_DEBUG_MEMORY
		DRM_MEM_ERROR(area, "Attempt to free address 0\n");
#endif
	} else {
		/* XXX - Is virt_to_page() legal for consistent mem? */
		/* Unreserve */
		for (addr = (unsigned long)dmah->vaddr, sz = dmah->size;
		     sz > 0; addr += PAGE_SIZE, sz -= PAGE_SIZE) {
			ClearPageReserved(virt_to_page(addr));
		}
		dma_free_coherent(&dev->pdev->dev, dmah->size, dmah->vaddr,
				  dmah->busaddr);
	}

#ifdef DRM_DEBUG_MEMORY
	spin_lock(&drm_mem_lock);
	free_count = ++drm_mem_stats[area].free_count;
	alloc_count = drm_mem_stats[area].succeed_count;
	drm_mem_stats[area].bytes_freed += size;
	drm_ram_used -= size;
	spin_unlock(&drm_mem_lock);
	if (free_count > alloc_count) {
		DRM_MEM_ERROR(area,
			      "Excess frees: %d frees, %d allocs\n",
			      free_count, alloc_count);
	}
#endif

}

void drm_pci_free(struct drm_device * dev, drm_dma_handle_t * dmah)
{
	__drm_pci_free(dev, dmah);
	kfree(dmah);
}

EXPORT_SYMBOL(drm_pci_free);

/*@}*/
