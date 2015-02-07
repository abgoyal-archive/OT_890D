
#ifndef _M68K_DMA_H
#define _M68K_DMA_H 1


#define MAX_DMA_ADDRESS PAGE_OFFSET

#define MAX_DMA_CHANNELS 8

extern int request_dma(unsigned int dmanr, const char * device_id);	/* reserve a DMA channel */
extern void free_dma(unsigned int dmanr);	/* release it again */

#define isa_dma_bridge_buggy    (0)

#endif /* _M68K_DMA_H */
