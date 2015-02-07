


#ifndef _ME8200_DEVICE_H
#define _ME8200_DEVICE_H

#include <linux/pci.h>
#include <linux/spinlock.h>

#include "medevice.h"

#ifdef __KERNEL__

typedef struct me8200_version {
	uint16_t device_id;
	unsigned int di_subdevices;
	unsigned int do_subdevices;
	unsigned int dio_subdevices;
} me8200_version_t;

static me8200_version_t me8200_versions[] = {
	{PCI_DEVICE_ID_MEILHAUS_ME8200_A, 1, 1, 2},
	{PCI_DEVICE_ID_MEILHAUS_ME8200_B, 2, 2, 2},
	{0},
};

#define ME8200_DEVICE_VERSIONS (sizeof(me8200_versions) / sizeof(me8200_version_t) - 1)	/**< Returns the number of entries in #me8200_versions. */

static inline unsigned int me8200_versions_get_device_index(uint16_t device_id)
{
	unsigned int i;
	for (i = 0; i < ME8200_DEVICE_VERSIONS; i++)
		if (me8200_versions[i].device_id == device_id)
			break;
	return i;
}

typedef struct me8200_device {
	me_device_t base;		/**< The Meilhaus device base class. */

	/* Child class attributes. */
	spinlock_t irq_ctrl_lock;	/**< Lock for the interrupt control register. */
	spinlock_t irq_mode_lock;	/**< Lock for the interrupt mode register. */
	spinlock_t dio_ctrl_lock;	/**< Lock for the digital i/o control register. */
} me8200_device_t;

me_device_t *me8200_pci_constructor(struct pci_dev *pci_device)
    __attribute__ ((weak));

#endif
#endif
