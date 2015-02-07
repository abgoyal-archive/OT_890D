

#ifndef _ME0600_EXT_IRQ_H_
#define _ME0600_EXT_IRQ_H_

#include <linux/sched.h>

#include "mesubdevice.h"
#include "meslock.h"

#ifdef __KERNEL__

typedef struct me0600_ext_irq_subdevice {
	/* Inheritance */
	me_subdevice_t base;			/**< The subdevice base class. */

	/* Attributes */
	spinlock_t subdevice_lock;		/**< Spin lock to protect the subdevice from concurrent access. */
	spinlock_t *intcsr_lock;		/**< Spin lock to protect #intcsr. */

	wait_queue_head_t wait_queue;		/**< Queue to put on threads waiting for an interrupt. */

	int irq;				/**< The irq number assigned by PCI BIOS. */
	int rised;				/**< If true an interrupt has occured. */
	unsigned int n;				/**< The number of interrupt since the driver was loaded. */
	unsigned int lintno;			/**< The number of the local PCI interrupt. */

	uint32_t intcsr;			/**< The PLX interrupt control and status register. */
	uint32_t reset_reg;			/**< The control register. */
} me0600_ext_irq_subdevice_t;

me0600_ext_irq_subdevice_t *me0600_ext_irq_constructor(uint32_t plx_reg_base,
						       uint32_t me0600_reg_base,
						       spinlock_t * intcsr_lock,
						       unsigned int ext_irq_idx,
						       int irq);

#endif
#endif
