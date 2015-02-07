


#include <linux/spinlock.h>

#include "medefines.h"
#include "meerror.h"

#include "medebug.h"
#include "meslist.h"
#include "mesubdevice.h"
#include "medlock.h"

int me_dlock_enter(struct me_dlock *dlock, struct file *filep)
{
	PDEBUG_LOCKS("executed.\n");

	spin_lock(&dlock->spin_lock);

	if ((dlock->filep) != NULL && (dlock->filep != filep)) {
		PERROR("Device is locked by another process.\n");
		spin_unlock(&dlock->spin_lock);
		return ME_ERRNO_LOCKED;
	}

	dlock->count++;

	spin_unlock(&dlock->spin_lock);

	return ME_ERRNO_SUCCESS;
}

int me_dlock_exit(struct me_dlock *dlock, struct file *filep)
{
	PDEBUG_LOCKS("executed.\n");

	spin_lock(&dlock->spin_lock);
	dlock->count--;
	spin_unlock(&dlock->spin_lock);

	return ME_ERRNO_SUCCESS;
}

int me_dlock_lock(struct me_dlock *dlock,
		  struct file *filep, int lock, int flags, me_slist_t * slist)
{
	int err = ME_ERRNO_SUCCESS;
	int i;
	me_subdevice_t *subdevice;

	PDEBUG_LOCKS("executed.\n");

	spin_lock(&dlock->spin_lock);

	switch (lock) {

	case ME_LOCK_RELEASE:
		if ((dlock->filep == filep) || (dlock->filep == NULL)) {
			dlock->filep = NULL;

			/* Unlock all possibly locked subdevices. */

			for (i = 0; i < me_slist_get_number_subdevices(slist);
			     i++) {
				subdevice = me_slist_get_subdevice(slist, i);

				if (subdevice)
					err =
					    subdevice->
					    me_subdevice_lock_subdevice
					    (subdevice, filep, ME_LOCK_RELEASE,
					     flags);
				else
					err = ME_ERRNO_INTERNAL;
			}
		}

		break;

	case ME_LOCK_SET:
		if (dlock->count) {
			PERROR("Device is used by another process.\n");
			err = ME_ERRNO_USED;
		} else if ((dlock->filep != NULL) && (dlock->filep != filep)) {
			PERROR("Device is locked by another process.\n");
			err = ME_ERRNO_LOCKED;
		} else if (dlock->filep == NULL) {
			/* Check any subdevice is locked by another process. */

			for (i = 0; i < me_slist_get_number_subdevices(slist);
			     i++) {
				subdevice = me_slist_get_subdevice(slist, i);

				if (subdevice) {
					if ((err =
					     subdevice->
					     me_subdevice_lock_subdevice
					     (subdevice, filep, ME_LOCK_CHECK,
					      flags))) {
						PERROR
						    ("A subdevice is locked by another process.\n");
						break;
					}
				} else {
					err = ME_ERRNO_INTERNAL;
				}
			}

			/* If no subdevices are locked by other processes,
			   we can take ownership of the device. Otherwise we jump ahead. */
			if (!err)
				dlock->filep = filep;
		}

		break;

	case ME_LOCK_CHECK:
		if (dlock->count) {
			err = ME_ERRNO_USED;
		} else if ((dlock->filep != NULL) && (dlock->filep != filep)) {
			err = ME_ERRNO_LOCKED;
		} else if (dlock->filep == NULL) {
			for (i = 0; i < me_slist_get_number_subdevices(slist);
			     i++) {
				subdevice = me_slist_get_subdevice(slist, i);

				if (subdevice) {
					if ((err =
					     subdevice->
					     me_subdevice_lock_subdevice
					     (subdevice, filep, ME_LOCK_CHECK,
					      flags))) {
						PERROR
						    ("A subdevice is locked by another process.\n");
						break;
					}
				} else {
					err = ME_ERRNO_INTERNAL;
				}
			}
		}

		break;

	default:
		PERROR("Invalid lock.\n");

		err = ME_ERRNO_INVALID_LOCK;

		break;
	}

	spin_unlock(&dlock->spin_lock);

	return err;
}

void me_dlock_deinit(struct me_dlock *dlock)
{
	PDEBUG_LOCKS("executed.\n");
}

int me_dlock_init(me_dlock_t * dlock)
{
	PDEBUG_LOCKS("executed.\n");

	dlock->filep = NULL;
	dlock->count = 0;
	spin_lock_init(&dlock->spin_lock);

	return 0;
}
