
#include <linux/rculist.h>
#include <linux/kernel.h>
#include <linux/async_tx.h>

#ifdef CONFIG_DMA_ENGINE
static int __init async_tx_init(void)
{
	dmaengine_get();

	printk(KERN_INFO "async_tx: api initialized (async)\n");

	return 0;
}

static void __exit async_tx_exit(void)
{
	dmaengine_put();
}

struct dma_chan *
__async_tx_find_channel(struct dma_async_tx_descriptor *depend_tx,
	enum dma_transaction_type tx_type)
{
	/* see if we can keep the chain on one channel */
	if (depend_tx &&
	    dma_has_cap(tx_type, depend_tx->chan->device->cap_mask))
		return depend_tx->chan;
	return dma_find_channel(tx_type);
}
EXPORT_SYMBOL_GPL(__async_tx_find_channel);
#else
static int __init async_tx_init(void)
{
	printk(KERN_INFO "async_tx: api initialized (sync-only)\n");
	return 0;
}

static void __exit async_tx_exit(void)
{
	do { } while (0);
}
#endif


static void
async_tx_channel_switch(struct dma_async_tx_descriptor *depend_tx,
			struct dma_async_tx_descriptor *tx)
{
	struct dma_chan *chan;
	struct dma_device *device;
	struct dma_async_tx_descriptor *intr_tx = (void *) ~0;

	/* first check to see if we can still append to depend_tx */
	spin_lock_bh(&depend_tx->lock);
	if (depend_tx->parent && depend_tx->chan == tx->chan) {
		tx->parent = depend_tx;
		depend_tx->next = tx;
		intr_tx = NULL;
	}
	spin_unlock_bh(&depend_tx->lock);

	if (!intr_tx)
		return;

	chan = depend_tx->chan;
	device = chan->device;

	/* see if we can schedule an interrupt
	 * otherwise poll for completion
	 */
	if (dma_has_cap(DMA_INTERRUPT, device->cap_mask))
		intr_tx = device->device_prep_dma_interrupt(chan, 0);
	else
		intr_tx = NULL;

	if (intr_tx) {
		intr_tx->callback = NULL;
		intr_tx->callback_param = NULL;
		tx->parent = intr_tx;
		/* safe to set ->next outside the lock since we know we are
		 * not submitted yet
		 */
		intr_tx->next = tx;

		/* check if we need to append */
		spin_lock_bh(&depend_tx->lock);
		if (depend_tx->parent) {
			intr_tx->parent = depend_tx;
			depend_tx->next = intr_tx;
			async_tx_ack(intr_tx);
			intr_tx = NULL;
		}
		spin_unlock_bh(&depend_tx->lock);

		if (intr_tx) {
			intr_tx->parent = NULL;
			intr_tx->tx_submit(intr_tx);
			async_tx_ack(intr_tx);
		}
	} else {
		if (dma_wait_for_async_tx(depend_tx) == DMA_ERROR)
			panic("%s: DMA_ERROR waiting for depend_tx\n",
			      __func__);
		tx->tx_submit(tx);
	}
}


enum submit_disposition {
	ASYNC_TX_SUBMITTED,
	ASYNC_TX_CHANNEL_SWITCH,
	ASYNC_TX_DIRECT_SUBMIT,
};

void
async_tx_submit(struct dma_chan *chan, struct dma_async_tx_descriptor *tx,
	enum async_tx_flags flags, struct dma_async_tx_descriptor *depend_tx,
	dma_async_tx_callback cb_fn, void *cb_param)
{
	tx->callback = cb_fn;
	tx->callback_param = cb_param;

	if (depend_tx) {
		enum submit_disposition s;

		/* sanity check the dependency chain:
		 * 1/ if ack is already set then we cannot be sure
		 * we are referring to the correct operation
		 * 2/ dependencies are 1:1 i.e. two transactions can
		 * not depend on the same parent
		 */
		BUG_ON(async_tx_test_ack(depend_tx) || depend_tx->next ||
		       tx->parent);

		/* the lock prevents async_tx_run_dependencies from missing
		 * the setting of ->next when ->parent != NULL
		 */
		spin_lock_bh(&depend_tx->lock);
		if (depend_tx->parent) {
			/* we have a parent so we can not submit directly
			 * if we are staying on the same channel: append
			 * else: channel switch
			 */
			if (depend_tx->chan == chan) {
				tx->parent = depend_tx;
				depend_tx->next = tx;
				s = ASYNC_TX_SUBMITTED;
			} else
				s = ASYNC_TX_CHANNEL_SWITCH;
		} else {
			/* we do not have a parent so we may be able to submit
			 * directly if we are staying on the same channel
			 */
			if (depend_tx->chan == chan)
				s = ASYNC_TX_DIRECT_SUBMIT;
			else
				s = ASYNC_TX_CHANNEL_SWITCH;
		}
		spin_unlock_bh(&depend_tx->lock);

		switch (s) {
		case ASYNC_TX_SUBMITTED:
			break;
		case ASYNC_TX_CHANNEL_SWITCH:
			async_tx_channel_switch(depend_tx, tx);
			break;
		case ASYNC_TX_DIRECT_SUBMIT:
			tx->parent = NULL;
			tx->tx_submit(tx);
			break;
		}
	} else {
		tx->parent = NULL;
		tx->tx_submit(tx);
	}

	if (flags & ASYNC_TX_ACK)
		async_tx_ack(tx);

	if (depend_tx && (flags & ASYNC_TX_DEP_ACK))
		async_tx_ack(depend_tx);
}
EXPORT_SYMBOL_GPL(async_tx_submit);

struct dma_async_tx_descriptor *
async_trigger_callback(enum async_tx_flags flags,
	struct dma_async_tx_descriptor *depend_tx,
	dma_async_tx_callback cb_fn, void *cb_param)
{
	struct dma_chan *chan;
	struct dma_device *device;
	struct dma_async_tx_descriptor *tx;

	if (depend_tx) {
		chan = depend_tx->chan;
		device = chan->device;

		/* see if we can schedule an interrupt
		 * otherwise poll for completion
		 */
		if (device && !dma_has_cap(DMA_INTERRUPT, device->cap_mask))
			device = NULL;

		tx = device ? device->device_prep_dma_interrupt(chan, 0) : NULL;
	} else
		tx = NULL;

	if (tx) {
		pr_debug("%s: (async)\n", __func__);

		async_tx_submit(chan, tx, flags, depend_tx, cb_fn, cb_param);
	} else {
		pr_debug("%s: (sync)\n", __func__);

		/* wait for any prerequisite operations */
		async_tx_quiesce(&depend_tx);

		async_tx_sync_epilog(cb_fn, cb_param);
	}

	return tx;
}
EXPORT_SYMBOL_GPL(async_trigger_callback);

void async_tx_quiesce(struct dma_async_tx_descriptor **tx)
{
	if (*tx) {
		/* if ack is already set then we cannot be sure
		 * we are referring to the correct operation
		 */
		BUG_ON(async_tx_test_ack(*tx));
		if (dma_wait_for_async_tx(*tx) == DMA_ERROR)
			panic("DMA_ERROR waiting for transaction\n");
		async_tx_ack(*tx);
		*tx = NULL;
	}
}
EXPORT_SYMBOL_GPL(async_tx_quiesce);

module_init(async_tx_init);
module_exit(async_tx_exit);

MODULE_AUTHOR("Intel Corporation");
MODULE_DESCRIPTION("Asynchronous Bulk Memory Transactions API");
MODULE_LICENSE("GPL");
