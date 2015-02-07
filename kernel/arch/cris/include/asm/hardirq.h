
#ifndef __ASM_HARDIRQ_H
#define __ASM_HARDIRQ_H

#include <asm/irq.h>
#include <linux/threads.h>
#include <linux/cache.h>

typedef struct {
	unsigned int __softirq_pending;
} ____cacheline_aligned irq_cpustat_t;

#include <linux/irq_cpustat.h> /* Standard mappings for irq_cpustat_t above */

void ack_bad_irq(unsigned int irq);

#define HARDIRQ_BITS	8

#if (1 << HARDIRQ_BITS) < NR_IRQS
# error HARDIRQ_BITS is too low!
#endif

#endif /* __ASM_HARDIRQ_H */
