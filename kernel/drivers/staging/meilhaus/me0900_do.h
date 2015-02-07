


#ifndef _ME0900_DO_H_
#define _ME0900_DO_H_

#include "mesubdevice.h"

#ifdef __KERNEL__

typedef struct me0900_do_subdevice {
	/* Inheritance */
	me_subdevice_t base;			/**< The subdevice base class. */

	/* Attributes */
	spinlock_t subdevice_lock;		/**< Spin lock to protect the subdevice from concurrent access. */

	unsigned int do_idx;

	unsigned long ctrl_reg;
	unsigned long port_reg;
	unsigned long enable_reg;
	unsigned long disable_reg;
#ifdef MEDEBUG_DEBUG_REG
	unsigned long reg_base;
#endif
} me0900_do_subdevice_t;

me0900_do_subdevice_t *me0900_do_constructor(uint32_t reg_base,
					     unsigned int do_idx);

#endif
#endif
