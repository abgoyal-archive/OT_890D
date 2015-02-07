


#ifndef _ME1000_H_
#define _ME1000_H_

#include <linux/pci.h>
#include <linux/spinlock.h>

#include "medevice.h"

#ifdef __KERNEL__

#define ME1000_MAGIC_NUMBER	1000

typedef struct me1000_device {
	me_device_t base;		/**< The Meilhaus device base class. */
	spinlock_t ctrl_lock;		/**< Guards the DIO mode register. */
} me1000_device_t;

me_device_t *me1000_pci_constructor(struct pci_dev *pci_device)
    __attribute__ ((weak));

#endif
#endif
