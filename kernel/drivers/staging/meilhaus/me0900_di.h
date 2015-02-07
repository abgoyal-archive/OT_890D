


#ifndef _ME0900_DI_H_
#define _ME0900_DI_H_

#include "mesubdevice.h"

#ifdef __KERNEL__

typedef struct me0900_di_subdevice {
	/* Inheritance */
	me_subdevice_t base;			/**< The subdevice base class. */

	/* Attributes */
	spinlock_t subdevice_lock;		/**< Spin lock to protect the subdevice from concurrent access. */

	unsigned int di_idx;

	unsigned long ctrl_reg;
	unsigned long port_reg;
#ifdef MEDEBUG_DEBUG_REG
	unsigned long reg_base;
#endif
} me0900_di_subdevice_t;

me0900_di_subdevice_t *me0900_di_constructor(uint32_t me0900_reg_base,
					     unsigned int di_idx);

#endif
#endif
