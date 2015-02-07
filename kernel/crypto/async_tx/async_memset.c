
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/async_tx.h>

struct dma_async_tx_descriptor *
async_memset(struct page *dest, int val, unsigned int offset,
	size_t len, enum async_tx_flags flags,
	struct dma_async_tx_descriptor *depend_tx,
	dma_async_tx_callback cb_fn, void *cb_param)
{
	struct dma_chan *chan = async_tx_find_channel(depend_tx, DMA_MEMSET,
						      &dest, 1, NULL, 0, len);
	struct dma_device *device = chan ? chan->device : NULL;
	struct dma_async_tx_descriptor *tx = NULL;

	if (device) {
		dma_addr_t dma_dest;
		unsigned long dma_prep_flags = cb_fn ? DMA_PREP_INTERRUPT : 0;

		dma_dest = dma_map_page(device->dev, dest, offset, len,
					DMA_FROM_DEVICE);

		tx = device->device_prep_dma_memset(chan, dma_dest, val, len,
						    dma_prep_flags);
	}

	if (tx) {
		pr_debug("%s: (async) len: %zu\n", __func__, len);
		async_tx_submit(chan, tx, flags, depend_tx, cb_fn, cb_param);
	} else { /* run the memset synchronously */
		void *dest_buf;
		pr_debug("%s: (sync) len: %zu\n", __func__, len);

		dest_buf = (void *) (((char *) page_address(dest)) + offset);

		/* wait for any prerequisite operations */
		async_tx_quiesce(&depend_tx);

		memset(dest_buf, val, len);

		async_tx_sync_epilog(cb_fn, cb_param);
	}

	return tx;
}
EXPORT_SYMBOL_GPL(async_memset);

static int __init async_memset_init(void)
{
	return 0;
}

static void __exit async_memset_exit(void)
{
	do { } while (0);
}

module_init(async_memset_init);
module_exit(async_memset_exit);

MODULE_AUTHOR("Intel Corporation");
MODULE_DESCRIPTION("asynchronous memset api");
MODULE_LICENSE("GPL");
