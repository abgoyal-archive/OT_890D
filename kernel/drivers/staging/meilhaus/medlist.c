


#include "meerror.h"
#include "medefines.h"

#include "medlist.h"
#include "medebug.h"

int me_dlist_query_number_devices(struct me_dlist *dlist, int *number)
{
	PDEBUG_LOCKS("called.\n");
	*number = dlist->n;
	return ME_ERRNO_SUCCESS;
}

unsigned int me_dlist_get_number_devices(struct me_dlist *dlist)
{
	PDEBUG_LOCKS("called.\n");
	return dlist->n;
}

me_device_t *me_dlist_get_device(struct me_dlist * dlist, unsigned int index)
{

	struct list_head *pos;
	me_device_t *device = NULL;
	unsigned int i = 0;

	PDEBUG_LOCKS("called.\n");

	if (index >= dlist->n) {
		PERROR("Index out of range.\n");
		return NULL;
	}

	list_for_each(pos, &dlist->head) {
		if (i == index) {
			device = list_entry(pos, me_device_t, list);
			break;
		}

		++i;
	}

	return device;
}

void me_dlist_add_device_tail(struct me_dlist *dlist, me_device_t * device)
{
	PDEBUG_LOCKS("called.\n");

	list_add_tail(&device->list, &dlist->head);
	++dlist->n;
}

me_device_t *me_dlist_del_device_tail(struct me_dlist *dlist)
{

	struct list_head *last;
	me_device_t *device;

	PDEBUG_LOCKS("called.\n");

	if (list_empty(&dlist->head))
		return NULL;

	last = dlist->head.prev;

	device = list_entry(last, me_device_t, list);

	list_del(last);

	--dlist->n;

	return device;
}

int me_dlist_init(me_dlist_t * dlist)
{
	PDEBUG_LOCKS("called.\n");

	INIT_LIST_HEAD(&dlist->head);
	dlist->n = 0;
	return 0;
}

void me_dlist_deinit(me_dlist_t * dlist)
{

	struct list_head *s;
	me_device_t *device;

	PDEBUG_LOCKS("called.\n");

	while (!list_empty(&dlist->head)) {
		s = dlist->head.next;
		list_del(s);
		device = list_entry(s, me_device_t, list);
		device->me_device_destructor(device);
	}

	dlist->n = 0;
}
