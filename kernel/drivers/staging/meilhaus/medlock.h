

#ifndef _MEDLOCK_H_
#define _MEDLOCK_H_

#include <linux/spinlock.h>

#ifdef __KERNEL__

typedef struct me_dlock {
	struct file *filep;		/**< Pointer to file structure holding the device. */
	int count;				/**< Number of tasks which are inside the device. */
	spinlock_t spin_lock;	/**< Spin lock protecting the attributes from concurrent access. */
} me_dlock_t;

int me_dlock_enter(struct me_dlock *dlock, struct file *filep);

int me_dlock_exit(struct me_dlock *dlock, struct file *filep);

int me_dlock_lock(struct me_dlock *dlock,
		  struct file *filep, int lock, int flags, me_slist_t * slist);

int me_dlock_init(me_dlock_t * dlock);

void me_dlock_deinit(me_dlock_t * dlock);

#endif
#endif
