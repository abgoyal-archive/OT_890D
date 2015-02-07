


#ifndef _ME8100_DO_H_
#define _ME8100_DO_H_

#include "mesubdevice.h"

#ifdef __KERNEL__

typedef struct me8100_do_subdevice {
	/* Inheritance */
	me_subdevice_t base;			/**< The subdevice base class. */

	/* Attributes */
	spinlock_t subdevice_lock;		/**< Spin lock to protect the subdevice from concurrent access. */
	spinlock_t *ctrl_reg_lock;		/**< Spin lock to protect the #ctrl_reg. */

	unsigned int do_idx;

	uint16_t port_reg_mirror;		/**< Mirror used to store current port register setting which is write only. */

	unsigned long port_reg;			/**< Register holding the port status. */
	unsigned long ctrl_reg;			/**< Control register. */
#ifdef MEDEBUG_DEBUG_REG
	unsigned long reg_base;
#endif
} me8100_do_subdevice_t;

me8100_do_subdevice_t *me8100_do_constructor(uint32_t reg_base,
					     unsigned int do_idx,
					     spinlock_t * ctrl_reg_lock);

#endif
#endif
