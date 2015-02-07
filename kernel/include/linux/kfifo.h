
#ifndef _LINUX_KFIFO_H
#define _LINUX_KFIFO_H

#include <linux/kernel.h>
#include <linux/spinlock.h>

struct kfifo {
	unsigned char *buffer;	/* the buffer holding the data */
	unsigned int size;	/* the size of the allocated buffer */
	unsigned int in;	/* data is added at offset (in % size) */
	unsigned int out;	/* data is extracted from off. (out % size) */
	spinlock_t *lock;	/* protects concurrent modifications */
};

extern struct kfifo *kfifo_init(unsigned char *buffer, unsigned int size,
				gfp_t gfp_mask, spinlock_t *lock);
extern struct kfifo *kfifo_alloc(unsigned int size, gfp_t gfp_mask,
				 spinlock_t *lock);
extern void kfifo_free(struct kfifo *fifo);
extern unsigned int __kfifo_put(struct kfifo *fifo,
				unsigned char *buffer, unsigned int len);
extern unsigned int __kfifo_get(struct kfifo *fifo,
				unsigned char *buffer, unsigned int len);

static inline void __kfifo_reset(struct kfifo *fifo)
{
	fifo->in = fifo->out = 0;
}

static inline void kfifo_reset(struct kfifo *fifo)
{
	unsigned long flags;

	spin_lock_irqsave(fifo->lock, flags);

	__kfifo_reset(fifo);

	spin_unlock_irqrestore(fifo->lock, flags);
}

static inline unsigned int kfifo_put(struct kfifo *fifo,
				     unsigned char *buffer, unsigned int len)
{
	unsigned long flags;
	unsigned int ret;

	spin_lock_irqsave(fifo->lock, flags);

	ret = __kfifo_put(fifo, buffer, len);

	spin_unlock_irqrestore(fifo->lock, flags);

	return ret;
}

static inline unsigned int kfifo_get(struct kfifo *fifo,
				     unsigned char *buffer, unsigned int len)
{
	unsigned long flags;
	unsigned int ret;

	spin_lock_irqsave(fifo->lock, flags);

	ret = __kfifo_get(fifo, buffer, len);

	/*
	 * optimization: if the FIFO is empty, set the indices to 0
	 * so we don't wrap the next time
	 */
	if (fifo->in == fifo->out)
		fifo->in = fifo->out = 0;

	spin_unlock_irqrestore(fifo->lock, flags);

	return ret;
}

static inline unsigned int __kfifo_len(struct kfifo *fifo)
{
	return fifo->in - fifo->out;
}

static inline unsigned int kfifo_len(struct kfifo *fifo)
{
	unsigned long flags;
	unsigned int ret;

	spin_lock_irqsave(fifo->lock, flags);

	ret = __kfifo_len(fifo);

	spin_unlock_irqrestore(fifo->lock, flags);

	return ret;
}

#endif
