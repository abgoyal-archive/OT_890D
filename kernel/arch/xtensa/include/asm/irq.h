

#ifndef _XTENSA_IRQ_H
#define _XTENSA_IRQ_H

#include <platform/hardware.h>
#include <variant/core.h>

#ifndef PLATFORM_NR_IRQS
# define PLATFORM_NR_IRQS 0
#endif
#define XTENSA_NR_IRQS XCHAL_NUM_INTERRUPTS
#define NR_IRQS (XTENSA_NR_IRQS + PLATFORM_NR_IRQS)

static __inline__ int irq_canonicalize(int irq)
{
	return (irq);
}

struct irqaction;

#endif	/* _XTENSA_IRQ_H */
