


#ifndef _ME0900_DEVICE_H
#define _ME0900_DEVICE_H

#include <linux/pci.h>
#include <linux/spinlock.h>

#include "medevice.h"

#ifdef __KERNEL__

typedef struct me0900_version {
	uint16_t device_id;
	unsigned int di_subdevices;
	unsigned int do_subdevices;
} me0900_version_t;

static me0900_version_t me0900_versions[] = {
	{PCI_DEVICE_ID_MEILHAUS_ME0940, 2, 0},
	{PCI_DEVICE_ID_MEILHAUS_ME0950, 0, 2},
	{PCI_DEVICE_ID_MEILHAUS_ME0960, 1, 1},
	{0},
};

#define ME0900_DEVICE_VERSIONS (sizeof(me0900_versions) / sizeof(me0900_version_t) - 1)	/**< Returns the number of entries in #me0900_versions. */

static inline unsigned int me0900_versions_get_device_index(uint16_t device_id)
{
	unsigned int i;
	for (i = 0; i < ME0900_DEVICE_VERSIONS; i++)
		if (me0900_versions[i].device_id == device_id)
			break;
	return i;
}

typedef struct me0900_device {
	me_device_t base;			/**< The Meilhaus device base class. */
} me0900_device_t;

me_device_t *me0900_pci_constructor(struct pci_dev *pci_device)
    __attribute__ ((weak));

#endif
#endif
