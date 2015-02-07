
#ifndef _ASM_POWERPC_SCATTERLIST_H
#define _ASM_POWERPC_SCATTERLIST_H

#ifdef __KERNEL__
#include <linux/types.h>
#include <asm/dma.h>

struct scatterlist {
#ifdef CONFIG_DEBUG_SG
	unsigned long sg_magic;
#endif
	unsigned long page_link;
	unsigned int offset;
	unsigned int length;

	/* For TCE support */
	dma_addr_t dma_address;
	u32 dma_length;
};

#define sg_dma_address(sg)	((sg)->dma_address)
#ifdef __powerpc64__
#define sg_dma_len(sg)		((sg)->dma_length)
#else
#define sg_dma_len(sg)		((sg)->length)
#endif

#ifdef __powerpc64__
#define ISA_DMA_THRESHOLD	(~0UL)
#endif

#define ARCH_HAS_SG_CHAIN

#endif /* __KERNEL__ */
#endif /* _ASM_POWERPC_SCATTERLIST_H */
