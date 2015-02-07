

#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <asm/dma.h>
#include <asm/dmabrg.h>
#include <asm/io.h>


#define DMARSRA		0xfe090000
#define DMAOR		0xffa00040
#define DMACHCR0	0xffa0000c
#define DMABRGCR	0xfe3c0000

#define DMAOR_BRG	0x0000c000
#define DMAOR_DMEN	0x00000001

#define DMABRGI0	68
#define DMABRGI1	69
#define DMABRGI2	70

struct dmabrg_handler {
	void (*handler)(void *);
	void *data;
} *dmabrg_handlers;

static inline void dmabrg_call_handler(int i)
{
	dmabrg_handlers[i].handler(dmabrg_handlers[i].data);
}

static irqreturn_t dmabrg_irq(int irq, void *data)
{
	unsigned long dcr;
	unsigned int i;

	dcr = ctrl_inl(DMABRGCR);
	ctrl_outl(dcr & ~0x00ff0003, DMABRGCR);	/* ack all */
	dcr &= dcr >> 8;	/* ignore masked */

	/* USB stuff, get it out of the way first */
	if (dcr & 1)
		dmabrg_call_handler(DMABRGIRQ_USBDMA);
	if (dcr & 2)
		dmabrg_call_handler(DMABRGIRQ_USBDMAERR);

	/* Audio */
	dcr >>= 16;
	while (dcr) {
		i = __ffs(dcr);
		dcr &= dcr - 1;
		dmabrg_call_handler(i + DMABRGIRQ_A0TXF);
	}
	return IRQ_HANDLED;
}

static void dmabrg_disable_irq(unsigned int dmairq)
{
	unsigned long dcr;
	dcr = ctrl_inl(DMABRGCR);
	dcr &= ~(1 << ((dmairq > 1) ? dmairq + 22 : dmairq + 8));
	ctrl_outl(dcr, DMABRGCR);
}

static void dmabrg_enable_irq(unsigned int dmairq)
{
	unsigned long dcr;
	dcr = ctrl_inl(DMABRGCR);
	dcr |= (1 << ((dmairq > 1) ? dmairq + 22 : dmairq + 8));
	ctrl_outl(dcr, DMABRGCR);
}

int dmabrg_request_irq(unsigned int dmairq, void(*handler)(void*),
		       void *data)
{
	if ((dmairq > 9) || !handler)
		return -ENOENT;
	if (dmabrg_handlers[dmairq].handler)
		return -EBUSY;

	dmabrg_handlers[dmairq].handler = handler;
	dmabrg_handlers[dmairq].data = data;
	
	dmabrg_enable_irq(dmairq);
	return 0;
}
EXPORT_SYMBOL_GPL(dmabrg_request_irq);

void dmabrg_free_irq(unsigned int dmairq)
{
	if (likely(dmairq < 10)) {
		dmabrg_disable_irq(dmairq);
		dmabrg_handlers[dmairq].handler = NULL;
		dmabrg_handlers[dmairq].data = NULL;
	}
}
EXPORT_SYMBOL_GPL(dmabrg_free_irq);

static int __init dmabrg_init(void)
{
	unsigned long or;
	int ret;

	dmabrg_handlers = kzalloc(10 * sizeof(struct dmabrg_handler),
				  GFP_KERNEL);
	if (!dmabrg_handlers)
		return -ENOMEM;

#ifdef CONFIG_SH_DMA
	/* request DMAC channel 0 before anyone else can get it */
	ret = request_dma(0, "DMAC 0 (DMABRG)");
	if (ret < 0)
		printk(KERN_INFO "DMABRG: DMAC ch0 not reserved!\n");
#endif

	ctrl_outl(0, DMABRGCR);
	ctrl_outl(0, DMACHCR0);
	ctrl_outl(0x94000000, DMARSRA);	/* enable DMABRG in DMAC 0 */

	/* enable DMABRG mode, enable the DMAC */
	or = ctrl_inl(DMAOR);
	ctrl_outl(or | DMAOR_BRG | DMAOR_DMEN, DMAOR);

	ret = request_irq(DMABRGI0, dmabrg_irq, IRQF_DISABLED,
			"DMABRG USB address error", NULL);
	if (ret)
		goto out0;

	ret = request_irq(DMABRGI1, dmabrg_irq, IRQF_DISABLED,
			"DMABRG Transfer End", NULL);
	if (ret)
		goto out1;

	ret = request_irq(DMABRGI2, dmabrg_irq, IRQF_DISABLED,
			"DMABRG Transfer Half", NULL);
	if (ret == 0)
		return ret;

	free_irq(DMABRGI1, 0);
out1:	free_irq(DMABRGI0, 0);
out0:	kfree(dmabrg_handlers);
	return ret;
}
subsys_initcall(dmabrg_init);
