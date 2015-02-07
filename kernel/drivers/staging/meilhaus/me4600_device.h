


#ifndef _ME4600_DEVICE_H
#define _ME4600_DEVICE_H

#include <linux/pci.h>
#include <linux/spinlock.h>

#include "medevice.h"

#ifdef __KERNEL__

typedef struct me4600_version {
	uint16_t device_id;
	unsigned int do_subdevices;
	unsigned int di_subdevices;
	unsigned int dio_subdevices;
	unsigned int ctr_subdevices;
	unsigned int ai_subdevices;
	unsigned int ai_channels;
	unsigned int ai_ranges;
	unsigned int ai_isolated;
	unsigned int ai_sh;
	unsigned int ao_subdevices;
	unsigned int ao_fifo;	//How many devices have FIFO
	unsigned int ext_irq_subdevices;
} me4600_version_t;

static me4600_version_t me4600_versions[] = {
	{PCI_DEVICE_ID_MEILHAUS_ME4610, 0, 0, 4, 3, 1, 16, 1, 0, 0, 0, 0, 1},

	{PCI_DEVICE_ID_MEILHAUS_ME4650, 0, 0, 4, 0, 1, 16, 4, 0, 0, 0, 0, 1},

	{PCI_DEVICE_ID_MEILHAUS_ME4660, 0, 0, 4, 3, 1, 16, 4, 0, 0, 2, 0, 1},
	{PCI_DEVICE_ID_MEILHAUS_ME4660I, 1, 1, 2, 3, 1, 16, 4, 1, 0, 2, 0, 1},
	{PCI_DEVICE_ID_MEILHAUS_ME4660S, 0, 0, 4, 3, 1, 16, 4, 0, 1, 2, 0, 1},
	{PCI_DEVICE_ID_MEILHAUS_ME4660IS, 1, 1, 2, 3, 1, 16, 4, 1, 1, 2, 0, 1},

	{PCI_DEVICE_ID_MEILHAUS_ME4670, 0, 0, 4, 3, 1, 32, 4, 0, 0, 4, 0, 1},
	{PCI_DEVICE_ID_MEILHAUS_ME4670I, 1, 1, 2, 3, 1, 32, 4, 1, 0, 4, 0, 1},
	{PCI_DEVICE_ID_MEILHAUS_ME4670S, 0, 0, 4, 3, 1, 32, 4, 0, 1, 4, 0, 1},
	{PCI_DEVICE_ID_MEILHAUS_ME4670IS, 1, 1, 2, 3, 1, 32, 4, 1, 1, 4, 0, 1},

	{PCI_DEVICE_ID_MEILHAUS_ME4680, 0, 0, 4, 3, 1, 32, 4, 0, 0, 4, 4, 1},
	{PCI_DEVICE_ID_MEILHAUS_ME4680I, 1, 1, 2, 3, 1, 32, 4, 1, 0, 4, 4, 1},
	{PCI_DEVICE_ID_MEILHAUS_ME4680S, 0, 0, 4, 3, 1, 32, 4, 0, 1, 4, 4, 1},
	{PCI_DEVICE_ID_MEILHAUS_ME4680IS, 1, 1, 2, 3, 1, 32, 4, 1, 1, 4, 4, 1},

	{0},
};

#define ME4600_DEVICE_VERSIONS (sizeof(me4600_versions) / sizeof(me4600_version_t) - 1)	/**< Returns the number of entries in #me4600_versions. */

static inline unsigned int me4600_versions_get_device_index(uint16_t device_id)
{
	unsigned int i;
	for (i = 0; i < ME4600_DEVICE_VERSIONS; i++)
		if (me4600_versions[i].device_id == device_id)
			break;
	return i;
}

typedef struct me4600_device {
	me_device_t base;					/**< The Meilhaus device base class. */

	/* Child class attributes. */
	spinlock_t preload_reg_lock;		/**< Guards the preload register of the anaolog output devices. */
	unsigned int preload_flags;			/**< Used in conjunction with #preload_reg_lock. */
	spinlock_t dio_lock;				/**< Locks the control register of the digital input/output subdevices. */
	spinlock_t ai_ctrl_lock;			/**< Locks the control register of the analog input subdevice. */
	spinlock_t ctr_ctrl_reg_lock;		/**< Locks the counter control register. */
	spinlock_t ctr_clk_src_reg_lock;	/**< Not used on this device but needed for the me8254 subdevice constructor call. */
} me4600_device_t;


#ifdef BOSCH
me_device_t *me4600_pci_constructor(struct pci_dev *pci_device, int me_bosch_fw)
    __attribute__ ((weak));
#else //~BOSCH
me_device_t *me4600_pci_constructor(struct pci_dev *pci_device)
    __attribute__ ((weak));
#endif

#endif
#endif
