

#include "global.h"

/* Get frame buffer size from VGA BIOS */

unsigned int viafb_get_memsize(void)
{
	unsigned int m;

	/* If memory size provided by user */
	if (viafb_memsize)
		m = viafb_memsize * Mb;
	else {
		m = (unsigned int)viafb_read_reg(VIASR, SR39);
		m = m * (4 * Mb);

		if ((m < (16 * Mb)) || (m > (64 * Mb)))
			m = 16 * Mb;
	}
	DEBUG_MSG(KERN_INFO "framebuffer size = %d Mb\n", m / Mb);
	return m;
}

/* Get Video Buffer Starting Physical Address(back door)*/

unsigned long viafb_get_videobuf_addr(void)
{
	struct pci_dev *pdev = NULL;
	unsigned char sys_mem;
	unsigned char video_mem;
	unsigned long sys_mem_size;
	unsigned long video_mem_size;
	/*system memory = 256 MB, video memory 64 MB */
	unsigned long vmem_starting_adr = 0x0C000000;

	pdev =
	    (struct pci_dev *)pci_get_device(VIA_K800_BRIDGE_VID,
					     VIA_K800_BRIDGE_DID, NULL);
	if (pdev != NULL) {
		pci_read_config_byte(pdev, VIA_K800_SYSTEM_MEMORY_REG,
				     &sys_mem);
		pci_read_config_byte(pdev, VIA_K800_VIDEO_MEMORY_REG,
				     &video_mem);
		video_mem = (video_mem & 0x70) >> 4;
		sys_mem_size = ((unsigned long)sys_mem) << 24;
		if (video_mem != 0)
			video_mem_size = (1 << (video_mem)) * 1024 * 1024;
		else
			video_mem_size = 0;

		vmem_starting_adr = sys_mem_size - video_mem_size;
		pci_dev_put(pdev);
	}

	DEBUG_MSG(KERN_INFO "Video Memory Starting Address = %lx \n",
		  vmem_starting_adr);
	return vmem_starting_adr;
}
