

#include <mach/dma.h>

#ifndef __ASM_ARCH_IMX_DMA_H
#define __ASM_ARCH_IMX_DMA_H

#define IMX_DMA_CHANNELS  11


struct imx_dma_channel {
	const char *name;
	void (*irq_handler) (int, void *);
	void (*err_handler) (int, void *, int errcode);
	void *data;
	unsigned int  dma_mode;
	struct scatterlist *sg;
	unsigned int sgbc;
	unsigned int sgcount;
	unsigned int resbytes;
	int dma_num;
};

extern struct imx_dma_channel imx_dma_channels[IMX_DMA_CHANNELS];

#define IMX_DMA_ERR_BURST     1
#define IMX_DMA_ERR_REQUEST   2
#define IMX_DMA_ERR_TRANSFER  4
#define IMX_DMA_ERR_BUFFER    8

/* The type to distinguish channel numbers parameter from ordinal int type */
typedef int imx_dmach_t;

#define DMA_MODE_READ		0
#define DMA_MODE_WRITE		1
#define DMA_MODE_MASK		1

int
imx_dma_setup_single(imx_dmach_t dma_ch, dma_addr_t dma_address,
		unsigned int dma_length, unsigned int dev_addr, unsigned int dmamode);

int
imx_dma_setup_sg(imx_dmach_t dma_ch,
		 struct scatterlist *sg, unsigned int sgcount, unsigned int dma_length,
		 unsigned int dev_addr, unsigned int dmamode);

int
imx_dma_setup_handlers(imx_dmach_t dma_ch,
		void (*irq_handler) (int, void *),
		void (*err_handler) (int, void *, int), void *data);

void imx_dma_enable(imx_dmach_t dma_ch);

void imx_dma_disable(imx_dmach_t dma_ch);

int imx_dma_request(imx_dmach_t dma_ch, const char *name);

void imx_dma_free(imx_dmach_t dma_ch);

imx_dmach_t imx_dma_request_by_prio(const char *name, imx_dma_prio prio);


#endif	/* _ASM_ARCH_IMX_DMA_H */
