

#ifndef __LINUX_RCUPDATE_H
#define __LINUX_RCUPDATE_H

#include <linux/cache.h>
#include <linux/spinlock.h>
#include <linux/threads.h>
#include <linux/percpu.h>
#include <linux/cpumask.h>
#include <linux/seqlock.h>
#include <linux/lockdep.h>
#include <linux/completion.h>

struct rcu_head {
	struct rcu_head *next;
	void (*func)(struct rcu_head *head);
};

/* Internal to kernel, but needed by rcupreempt.h. */
extern int rcu_scheduler_active;

#if defined(CONFIG_CLASSIC_RCU)
#include <linux/rcuclassic.h>
#elif defined(CONFIG_TREE_RCU)
#include <linux/rcutree.h>
#elif defined(CONFIG_PREEMPT_RCU)
#include <linux/rcupreempt.h>
#else
#error "Unknown RCU implementation specified to kernel configuration"
#endif /* #else #if defined(CONFIG_CLASSIC_RCU) */

#define RCU_HEAD_INIT 	{ .next = NULL, .func = NULL }
#define RCU_HEAD(head) struct rcu_head head = RCU_HEAD_INIT
#define INIT_RCU_HEAD(ptr) do { \
       (ptr)->next = NULL; (ptr)->func = NULL; \
} while (0)

#define rcu_read_lock() __rcu_read_lock()


#define rcu_read_unlock() __rcu_read_unlock()

#define rcu_read_lock_bh() __rcu_read_lock_bh()

#define rcu_read_unlock_bh() __rcu_read_unlock_bh()

#define rcu_read_lock_sched() preempt_disable()
#define rcu_read_lock_sched_notrace() preempt_disable_notrace()

#define rcu_read_unlock_sched() preempt_enable()
#define rcu_read_unlock_sched_notrace() preempt_enable_notrace()




#define rcu_dereference(p)     ({ \
				typeof(p) _________p1 = ACCESS_ONCE(p); \
				smp_read_barrier_depends(); \
				(_________p1); \
				})


#define rcu_assign_pointer(p, v) \
	({ \
		if (!__builtin_constant_p(v) || \
		    ((v) != NULL)) \
			smp_wmb(); \
		(p) = (v); \
	})

/* Infrastructure to implement the synchronize_() primitives. */

struct rcu_synchronize {
	struct rcu_head head;
	struct completion completion;
};

extern void wakeme_after_rcu(struct rcu_head  *head);

#define synchronize_sched() __synchronize_sched()

extern void call_rcu(struct rcu_head *head,
			      void (*func)(struct rcu_head *head));

extern void call_rcu_bh(struct rcu_head *head,
			void (*func)(struct rcu_head *head));

/* Exported common interfaces */
extern void synchronize_rcu(void);
extern void rcu_barrier(void);
extern void rcu_barrier_bh(void);
extern void rcu_barrier_sched(void);

/* Internal to kernel */
extern void rcu_init(void);
extern void rcu_scheduler_starting(void);
extern int rcu_needs_cpu(int cpu);

#endif /* __LINUX_RCUPDATE_H */
