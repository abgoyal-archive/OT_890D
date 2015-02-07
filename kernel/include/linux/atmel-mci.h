
#ifndef __LINUX_ATMEL_MCI_H
#define __LINUX_ATMEL_MCI_H

#define ATMEL_MCI_MAX_NR_SLOTS	2

#include <linux/dw_dmac.h>

struct mci_slot_pdata {
	unsigned int		bus_width;
	int			detect_pin;
	int			wp_pin;
};

struct mci_platform_data {
	struct dw_dma_slave	dma_slave;
	struct mci_slot_pdata	slot[ATMEL_MCI_MAX_NR_SLOTS];
};

#endif /* __LINUX_ATMEL_MCI_H */
