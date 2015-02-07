
#ifndef _ASYNC_TX_H_
#define _ASYNC_TX_H_
#include <linux/dmaengine.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>

struct dma_chan_ref {
	struct dma_chan *chan;
	struct list_head node;
	struct rcu_head rcu;
	atomic_t count;
};

enum async_tx_flags {
	ASYNC_TX_XOR_ZERO_DST	 = (1 << 0),
	ASYNC_TX_XOR_DROP_DST	 = (1 << 1),
	ASYNC_TX_ACK		 = (1 << 3),
	ASYNC_TX_DEP_ACK	 = (1 << 4),
};

#ifdef CONFIG_DMA_ENGINE
#define async_tx_issue_pending_all dma_issue_pending_all
#ifdef CONFIG_ARCH_HAS_ASYNC_TX_FIND_CHANNEL
#include <asm/async_tx.h>
#else
#define async_tx_find_channel(dep, type, dst, dst_count, src, src_count, len) \
	 __async_tx_find_channel(dep, type)
struct dma_chan *
__async_tx_find_channel(struct dma_async_tx_descriptor *depend_tx,
	enum dma_transaction_type tx_type);
#endif /* CONFIG_ARCH_HAS_ASYNC_TX_FIND_CHANNEL */
#else
static inline void async_tx_issue_pending_all(void)
{
	do { } while (0);
}

static inline struct dma_chan *
async_tx_find_channel(struct dma_async_tx_descriptor *depend_tx,
	enum dma_transaction_type tx_type, struct page **dst, int dst_count,
	struct page **src, int src_count, size_t len)
{
	return NULL;
}
#endif

static inline void
async_tx_sync_epilog(dma_async_tx_callback cb_fn, void *cb_fn_param)
{
	if (cb_fn)
		cb_fn(cb_fn_param);
}

void
async_tx_submit(struct dma_chan *chan, struct dma_async_tx_descriptor *tx,
	enum async_tx_flags flags, struct dma_async_tx_descriptor *depend_tx,
	dma_async_tx_callback cb_fn, void *cb_fn_param);

struct dma_async_tx_descriptor *
async_xor(struct page *dest, struct page **src_list, unsigned int offset,
	int src_cnt, size_t len, enum async_tx_flags flags,
	struct dma_async_tx_descriptor *depend_tx,
	dma_async_tx_callback cb_fn, void *cb_fn_param);

struct dma_async_tx_descriptor *
async_xor_zero_sum(struct page *dest, struct page **src_list,
	unsigned int offset, int src_cnt, size_t len,
	u32 *result, enum async_tx_flags flags,
	struct dma_async_tx_descriptor *depend_tx,
	dma_async_tx_callback cb_fn, void *cb_fn_param);

struct dma_async_tx_descriptor *
async_memcpy(struct page *dest, struct page *src, unsigned int dest_offset,
	unsigned int src_offset, size_t len, enum async_tx_flags flags,
	struct dma_async_tx_descriptor *depend_tx,
	dma_async_tx_callback cb_fn, void *cb_fn_param);

struct dma_async_tx_descriptor *
async_memset(struct page *dest, int val, unsigned int offset,
	size_t len, enum async_tx_flags flags,
	struct dma_async_tx_descriptor *depend_tx,
	dma_async_tx_callback cb_fn, void *cb_fn_param);

struct dma_async_tx_descriptor *
async_trigger_callback(enum async_tx_flags flags,
	struct dma_async_tx_descriptor *depend_tx,
	dma_async_tx_callback cb_fn, void *cb_fn_param);

void async_tx_quiesce(struct dma_async_tx_descriptor **tx);
#endif /* _ASYNC_TX_H_ */
