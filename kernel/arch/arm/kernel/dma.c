
#include <linux/module.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/errno.h>

#include <asm/dma.h>

#include <asm/mach/dma.h>

DEFINE_SPINLOCK(dma_spin_lock);
EXPORT_SYMBOL(dma_spin_lock);

static dma_t dma_chan[MAX_DMA_CHANNELS];

int request_dma(dmach_t channel, const char *device_id)
{
	dma_t *dma = dma_chan + channel;
	int ret;

	if (channel >= MAX_DMA_CHANNELS || !dma->d_ops)
		goto bad_dma;

	if (xchg(&dma->lock, 1) != 0)
		goto busy;

	dma->device_id = device_id;
	dma->active    = 0;
	dma->invalid   = 1;

	ret = 0;
	if (dma->d_ops->request)
		ret = dma->d_ops->request(channel, dma);

	if (ret)
		xchg(&dma->lock, 0);

	return ret;

bad_dma:
	printk(KERN_ERR "dma: trying to allocate DMA%d\n", channel);
	return -EINVAL;

busy:
	return -EBUSY;
}
EXPORT_SYMBOL(request_dma);

void free_dma(dmach_t channel)
{
	dma_t *dma = dma_chan + channel;

	if (channel >= MAX_DMA_CHANNELS || !dma->d_ops)
		goto bad_dma;

	if (dma->active) {
		printk(KERN_ERR "dma%d: freeing active DMA\n", channel);
		dma->d_ops->disable(channel, dma);
		dma->active = 0;
	}

	if (xchg(&dma->lock, 0) != 0) {
		if (dma->d_ops->free)
			dma->d_ops->free(channel, dma);
		return;
	}

	printk(KERN_ERR "dma%d: trying to free free DMA\n", channel);
	return;

bad_dma:
	printk(KERN_ERR "dma: trying to free DMA%d\n", channel);
}
EXPORT_SYMBOL(free_dma);

void set_dma_sg (dmach_t channel, struct scatterlist *sg, int nr_sg)
{
	dma_t *dma = dma_chan + channel;

	if (dma->active)
		printk(KERN_ERR "dma%d: altering DMA SG while "
		       "DMA active\n", channel);

	dma->sg = sg;
	dma->sgcount = nr_sg;
	dma->invalid = 1;
}
EXPORT_SYMBOL(set_dma_sg);

void __set_dma_addr (dmach_t channel, void *addr)
{
	dma_t *dma = dma_chan + channel;

	if (dma->active)
		printk(KERN_ERR "dma%d: altering DMA address while "
		       "DMA active\n", channel);

	dma->sg = NULL;
	dma->addr = addr;
	dma->invalid = 1;
}
EXPORT_SYMBOL(__set_dma_addr);

void set_dma_count (dmach_t channel, unsigned long count)
{
	dma_t *dma = dma_chan + channel;

	if (dma->active)
		printk(KERN_ERR "dma%d: altering DMA count while "
		       "DMA active\n", channel);

	dma->sg = NULL;
	dma->count = count;
	dma->invalid = 1;
}
EXPORT_SYMBOL(set_dma_count);

void set_dma_mode (dmach_t channel, dmamode_t mode)
{
	dma_t *dma = dma_chan + channel;

	if (dma->active)
		printk(KERN_ERR "dma%d: altering DMA mode while "
		       "DMA active\n", channel);

	dma->dma_mode = mode;
	dma->invalid = 1;
}
EXPORT_SYMBOL(set_dma_mode);

void enable_dma (dmach_t channel)
{
	dma_t *dma = dma_chan + channel;

	if (!dma->lock)
		goto free_dma;

	if (dma->active == 0) {
		dma->active = 1;
		dma->d_ops->enable(channel, dma);
	}
	return;

free_dma:
	printk(KERN_ERR "dma%d: trying to enable free DMA\n", channel);
	BUG();
}
EXPORT_SYMBOL(enable_dma);

void disable_dma (dmach_t channel)
{
	dma_t *dma = dma_chan + channel;

	if (!dma->lock)
		goto free_dma;

	if (dma->active == 1) {
		dma->active = 0;
		dma->d_ops->disable(channel, dma);
	}
	return;

free_dma:
	printk(KERN_ERR "dma%d: trying to disable free DMA\n", channel);
	BUG();
}
EXPORT_SYMBOL(disable_dma);

int dma_channel_active(dmach_t channel)
{
	return dma_chan[channel].active;
}
EXPORT_SYMBOL(dma_channel_active);

void set_dma_page(dmach_t channel, char pagenr)
{
	printk(KERN_ERR "dma%d: trying to set_dma_page\n", channel);
}
EXPORT_SYMBOL(set_dma_page);

void set_dma_speed(dmach_t channel, int cycle_ns)
{
	dma_t *dma = dma_chan + channel;
	int ret = 0;

	if (dma->d_ops->setspeed)
		ret = dma->d_ops->setspeed(channel, dma, cycle_ns);
	dma->speed = ret;
}
EXPORT_SYMBOL(set_dma_speed);

int get_dma_residue(dmach_t channel)
{
	dma_t *dma = dma_chan + channel;
	int ret = 0;

	if (dma->d_ops->residue)
		ret = dma->d_ops->residue(channel, dma);

	return ret;
}
EXPORT_SYMBOL(get_dma_residue);

static int __init init_dma(void)
{
	arch_dma_init(dma_chan);
	return 0;
}

core_initcall(init_dma);
