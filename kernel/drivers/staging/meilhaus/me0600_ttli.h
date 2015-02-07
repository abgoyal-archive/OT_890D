


#ifndef _ME0600_TTLI_H_
#define _ME0600_TTLI_H_

#include "mesubdevice.h"

#ifdef __KERNEL__

typedef struct me0600_ttli_subdevice {
	/* Inheritance */
	me_subdevice_t base;			/**< The subdevice base class. */

	/* Attributes */
	spinlock_t subdevice_lock;		/**< Spin lock to protect the subdevice from concurrent access. */

	uint32_t port_reg;			/**< Register holding the port status. */
} me0600_ttli_subdevice_t;

me0600_ttli_subdevice_t *me0600_ttli_constructor(uint32_t reg_base);

#endif
#endif
