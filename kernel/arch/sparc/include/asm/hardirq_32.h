

#ifndef __SPARC_HARDIRQ_H
#define __SPARC_HARDIRQ_H

#include <linux/threads.h>
#include <linux/spinlock.h>
#include <linux/cache.h>

/* entry.S is sensitive to the offsets of these fields */ /* XXX P3 Is it? */
typedef struct {
	unsigned int __softirq_pending;
} ____cacheline_aligned irq_cpustat_t;

#include <linux/irq_cpustat.h>	/* Standard mappings for irq_cpustat_t above */

#define HARDIRQ_BITS    8

#endif /* __SPARC_HARDIRQ_H */
