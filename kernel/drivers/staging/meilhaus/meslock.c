


#include <linux/spinlock.h>

#include "medefines.h"
#include "meerror.h"

#include "medebug.h"
#include "meslock.h"

int me_slock_enter(struct me_slock *slock, struct file *filep)
{
	PDEBUG_LOCKS("executed.\n");

	spin_lock(&slock->spin_lock);

	if ((slock->filep) != NULL && (slock->filep != filep)) {
		PERROR("Subdevice is locked by another process.\n");
		spin_unlock(&slock->spin_lock);
		return ME_ERRNO_LOCKED;
	}

	slock->count++;

	spin_unlock(&slock->spin_lock);

	return ME_ERRNO_SUCCESS;
}

int me_slock_exit(struct me_slock *slock, struct file *filep)
{
	PDEBUG_LOCKS("executed.\n");

	spin_lock(&slock->spin_lock);
	slock->count--;
	spin_unlock(&slock->spin_lock);

	return ME_ERRNO_SUCCESS;
}

int me_slock_lock(struct me_slock *slock, struct file *filep, int lock)
{
	PDEBUG_LOCKS("executed.\n");

	switch (lock) {

	case ME_LOCK_RELEASE:
		spin_lock(&slock->spin_lock);

		if (slock->filep == filep)
			slock->filep = NULL;

		spin_unlock(&slock->spin_lock);

		break;

	case ME_LOCK_SET:
		spin_lock(&slock->spin_lock);

		if (slock->count) {
			spin_unlock(&slock->spin_lock);
			PERROR("Subdevice is used by another process.\n");
			return ME_ERRNO_USED;
		} else if (slock->filep == NULL)
			slock->filep = filep;
		else if (slock->filep != filep) {
			spin_unlock(&slock->spin_lock);
			PERROR("Subdevice is locked by another process.\n");
			return ME_ERRNO_LOCKED;
		}

		spin_unlock(&slock->spin_lock);

		break;

	case ME_LOCK_CHECK:
		spin_lock(&slock->spin_lock);

		if (slock->count) {
			spin_unlock(&slock->spin_lock);
			return ME_ERRNO_USED;
		} else if ((slock->filep != NULL) && (slock->filep != filep)) {
			spin_unlock(&slock->spin_lock);
			return ME_ERRNO_LOCKED;
		}

		spin_unlock(&slock->spin_lock);

		break;

	default:
		break;
	}

	return ME_ERRNO_SUCCESS;
}

void me_slock_deinit(struct me_slock *slock)
{
	PDEBUG_LOCKS("executed.\n");
}

int me_slock_init(me_slock_t * slock)
{
	PDEBUG_LOCKS("executed.\n");

	slock->filep = NULL;
	slock->count = 0;
	spin_lock_init(&slock->spin_lock);

	return 0;
}
