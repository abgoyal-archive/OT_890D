


#ifndef _ME8200_DIO_H_
#define _ME8200_DIO_H_

#include "mesubdevice.h"

#ifdef __KERNEL__

typedef struct me8200_dio_subdevice {
	/* Inheritance */
	me_subdevice_t base;			/**< The subdevice base class. */

	/* Attributes */
	spinlock_t subdevice_lock;		/**< Spin lock to protect the subdevice from concurrent access. */
	spinlock_t *ctrl_reg_lock;		/**< Spin lock to protect #ctrl_reg from concurrent access. */
	unsigned int dio_idx;			/**< The index of the digital i/o on the device. */

	unsigned long port_reg;			/**< Register holding the port status. */
	unsigned long ctrl_reg;			/**< Register to configure the port direction. */
#ifdef MEDEBUG_DEBUG_REG
	unsigned long reg_base;
#endif
} me8200_dio_subdevice_t;

me8200_dio_subdevice_t *me8200_dio_constructor(uint32_t reg_base,
					       unsigned int dio_idx,
					       spinlock_t * ctrl_reg_lock);

#endif
#endif
