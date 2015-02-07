

#ifndef __OMAP_PCM_H__
#define __OMAP_PCM_H__

struct omap_pcm_dma_data {
	char		*name;		/* stream identifier */
	int		dma_req;	/* DMA request line */
	unsigned long	port_addr;	/* transmit/receive register */
};

extern struct snd_soc_platform omap_soc_platform;

#endif
