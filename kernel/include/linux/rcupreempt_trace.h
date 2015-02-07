

#ifndef __LINUX_RCUPREEMPT_TRACE_H
#define __LINUX_RCUPREEMPT_TRACE_H

#include <linux/types.h>
#include <linux/kernel.h>

#include <asm/atomic.h>


struct rcupreempt_trace {
	long		next_length;
	long		next_add;
	long		wait_length;
	long		wait_add;
	long		done_length;
	long		done_add;
	long		done_remove;
	atomic_t	done_invoked;
	long		rcu_check_callbacks;
	atomic_t	rcu_try_flip_1;
	atomic_t	rcu_try_flip_e1;
	long		rcu_try_flip_i1;
	long		rcu_try_flip_ie1;
	long		rcu_try_flip_g1;
	long		rcu_try_flip_a1;
	long		rcu_try_flip_ae1;
	long		rcu_try_flip_a2;
	long		rcu_try_flip_z1;
	long		rcu_try_flip_ze1;
	long		rcu_try_flip_z2;
	long		rcu_try_flip_m1;
	long		rcu_try_flip_me1;
	long		rcu_try_flip_m2;
};

#ifdef CONFIG_RCU_TRACE
#define RCU_TRACE(fn, arg) 	fn(arg);
#else
#define RCU_TRACE(fn, arg)
#endif

extern void rcupreempt_trace_move2done(struct rcupreempt_trace *trace);
extern void rcupreempt_trace_move2wait(struct rcupreempt_trace *trace);
extern void rcupreempt_trace_try_flip_1(struct rcupreempt_trace *trace);
extern void rcupreempt_trace_try_flip_e1(struct rcupreempt_trace *trace);
extern void rcupreempt_trace_try_flip_i1(struct rcupreempt_trace *trace);
extern void rcupreempt_trace_try_flip_ie1(struct rcupreempt_trace *trace);
extern void rcupreempt_trace_try_flip_g1(struct rcupreempt_trace *trace);
extern void rcupreempt_trace_try_flip_a1(struct rcupreempt_trace *trace);
extern void rcupreempt_trace_try_flip_ae1(struct rcupreempt_trace *trace);
extern void rcupreempt_trace_try_flip_a2(struct rcupreempt_trace *trace);
extern void rcupreempt_trace_try_flip_z1(struct rcupreempt_trace *trace);
extern void rcupreempt_trace_try_flip_ze1(struct rcupreempt_trace *trace);
extern void rcupreempt_trace_try_flip_z2(struct rcupreempt_trace *trace);
extern void rcupreempt_trace_try_flip_m1(struct rcupreempt_trace *trace);
extern void rcupreempt_trace_try_flip_me1(struct rcupreempt_trace *trace);
extern void rcupreempt_trace_try_flip_m2(struct rcupreempt_trace *trace);
extern void rcupreempt_trace_check_callbacks(struct rcupreempt_trace *trace);
extern void rcupreempt_trace_done_remove(struct rcupreempt_trace *trace);
extern void rcupreempt_trace_invoke(struct rcupreempt_trace *trace);
extern void rcupreempt_trace_next_add(struct rcupreempt_trace *trace);

#endif /* __LINUX_RCUPREEMPT_TRACE_H */
