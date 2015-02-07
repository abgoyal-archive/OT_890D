


#ifndef _METEMPL_DEVICE_H
#define _METEMPL_DEVICE_H

#include <linux/pci.h>
#include <linux/spinlock.h>

#include "medevice.h"

#ifdef __KERNEL__

typedef struct metempl_version {
	uint16_t device_id;
	unsigned int subdevices;
} metempl_version_t;

static metempl_version_t metempl_versions[] = {
	{0xDEAD, 1},
	{0},
};

#define METEMPL_DEVICE_VERSIONS (sizeof(metempl_versions) / sizeof(metempl_version_t) - 1) /**< Returns the number of entries in #metempl_versions. */

static inline unsigned int metempl_versions_get_device_index(uint16_t device_id)
{
	unsigned int i;
	for (i = 0; i < METEMPL_DEVICE_VERSIONS; i++)
		if (metempl_versions[i].device_id == device_id)
			break;
	return i;
}

typedef struct metempl_device {
	me_device_t base;			/**< The Meilhaus device base class. */

	/* Child class attributes. */
	spinlock_t ctrl_reg_lock;
} metempl_device_t;

me_device_t *metempl_pci_constructor(struct pci_dev *pci_device)
    __attribute__ ((weak));

#endif
#endif
