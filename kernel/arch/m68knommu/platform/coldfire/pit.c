
/***************************************************************************/


/***************************************************************************/

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/param.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/clockchips.h>
#include <asm/machdep.h>
#include <asm/io.h>
#include <asm/coldfire.h>
#include <asm/mcfpit.h>
#include <asm/mcfsim.h>

/***************************************************************************/

#define	FREQ	((MCF_CLK / 2) / 64)
#define	TA(a)	(MCF_IPSBAR + MCFPIT_BASE1 + (a))
#define	INTC0	(MCF_IPSBAR + MCFICM_INTC0)
#define PIT_CYCLES_PER_JIFFY (FREQ / HZ)

static u32 pit_cnt;


static void init_cf_pit_timer(enum clock_event_mode mode,
                             struct clock_event_device *evt)
{
	switch (mode) {
	case CLOCK_EVT_MODE_PERIODIC:

		__raw_writew(MCFPIT_PCSR_DISABLE, TA(MCFPIT_PCSR));
		__raw_writew(PIT_CYCLES_PER_JIFFY, TA(MCFPIT_PMR));
		__raw_writew(MCFPIT_PCSR_EN | MCFPIT_PCSR_PIE | \
				MCFPIT_PCSR_OVW | MCFPIT_PCSR_RLD | \
				MCFPIT_PCSR_CLK64, TA(MCFPIT_PCSR));
		break;

	case CLOCK_EVT_MODE_SHUTDOWN:
	case CLOCK_EVT_MODE_UNUSED:

		__raw_writew(MCFPIT_PCSR_DISABLE, TA(MCFPIT_PCSR));
		break;

	case CLOCK_EVT_MODE_ONESHOT:

		__raw_writew(MCFPIT_PCSR_DISABLE, TA(MCFPIT_PCSR));
		__raw_writew(MCFPIT_PCSR_EN | MCFPIT_PCSR_PIE | \
				MCFPIT_PCSR_OVW | MCFPIT_PCSR_CLK64, \
				TA(MCFPIT_PCSR));
		break;

	case CLOCK_EVT_MODE_RESUME:
		/* Nothing to do here */
		break;
	}
}

static int cf_pit_next_event(unsigned long delta,
		struct clock_event_device *evt)
{
	__raw_writew(delta, TA(MCFPIT_PMR));
	return 0;
}

struct clock_event_device cf_pit_clockevent = {
	.name		= "pit",
	.features	= CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
	.set_mode	= init_cf_pit_timer,
	.set_next_event	= cf_pit_next_event,
	.shift		= 32,
	.irq		= MCFINT_VECBASE + MCFINT_PIT1,
};



/***************************************************************************/

static irqreturn_t pit_tick(int irq, void *dummy)
{
	struct clock_event_device *evt = &cf_pit_clockevent;
	u16 pcsr;

	/* Reset the ColdFire timer */
	pcsr = __raw_readw(TA(MCFPIT_PCSR));
	__raw_writew(pcsr | MCFPIT_PCSR_PIF, TA(MCFPIT_PCSR));

	pit_cnt += PIT_CYCLES_PER_JIFFY;
	evt->event_handler(evt);
	return IRQ_HANDLED;
}

/***************************************************************************/

static struct irqaction pit_irq = {
	.name	 = "timer",
	.flags	 = IRQF_DISABLED | IRQF_TIMER,
	.handler = pit_tick,
};

/***************************************************************************/

static cycle_t pit_read_clk(void)
{
	unsigned long flags;
	u32 cycles;
	u16 pcntr;

	local_irq_save(flags);
	pcntr = __raw_readw(TA(MCFPIT_PCNTR));
	cycles = pit_cnt;
	local_irq_restore(flags);

	return cycles + PIT_CYCLES_PER_JIFFY - pcntr;
}

/***************************************************************************/

static struct clocksource pit_clk = {
	.name	= "pit",
	.rating	= 100,
	.read	= pit_read_clk,
	.shift	= 20,
	.mask	= CLOCKSOURCE_MASK(32),
	.flags	= CLOCK_SOURCE_IS_CONTINUOUS,
};

/***************************************************************************/

void hw_timer_init(void)
{
	u32 imr;

	cf_pit_clockevent.cpumask = cpumask_of(smp_processor_id());
	cf_pit_clockevent.mult = div_sc(FREQ, NSEC_PER_SEC, 32);
	cf_pit_clockevent.max_delta_ns =
		clockevent_delta2ns(0xFFFF, &cf_pit_clockevent);
	cf_pit_clockevent.min_delta_ns =
		clockevent_delta2ns(0x3f, &cf_pit_clockevent);
	clockevents_register_device(&cf_pit_clockevent);

	setup_irq(MCFINT_VECBASE + MCFINT_PIT1, &pit_irq);

	__raw_writeb(ICR_INTRCONF, INTC0 + MCFINTC_ICR0 + MCFINT_PIT1);
	imr = __raw_readl(INTC0 + MCFPIT_IMR);
	imr &= ~MCFPIT_IMR_IBIT;
	__raw_writel(imr, INTC0 + MCFPIT_IMR);

	pit_clk.mult = clocksource_hz2mult(FREQ, pit_clk.shift);
	clocksource_register(&pit_clk);
}

/***************************************************************************/
