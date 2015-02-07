


#ifndef _ME0600_RELAY_H_
#define _ME0600_RELAY_H_

#include "mesubdevice.h"

#ifdef __KERNEL__

typedef struct me0600_relay_subdevice {
	/* Inheritance */
	me_subdevice_t base;			/**< The subdevice base class. */

	/* Attributes */
	spinlock_t subdevice_lock;		/**< Spin lock to protect the subdevice from concurrent access. */

	unsigned long port_0_reg;			/**< Register holding the port status. */
	unsigned long port_1_reg;			/**< Register holding the port status. */
#ifdef MEDEBUG_DEBUG_REG
	unsigned long reg_base;
#endif
} me0600_relay_subdevice_t;

me0600_relay_subdevice_t *me0600_relay_constructor(uint32_t reg_base);

#endif
#endif
