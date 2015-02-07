
#include <linux/types.h>
#include <linux/major.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/fcntl.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/tty_flip.h>
#include <linux/devpts_fs.h>
#include <linux/file.h>
#include <linux/fdtable.h>
#include <linux/console.h>
#include <linux/timer.h>
#include <linux/ctype.h>
#include <linux/kd.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/smp_lock.h>
#include <linux/device.h>
#include <linux/wait.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/seq_file.h>

#include <linux/uaccess.h>
#include <asm/system.h>

#include <linux/kbd_kern.h>
#include <linux/vt_kern.h>
#include <linux/selection.h>

#include <linux/kmod.h>
#include <linux/nsproxy.h>


static DEFINE_SPINLOCK(tty_ldisc_lock);
static DECLARE_WAIT_QUEUE_HEAD(tty_ldisc_wait);
/* Line disc dispatch table */
static struct tty_ldisc_ops *tty_ldiscs[NR_LDISCS];


int tty_register_ldisc(int disc, struct tty_ldisc_ops *new_ldisc)
{
	unsigned long flags;
	int ret = 0;

	if (disc < N_TTY || disc >= NR_LDISCS)
		return -EINVAL;

	spin_lock_irqsave(&tty_ldisc_lock, flags);
	tty_ldiscs[disc] = new_ldisc;
	new_ldisc->num = disc;
	new_ldisc->refcount = 0;
	spin_unlock_irqrestore(&tty_ldisc_lock, flags);

	return ret;
}
EXPORT_SYMBOL(tty_register_ldisc);


int tty_unregister_ldisc(int disc)
{
	unsigned long flags;
	int ret = 0;

	if (disc < N_TTY || disc >= NR_LDISCS)
		return -EINVAL;

	spin_lock_irqsave(&tty_ldisc_lock, flags);
	if (tty_ldiscs[disc]->refcount)
		ret = -EBUSY;
	else
		tty_ldiscs[disc] = NULL;
	spin_unlock_irqrestore(&tty_ldisc_lock, flags);

	return ret;
}
EXPORT_SYMBOL(tty_unregister_ldisc);



static int tty_ldisc_try_get(int disc, struct tty_ldisc *ld)
{
	unsigned long flags;
	struct tty_ldisc_ops *ldops;
	int err = -EINVAL;
	
	spin_lock_irqsave(&tty_ldisc_lock, flags);
	ld->ops = NULL;
	ldops = tty_ldiscs[disc];
	/* Check the entry is defined */
	if (ldops) {
		/* If the module is being unloaded we can't use it */
		if (!try_module_get(ldops->owner))
			err = -EAGAIN;
		else {
			/* lock it */
			ldops->refcount++;
			ld->ops = ldops;
			err = 0;
		}
	}
	spin_unlock_irqrestore(&tty_ldisc_lock, flags);
	return err;
}


static int tty_ldisc_get(int disc, struct tty_ldisc *ld)
{
	int err;

	if (disc < N_TTY || disc >= NR_LDISCS)
		return -EINVAL;
	err = tty_ldisc_try_get(disc, ld);
	if (err < 0) {
		request_module("tty-ldisc-%d", disc);
		err = tty_ldisc_try_get(disc, ld);
	}
	return err;
}


static void tty_ldisc_put(struct tty_ldisc_ops *ld)
{
	unsigned long flags;
	int disc = ld->num;

	BUG_ON(disc < N_TTY || disc >= NR_LDISCS);

	spin_lock_irqsave(&tty_ldisc_lock, flags);
	ld = tty_ldiscs[disc];
	BUG_ON(ld->refcount == 0);
	ld->refcount--;
	module_put(ld->owner);
	spin_unlock_irqrestore(&tty_ldisc_lock, flags);
}

static void * tty_ldiscs_seq_start(struct seq_file *m, loff_t *pos)
{
	return (*pos < NR_LDISCS) ? pos : NULL;
}

static void * tty_ldiscs_seq_next(struct seq_file *m, void *v, loff_t *pos)
{
	(*pos)++;
	return (*pos < NR_LDISCS) ? pos : NULL;
}

static void tty_ldiscs_seq_stop(struct seq_file *m, void *v)
{
}

static int tty_ldiscs_seq_show(struct seq_file *m, void *v)
{
	int i = *(loff_t *)v;
	struct tty_ldisc ld;
	
	if (tty_ldisc_get(i, &ld) < 0)
		return 0;
	seq_printf(m, "%-10s %2d\n", ld.ops->name ? ld.ops->name : "???", i);
	tty_ldisc_put(ld.ops);
	return 0;
}

static const struct seq_operations tty_ldiscs_seq_ops = {
	.start	= tty_ldiscs_seq_start,
	.next	= tty_ldiscs_seq_next,
	.stop	= tty_ldiscs_seq_stop,
	.show	= tty_ldiscs_seq_show,
};

static int proc_tty_ldiscs_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &tty_ldiscs_seq_ops);
}

const struct file_operations tty_ldiscs_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= proc_tty_ldiscs_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};


static void tty_ldisc_assign(struct tty_struct *tty, struct tty_ldisc *ld)
{
	ld->refcount = 0;
	tty->ldisc = *ld;
}


static int tty_ldisc_try(struct tty_struct *tty)
{
	unsigned long flags;
	struct tty_ldisc *ld;
	int ret = 0;

	spin_lock_irqsave(&tty_ldisc_lock, flags);
	ld = &tty->ldisc;
	if (test_bit(TTY_LDISC, &tty->flags)) {
		ld->refcount++;
		ret = 1;
	}
	spin_unlock_irqrestore(&tty_ldisc_lock, flags);
	return ret;
}


struct tty_ldisc *tty_ldisc_ref_wait(struct tty_struct *tty)
{
	/* wait_event is a macro */
	wait_event(tty_ldisc_wait, tty_ldisc_try(tty));
	WARN_ON(tty->ldisc.refcount == 0);
	return &tty->ldisc;
}

EXPORT_SYMBOL_GPL(tty_ldisc_ref_wait);


struct tty_ldisc *tty_ldisc_ref(struct tty_struct *tty)
{
	if (tty_ldisc_try(tty))
		return &tty->ldisc;
	return NULL;
}

EXPORT_SYMBOL_GPL(tty_ldisc_ref);


void tty_ldisc_deref(struct tty_ldisc *ld)
{
	unsigned long flags;

	BUG_ON(ld == NULL);

	spin_lock_irqsave(&tty_ldisc_lock, flags);
	if (ld->refcount == 0)
		printk(KERN_ERR "tty_ldisc_deref: no references.\n");
	else
		ld->refcount--;
	if (ld->refcount == 0)
		wake_up(&tty_ldisc_wait);
	spin_unlock_irqrestore(&tty_ldisc_lock, flags);
}

EXPORT_SYMBOL_GPL(tty_ldisc_deref);


void tty_ldisc_enable(struct tty_struct *tty)
{
	set_bit(TTY_LDISC, &tty->flags);
	clear_bit(TTY_LDISC_CHANGING, &tty->flags);
	wake_up(&tty_ldisc_wait);
}


static void tty_set_termios_ldisc(struct tty_struct *tty, int num)
{
	mutex_lock(&tty->termios_mutex);
	tty->termios->c_line = num;
	mutex_unlock(&tty->termios_mutex);
}



static void tty_ldisc_restore(struct tty_struct *tty, struct tty_ldisc *old)
{
	char buf[64];
	struct tty_ldisc new_ldisc;

	/* There is an outstanding reference here so this is safe */
	tty_ldisc_get(old->ops->num, old);
	tty_ldisc_assign(tty, old);
	tty_set_termios_ldisc(tty, old->ops->num);
	if (old->ops->open && (old->ops->open(tty) < 0)) {
		tty_ldisc_put(old->ops);
		/* This driver is always present */
		if (tty_ldisc_get(N_TTY, &new_ldisc) < 0)
			panic("n_tty: get");
		tty_ldisc_assign(tty, &new_ldisc);
		tty_set_termios_ldisc(tty, N_TTY);
		if (new_ldisc.ops->open) {
			int r = new_ldisc.ops->open(tty);
				if (r < 0)
				panic("Couldn't open N_TTY ldisc for "
				      "%s --- error %d.",
				      tty_name(tty, buf), r);
		}
	}
}


int tty_set_ldisc(struct tty_struct *tty, int ldisc)
{
	int retval;
	struct tty_ldisc o_ldisc, new_ldisc;
	int work;
	unsigned long flags;
	struct tty_struct *o_tty;

restart:
	/* This is a bit ugly for now but means we can break the 'ldisc
	   is part of the tty struct' assumption later */
	retval = tty_ldisc_get(ldisc, &new_ldisc);
	if (retval)
		return retval;

	/*
	 *	Problem: What do we do if this blocks ?
	 */

	tty_wait_until_sent(tty, 0);

	if (tty->ldisc.ops->num == ldisc) {
		tty_ldisc_put(new_ldisc.ops);
		return 0;
	}

	/*
	 *	No more input please, we are switching. The new ldisc
	 *	will update this value in the ldisc open function
	 */

	tty->receive_room = 0;

	o_ldisc = tty->ldisc;
	o_tty = tty->link;

	/*
	 *	Make sure we don't change while someone holds a
	 *	reference to the line discipline. The TTY_LDISC bit
	 *	prevents anyone taking a reference once it is clear.
	 *	We need the lock to avoid racing reference takers.
	 *
	 *	We must clear the TTY_LDISC bit here to avoid a livelock
	 *	with a userspace app continually trying to use the tty in
	 *	parallel to the change and re-referencing the tty.
	 */
	clear_bit(TTY_LDISC, &tty->flags);
	if (o_tty)
		clear_bit(TTY_LDISC, &o_tty->flags);

	spin_lock_irqsave(&tty_ldisc_lock, flags);
	if (tty->ldisc.refcount || (o_tty && o_tty->ldisc.refcount)) {
		if (tty->ldisc.refcount) {
			/* Free the new ldisc we grabbed. Must drop the lock
			   first. */
			spin_unlock_irqrestore(&tty_ldisc_lock, flags);
			tty_ldisc_put(o_ldisc.ops);
			/*
			 * There are several reasons we may be busy, including
			 * random momentary I/O traffic. We must therefore
			 * retry. We could distinguish between blocking ops
			 * and retries if we made tty_ldisc_wait() smarter.
			 * That is up for discussion.
			 */
			if (wait_event_interruptible(tty_ldisc_wait, tty->ldisc.refcount == 0) < 0)
				return -ERESTARTSYS;
			goto restart;
		}
		if (o_tty && o_tty->ldisc.refcount) {
			spin_unlock_irqrestore(&tty_ldisc_lock, flags);
			tty_ldisc_put(o_tty->ldisc.ops);
			if (wait_event_interruptible(tty_ldisc_wait, o_tty->ldisc.refcount == 0) < 0)
				return -ERESTARTSYS;
			goto restart;
		}
	}
	/*
	 *	If the TTY_LDISC bit is set, then we are racing against
	 *	another ldisc change
	 */
	if (test_bit(TTY_LDISC_CHANGING, &tty->flags)) {
		struct tty_ldisc *ld;
		spin_unlock_irqrestore(&tty_ldisc_lock, flags);
		tty_ldisc_put(new_ldisc.ops);
		ld = tty_ldisc_ref_wait(tty);
		tty_ldisc_deref(ld);
		goto restart;
	}
	/*
	 *	This flag is used to avoid two parallel ldisc changes. Once
	 *	open and close are fine grained locked this may work better
	 *	as a mutex shared with the open/close/hup paths
	 */
	set_bit(TTY_LDISC_CHANGING, &tty->flags);
	if (o_tty)
		set_bit(TTY_LDISC_CHANGING, &o_tty->flags);
	spin_unlock_irqrestore(&tty_ldisc_lock, flags);
	
	/*
	 *	From this point on we know nobody has an ldisc
	 *	usage reference, nor can they obtain one until
	 *	we say so later on.
	 */

	work = cancel_delayed_work(&tty->buf.work);
	/*
	 * Wait for ->hangup_work and ->buf.work handlers to terminate
	 * MUST NOT hold locks here.
	 */
	flush_scheduled_work();
	/* Shutdown the current discipline. */
	if (o_ldisc.ops->close)
		(o_ldisc.ops->close)(tty);

	/* Now set up the new line discipline. */
	tty_ldisc_assign(tty, &new_ldisc);
	tty_set_termios_ldisc(tty, ldisc);
	if (new_ldisc.ops->open)
		retval = (new_ldisc.ops->open)(tty);
	if (retval < 0) {
		tty_ldisc_put(new_ldisc.ops);
		tty_ldisc_restore(tty, &o_ldisc);
	}
	/* At this point we hold a reference to the new ldisc and a
	   a reference to the old ldisc. If we ended up flipping back
	   to the existing ldisc we have two references to it */

	if (tty->ldisc.ops->num != o_ldisc.ops->num && tty->ops->set_ldisc)
		tty->ops->set_ldisc(tty);

	tty_ldisc_put(o_ldisc.ops);

	/*
	 *	Allow ldisc referencing to occur as soon as the driver
	 *	ldisc callback completes.
	 */

	tty_ldisc_enable(tty);
	if (o_tty)
		tty_ldisc_enable(o_tty);

	/* Restart it in case no characters kick it off. Safe if
	   already running */
	if (work)
		schedule_delayed_work(&tty->buf.work, 1);
	return retval;
}



int tty_ldisc_setup(struct tty_struct *tty, struct tty_struct *o_tty)
{
	struct tty_ldisc *ld = &tty->ldisc;
	int retval;

	if (ld->ops->open) {
		retval = (ld->ops->open)(tty);
		if (retval)
			return retval;
	}
	if (o_tty && o_tty->ldisc.ops->open) {
		retval = (o_tty->ldisc.ops->open)(o_tty);
		if (retval) {
			if (ld->ops->close)
				(ld->ops->close)(tty);
			return retval;
		}
		tty_ldisc_enable(o_tty);
	}
	tty_ldisc_enable(tty);
	return 0;
}


void tty_ldisc_release(struct tty_struct *tty, struct tty_struct *o_tty)
{
	unsigned long flags;
	struct tty_ldisc ld;
	/*
	 * Prevent flush_to_ldisc() from rescheduling the work for later.  Then
	 * kill any delayed work. As this is the final close it does not
	 * race with the set_ldisc code path.
	 */
	clear_bit(TTY_LDISC, &tty->flags);
	cancel_delayed_work(&tty->buf.work);

	/*
	 * Wait for ->hangup_work and ->buf.work handlers to terminate
	 */

	flush_scheduled_work();

	/*
	 * Wait for any short term users (we know they are just driver
	 * side waiters as the file is closing so user count on the file
	 * side is zero.
	 */
	spin_lock_irqsave(&tty_ldisc_lock, flags);
	while (tty->ldisc.refcount) {
		spin_unlock_irqrestore(&tty_ldisc_lock, flags);
		wait_event(tty_ldisc_wait, tty->ldisc.refcount == 0);
		spin_lock_irqsave(&tty_ldisc_lock, flags);
	}
	spin_unlock_irqrestore(&tty_ldisc_lock, flags);
	/*
	 * Shutdown the current line discipline, and reset it to N_TTY.
	 *
	 * FIXME: this MUST get fixed for the new reflocking
	 */
	if (tty->ldisc.ops->close)
		(tty->ldisc.ops->close)(tty);
	tty_ldisc_put(tty->ldisc.ops);

	/*
	 *	Switch the line discipline back
	 */
	WARN_ON(tty_ldisc_get(N_TTY, &ld));
	tty_ldisc_assign(tty, &ld);
	tty_set_termios_ldisc(tty, N_TTY);
	if (o_tty) {
		/* FIXME: could o_tty be in setldisc here ? */
		clear_bit(TTY_LDISC, &o_tty->flags);
		if (o_tty->ldisc.ops->close)
			(o_tty->ldisc.ops->close)(o_tty);
		tty_ldisc_put(o_tty->ldisc.ops);
		WARN_ON(tty_ldisc_get(N_TTY, &ld));
		tty_ldisc_assign(o_tty, &ld);
		tty_set_termios_ldisc(o_tty, N_TTY);
	}
}


void tty_ldisc_init(struct tty_struct *tty)
{
	struct tty_ldisc ld;
	if (tty_ldisc_get(N_TTY, &ld) < 0)
		panic("n_tty: init_tty");
	tty_ldisc_assign(tty, &ld);
}

void tty_ldisc_begin(void)
{
	/* Setup the default TTY line discipline. */
	(void) tty_register_ldisc(N_TTY, &tty_ldisc_N_TTY);
}
