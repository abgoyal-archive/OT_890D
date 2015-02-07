

#ifndef _MESLOCK_H_
#define _MESLOCK_H_

#include <linux/spinlock.h>

#ifdef __KERNEL__

typedef struct me_slock {
	struct file *filep;		/**< Pointer to file structure holding the subdevice. */
	int count;			/**< Number of tasks which are inside the subdevice. */
	spinlock_t spin_lock;		/**< Spin lock protecting the attributes from concurrent access. */
} me_slock_t;

int me_slock_enter(struct me_slock *slock, struct file *filep);

int me_slock_exit(struct me_slock *slock, struct file *filep);

int me_slock_lock(struct me_slock *slock, struct file *filep, int lock);

int me_slock_init(me_slock_t * slock);

void me_slock_deinit(me_slock_t * slock);

#endif
#endif
