
#ifndef _ASM_IA64_SCATTERLIST_H
#define _ASM_IA64_SCATTERLIST_H


#include <asm/types.h>

struct scatterlist {
#ifdef CONFIG_DEBUG_SG
	unsigned long sg_magic;
#endif
	unsigned long page_link;
	unsigned int offset;
	unsigned int length;	/* buffer length */

	dma_addr_t dma_address;
	unsigned int dma_length;
};

#define ISA_DMA_THRESHOLD	0xffffffff

#define sg_dma_len(sg)		((sg)->dma_length)
#define sg_dma_address(sg)	((sg)->dma_address)

#define	ARCH_HAS_SG_CHAIN

#endif /* _ASM_IA64_SCATTERLIST_H */
