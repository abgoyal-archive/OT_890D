
#ifndef _BLACKFIN_CURRENT_H
#define _BLACKFIN_CURRENT_H
#include <linux/thread_info.h>

struct task_struct;

static inline struct task_struct *get_current(void) __attribute__ ((__const__));
static inline struct task_struct *get_current(void)
{
	return (current_thread_info()->task);
}

#define	current	(get_current())

#endif				/* _BLACKFIN_CURRENT_H */
