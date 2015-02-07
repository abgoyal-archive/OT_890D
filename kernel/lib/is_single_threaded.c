

#include <linux/sched.h>

bool is_single_threaded(struct task_struct *p)
{
	struct task_struct *g, *t;
	struct mm_struct *mm = p->mm;

	if (atomic_read(&p->signal->count) != 1)
		goto no;

	if (atomic_read(&p->mm->mm_users) != 1) {
		read_lock(&tasklist_lock);
		do_each_thread(g, t) {
			if (t->mm == mm && t != p)
				goto no_unlock;
		} while_each_thread(g, t);
		read_unlock(&tasklist_lock);
	}

	return true;

no_unlock:
	read_unlock(&tasklist_lock);
no:
	return false;
}
