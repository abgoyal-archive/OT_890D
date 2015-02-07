


#ifndef _ME1400_DEVICE_H_
#define _ME1400_DEVICE_H_

#include "metypes.h"
#include "medefines.h"
#include "meinternal.h"

#include "medevice.h"

#ifdef __KERNEL__

typedef struct me1400_version {
	uint16_t device_id;					/**< The PCI device id of the device. */
	unsigned int dio_chips;				/**< The number of 8255 chips on the device. */
	unsigned int ctr_chips;				/**< The number of 8254 chips on the device. */
	unsigned int ext_irq_subdevices;	/**< The number of external interrupt inputs on the device. */
} me1400_version_t;

static me1400_version_t me1400_versions[] = {
	{PCI_DEVICE_ID_MEILHAUS_ME1400, 1, 0, 0},
	{PCI_DEVICE_ID_MEILHAUS_ME140A, 1, 1, 1},
	{PCI_DEVICE_ID_MEILHAUS_ME140B, 2, 2, 1},
	{PCI_DEVICE_ID_MEILHAUS_ME14E0, 1, 0, 0},
	{PCI_DEVICE_ID_MEILHAUS_ME14EA, 1, 1, 1},
	{PCI_DEVICE_ID_MEILHAUS_ME14EB, 2, 2, 1},
	{PCI_DEVICE_ID_MEILHAUS_ME140C, 1, 5, 1},
	{PCI_DEVICE_ID_MEILHAUS_ME140D, 2, 10, 1},
	{0}
};

#define ME1400_DEVICE_VERSIONS (sizeof(me1400_versions) / sizeof(me1400_version_t) - 1)	/**< Returns the number of entries in #me1400_versions. */

static inline unsigned int me1400_versions_get_device_index(uint16_t device_id)
{
	unsigned int i;
	for (i = 0; i < ME1400_DEVICE_VERSIONS; i++)
		if (me1400_versions[i].device_id == device_id)
			break;
	return i;
}

#define ME1400_MAX_8254		10	/**< The maximum number of 8254 counter subdevices available on any ME-1400 device. */
#define ME1400_MAX_8255		2	/**< The maximum number of 8255 digital i/o subdevices available on any ME-1400 device. */

typedef struct me1400_device {
	me_device_t base;									/**< The Meilhaus device base class. */

	spinlock_t clk_src_reg_lock;						/**< Guards the 8254 clock source registers. */
	spinlock_t ctr_ctrl_reg_lock[ME1400_MAX_8254];		/**< Guards the 8254 ctrl registers. */

	int dio_current_mode[ME1400_MAX_8255];				/**< Saves the current mode setting of a single 8255 DIO chip. */
	spinlock_t dio_ctrl_reg_lock[ME1400_MAX_8255];		/**< Guards the 8255 ctrl register and #dio_current_mode. */
} me1400_device_t;

me_device_t *me1400_pci_constructor(struct pci_dev *pci_device)
    __attribute__ ((weak));

#endif
#endif
