


#ifndef _METEMPL_SUB_H_
#define _METEMPL_SUB_H_

#include "mesubdevice.h"

#ifdef __KERNEL__

typedef struct metempl_sub_subdevice {
	/* Inheritance */
	me_subdevice_t base;			/**< The subdevice base class. */

	/* Attributes */
	spinlock_t subdevice_lock;		/**< Spin lock to protect the subdevice from concurrent access. */
	spinlock_t *ctrl_reg_lock;		/**< Spin lock to protect #ctrl_reg from concurrent access. */
	int sub_idx;				/**< The index of the subdevice on the device. */

	unsigned long ctrl_reg;			/**< Register to configure the modes. */
} metempl_sub_subdevice_t;

metempl_sub_subdevice_t *metempl_sub_constructor(uint32_t reg_base,
						 unsigned int sub_idx,
						 spinlock_t * ctrl_reg_lock);

#endif
#endif
