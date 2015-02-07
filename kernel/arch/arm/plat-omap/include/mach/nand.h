

#include <linux/mtd/partitions.h>

struct omap_nand_platform_data {
	unsigned int		options;
	int			cs;
	int			gpio_irq;
	struct mtd_partition	*parts;
	int			nr_parts;
	int			(*nand_setup)(void __iomem *);
	int			(*dev_ready)(struct omap_nand_platform_data *);
	int			dma_channel;
	void __iomem		*gpmc_cs_baseaddr;
	void __iomem		*gpmc_baseaddr;
};
