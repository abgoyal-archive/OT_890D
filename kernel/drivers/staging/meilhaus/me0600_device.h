


#ifndef _ME0600_DEVICE_H
#define _ME0600_DEVICE_H

#include <linux/pci.h>
#include <linux/spinlock.h>

#include "medevice.h"

#ifdef __KERNEL__

typedef struct me0600_version {
	uint16_t device_id;
	unsigned int relay_subdevices;
	unsigned int ttli_subdevices;
	unsigned int optoi_subdevices;
	unsigned int dio_subdevices;
	unsigned int ext_irq_subdevices;
} me0600_version_t;

static me0600_version_t me0600_versions[] = {
	{PCI_DEVICE_ID_MEILHAUS_ME0630, 1, 1, 1, 2, 2},
	{0},
};

#define ME0600_DEVICE_VERSIONS (sizeof(me0600_versions) / sizeof(me0600_version_t) - 1)	/**< Returns the number of entries in #me0600_versions. */

static inline unsigned int me0600_versions_get_device_index(uint16_t device_id)
{
	unsigned int i;
	for (i = 0; i < ME0600_DEVICE_VERSIONS; i++)
		if (me0600_versions[i].device_id == device_id)
			break;
	return i;
}

typedef struct me0600_device {
	me_device_t base;			/**< The Meilhaus device base class. */

	/* Child class attributes. */
	spinlock_t dio_ctrl_reg_lock;
	spinlock_t intcsr_lock;
} me0600_device_t;

me_device_t *me0600_pci_constructor(struct pci_dev *pci_device)
    __attribute__ ((weak));

#endif
#endif
