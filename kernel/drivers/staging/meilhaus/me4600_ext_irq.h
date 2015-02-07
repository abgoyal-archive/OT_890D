


#ifndef _ME4600_EXT_IRQ_H_
#define _ME4600_EXT_IRQ_H_

#include "mesubdevice.h"

#ifdef __KERNEL__

typedef struct me4600_ext_irq_subdevice {
	/* Inheritance */
	me_subdevice_t base;			/**< The subdevice base class. */

	/* Attributes */
	spinlock_t subdevice_lock;		/**< Spin lock to protect the subdevice from concurrent access. */
	spinlock_t *ctrl_reg_lock;		/**< Spin lock to protect #ctrl_reg from concurrent access. */

	wait_queue_head_t wait_queue;

	int irq;

	int rised;
	int value;
	int count;

	unsigned long ctrl_reg;
	unsigned long irq_status_reg;
	unsigned long ext_irq_config_reg;
	unsigned long ext_irq_value_reg;
#ifdef MEDEBUG_DEBUG_REG
	unsigned long reg_base;
#endif
} me4600_ext_irq_subdevice_t;

me4600_ext_irq_subdevice_t *me4600_ext_irq_constructor(uint32_t reg_base,
						       int irq,
						       spinlock_t *
						       ctrl_reg_lock);

#endif
#endif
