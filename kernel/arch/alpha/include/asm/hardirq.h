
#ifndef _ALPHA_HARDIRQ_H
#define _ALPHA_HARDIRQ_H

#include <linux/threads.h>
#include <linux/cache.h>


/* entry.S is sensitive to the offsets of these fields */
typedef struct {
	unsigned long __softirq_pending;
} ____cacheline_aligned irq_cpustat_t;

#include <linux/irq_cpustat.h>	/* Standard mappings for irq_cpustat_t above */

void ack_bad_irq(unsigned int irq);

#define HARDIRQ_BITS	12

#if (1 << HARDIRQ_BITS) < 16
#error HARDIRQ_BITS is too low!
#endif

#endif /* _ALPHA_HARDIRQ_H */
