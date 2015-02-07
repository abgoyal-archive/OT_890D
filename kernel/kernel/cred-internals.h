

static inline void sched_switch_user(struct task_struct *p)
{
#ifdef CONFIG_USER_SCHED
	sched_move_task(p);
#endif	/* CONFIG_USER_SCHED */
}

