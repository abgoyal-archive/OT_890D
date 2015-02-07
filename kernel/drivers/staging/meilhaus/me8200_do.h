


#ifndef _ME8200_DO_H_
#define _ME8200_DO_H_

#include "mesubdevice.h"

#ifdef __KERNEL__

typedef struct me8200_do_subdevice {
	/* Inheritance */
	me_subdevice_t base;			/**< The subdevice base class. */

	/* Attributes */
	spinlock_t subdevice_lock;		/**< Spin lock to protect the subdevice from concurrent access. */
	spinlock_t *irq_mode_lock;

	int irq;				/**< The number of the interrupt request */
	int rised;				/**< Flag to indicate if an interrupt occured */
	int count;				/**< Counts the number of interrupts occured */
	wait_queue_head_t wait_queue;		/**< To wait on interrupts */

	unsigned int do_idx;			/**< The number of the digital output */

	unsigned long port_reg;			/**< The digital output port */
	unsigned long irq_ctrl_reg;		/**< The interrupt control register */
	unsigned long irq_status_reg;		/**< The interrupt status register */
#ifdef MEDEBUG_DEBUG_REG
	unsigned long reg_base;
#endif
} me8200_do_subdevice_t;

me8200_do_subdevice_t *me8200_do_constructor(uint32_t reg_base,
					     unsigned int do_idx,
					     int irq,
					     spinlock_t * irq_mode_lock);

#endif
#endif
