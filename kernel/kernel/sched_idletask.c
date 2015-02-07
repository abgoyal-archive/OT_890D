

#ifdef CONFIG_SMP
static int select_task_rq_idle(struct task_struct *p, int sync)
{
	return task_cpu(p); /* IDLE tasks as never migrated */
}
#endif /* CONFIG_SMP */
static void check_preempt_curr_idle(struct rq *rq, struct task_struct *p, int sync)
{
	resched_task(rq->idle);
}

static struct task_struct *pick_next_task_idle(struct rq *rq)
{
	schedstat_inc(rq, sched_goidle);

	return rq->idle;
}

static void
dequeue_task_idle(struct rq *rq, struct task_struct *p, int sleep)
{
	spin_unlock_irq(&rq->lock);
	printk(KERN_ERR "bad: scheduling from the idle thread!\n");
	dump_stack();
	spin_lock_irq(&rq->lock);
}

static void put_prev_task_idle(struct rq *rq, struct task_struct *prev)
{
}

#ifdef CONFIG_SMP
static unsigned long
load_balance_idle(struct rq *this_rq, int this_cpu, struct rq *busiest,
		  unsigned long max_load_move,
		  struct sched_domain *sd, enum cpu_idle_type idle,
		  int *all_pinned, int *this_best_prio)
{
	return 0;
}

static int
move_one_task_idle(struct rq *this_rq, int this_cpu, struct rq *busiest,
		   struct sched_domain *sd, enum cpu_idle_type idle)
{
	return 0;
}
#endif

static void task_tick_idle(struct rq *rq, struct task_struct *curr, int queued)
{
}

static void set_curr_task_idle(struct rq *rq)
{
}

static void switched_to_idle(struct rq *rq, struct task_struct *p,
			     int running)
{
	/* Can this actually happen?? */
	if (running)
		resched_task(rq->curr);
	else
		check_preempt_curr(rq, p, 0);
}

static void prio_changed_idle(struct rq *rq, struct task_struct *p,
			      int oldprio, int running)
{
	/* This can happen for hot plug CPUS */

	/*
	 * Reschedule if we are currently running on this runqueue and
	 * our priority decreased, or if we are not currently running on
	 * this runqueue and our priority is higher than the current's
	 */
	if (running) {
		if (p->prio > oldprio)
			resched_task(rq->curr);
	} else
		check_preempt_curr(rq, p, 0);
}

static const struct sched_class idle_sched_class = {
	/* .next is NULL */
	/* no enqueue/yield_task for idle tasks */

	/* dequeue is not valid, we print a debug message there: */
	.dequeue_task		= dequeue_task_idle,

	.check_preempt_curr	= check_preempt_curr_idle,

	.pick_next_task		= pick_next_task_idle,
	.put_prev_task		= put_prev_task_idle,

#ifdef CONFIG_SMP
	.select_task_rq		= select_task_rq_idle,

	.load_balance		= load_balance_idle,
	.move_one_task		= move_one_task_idle,
#endif

	.set_curr_task          = set_curr_task_idle,
	.task_tick		= task_tick_idle,

	.prio_changed		= prio_changed_idle,
	.switched_to		= switched_to_idle,

	/* no .task_new for idle tasks */
};
