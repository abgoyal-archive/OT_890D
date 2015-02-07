

#include <linux/init.h>

#include <asm/dma.h>
#include <asm/mach/dma.h>

void __init arch_dma_init(dma_t *dma)
{
#ifdef CONFIG_ISA_DMA
	isa_init_dma(dma);
#endif
}
