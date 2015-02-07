
#ifndef __ASM_AVR32_SCATTERLIST_H
#define __ASM_AVR32_SCATTERLIST_H

#include <asm/types.h>

struct scatterlist {
#ifdef CONFIG_DEBUG_SG
   unsigned long	sg_magic;
#endif
    unsigned long	page_link;
    unsigned int	offset;
    dma_addr_t		dma_address;
    unsigned int	length;
};

#define sg_dma_address(sg)	((sg)->dma_address)
#define sg_dma_len(sg)		((sg)->length)

#define ISA_DMA_THRESHOLD (0xffffffff)

#endif /* __ASM_AVR32_SCATTERLIST_H */
