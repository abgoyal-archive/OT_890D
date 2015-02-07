

#ifndef _ME_DLIST_H_
#define _ME_DLIST_H_

#include <linux/list.h>

#include "medevice.h"

#ifdef __KERNEL__

typedef struct me_dlist {
	struct list_head head;		/**< The head of the internal list. */
	unsigned int n;			/**< The number of devices in the list. */
} me_dlist_t;

int me_dlist_query_number_devices(struct me_dlist *dlist, int *number);

unsigned int me_dlist_get_number_devices(struct me_dlist *dlist);

me_device_t *me_dlist_get_device(struct me_dlist *dlist, unsigned int index);

void me_dlist_add_device_tail(struct me_dlist *dlist, me_device_t * device);

me_device_t *me_dlist_del_device_tail(struct me_dlist *dlist);

int me_dlist_init(me_dlist_t * dlist);

void me_dlist_deinit(me_dlist_t * dlist);

#endif
#endif
