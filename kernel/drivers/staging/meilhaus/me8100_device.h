


#ifndef _ME8100_DEVICE_H
#define _ME8100_DEVICE_H

#include <linux/pci.h>
#include <linux/spinlock.h>

#include "medevice.h"

#ifdef __KERNEL__

typedef struct me8100_version {
	uint16_t device_id;
	unsigned int di_subdevices;
	unsigned int do_subdevices;
	unsigned int ctr_subdevices;
} me8100_version_t;

static me8100_version_t me8100_versions[] = {
	{PCI_DEVICE_ID_MEILHAUS_ME8100_A, 1, 1, 3},
	{PCI_DEVICE_ID_MEILHAUS_ME8100_B, 2, 2, 3},
	{0},
};

#define ME8100_DEVICE_VERSIONS (sizeof(me8100_versions) / sizeof(me8100_version_t) - 1)	/**< Returns the number of entries in #me8100_versions. */

static inline unsigned int me8100_versions_get_device_index(uint16_t device_id)
{
	unsigned int i;
	for (i = 0; i < ME8100_DEVICE_VERSIONS; i++)
		if (me8100_versions[i].device_id == device_id)
			break;
	return i;
}

typedef struct me8100_device {
	me_device_t base;			/**< The Meilhaus device base class. */

	/* Child class attributes. */
	spinlock_t dio_ctrl_reg_lock;
	spinlock_t ctr_ctrl_reg_lock;
	spinlock_t clk_src_reg_lock;
} me8100_device_t;

me_device_t *me8100_pci_constructor(struct pci_dev *pci_device)
    __attribute__ ((weak));

#endif
#endif
