


#ifndef _ME8254_H_
#define _ME8254_H_

#include "mesubdevice.h"
#include "meslock.h"

#ifdef __KERNEL__

typedef struct me8254_subdevice {
	/* Inheritance */
	me_subdevice_t base;			/**< The subdevice base class. */

	/* Attributes */
	spinlock_t subdevice_lock;		/**< Spin lock to protect the subdevice from concurrent access. */

	spinlock_t *ctrl_reg_lock;		/**< Spin lock to protect the control register from concurrent access. */
	spinlock_t *clk_src_reg_lock;		/**< Spin lock to protect the clock source register from concurrent access. */

	uint32_t device_id;			/**< The Meilhaus device type carrying the 8254 chip. */
	int me8254_idx;				/**< The index of the 8254 chip on the device. */
	int ctr_idx;				/**< The index of the counter on the 8254 chip. */

	int caps;				/**< Holds the device capabilities. */

	unsigned long val_reg;			/**< Holds the actual counter value. */
	unsigned long ctrl_reg;			/**< Register to configure the 8254 modes. */
	unsigned long clk_src_reg;		/**< Register to configure the counter connections. */
} me8254_subdevice_t;

me8254_subdevice_t *me8254_constructor(uint32_t device_id,
				       uint32_t reg_base,
				       unsigned int me8254_idx,
				       unsigned int ctr_idx,
				       spinlock_t * ctrl_reg_lock,
				       spinlock_t * clk_src_reg_lock);

#endif
#endif
