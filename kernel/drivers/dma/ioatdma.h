
#ifndef IOATDMA_H
#define IOATDMA_H

#include <linux/dmaengine.h>
#include "ioatdma_hw.h"
#include <linux/init.h>
#include <linux/dmapool.h>
#include <linux/cache.h>
#include <linux/pci_ids.h>
#include <net/tcp.h>

#define IOAT_DMA_VERSION  "3.64"

enum ioat_interrupt {
	none = 0,
	msix_multi_vector = 1,
	msix_single_vector = 2,
	msi = 3,
	intx = 4,
};

#define IOAT_LOW_COMPLETION_MASK	0xffffffc0
#define IOAT_DMA_DCA_ANY_CPU		~0
#define IOAT_WATCHDOG_PERIOD		(2 * HZ)



struct ioatdma_device {
	struct pci_dev *pdev;
	void __iomem *reg_base;
	struct pci_pool *dma_pool;
	struct pci_pool *completion_pool;
	struct dma_device common;
	u8 version;
	enum ioat_interrupt irq_mode;
	struct delayed_work work;
	struct msix_entry msix_entries[4];
	struct ioat_dma_chan *idx[4];
};

struct ioat_dma_chan {

	void __iomem *reg_base;

	dma_cookie_t completed_cookie;
	unsigned long last_completion;
	unsigned long last_completion_time;

	size_t xfercap;	/* XFERCAP register value expanded out */

	spinlock_t cleanup_lock;
	spinlock_t desc_lock;
	struct list_head free_desc;
	struct list_head used_desc;
	unsigned long watchdog_completion;
	int watchdog_tcp_cookie;
	u32 watchdog_last_tcp_cookie;
	struct delayed_work work;

	int pending;
	int dmacount;
	int desccount;

	struct ioatdma_device *device;
	struct dma_chan common;

	dma_addr_t completion_addr;
	union {
		u64 full; /* HW completion writeback */
		struct {
			u32 low;
			u32 high;
		};
	} *completion_virt;
	unsigned long last_compl_desc_addr_hw;
	struct tasklet_struct cleanup_task;
};

/* wrapper around hardware descriptor format + additional software fields */

struct ioat_desc_sw {
	struct ioat_dma_descriptor *hw;
	struct list_head node;
	int tx_cnt;
	size_t len;
	dma_addr_t src;
	dma_addr_t dst;
	struct dma_async_tx_descriptor async_tx;
};

static inline void ioat_set_tcp_copy_break(struct ioatdma_device *dev)
{
	#ifdef CONFIG_NET_DMA
	switch (dev->version) {
	case IOAT_VER_1_2:
		sysctl_tcp_dma_copybreak = 4096;
		break;
	case IOAT_VER_2_0:
		sysctl_tcp_dma_copybreak = 2048;
		break;
	case IOAT_VER_3_0:
		sysctl_tcp_dma_copybreak = 262144;
		break;
	}
	#endif
}

#if defined(CONFIG_INTEL_IOATDMA) || defined(CONFIG_INTEL_IOATDMA_MODULE)
struct ioatdma_device *ioat_dma_probe(struct pci_dev *pdev,
				      void __iomem *iobase);
void ioat_dma_remove(struct ioatdma_device *device);
struct dca_provider *ioat_dca_init(struct pci_dev *pdev, void __iomem *iobase);
struct dca_provider *ioat2_dca_init(struct pci_dev *pdev, void __iomem *iobase);
struct dca_provider *ioat3_dca_init(struct pci_dev *pdev, void __iomem *iobase);
#else
#define ioat_dma_probe(pdev, iobase)    NULL
#define ioat_dma_remove(device)         do { } while (0)
#define ioat_dca_init(pdev, iobase)	NULL
#define ioat2_dca_init(pdev, iobase)	NULL
#define ioat3_dca_init(pdev, iobase)	NULL
#endif

#endif /* IOATDMA_H */
