

#ifndef __KERNEL_RTMUTEX_COMMON_H
#define __KERNEL_RTMUTEX_COMMON_H

#include <linux/rtmutex.h>

#ifdef CONFIG_RT_MUTEX_TESTER

extern void schedule_rt_mutex_test(struct rt_mutex *lock);

#define schedule_rt_mutex(_lock)				\
  do {								\
	if (!(current->flags & PF_MUTEX_TESTER))		\
		schedule();					\
	else							\
		schedule_rt_mutex_test(_lock);			\
  } while (0)

#else
# define schedule_rt_mutex(_lock)			schedule()
#endif

struct rt_mutex_waiter {
	struct plist_node	list_entry;
	struct plist_node	pi_list_entry;
	struct task_struct	*task;
	struct rt_mutex		*lock;
#ifdef CONFIG_DEBUG_RT_MUTEXES
	unsigned long		ip;
	struct pid		*deadlock_task_pid;
	struct rt_mutex		*deadlock_lock;
#endif
};

static inline int rt_mutex_has_waiters(struct rt_mutex *lock)
{
	return !plist_head_empty(&lock->wait_list);
}

static inline struct rt_mutex_waiter *
rt_mutex_top_waiter(struct rt_mutex *lock)
{
	struct rt_mutex_waiter *w;

	w = plist_first_entry(&lock->wait_list, struct rt_mutex_waiter,
			       list_entry);
	BUG_ON(w->lock != lock);

	return w;
}

static inline int task_has_pi_waiters(struct task_struct *p)
{
	return !plist_head_empty(&p->pi_waiters);
}

static inline struct rt_mutex_waiter *
task_top_pi_waiter(struct task_struct *p)
{
	return plist_first_entry(&p->pi_waiters, struct rt_mutex_waiter,
				  pi_list_entry);
}

#define RT_MUTEX_OWNER_PENDING	1UL
#define RT_MUTEX_HAS_WAITERS	2UL
#define RT_MUTEX_OWNER_MASKALL	3UL

static inline struct task_struct *rt_mutex_owner(struct rt_mutex *lock)
{
	return (struct task_struct *)
		((unsigned long)lock->owner & ~RT_MUTEX_OWNER_MASKALL);
}

static inline struct task_struct *rt_mutex_real_owner(struct rt_mutex *lock)
{
	return (struct task_struct *)
		((unsigned long)lock->owner & ~RT_MUTEX_HAS_WAITERS);
}

static inline unsigned long rt_mutex_owner_pending(struct rt_mutex *lock)
{
	return (unsigned long)lock->owner & RT_MUTEX_OWNER_PENDING;
}

extern struct task_struct *rt_mutex_next_owner(struct rt_mutex *lock);
extern void rt_mutex_init_proxy_locked(struct rt_mutex *lock,
				       struct task_struct *proxy_owner);
extern void rt_mutex_proxy_unlock(struct rt_mutex *lock,
				  struct task_struct *proxy_owner);

#ifdef CONFIG_DEBUG_RT_MUTEXES
# include "rtmutex-debug.h"
#else
# include "rtmutex.h"
#endif

#endif
