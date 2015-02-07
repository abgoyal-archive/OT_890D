
#include <linux/kernel.h>
#include <linux/highmem.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/async_tx.h>

struct dma_async_tx_descriptor *
async_memcpy(struct page *dest, struct page *src, unsigned int dest_offset,
	unsigned int src_offset, size_t len, enum async_tx_flags flags,
	struct dma_async_tx_descriptor *depend_tx,
	dma_async_tx_callback cb_fn, void *cb_param)
{
	struct dma_chan *chan = async_tx_find_channel(depend_tx, DMA_MEMCPY,
						      &dest, 1, &src, 1, len);
	struct dma_device *device = chan ? chan->device : NULL;
	struct dma_async_tx_descriptor *tx = NULL;

	if (device) {
		dma_addr_t dma_dest, dma_src;
		unsigned long dma_prep_flags = cb_fn ? DMA_PREP_INTERRUPT : 0;

		dma_dest = dma_map_page(device->dev, dest, dest_offset, len,
					DMA_FROM_DEVICE);

		dma_src = dma_map_page(device->dev, src, src_offset, len,
				       DMA_TO_DEVICE);

		tx = device->device_prep_dma_memcpy(chan, dma_dest, dma_src,
						    len, dma_prep_flags);
	}

	if (tx) {
		pr_debug("%s: (async) len: %zu\n", __func__, len);
		async_tx_submit(chan, tx, flags, depend_tx, cb_fn, cb_param);
	} else {
		void *dest_buf, *src_buf;
		pr_debug("%s: (sync) len: %zu\n", __func__, len);

		/* wait for any prerequisite operations */
		async_tx_quiesce(&depend_tx);

		dest_buf = kmap_atomic(dest, KM_USER0) + dest_offset;
		src_buf = kmap_atomic(src, KM_USER1) + src_offset;

		memcpy(dest_buf, src_buf, len);

		kunmap_atomic(dest_buf, KM_USER0);
		kunmap_atomic(src_buf, KM_USER1);

		async_tx_sync_epilog(cb_fn, cb_param);
	}

	return tx;
}
EXPORT_SYMBOL_GPL(async_memcpy);

static int __init async_memcpy_init(void)
{
	return 0;
}

static void __exit async_memcpy_exit(void)
{
	do { } while (0);
}

module_init(async_memcpy_init);
module_exit(async_memcpy_exit);

MODULE_AUTHOR("Intel Corporation");
MODULE_DESCRIPTION("asynchronous memcpy api");
MODULE_LICENSE("GPL");
