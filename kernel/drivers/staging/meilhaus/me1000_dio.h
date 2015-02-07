


#ifndef _ME1000_DIO_H_
#define _ME1000_DIO_H_

#include "mesubdevice.h"
#include "meslock.h"

#ifdef __KERNEL__

typedef struct me1000_dio_subdevice {
	/* Inheritance */
	me_subdevice_t base;			/**< The subdevice base class. */

	/* Attributes */
//      uint32_t magic;                                 /**< The magic number unique for this structure. */

	spinlock_t subdevice_lock;		/**< Spin lock to protect the subdevice from concurrent access. */
	spinlock_t *ctrl_reg_lock;		/**< Spin lock to protect #ctrl_reg and #ctrl_reg_mirror from concurrent access. */
	int dio_idx;					/**< The index of the DIO port on the device. */

	unsigned long port_reg;			/**< Register to read or write a value from or to the port respectively. */
	unsigned long ctrl_reg;			/**< Register to configure the DIO modes. */
#ifdef MEDEBUG_DEBUG_REG
	unsigned long reg_base;
#endif
} me1000_dio_subdevice_t;

me1000_dio_subdevice_t *me1000_dio_constructor(uint32_t reg_base,
					       unsigned int dio_idx,
					       spinlock_t * ctrl_reg_lock);

#endif
#endif
