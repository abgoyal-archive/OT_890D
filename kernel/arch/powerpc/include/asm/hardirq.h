
#ifndef _ASM_POWERPC_HARDIRQ_H
#define _ASM_POWERPC_HARDIRQ_H
#ifdef __KERNEL__

#include <asm/irq.h>
#include <asm/bug.h>

typedef struct {
	unsigned int __softirq_pending;	/* set_bit is used on this */
	unsigned int __last_jiffy_stamp;
} ____cacheline_aligned irq_cpustat_t;

#include <linux/irq_cpustat.h>	/* Standard mappings for irq_cpustat_t above */

#define last_jiffy_stamp(cpu) __IRQ_STAT((cpu), __last_jiffy_stamp)

static inline void ack_bad_irq(int irq)
{
	printk(KERN_CRIT "illegal vector %d received!\n", irq);
	BUG();
}

#endif /* __KERNEL__ */
#endif /* _ASM_POWERPC_HARDIRQ_H */
