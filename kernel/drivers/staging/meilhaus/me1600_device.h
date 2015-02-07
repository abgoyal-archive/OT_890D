


#ifndef _ME1600_H
#define _ME1600_H

#include <linux/pci.h>
#include <linux/spinlock.h>

#include "medevice.h"
#include "me1600_ao.h"
#include "me1600_ao_reg.h"

#ifdef __KERNEL__

typedef struct me1600_version {
	uint16_t device_id;				/**< The PCI device id of the device. */
	unsigned int ao_chips;			/**< The number of analog outputs on the device. */
	int curr;						/**< Flag to identify amounts of current output. */
} me1600_version_t;

static me1600_version_t me1600_versions[] = {
	{PCI_DEVICE_ID_MEILHAUS_ME1600_4U, 4, 0},
	{PCI_DEVICE_ID_MEILHAUS_ME1600_8U, 8, 0},
	{PCI_DEVICE_ID_MEILHAUS_ME1600_12U, 12, 0},
	{PCI_DEVICE_ID_MEILHAUS_ME1600_16U, 16, 0},
	{PCI_DEVICE_ID_MEILHAUS_ME1600_16U_8I, 16, 8},
	{0}
};

/**< Returns the number of entries in #me1600_versions. */
#define ME1600_DEVICE_VERSIONS (sizeof(me1600_versions) / sizeof(me1600_version_t) - 1)

static inline unsigned int me1600_versions_get_device_index(uint16_t device_id)
{
	unsigned int i;
	for (i = 0; i < ME1600_DEVICE_VERSIONS; i++)
		if (me1600_versions[i].device_id == device_id)
			break;
	return i;
}

typedef struct me1600_device {
	me_device_t base;						/**< The Meilhaus device base class. */
	spinlock_t config_regs_lock;			/**< Protects the configuration registers. */

	me1600_ao_shadow_t ao_regs_shadows;		/**< Addresses and shadows of output's registers. */
	spinlock_t ao_shadows_lock;				/**< Protects the shadow's struct. */
} me1600_device_t;

me_device_t *me1600_pci_constructor(struct pci_dev *pci_device)
    __attribute__ ((weak));

#endif
#endif
