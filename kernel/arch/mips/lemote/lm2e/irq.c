
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>

#include <asm/irq_cpu.h>
#include <asm/i8259.h>
#include <asm/mipsregs.h>
#include <asm/mips-boards/bonito64.h>


static void bonito_irqdispatch(void)
{
	u32 int_status;
	int i;

	/* workaround the IO dma problem: let cpu looping to allow DMA finish */
	int_status = BONITO_INTISR;
	if (int_status & (1 << 10)) {
		while (int_status & (1 << 10)) {
			udelay(1);
			int_status = BONITO_INTISR;
		}
	}

	/* Get pending sources, masked by current enables */
	int_status = BONITO_INTISR & BONITO_INTEN;

	if (int_status != 0) {
		i = __ffs(int_status);
		int_status &= ~(1 << i);
		do_IRQ(BONITO_IRQ_BASE + i);
	}
}

static void i8259_irqdispatch(void)
{
	int irq;

	irq = i8259_irq();
	if (irq >= 0) {
		do_IRQ(irq);
	} else {
		spurious_interrupt();
	}

}

asmlinkage void plat_irq_dispatch(void)
{
	unsigned int pending = read_c0_cause() & read_c0_status() & ST0_IM;

	if (pending & CAUSEF_IP7) {
		do_IRQ(MIPS_CPU_IRQ_BASE + 7);
	} else if (pending & CAUSEF_IP5) {
		i8259_irqdispatch();
	} else if (pending & CAUSEF_IP2) {
		bonito_irqdispatch();
	} else {
		spurious_interrupt();
	}
}

static struct irqaction cascade_irqaction = {
	.handler = no_action,
	.mask = CPU_MASK_NONE,
	.name = "cascade",
};

void __init arch_init_irq(void)
{
	extern void bonito_irq_init(void);

	/*
	 * Clear all of the interrupts while we change the able around a bit.
	 * int-handler is not on bootstrap
	 */
	clear_c0_status(ST0_IM | ST0_BEV);
	local_irq_disable();

	/* most bonito irq should be level triggered */
	BONITO_INTEDGE = BONITO_ICU_SYSTEMERR | BONITO_ICU_MASTERERR |
		BONITO_ICU_RETRYERR | BONITO_ICU_MBOXES;
	BONITO_INTSTEER = 0;

	/*
	 * Mask out all interrupt by writing "1" to all bit position in
	 * the interrupt reset reg.
	 */
	BONITO_INTENCLR = ~0;

	/* init all controller
	 *   0-15         ------> i8259 interrupt
	 *   16-23        ------> mips cpu interrupt
	 *   32-63        ------> bonito irq
	 */

	/* Sets the first-level interrupt dispatcher. */
	mips_cpu_irq_init();
	init_i8259_irqs();
	bonito_irq_init();

	/*
	printk("GPIODATA=%x, GPIOIE=%x\n", BONITO_GPIODATA, BONITO_GPIOIE);
	printk("INTEN=%x, INTSET=%x, INTCLR=%x, INTISR=%x\n",
			BONITO_INTEN, BONITO_INTENSET,
			BONITO_INTENCLR, BONITO_INTISR);
	*/

	/* bonito irq at IP2 */
	setup_irq(MIPS_CPU_IRQ_BASE + 2, &cascade_irqaction);
	/* 8259 irq at IP5 */
	setup_irq(MIPS_CPU_IRQ_BASE + 5, &cascade_irqaction);

}
