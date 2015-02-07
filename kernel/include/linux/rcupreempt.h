

#ifndef __LINUX_RCUPREEMPT_H
#define __LINUX_RCUPREEMPT_H

#include <linux/cache.h>
#include <linux/spinlock.h>
#include <linux/threads.h>
#include <linux/percpu.h>
#include <linux/cpumask.h>
#include <linux/seqlock.h>

struct rcu_dyntick_sched {
	int dynticks;
	int dynticks_snap;
	int sched_qs;
	int sched_qs_snap;
	int sched_dynticks_snap;
};

DECLARE_PER_CPU(struct rcu_dyntick_sched, rcu_dyntick_sched);

static inline void rcu_qsctr_inc(int cpu)
{
	struct rcu_dyntick_sched *rdssp = &per_cpu(rcu_dyntick_sched, cpu);

	rdssp->sched_qs++;
}
#define rcu_bh_qsctr_inc(cpu)

#define call_rcu_bh	 	call_rcu

extern void call_rcu_sched(struct rcu_head *head,
			   void (*func)(struct rcu_head *head));

extern void __rcu_read_lock(void)	__acquires(RCU);
extern void __rcu_read_unlock(void)	__releases(RCU);
extern int rcu_pending(int cpu);
extern int rcu_needs_cpu(int cpu);

#define __rcu_read_lock_bh()	{ rcu_read_lock(); local_bh_disable(); }
#define __rcu_read_unlock_bh()	{ local_bh_enable(); rcu_read_unlock(); }

extern void __synchronize_sched(void);

extern void __rcu_init(void);
extern void rcu_init_sched(void);
extern void rcu_check_callbacks(int cpu, int user);
extern void rcu_restart_cpu(int cpu);
extern long rcu_batches_completed(void);

static inline long rcu_batches_completed_bh(void)
{
	return rcu_batches_completed();
}

#ifdef CONFIG_RCU_TRACE
struct rcupreempt_trace;
extern long *rcupreempt_flipctr(int cpu);
extern long rcupreempt_data_completed(void);
extern int rcupreempt_flip_flag(int cpu);
extern int rcupreempt_mb_flag(int cpu);
extern char *rcupreempt_try_flip_state_name(void);
extern struct rcupreempt_trace *rcupreempt_trace_cpu(int cpu);
#endif

struct softirq_action;

#ifdef CONFIG_NO_HZ

static inline void rcu_enter_nohz(void)
{
	static DEFINE_RATELIMIT_STATE(rs, 10 * HZ, 1);

	smp_mb(); /* CPUs seeing ++ must see prior RCU read-side crit sects */
	__get_cpu_var(rcu_dyntick_sched).dynticks++;
	WARN_ON_RATELIMIT(__get_cpu_var(rcu_dyntick_sched).dynticks & 0x1, &rs);
}

static inline void rcu_exit_nohz(void)
{
	static DEFINE_RATELIMIT_STATE(rs, 10 * HZ, 1);

	__get_cpu_var(rcu_dyntick_sched).dynticks++;
	smp_mb(); /* CPUs seeing ++ must see later RCU read-side crit sects */
	WARN_ON_RATELIMIT(!(__get_cpu_var(rcu_dyntick_sched).dynticks & 0x1),
				&rs);
}

#else /* CONFIG_NO_HZ */
#define rcu_enter_nohz()	do { } while (0)
#define rcu_exit_nohz()		do { } while (0)
#endif /* CONFIG_NO_HZ */

static inline int rcu_blocking_is_gp(void)
{
	return num_online_cpus() == 1 && !rcu_scheduler_active;
}

#endif /* __LINUX_RCUPREEMPT_H */
