
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/irq.h>

#include <asm/mips-boards/bonito64.h>


static inline void bonito_irq_enable(unsigned int irq)
{
	BONITO_INTENSET = (1 << (irq - BONITO_IRQ_BASE));
	mmiowb();
}

static inline void bonito_irq_disable(unsigned int irq)
{
	BONITO_INTENCLR = (1 << (irq - BONITO_IRQ_BASE));
	mmiowb();
}

static struct irq_chip bonito_irq_type = {
	.name	= "bonito_irq",
	.ack	= bonito_irq_disable,
	.mask	= bonito_irq_disable,
	.mask_ack = bonito_irq_disable,
	.unmask	= bonito_irq_enable,
};

static struct irqaction dma_timeout_irqaction = {
	.handler	= no_action,
	.name		= "dma_timeout",
};

void bonito_irq_init(void)
{
	u32 i;

	for (i = BONITO_IRQ_BASE; i < BONITO_IRQ_BASE + 32; i++) {
		set_irq_chip_and_handler(i, &bonito_irq_type, handle_level_irq);
	}

	setup_irq(BONITO_IRQ_BASE + 10, &dma_timeout_irqaction);
}
