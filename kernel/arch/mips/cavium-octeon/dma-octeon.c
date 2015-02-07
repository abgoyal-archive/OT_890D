
#include <linux/types.h>
#include <linux/mm.h>

#include <dma-coherence.h>

dma_addr_t octeon_map_dma_mem(struct device *dev, void *ptr, size_t size)
{
	/* Without PCI/PCIe this function can be called for Octeon internal
	   devices such as USB. These devices all support 64bit addressing */
	mb();
	return virt_to_phys(ptr);
}

void octeon_unmap_dma_mem(struct device *dev, dma_addr_t dma_addr)
{
	/* Without PCI/PCIe this function can be called for Octeon internal
	 * devices such as USB. These devices all support 64bit addressing */
	return;
}
