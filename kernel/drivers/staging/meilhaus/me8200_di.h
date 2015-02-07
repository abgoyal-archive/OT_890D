


#ifndef _ME8200_DI_H_
#define _ME8200_DI_H_

#include "mesubdevice.h"

#ifdef __KERNEL__

typedef struct me8200_di_subdevice {
	/* Inheritance */
	me_subdevice_t base;			/**< The subdevice base class. */

	/* Attributes */
	spinlock_t subdevice_lock;		/**< Spin lock to protect the subdevice from concurrent access. */
	spinlock_t *ctrl_reg_lock;
	spinlock_t *irq_ctrl_lock;
	spinlock_t *irq_mode_lock;

	unsigned int di_idx;
	unsigned int version;

	int irq;						/**< The number of the interrupt request. */
	volatile int rised;				/**< Flag to indicate if an interrupt occured. */
	uint status_flag;				/**< Default interupt status flag */
	uint status_value;				/**< Interupt status */
	uint status_value_edges;			/**< Extended interupt status */
	uint line_value;
	int count;						/**< Counts the number of interrupts occured. */
	uint8_t compare_value;
	uint8_t filtering_flag;

	wait_queue_head_t wait_queue;	/**< To wait on interrupts. */

	unsigned long port_reg;			/**< The digital input port. */
	unsigned long compare_reg;		/**< The register to hold the value to compare with. */
	unsigned long mask_reg;			/**< The register to hold the mask. */
	unsigned long irq_mode_reg;		/**< The interrupt mode register. */
	unsigned long irq_ctrl_reg;		/**< The interrupt control register. */
	unsigned long irq_status_reg;	/**< The interrupt status register. Also interrupt reseting register (firmware version 7 and later).*/
#ifdef MEDEBUG_DEBUG_REG
	unsigned long reg_base;
#endif
	unsigned long firmware_version_reg;	/**< The interrupt reseting register. */

	unsigned long irq_status_low_reg;	/**< The interrupt extended status register (low part). */
	unsigned long irq_status_high_reg;	/**< The interrupt extended status register (high part). */
} me8200_di_subdevice_t;

me8200_di_subdevice_t *me8200_di_constructor(uint32_t me8200_reg_base,
					     unsigned int di_idx,
					     int irq,
					     spinlock_t * irq_ctrl_lock,
					     spinlock_t * irq_mode_lock);

#endif
#endif
