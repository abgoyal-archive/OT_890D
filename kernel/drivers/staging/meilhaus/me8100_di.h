


#ifndef _ME8100_DI_H_
#define _ME8100_DI_H_

#include "mesubdevice.h"

#ifdef __KERNEL__

typedef struct me8100_di_subdevice {
	// Inheritance
	me_subdevice_t base;			/**< The subdevice base class. */

	/* Attributes */
	spinlock_t subdevice_lock;		/**< Spin lock to protect the subdevice from concurrent access. */
	spinlock_t *ctrl_reg_lock;

	unsigned di_idx;

	int irq;
	volatile int rised;
	unsigned int irq_count;

	uint status_flag;				/**< Default interupt status flag */
	uint status_value;				/**< Interupt status */
	uint status_value_edges;		/**< Extended interupt status */
	uint line_value;

	uint16_t compare_value;
	uint8_t filtering_flag;

	wait_queue_head_t wait_queue;

	unsigned long ctrl_reg;
	unsigned long port_reg;
	unsigned long mask_reg;
	unsigned long pattern_reg;
	unsigned long long din_int_reg;
	unsigned long irq_reset_reg;
	unsigned long irq_status_reg;
#ifdef MEDEBUG_DEBUG_REG
	unsigned long reg_base;
#endif

} me8100_di_subdevice_t;

me8100_di_subdevice_t *me8100_di_constructor(uint32_t me8100_reg_base,
					     uint32_t plx_reg_base,
					     unsigned int di_idx,
					     int irq,
					     spinlock_t * ctrl_leg_lock);

#endif
#endif
