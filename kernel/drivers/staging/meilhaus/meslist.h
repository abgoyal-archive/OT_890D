

#ifndef _ME_SLIST_H_
#define _ME_SLIST_H_

#include <linux/list.h>

#include "mesubdevice.h"

#ifdef __KERNEL__

typedef struct me_slist {
	struct list_head head;		/**< The head of the internal list. */
	unsigned int n;			/**< The number of subdevices in the list. */
} me_slist_t;

int me_slist_query_number_subdevices(struct me_slist *slist, int *number);

unsigned int me_slist_get_number_subdevices(struct me_slist *slist);

me_subdevice_t *me_slist_get_subdevice(struct me_slist *slist,
				       unsigned int index);

int me_slist_get_subdevice_by_type(struct me_slist *slist,
				   unsigned int start_subdevice,
				   int type, int subtype, int *subdevice);

void me_slist_add_subdevice_tail(struct me_slist *slist,
				 me_subdevice_t * subdevice);

me_subdevice_t *me_slist_del_subdevice_tail(struct me_slist *slist);

int me_slist_init(me_slist_t * slist);

void me_slist_deinit(me_slist_t * slist);

#endif
#endif
