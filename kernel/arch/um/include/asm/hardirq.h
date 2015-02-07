
/* (c) 2004 cw@f00f.org, GPLv2 blah blah */

#ifndef __ASM_UM_HARDIRQ_H
#define __ASM_UM_HARDIRQ_H

#include <linux/threads.h>
#include <linux/irq.h>

typedef struct {
	unsigned int __softirq_pending;
} irq_cpustat_t;

#include <linux/irq_cpustat.h>

static inline void ack_bad_irq(unsigned int irq)
{
	printk(KERN_ERR "unexpected IRQ %02x\n", irq);
	BUG();
}

#endif /* __ASM_UM_HARDIRQ_H */
