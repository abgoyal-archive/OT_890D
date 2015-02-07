

#ifndef _ME8255_H_
#define _ME8255_H_

#include "mesubdevice.h"
#include "meslock.h"

#ifdef __KERNEL__

typedef struct me8255_subdevice {
	/* Inheritance */
	me_subdevice_t base;			/**< The subdevice base class. */

	/* Attributes */
	spinlock_t subdevice_lock;		/**< Spin lock to protect the subdevice from concurrent access. */

	int *ctrl_reg_mirror;			/**< Pointer to mirror of the control register. */
	spinlock_t *ctrl_reg_lock;		/**< Spin lock to protect #ctrl_reg and #ctrl_reg_mirror from concurrent access. */

	uint32_t device_id;				/**< The PCI device id of the device holding the 8255 chip. */
	int me8255_idx;					/**< The index of the 8255 chip on the device. */
	int dio_idx;					/**< The index of the DIO port on the 8255 chip. */

	unsigned long port_reg;			/**< Register to read or write a value from or to the port respectively. */
	unsigned long ctrl_reg;			/**< Register to configure the 8255 modes. */
} me8255_subdevice_t;

me8255_subdevice_t *me8255_constructor(uint32_t device_id,
				       uint32_t reg_base,
				       unsigned int me8255_idx,
				       unsigned int dio_idx,
				       int *ctrl_reg_mirror,
				       spinlock_t * ctrl_reg_lock);

#endif
#endif
