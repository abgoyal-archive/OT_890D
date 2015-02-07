
#ifndef __H8300_HARDIRQ_H
#define __H8300_HARDIRQ_H

#include <linux/kernel.h>
#include <linux/threads.h>
#include <linux/interrupt.h>
#include <linux/irq.h>

typedef struct {
	unsigned int __softirq_pending;
} ____cacheline_aligned irq_cpustat_t;

#include <linux/irq_cpustat.h>	/* Standard mappings for irq_cpustat_t above */

extern void ack_bad_irq(unsigned int irq);

#define HARDIRQ_BITS	8

#if (1 << HARDIRQ_BITS) < NR_IRQS
# error HARDIRQ_BITS is too low!
#endif

#endif
