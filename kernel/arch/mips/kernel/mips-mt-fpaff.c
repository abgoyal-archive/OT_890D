
#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/security.h>
#include <linux/types.h>
#include <asm/uaccess.h>

cpumask_t mt_fpu_cpumask;

static int fpaff_threshold = -1;
unsigned long mt_fpemul_threshold = 0;


static inline struct task_struct *find_process_by_pid(pid_t pid)
{
	return pid ? find_task_by_vpid(pid) : current;
}


asmlinkage long mipsmt_sys_sched_setaffinity(pid_t pid, unsigned int len,
				      unsigned long __user *user_mask_ptr)
{
	cpumask_t new_mask;
	cpumask_t effective_mask;
	int retval;
	struct task_struct *p;
	struct thread_info *ti;
	uid_t euid;

	if (len < sizeof(new_mask))
		return -EINVAL;

	if (copy_from_user(&new_mask, user_mask_ptr, sizeof(new_mask)))
		return -EFAULT;

	get_online_cpus();
	read_lock(&tasklist_lock);

	p = find_process_by_pid(pid);
	if (!p) {
		read_unlock(&tasklist_lock);
		put_online_cpus();
		return -ESRCH;
	}

	/*
	 * It is not safe to call set_cpus_allowed with the
	 * tasklist_lock held.  We will bump the task_struct's
	 * usage count and drop tasklist_lock before invoking
	 * set_cpus_allowed.
	 */
	get_task_struct(p);

	euid = current_euid();
	retval = -EPERM;
	if (euid != p->cred->euid && euid != p->cred->uid &&
	    !capable(CAP_SYS_NICE)) {
		read_unlock(&tasklist_lock);
		goto out_unlock;
	}

	retval = security_task_setscheduler(p, 0, NULL);
	if (retval)
		goto out_unlock;

	/* Record new user-specified CPU set for future reference */
	p->thread.user_cpus_allowed = new_mask;

	/* Unlock the task list */
	read_unlock(&tasklist_lock);

	/* Compute new global allowed CPU set if necessary */
	ti = task_thread_info(p);
	if (test_ti_thread_flag(ti, TIF_FPUBOUND) &&
	    cpus_intersects(new_mask, mt_fpu_cpumask)) {
		cpus_and(effective_mask, new_mask, mt_fpu_cpumask);
		retval = set_cpus_allowed(p, effective_mask);
	} else {
		clear_ti_thread_flag(ti, TIF_FPUBOUND);
		retval = set_cpus_allowed(p, new_mask);
	}

out_unlock:
	put_task_struct(p);
	put_online_cpus();
	return retval;
}

asmlinkage long mipsmt_sys_sched_getaffinity(pid_t pid, unsigned int len,
				      unsigned long __user *user_mask_ptr)
{
	unsigned int real_len;
	cpumask_t mask;
	int retval;
	struct task_struct *p;

	real_len = sizeof(mask);
	if (len < real_len)
		return -EINVAL;

	get_online_cpus();
	read_lock(&tasklist_lock);

	retval = -ESRCH;
	p = find_process_by_pid(pid);
	if (!p)
		goto out_unlock;
	retval = security_task_getscheduler(p);
	if (retval)
		goto out_unlock;

	cpus_and(mask, p->thread.user_cpus_allowed, cpu_possible_map);

out_unlock:
	read_unlock(&tasklist_lock);
	put_online_cpus();
	if (retval)
		return retval;
	if (copy_to_user(user_mask_ptr, &mask, real_len))
		return -EFAULT;
	return real_len;
}


static int __init fpaff_thresh(char *str)
{
	get_option(&str, &fpaff_threshold);
	return 1;
}
__setup("fpaff=", fpaff_thresh);

#define FPUSEFACTOR 2000

static __init int mt_fp_affinity_init(void)
{
	if (fpaff_threshold >= 0) {
		mt_fpemul_threshold = fpaff_threshold;
	} else {
		mt_fpemul_threshold =
			(FPUSEFACTOR * (loops_per_jiffy/(500000/HZ))) / HZ;
	}
	printk(KERN_DEBUG "FPU Affinity set after %ld emulations\n",
	       mt_fpemul_threshold);

	return 0;
}
arch_initcall(mt_fp_affinity_init);
