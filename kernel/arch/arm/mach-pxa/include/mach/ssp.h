

#ifndef __ASM_ARCH_SSP_H
#define __ASM_ARCH_SSP_H

#include <linux/list.h>
#include <linux/io.h>

enum pxa_ssp_type {
	SSP_UNDEFINED = 0,
	PXA25x_SSP,  /* pxa 210, 250, 255, 26x */
	PXA25x_NSSP, /* pxa 255, 26x (including ASSP) */
	PXA27x_SSP,
};

struct ssp_device {
	struct platform_device *pdev;
	struct list_head	node;

	struct clk	*clk;
	void __iomem	*mmio_base;
	unsigned long	phys_base;

	const char	*label;
	int		port_id;
	int		type;
	int		use_count;
	int		irq;
	int		drcmr_rx;
	int		drcmr_tx;
};

#define SSP_NO_IRQ	0x1		/* don't register an irq handler in SSP driver */

struct ssp_state {
	u32	cr0;
	u32 cr1;
	u32 to;
	u32 psp;
};

struct ssp_dev {
	struct ssp_device *ssp;
	u32 port;
	u32 mode;
	u32 flags;
	u32 psp_flags;
	u32 speed;
	int irq;
};

int ssp_write_word(struct ssp_dev *dev, u32 data);
int ssp_read_word(struct ssp_dev *dev, u32 *data);
int ssp_flush(struct ssp_dev *dev);
void ssp_enable(struct ssp_dev *dev);
void ssp_disable(struct ssp_dev *dev);
void ssp_save_state(struct ssp_dev *dev, struct ssp_state *ssp);
void ssp_restore_state(struct ssp_dev *dev, struct ssp_state *ssp);
int ssp_init(struct ssp_dev *dev, u32 port, u32 init_flags);
int ssp_config(struct ssp_dev *dev, u32 mode, u32 flags, u32 psp_flags, u32 speed);
void ssp_exit(struct ssp_dev *dev);

static inline void ssp_write_reg(struct ssp_device *dev, u32 reg, u32 val)
{
	__raw_writel(val, dev->mmio_base + reg);
}

static inline u32 ssp_read_reg(struct ssp_device *dev, u32 reg)
{
	return __raw_readl(dev->mmio_base + reg);
}

struct ssp_device *ssp_request(int port, const char *label);
void ssp_free(struct ssp_device *);
#endif /* __ASM_ARCH_SSP_H */
