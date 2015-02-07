

#include "sched_cpupri.h"

/* Convert between a 140 based task->prio, and our 102 based cpupri */
static int convert_prio(int prio)
{
	int cpupri;

	if (prio == CPUPRI_INVALID)
		cpupri = CPUPRI_INVALID;
	else if (prio == MAX_PRIO)
		cpupri = CPUPRI_IDLE;
	else if (prio >= MAX_RT_PRIO)
		cpupri = CPUPRI_NORMAL;
	else
		cpupri = MAX_RT_PRIO - prio + 1;

	return cpupri;
}

#define for_each_cpupri_active(array, idx)                    \
  for (idx = find_first_bit(array, CPUPRI_NR_PRIORITIES);     \
       idx < CPUPRI_NR_PRIORITIES;                            \
       idx = find_next_bit(array, CPUPRI_NR_PRIORITIES, idx+1))

int cpupri_find(struct cpupri *cp, struct task_struct *p,
		struct cpumask *lowest_mask)
{
	int                  idx      = 0;
	int                  task_pri = convert_prio(p->prio);

	for_each_cpupri_active(cp->pri_active, idx) {
		struct cpupri_vec *vec  = &cp->pri_to_cpu[idx];

		if (idx >= task_pri)
			break;

		if (cpumask_any_and(&p->cpus_allowed, vec->mask) >= nr_cpu_ids)
			continue;

		cpumask_and(lowest_mask, &p->cpus_allowed, vec->mask);
		return 1;
	}

	return 0;
}

void cpupri_set(struct cpupri *cp, int cpu, int newpri)
{
	int                 *currpri = &cp->cpu_to_pri[cpu];
	int                  oldpri  = *currpri;
	unsigned long        flags;

	newpri = convert_prio(newpri);

	BUG_ON(newpri >= CPUPRI_NR_PRIORITIES);

	if (newpri == oldpri)
		return;

	/*
	 * If the cpu was currently mapped to a different value, we
	 * first need to unmap the old value
	 */
	if (likely(oldpri != CPUPRI_INVALID)) {
		struct cpupri_vec *vec  = &cp->pri_to_cpu[oldpri];

		spin_lock_irqsave(&vec->lock, flags);

		vec->count--;
		if (!vec->count)
			clear_bit(oldpri, cp->pri_active);
		cpumask_clear_cpu(cpu, vec->mask);

		spin_unlock_irqrestore(&vec->lock, flags);
	}

	if (likely(newpri != CPUPRI_INVALID)) {
		struct cpupri_vec *vec = &cp->pri_to_cpu[newpri];

		spin_lock_irqsave(&vec->lock, flags);

		cpumask_set_cpu(cpu, vec->mask);
		vec->count++;
		if (vec->count == 1)
			set_bit(newpri, cp->pri_active);

		spin_unlock_irqrestore(&vec->lock, flags);
	}

	*currpri = newpri;
}

int __init_refok cpupri_init(struct cpupri *cp, bool bootmem)
{
	int i;

	memset(cp, 0, sizeof(*cp));

	for (i = 0; i < CPUPRI_NR_PRIORITIES; i++) {
		struct cpupri_vec *vec = &cp->pri_to_cpu[i];

		spin_lock_init(&vec->lock);
		vec->count = 0;
		if (bootmem)
			alloc_bootmem_cpumask_var(&vec->mask);
		else if (!alloc_cpumask_var(&vec->mask, GFP_KERNEL))
			goto cleanup;
	}

	for_each_possible_cpu(i)
		cp->cpu_to_pri[i] = CPUPRI_INVALID;
	return 0;

cleanup:
	for (i--; i >= 0; i--)
		free_cpumask_var(cp->pri_to_cpu[i].mask);
	return -ENOMEM;
}

void cpupri_cleanup(struct cpupri *cp)
{
	int i;

	for (i = 0; i < CPUPRI_NR_PRIORITIES; i++)
		free_cpumask_var(cp->pri_to_cpu[i].mask);
}
