

#ifndef __ASM_ARCH_DMA_H
#define __ASM_ARCH_DMA_H

typedef enum {
	DMA_PRIO_HIGH = 0,
	DMA_PRIO_MEDIUM = 1,
	DMA_PRIO_LOW = 2
} imx_dma_prio;

#define DMA_REQ_UART3_T        2
#define DMA_REQ_UART3_R        3
#define DMA_REQ_SSI2_T         4
#define DMA_REQ_SSI2_R         5
#define DMA_REQ_CSI_STAT       6
#define DMA_REQ_CSI_R          7
#define DMA_REQ_MSHC           8
#define DMA_REQ_DSPA_DCT_DOUT  9
#define DMA_REQ_DSPA_DCT_DIN  10
#define DMA_REQ_DSPA_MAC      11
#define DMA_REQ_EXT           12
#define DMA_REQ_SDHC          13
#define DMA_REQ_SPI1_R        14
#define DMA_REQ_SPI1_T        15
#define DMA_REQ_SSI_T         16
#define DMA_REQ_SSI_R         17
#define DMA_REQ_ASP_DAC       18
#define DMA_REQ_ASP_ADC       19
#define DMA_REQ_USP_EP(x)    (20+(x))
#define DMA_REQ_SPI2_R        26
#define DMA_REQ_SPI2_T        27
#define DMA_REQ_UART2_T       28
#define DMA_REQ_UART2_R       29
#define DMA_REQ_UART1_T       30
#define DMA_REQ_UART1_R       31

#endif				/* _ASM_ARCH_DMA_H */
