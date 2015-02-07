


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

#undef TTY_DEBUG_HANGUP

#define TTY_PARANOIA_CHECK 1
#define CHECK_TTY_COUNT 1

struct ktermios tty_std_termios = {	/* for the benefit of tty drivers  */
	.c_iflag = ICRNL | IXON,
	.c_oflag = OPOST | ONLCR,
	.c_cflag = B38400 | CS8 | CREAD | HUPCL,
	.c_lflag = ISIG | ICANON | ECHO | ECHOE | ECHOK |
		   ECHOCTL | ECHOKE | IEXTEN,
	.c_cc = INIT_C_CC,
	.c_ispeed = 38400,
	.c_ospeed = 38400
};

EXPORT_SYMBOL(tty_std_termios);


LIST_HEAD(tty_drivers);			/* linked list of tty drivers */

DEFINE_MUTEX(tty_mutex);
EXPORT_SYMBOL(tty_mutex);

static ssize_t tty_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t tty_write(struct file *, const char __user *, size_t, loff_t *);
ssize_t redirected_tty_write(struct file *, const char __user *,
							size_t, loff_t *);
static unsigned int tty_poll(struct file *, poll_table *);
static int tty_open(struct inode *, struct file *);
static int tty_release(struct inode *, struct file *);
long tty_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
#ifdef CONFIG_COMPAT
static long tty_compat_ioctl(struct file *file, unsigned int cmd,
				unsigned long arg);
#else
#define tty_compat_ioctl NULL
#endif
static int tty_fasync(int fd, struct file *filp, int on);
static void release_tty(struct tty_struct *tty, int idx);
static void __proc_set_tty(struct task_struct *tsk, struct tty_struct *tty);
static void proc_set_tty(struct task_struct *tsk, struct tty_struct *tty);


struct tty_struct *alloc_tty_struct(void)
{
	return kzalloc(sizeof(struct tty_struct), GFP_KERNEL);
}


void free_tty_struct(struct tty_struct *tty)
{
	kfree(tty->write_buf);
	tty_buffer_free_all(tty);
	kfree(tty);
}

#define TTY_NUMBER(tty) ((tty)->index + (tty)->driver->name_base)


char *tty_name(struct tty_struct *tty, char *buf)
{
	if (!tty) /* Hmm.  NULL pointer.  That's fun. */
		strcpy(buf, "NULL tty");
	else
		strcpy(buf, tty->name);
	return buf;
}

EXPORT_SYMBOL(tty_name);

int tty_paranoia_check(struct tty_struct *tty, struct inode *inode,
			      const char *routine)
{
#ifdef TTY_PARANOIA_CHECK
	if (!tty) {
		printk(KERN_WARNING
			"null TTY for (%d:%d) in %s\n",
			imajor(inode), iminor(inode), routine);
		return 1;
	}
	if (tty->magic != TTY_MAGIC) {
		printk(KERN_WARNING
			"bad magic number for tty struct (%d:%d) in %s\n",
			imajor(inode), iminor(inode), routine);
		return 1;
	}
#endif
	return 0;
}

static int check_tty_count(struct tty_struct *tty, const char *routine)
{
#ifdef CHECK_TTY_COUNT
	struct list_head *p;
	int count = 0;

	file_list_lock();
	list_for_each(p, &tty->tty_files) {
		count++;
	}
	file_list_unlock();
	if (tty->driver->type == TTY_DRIVER_TYPE_PTY &&
	    tty->driver->subtype == PTY_TYPE_SLAVE &&
	    tty->link && tty->link->count)
		count++;
	if (tty->count != count) {
		printk(KERN_WARNING "Warning: dev (%s) tty->count(%d) "
				    "!= #fd's(%d) in %s\n",
		       tty->name, tty->count, count, routine);
		return count;
	}
#endif
	return 0;
}


static struct tty_driver *get_tty_driver(dev_t device, int *index)
{
	struct tty_driver *p;

	list_for_each_entry(p, &tty_drivers, tty_drivers) {
		dev_t base = MKDEV(p->major, p->minor_start);
		if (device < base || device >= base + p->num)
			continue;
		*index = device - base;
		return tty_driver_kref_get(p);
	}
	return NULL;
}

#ifdef CONFIG_CONSOLE_POLL

struct tty_driver *tty_find_polling_driver(char *name, int *line)
{
	struct tty_driver *p, *res = NULL;
	int tty_line = 0;
	int len;
	char *str;

	for (str = name; *str; str++)
		if ((*str >= '0' && *str <= '9') || *str == ',')
			break;
	if (!*str)
		return NULL;

	len = str - name;
	tty_line = simple_strtoul(str, &str, 10);

	mutex_lock(&tty_mutex);
	/* Search through the tty devices to look for a match */
	list_for_each_entry(p, &tty_drivers, tty_drivers) {
		if (strncmp(name, p->name, len) != 0)
			continue;
		if (*str == ',')
			str++;
		if (*str == '\0')
			str = NULL;

		if (tty_line >= 0 && tty_line <= p->num && p->ops &&
		    p->ops->poll_init && !p->ops->poll_init(p, tty_line, str)) {
			res = tty_driver_kref_get(p);
			*line = tty_line;
			break;
		}
	}
	mutex_unlock(&tty_mutex);

	return res;
}
EXPORT_SYMBOL_GPL(tty_find_polling_driver);
#endif


int tty_check_change(struct tty_struct *tty)
{
	unsigned long flags;
	int ret = 0;

	if (current->signal->tty != tty)
		return 0;

	spin_lock_irqsave(&tty->ctrl_lock, flags);

	if (!tty->pgrp) {
		printk(KERN_WARNING "tty_check_change: tty->pgrp == NULL!\n");
		goto out_unlock;
	}
	if (task_pgrp(current) == tty->pgrp)
		goto out_unlock;
	spin_unlock_irqrestore(&tty->ctrl_lock, flags);
	if (is_ignored(SIGTTOU))
		goto out;
	if (is_current_pgrp_orphaned()) {
		ret = -EIO;
		goto out;
	}
	kill_pgrp(task_pgrp(current), SIGTTOU, 1);
	set_thread_flag(TIF_SIGPENDING);
	ret = -ERESTARTSYS;
out:
	return ret;
out_unlock:
	spin_unlock_irqrestore(&tty->ctrl_lock, flags);
	return ret;
}

EXPORT_SYMBOL(tty_check_change);

static ssize_t hung_up_tty_read(struct file *file, char __user *buf,
				size_t count, loff_t *ppos)
{
	return 0;
}

static ssize_t hung_up_tty_write(struct file *file, const char __user *buf,
				 size_t count, loff_t *ppos)
{
	return -EIO;
}

/* No kernel lock held - none needed ;) */
static unsigned int hung_up_tty_poll(struct file *filp, poll_table *wait)
{
	return POLLIN | POLLOUT | POLLERR | POLLHUP | POLLRDNORM | POLLWRNORM;
}

static long hung_up_tty_ioctl(struct file *file, unsigned int cmd,
		unsigned long arg)
{
	return cmd == TIOCSPGRP ? -ENOTTY : -EIO;
}

static long hung_up_tty_compat_ioctl(struct file *file,
				     unsigned int cmd, unsigned long arg)
{
	return cmd == TIOCSPGRP ? -ENOTTY : -EIO;
}

static const struct file_operations tty_fops = {
	.llseek		= no_llseek,
	.read		= tty_read,
	.write		= tty_write,
	.poll		= tty_poll,
	.unlocked_ioctl	= tty_ioctl,
	.compat_ioctl	= tty_compat_ioctl,
	.open		= tty_open,
	.release	= tty_release,
	.fasync		= tty_fasync,
};

static const struct file_operations console_fops = {
	.llseek		= no_llseek,
	.read		= tty_read,
	.write		= redirected_tty_write,
	.poll		= tty_poll,
	.unlocked_ioctl	= tty_ioctl,
	.compat_ioctl	= tty_compat_ioctl,
	.open		= tty_open,
	.release	= tty_release,
	.fasync		= tty_fasync,
};

static const struct file_operations hung_up_tty_fops = {
	.llseek		= no_llseek,
	.read		= hung_up_tty_read,
	.write		= hung_up_tty_write,
	.poll		= hung_up_tty_poll,
	.unlocked_ioctl	= hung_up_tty_ioctl,
	.compat_ioctl	= hung_up_tty_compat_ioctl,
	.release	= tty_release,
};

static DEFINE_SPINLOCK(redirect_lock);
static struct file *redirect;


void tty_wakeup(struct tty_struct *tty)
{
	struct tty_ldisc *ld;

	if (test_bit(TTY_DO_WRITE_WAKEUP, &tty->flags)) {
		ld = tty_ldisc_ref(tty);
		if (ld) {
			if (ld->ops->write_wakeup)
				ld->ops->write_wakeup(tty);
			tty_ldisc_deref(ld);
		}
	}
	wake_up_interruptible(&tty->write_wait);
}

EXPORT_SYMBOL_GPL(tty_wakeup);


void tty_ldisc_flush(struct tty_struct *tty)
{
	struct tty_ldisc *ld = tty_ldisc_ref(tty);
	if (ld) {
		if (ld->ops->flush_buffer)
			ld->ops->flush_buffer(tty);
		tty_ldisc_deref(ld);
	}
	tty_buffer_flush(tty);
}

EXPORT_SYMBOL_GPL(tty_ldisc_flush);


static void tty_reset_termios(struct tty_struct *tty)
{
	mutex_lock(&tty->termios_mutex);
	*tty->termios = tty->driver->init_termios;
	tty->termios->c_ispeed = tty_termios_input_baud_rate(tty->termios);
	tty->termios->c_ospeed = tty_termios_baud_rate(tty->termios);
	mutex_unlock(&tty->termios_mutex);
}

static void do_tty_hangup(struct work_struct *work)
{
	struct tty_struct *tty =
		container_of(work, struct tty_struct, hangup_work);
	struct file *cons_filp = NULL;
	struct file *filp, *f = NULL;
	struct task_struct *p;
	struct tty_ldisc *ld;
	int    closecount = 0, n;
	unsigned long flags;
	int refs = 0;

	if (!tty)
		return;

	/* inuse_filps is protected by the single kernel lock */
	lock_kernel();

	spin_lock(&redirect_lock);
	if (redirect && redirect->private_data == tty) {
		f = redirect;
		redirect = NULL;
	}
	spin_unlock(&redirect_lock);

	check_tty_count(tty, "do_tty_hangup");
	file_list_lock();
	/* This breaks for file handles being sent over AF_UNIX sockets ? */
	list_for_each_entry(filp, &tty->tty_files, f_u.fu_list) {
		if (filp->f_op->write == redirected_tty_write)
			cons_filp = filp;
		if (filp->f_op->write != tty_write)
			continue;
		closecount++;
		tty_fasync(-1, filp, 0);	/* can't block */
		filp->f_op = &hung_up_tty_fops;
	}
	file_list_unlock();
	/*
	 * FIXME! What are the locking issues here? This may me overdoing
	 * things... This question is especially important now that we've
	 * removed the irqlock.
	 */
	ld = tty_ldisc_ref(tty);
	if (ld != NULL) {
		/* We may have no line discipline at this point */
		if (ld->ops->flush_buffer)
			ld->ops->flush_buffer(tty);
		tty_driver_flush_buffer(tty);
		if ((test_bit(TTY_DO_WRITE_WAKEUP, &tty->flags)) &&
		    ld->ops->write_wakeup)
			ld->ops->write_wakeup(tty);
		if (ld->ops->hangup)
			ld->ops->hangup(tty);
	}
	/*
	 * FIXME: Once we trust the LDISC code better we can wait here for
	 * ldisc completion and fix the driver call race
	 */
	wake_up_interruptible(&tty->write_wait);
	wake_up_interruptible(&tty->read_wait);
	/*
	 * Shutdown the current line discipline, and reset it to
	 * N_TTY.
	 */
	if (tty->driver->flags & TTY_DRIVER_RESET_TERMIOS)
		tty_reset_termios(tty);
	/* Defer ldisc switch */
	/* tty_deferred_ldisc_switch(N_TTY);

	  This should get done automatically when the port closes and
	  tty_release is called */

	read_lock(&tasklist_lock);
	if (tty->session) {
		do_each_pid_task(tty->session, PIDTYPE_SID, p) {
			spin_lock_irq(&p->sighand->siglock);
			if (p->signal->tty == tty) {
				p->signal->tty = NULL;
				/* We defer the dereferences outside fo
				   the tasklist lock */
				refs++;
			}
			if (!p->signal->leader) {
				spin_unlock_irq(&p->sighand->siglock);
				continue;
			}
			__group_send_sig_info(SIGHUP, SEND_SIG_PRIV, p);
			__group_send_sig_info(SIGCONT, SEND_SIG_PRIV, p);
			put_pid(p->signal->tty_old_pgrp);  /* A noop */
			spin_lock_irqsave(&tty->ctrl_lock, flags);
			if (tty->pgrp)
				p->signal->tty_old_pgrp = get_pid(tty->pgrp);
			spin_unlock_irqrestore(&tty->ctrl_lock, flags);
			spin_unlock_irq(&p->sighand->siglock);
		} while_each_pid_task(tty->session, PIDTYPE_SID, p);
	}
	read_unlock(&tasklist_lock);

	spin_lock_irqsave(&tty->ctrl_lock, flags);
	tty->flags = 0;
	put_pid(tty->session);
	put_pid(tty->pgrp);
	tty->session = NULL;
	tty->pgrp = NULL;
	tty->ctrl_status = 0;
	spin_unlock_irqrestore(&tty->ctrl_lock, flags);

	/* Account for the p->signal references we killed */
	while (refs--)
		tty_kref_put(tty);

	/*
	 * If one of the devices matches a console pointer, we
	 * cannot just call hangup() because that will cause
	 * tty->count and state->count to go out of sync.
	 * So we just call close() the right number of times.
	 */
	if (cons_filp) {
		if (tty->ops->close)
			for (n = 0; n < closecount; n++)
				tty->ops->close(tty, cons_filp);
	} else if (tty->ops->hangup)
		(tty->ops->hangup)(tty);
	/*
	 * We don't want to have driver/ldisc interactions beyond
	 * the ones we did here. The driver layer expects no
	 * calls after ->hangup() from the ldisc side. However we
	 * can't yet guarantee all that.
	 */
	set_bit(TTY_HUPPED, &tty->flags);
	if (ld) {
		tty_ldisc_enable(tty);
		tty_ldisc_deref(ld);
	}
	unlock_kernel();
	if (f)
		fput(f);
}


void tty_hangup(struct tty_struct *tty)
{
#ifdef TTY_DEBUG_HANGUP
	char	buf[64];
	printk(KERN_DEBUG "%s hangup...\n", tty_name(tty, buf));
#endif
	schedule_work(&tty->hangup_work);
}

EXPORT_SYMBOL(tty_hangup);


void tty_vhangup(struct tty_struct *tty)
{
#ifdef TTY_DEBUG_HANGUP
	char	buf[64];

	printk(KERN_DEBUG "%s vhangup...\n", tty_name(tty, buf));
#endif
	do_tty_hangup(&tty->hangup_work);
}

EXPORT_SYMBOL(tty_vhangup);


void tty_vhangup_self(void)
{
	struct tty_struct *tty;

	tty = get_current_tty();
	if (tty) {
		tty_vhangup(tty);
		tty_kref_put(tty);
	}
}


int tty_hung_up_p(struct file *filp)
{
	return (filp->f_op == &hung_up_tty_fops);
}

EXPORT_SYMBOL(tty_hung_up_p);

static void session_clear_tty(struct pid *session)
{
	struct task_struct *p;
	do_each_pid_task(session, PIDTYPE_SID, p) {
		proc_clear_tty(p);
	} while_each_pid_task(session, PIDTYPE_SID, p);
}


void disassociate_ctty(int on_exit)
{
	struct tty_struct *tty;
	struct pid *tty_pgrp = NULL;


	tty = get_current_tty();
	if (tty) {
		tty_pgrp = get_pid(tty->pgrp);
		lock_kernel();
		if (on_exit && tty->driver->type != TTY_DRIVER_TYPE_PTY)
			tty_vhangup(tty);
		unlock_kernel();
		tty_kref_put(tty);
	} else if (on_exit) {
		struct pid *old_pgrp;
		spin_lock_irq(&current->sighand->siglock);
		old_pgrp = current->signal->tty_old_pgrp;
		current->signal->tty_old_pgrp = NULL;
		spin_unlock_irq(&current->sighand->siglock);
		if (old_pgrp) {
			kill_pgrp(old_pgrp, SIGHUP, on_exit);
			kill_pgrp(old_pgrp, SIGCONT, on_exit);
			put_pid(old_pgrp);
		}
		return;
	}
	if (tty_pgrp) {
		kill_pgrp(tty_pgrp, SIGHUP, on_exit);
		if (!on_exit)
			kill_pgrp(tty_pgrp, SIGCONT, on_exit);
		put_pid(tty_pgrp);
	}

	spin_lock_irq(&current->sighand->siglock);
	put_pid(current->signal->tty_old_pgrp);
	current->signal->tty_old_pgrp = NULL;
	spin_unlock_irq(&current->sighand->siglock);

	tty = get_current_tty();
	if (tty) {
		unsigned long flags;
		spin_lock_irqsave(&tty->ctrl_lock, flags);
		put_pid(tty->session);
		put_pid(tty->pgrp);
		tty->session = NULL;
		tty->pgrp = NULL;
		spin_unlock_irqrestore(&tty->ctrl_lock, flags);
		tty_kref_put(tty);
	} else {
#ifdef TTY_DEBUG_HANGUP
		printk(KERN_DEBUG "error attempted to write to tty [0x%p]"
		       " = NULL", tty);
#endif
	}

	/* Now clear signal->tty under the lock */
	read_lock(&tasklist_lock);
	session_clear_tty(task_session(current));
	read_unlock(&tasklist_lock);
}

void no_tty(void)
{
	struct task_struct *tsk = current;
	lock_kernel();
	if (tsk->signal->leader)
		disassociate_ctty(0);
	unlock_kernel();
	proc_clear_tty(tsk);
}



void stop_tty(struct tty_struct *tty)
{
	unsigned long flags;
	spin_lock_irqsave(&tty->ctrl_lock, flags);
	if (tty->stopped) {
		spin_unlock_irqrestore(&tty->ctrl_lock, flags);
		return;
	}
	tty->stopped = 1;
	if (tty->link && tty->link->packet) {
		tty->ctrl_status &= ~TIOCPKT_START;
		tty->ctrl_status |= TIOCPKT_STOP;
		wake_up_interruptible(&tty->link->read_wait);
	}
	spin_unlock_irqrestore(&tty->ctrl_lock, flags);
	if (tty->ops->stop)
		(tty->ops->stop)(tty);
}

EXPORT_SYMBOL(stop_tty);


void start_tty(struct tty_struct *tty)
{
	unsigned long flags;
	spin_lock_irqsave(&tty->ctrl_lock, flags);
	if (!tty->stopped || tty->flow_stopped) {
		spin_unlock_irqrestore(&tty->ctrl_lock, flags);
		return;
	}
	tty->stopped = 0;
	if (tty->link && tty->link->packet) {
		tty->ctrl_status &= ~TIOCPKT_STOP;
		tty->ctrl_status |= TIOCPKT_START;
		wake_up_interruptible(&tty->link->read_wait);
	}
	spin_unlock_irqrestore(&tty->ctrl_lock, flags);
	if (tty->ops->start)
		(tty->ops->start)(tty);
	/* If we have a running line discipline it may need kicking */
	tty_wakeup(tty);
}

EXPORT_SYMBOL(start_tty);


static ssize_t tty_read(struct file *file, char __user *buf, size_t count,
			loff_t *ppos)
{
	int i;
	struct tty_struct *tty;
	struct inode *inode;
	struct tty_ldisc *ld;

	tty = (struct tty_struct *)file->private_data;
	inode = file->f_path.dentry->d_inode;
	if (tty_paranoia_check(tty, inode, "tty_read"))
		return -EIO;
	if (!tty || (test_bit(TTY_IO_ERROR, &tty->flags)))
		return -EIO;

	/* We want to wait for the line discipline to sort out in this
	   situation */
	ld = tty_ldisc_ref_wait(tty);
	if (ld->ops->read)
		i = (ld->ops->read)(tty, file, buf, count);
	else
		i = -EIO;
	tty_ldisc_deref(ld);
	if (i > 0)
		inode->i_atime = current_fs_time(inode->i_sb);
	return i;
}

void tty_write_unlock(struct tty_struct *tty)
{
	mutex_unlock(&tty->atomic_write_lock);
	wake_up_interruptible(&tty->write_wait);
}

int tty_write_lock(struct tty_struct *tty, int ndelay)
{
	if (!mutex_trylock(&tty->atomic_write_lock)) {
		if (ndelay)
			return -EAGAIN;
		if (mutex_lock_interruptible(&tty->atomic_write_lock))
			return -ERESTARTSYS;
	}
	return 0;
}

static inline ssize_t do_tty_write(
	ssize_t (*write)(struct tty_struct *, struct file *, const unsigned char *, size_t),
	struct tty_struct *tty,
	struct file *file,
	const char __user *buf,
	size_t count)
{
	ssize_t ret, written = 0;
	unsigned int chunk;

	ret = tty_write_lock(tty, file->f_flags & O_NDELAY);
	if (ret < 0)
		return ret;

	/*
	 * We chunk up writes into a temporary buffer. This
	 * simplifies low-level drivers immensely, since they
	 * don't have locking issues and user mode accesses.
	 *
	 * But if TTY_NO_WRITE_SPLIT is set, we should use a
	 * big chunk-size..
	 *
	 * The default chunk-size is 2kB, because the NTTY
	 * layer has problems with bigger chunks. It will
	 * claim to be able to handle more characters than
	 * it actually does.
	 *
	 * FIXME: This can probably go away now except that 64K chunks
	 * are too likely to fail unless switched to vmalloc...
	 */
	chunk = 2048;
	if (test_bit(TTY_NO_WRITE_SPLIT, &tty->flags))
		chunk = 65536;
	if (count < chunk)
		chunk = count;

	/* write_buf/write_cnt is protected by the atomic_write_lock mutex */
	if (tty->write_cnt < chunk) {
		unsigned char *buf_chunk;

		if (chunk < 1024)
			chunk = 1024;

		buf_chunk = kmalloc(chunk, GFP_KERNEL);
		if (!buf_chunk) {
			ret = -ENOMEM;
			goto out;
		}
		kfree(tty->write_buf);
		tty->write_cnt = chunk;
		tty->write_buf = buf_chunk;
	}

	/* Do the write .. */
	for (;;) {
		size_t size = count;
		if (size > chunk)
			size = chunk;
		ret = -EFAULT;
		if (copy_from_user(tty->write_buf, buf, size))
			break;
		ret = write(tty, file, tty->write_buf, size);
		if (ret <= 0)
			break;
		written += ret;
		buf += ret;
		count -= ret;
		if (!count)
			break;
		ret = -ERESTARTSYS;
		if (signal_pending(current))
			break;
		cond_resched();
	}
	if (written) {
		struct inode *inode = file->f_path.dentry->d_inode;
		inode->i_mtime = current_fs_time(inode->i_sb);
		ret = written;
	}
out:
	tty_write_unlock(tty);
	return ret;
}


void tty_write_message(struct tty_struct *tty, char *msg)
{
	lock_kernel();
	if (tty) {
		mutex_lock(&tty->atomic_write_lock);
		if (tty->ops->write && !test_bit(TTY_CLOSING, &tty->flags))
			tty->ops->write(tty, msg, strlen(msg));
		tty_write_unlock(tty);
	}
	unlock_kernel();
	return;
}



static ssize_t tty_write(struct file *file, const char __user *buf,
						size_t count, loff_t *ppos)
{
	struct tty_struct *tty;
	struct inode *inode = file->f_path.dentry->d_inode;
	ssize_t ret;
	struct tty_ldisc *ld;

	tty = (struct tty_struct *)file->private_data;
	if (tty_paranoia_check(tty, inode, "tty_write"))
		return -EIO;
	if (!tty || !tty->ops->write ||
		(test_bit(TTY_IO_ERROR, &tty->flags)))
			return -EIO;
	/* Short term debug to catch buggy drivers */
	if (tty->ops->write_room == NULL)
		printk(KERN_ERR "tty driver %s lacks a write_room method.\n",
			tty->driver->name);
	ld = tty_ldisc_ref_wait(tty);
	if (!ld->ops->write)
		ret = -EIO;
	else
		ret = do_tty_write(ld->ops->write, tty, file, buf, count);
	tty_ldisc_deref(ld);
	return ret;
}

ssize_t redirected_tty_write(struct file *file, const char __user *buf,
						size_t count, loff_t *ppos)
{
	struct file *p = NULL;

	spin_lock(&redirect_lock);
	if (redirect) {
		get_file(redirect);
		p = redirect;
	}
	spin_unlock(&redirect_lock);

	if (p) {
		ssize_t res;
		res = vfs_write(p, buf, count, &p->f_pos);
		fput(p);
		return res;
	}
	return tty_write(file, buf, count, ppos);
}

static char ptychar[] = "pqrstuvwxyzabcde";

static void pty_line_name(struct tty_driver *driver, int index, char *p)
{
	int i = index + driver->name_base;
	/* ->name is initialized to "ttyp", but "tty" is expected */
	sprintf(p, "%s%c%x",
		driver->subtype == PTY_TYPE_SLAVE ? "tty" : driver->name,
		ptychar[i >> 4 & 0xf], i & 0xf);
}

static void tty_line_name(struct tty_driver *driver, int index, char *p)
{
	sprintf(p, "%s%d", driver->name, index + driver->name_base);
}

static struct tty_struct *tty_driver_lookup_tty(struct tty_driver *driver,
		struct inode *inode, int idx)
{
	struct tty_struct *tty;

	if (driver->ops->lookup)
		return driver->ops->lookup(driver, inode, idx);

	tty = driver->ttys[idx];
	return tty;
}


int tty_init_termios(struct tty_struct *tty)
{
	struct ktermios *tp;
	int idx = tty->index;

	tp = tty->driver->termios[idx];
	if (tp == NULL) {
		tp = kzalloc(sizeof(struct ktermios[2]), GFP_KERNEL);
		if (tp == NULL)
			return -ENOMEM;
		memcpy(tp, &tty->driver->init_termios,
						sizeof(struct ktermios));
		tty->driver->termios[idx] = tp;
	}
	tty->termios = tp;
	tty->termios_locked = tp + 1;

	/* Compatibility until drivers always set this */
	tty->termios->c_ispeed = tty_termios_input_baud_rate(tty->termios);
	tty->termios->c_ospeed = tty_termios_baud_rate(tty->termios);
	return 0;
}

static int tty_driver_install_tty(struct tty_driver *driver,
						struct tty_struct *tty)
{
	int idx = tty->index;

	if (driver->ops->install)
		return driver->ops->install(driver, tty);

	if (tty_init_termios(tty) == 0) {
		tty_driver_kref_get(driver);
		tty->count++;
		driver->ttys[idx] = tty;
		return 0;
	}
	return -ENOMEM;
}

static void tty_driver_remove_tty(struct tty_driver *driver,
						struct tty_struct *tty)
{
	if (driver->ops->remove)
		driver->ops->remove(driver, tty);
	else
		driver->ttys[tty->index] = NULL;
}

static int tty_reopen(struct tty_struct *tty)
{
	struct tty_driver *driver = tty->driver;

	if (test_bit(TTY_CLOSING, &tty->flags))
		return -EIO;

	if (driver->type == TTY_DRIVER_TYPE_PTY &&
	    driver->subtype == PTY_TYPE_MASTER) {
		/*
		 * special case for PTY masters: only one open permitted,
		 * and the slave side open count is incremented as well.
		 */
		if (tty->count)
			return -EIO;

		tty->link->count++;
	}
	tty->count++;
	tty->driver = driver; /* N.B. why do this every time?? */

	WARN_ON(!test_bit(TTY_LDISC, &tty->flags));

	return 0;
}


struct tty_struct *tty_init_dev(struct tty_driver *driver, int idx,
								int first_ok)
{
	struct tty_struct *tty;
	int retval;

	/* Check if pty master is being opened multiple times */
	if (driver->subtype == PTY_TYPE_MASTER &&
		(driver->flags & TTY_DRIVER_DEVPTS_MEM) && !first_ok)
		return ERR_PTR(-EIO);

	/*
	 * First time open is complex, especially for PTY devices.
	 * This code guarantees that either everything succeeds and the
	 * TTY is ready for operation, or else the table slots are vacated
	 * and the allocated memory released.  (Except that the termios
	 * and locked termios may be retained.)
	 */

	if (!try_module_get(driver->owner))
		return ERR_PTR(-ENODEV);

	tty = alloc_tty_struct();
	if (!tty)
		goto fail_no_mem;
	initialize_tty_struct(tty, driver, idx);

	retval = tty_driver_install_tty(driver, tty);
	if (retval < 0) {
		free_tty_struct(tty);
		module_put(driver->owner);
		return ERR_PTR(retval);
	}

	/*
	 * Structures all installed ... call the ldisc open routines.
	 * If we fail here just call release_tty to clean up.  No need
	 * to decrement the use counts, as release_tty doesn't care.
	 */

	retval = tty_ldisc_setup(tty, tty->link);
	if (retval)
		goto release_mem_out;
	return tty;

fail_no_mem:
	module_put(driver->owner);
	return ERR_PTR(-ENOMEM);

	/* call the tty release_tty routine to clean out this slot */
release_mem_out:
	if (printk_ratelimit())
		printk(KERN_INFO "tty_init_dev: ldisc open failed, "
				 "clearing slot %d\n", idx);
	release_tty(tty, idx);
	return ERR_PTR(retval);
}

void tty_free_termios(struct tty_struct *tty)
{
	struct ktermios *tp;
	int idx = tty->index;
	/* Kill this flag and push into drivers for locking etc */
	if (tty->driver->flags & TTY_DRIVER_RESET_TERMIOS) {
		/* FIXME: Locking on ->termios array */
		tp = tty->termios;
		tty->driver->termios[idx] = NULL;
		kfree(tp);
	}
}
EXPORT_SYMBOL(tty_free_termios);

void tty_shutdown(struct tty_struct *tty)
{
	tty_driver_remove_tty(tty->driver, tty);
	tty_free_termios(tty);
}
EXPORT_SYMBOL(tty_shutdown);

static void release_one_tty(struct kref *kref)
{
	struct tty_struct *tty = container_of(kref, struct tty_struct, kref);
	struct tty_driver *driver = tty->driver;

	if (tty->ops->shutdown)
		tty->ops->shutdown(tty);
	else
		tty_shutdown(tty);
	tty->magic = 0;
	tty_driver_kref_put(driver);
	module_put(driver->owner);

	file_list_lock();
	list_del_init(&tty->tty_files);
	file_list_unlock();

	free_tty_struct(tty);
}


void tty_kref_put(struct tty_struct *tty)
{
	if (tty)
		kref_put(&tty->kref, release_one_tty);
}
EXPORT_SYMBOL(tty_kref_put);

static void release_tty(struct tty_struct *tty, int idx)
{
	/* This should always be true but check for the moment */
	WARN_ON(tty->index != idx);

	if (tty->link)
		tty_kref_put(tty->link);
	tty_kref_put(tty);
}

void tty_release_dev(struct file *filp)
{
	struct tty_struct *tty, *o_tty;
	int	pty_master, tty_closing, o_tty_closing, do_sleep;
	int	devpts;
	int	idx;
	char	buf[64];
	struct 	inode *inode;

	inode = filp->f_path.dentry->d_inode;
	tty = (struct tty_struct *)filp->private_data;
	if (tty_paranoia_check(tty, inode, "tty_release_dev"))
		return;

	check_tty_count(tty, "tty_release_dev");

	tty_fasync(-1, filp, 0);

	idx = tty->index;
	pty_master = (tty->driver->type == TTY_DRIVER_TYPE_PTY &&
		      tty->driver->subtype == PTY_TYPE_MASTER);
	devpts = (tty->driver->flags & TTY_DRIVER_DEVPTS_MEM) != 0;
	o_tty = tty->link;

#ifdef TTY_PARANOIA_CHECK
	if (idx < 0 || idx >= tty->driver->num) {
		printk(KERN_DEBUG "tty_release_dev: bad idx when trying to "
				  "free (%s)\n", tty->name);
		return;
	}
	if (!devpts) {
		if (tty != tty->driver->ttys[idx]) {
			printk(KERN_DEBUG "tty_release_dev: driver.table[%d] not tty "
			       "for (%s)\n", idx, tty->name);
			return;
		}
		if (tty->termios != tty->driver->termios[idx]) {
			printk(KERN_DEBUG "tty_release_dev: driver.termios[%d] not termios "
			       "for (%s)\n",
			       idx, tty->name);
			return;
		}
	}
#endif

#ifdef TTY_DEBUG_HANGUP
	printk(KERN_DEBUG "tty_release_dev of %s (tty count=%d)...",
	       tty_name(tty, buf), tty->count);
#endif

#ifdef TTY_PARANOIA_CHECK
	if (tty->driver->other &&
	     !(tty->driver->flags & TTY_DRIVER_DEVPTS_MEM)) {
		if (o_tty != tty->driver->other->ttys[idx]) {
			printk(KERN_DEBUG "tty_release_dev: other->table[%d] "
					  "not o_tty for (%s)\n",
			       idx, tty->name);
			return;
		}
		if (o_tty->termios != tty->driver->other->termios[idx]) {
			printk(KERN_DEBUG "tty_release_dev: other->termios[%d] "
					  "not o_termios for (%s)\n",
			       idx, tty->name);
			return;
		}
		if (o_tty->link != tty) {
			printk(KERN_DEBUG "tty_release_dev: bad pty pointers\n");
			return;
		}
	}
#endif
	if (tty->ops->close)
		tty->ops->close(tty, filp);

	/*
	 * Sanity check: if tty->count is going to zero, there shouldn't be
	 * any waiters on tty->read_wait or tty->write_wait.  We test the
	 * wait queues and kick everyone out _before_ actually starting to
	 * close.  This ensures that we won't block while releasing the tty
	 * structure.
	 *
	 * The test for the o_tty closing is necessary, since the master and
	 * slave sides may close in any order.  If the slave side closes out
	 * first, its count will be one, since the master side holds an open.
	 * Thus this test wouldn't be triggered at the time the slave closes,
	 * so we do it now.
	 *
	 * Note that it's possible for the tty to be opened again while we're
	 * flushing out waiters.  By recalculating the closing flags before
	 * each iteration we avoid any problems.
	 */
	while (1) {
		/* Guard against races with tty->count changes elsewhere and
		   opens on /dev/tty */

		mutex_lock(&tty_mutex);
		tty_closing = tty->count <= 1;
		o_tty_closing = o_tty &&
			(o_tty->count <= (pty_master ? 1 : 0));
		do_sleep = 0;

		if (tty_closing) {
			if (waitqueue_active(&tty->read_wait)) {
				wake_up(&tty->read_wait);
				do_sleep++;
			}
			if (waitqueue_active(&tty->write_wait)) {
				wake_up(&tty->write_wait);
				do_sleep++;
			}
		}
		if (o_tty_closing) {
			if (waitqueue_active(&o_tty->read_wait)) {
				wake_up(&o_tty->read_wait);
				do_sleep++;
			}
			if (waitqueue_active(&o_tty->write_wait)) {
				wake_up(&o_tty->write_wait);
				do_sleep++;
			}
		}
		if (!do_sleep)
			break;

		printk(KERN_WARNING "tty_release_dev: %s: read/write wait queue "
				    "active!\n", tty_name(tty, buf));
		mutex_unlock(&tty_mutex);
		schedule();
	}

	/*
	 * The closing flags are now consistent with the open counts on
	 * both sides, and we've completed the last operation that could
	 * block, so it's safe to proceed with closing.
	 */
	if (pty_master) {
		if (--o_tty->count < 0) {
			printk(KERN_WARNING "tty_release_dev: bad pty slave count "
					    "(%d) for %s\n",
			       o_tty->count, tty_name(o_tty, buf));
			o_tty->count = 0;
		}
	}
	if (--tty->count < 0) {
		printk(KERN_WARNING "tty_release_dev: bad tty->count (%d) for %s\n",
		       tty->count, tty_name(tty, buf));
		tty->count = 0;
	}

	/*
	 * We've decremented tty->count, so we need to remove this file
	 * descriptor off the tty->tty_files list; this serves two
	 * purposes:
	 *  - check_tty_count sees the correct number of file descriptors
	 *    associated with this tty.
	 *  - do_tty_hangup no longer sees this file descriptor as
	 *    something that needs to be handled for hangups.
	 */
	file_kill(filp);
	filp->private_data = NULL;

	/*
	 * Perform some housekeeping before deciding whether to return.
	 *
	 * Set the TTY_CLOSING flag if this was the last open.  In the
	 * case of a pty we may have to wait around for the other side
	 * to close, and TTY_CLOSING makes sure we can't be reopened.
	 */
	if (tty_closing)
		set_bit(TTY_CLOSING, &tty->flags);
	if (o_tty_closing)
		set_bit(TTY_CLOSING, &o_tty->flags);

	/*
	 * If _either_ side is closing, make sure there aren't any
	 * processes that still think tty or o_tty is their controlling
	 * tty.
	 */
	if (tty_closing || o_tty_closing) {
		read_lock(&tasklist_lock);
		session_clear_tty(tty->session);
		if (o_tty)
			session_clear_tty(o_tty->session);
		read_unlock(&tasklist_lock);
	}

	mutex_unlock(&tty_mutex);

	/* check whether both sides are closing ... */
	if (!tty_closing || (o_tty && !o_tty_closing))
		return;

#ifdef TTY_DEBUG_HANGUP
	printk(KERN_DEBUG "freeing tty structure...");
#endif
	/*
	 * Ask the line discipline code to release its structures
	 */
	tty_ldisc_release(tty, o_tty);
	/*
	 * The release_tty function takes care of the details of clearing
	 * the slots and preserving the termios structure.
	 */
	release_tty(tty, idx);

	/* Make this pty number available for reallocation */
	if (devpts)
		devpts_kill_index(inode, idx);
}


static int __tty_open(struct inode *inode, struct file *filp)
{
	struct tty_struct *tty = NULL;
	int noctty, retval;
	struct tty_driver *driver;
	int index;
	dev_t device = inode->i_rdev;
	unsigned short saved_flags = filp->f_flags;

	nonseekable_open(inode, filp);

retry_open:
	noctty = filp->f_flags & O_NOCTTY;
	index  = -1;
	retval = 0;

	mutex_lock(&tty_mutex);

	if (device == MKDEV(TTYAUX_MAJOR, 0)) {
		tty = get_current_tty();
		if (!tty) {
			mutex_unlock(&tty_mutex);
			return -ENXIO;
		}
		driver = tty_driver_kref_get(tty->driver);
		index = tty->index;
		filp->f_flags |= O_NONBLOCK; /* Don't let /dev/tty block */
		/* noctty = 1; */
		/* FIXME: Should we take a driver reference ? */
		tty_kref_put(tty);
		goto got_driver;
	}
#ifdef CONFIG_VT
	if (device == MKDEV(TTY_MAJOR, 0)) {
		extern struct tty_driver *console_driver;
		driver = tty_driver_kref_get(console_driver);
		index = fg_console;
		noctty = 1;
		goto got_driver;
	}
#endif
	if (device == MKDEV(TTYAUX_MAJOR, 1)) {
		struct tty_driver *console_driver = console_device(&index);
		if (console_driver) {
			driver = tty_driver_kref_get(console_driver);
			if (driver) {
				/* Don't let /dev/console block */
				filp->f_flags |= O_NONBLOCK;
				noctty = 1;
				goto got_driver;
			}
		}
		mutex_unlock(&tty_mutex);
		return -ENODEV;
	}

	driver = get_tty_driver(device, &index);
	if (!driver) {
		mutex_unlock(&tty_mutex);
		return -ENODEV;
	}
got_driver:
	if (!tty) {
		/* check whether we're reopening an existing tty */
		tty = tty_driver_lookup_tty(driver, inode, index);

		if (IS_ERR(tty)) {
			mutex_unlock(&tty_mutex);
			return PTR_ERR(tty);
		}
	}

	if (tty) {
		retval = tty_reopen(tty);
		if (retval)
			tty = ERR_PTR(retval);
	} else
		tty = tty_init_dev(driver, index, 0);

	mutex_unlock(&tty_mutex);
	tty_driver_kref_put(driver);
	if (IS_ERR(tty))
		return PTR_ERR(tty);

	filp->private_data = tty;
	file_move(filp, &tty->tty_files);
	check_tty_count(tty, "tty_open");
	if (tty->driver->type == TTY_DRIVER_TYPE_PTY &&
	    tty->driver->subtype == PTY_TYPE_MASTER)
		noctty = 1;
#ifdef TTY_DEBUG_HANGUP
	printk(KERN_DEBUG "opening %s...", tty->name);
#endif
	if (!retval) {
		if (tty->ops->open)
			retval = tty->ops->open(tty, filp);
		else
			retval = -ENODEV;
	}
	filp->f_flags = saved_flags;

	if (!retval && test_bit(TTY_EXCLUSIVE, &tty->flags) &&
						!capable(CAP_SYS_ADMIN))
		retval = -EBUSY;

	if (retval) {
#ifdef TTY_DEBUG_HANGUP
		printk(KERN_DEBUG "error %d in opening %s...", retval,
		       tty->name);
#endif
		tty_release_dev(filp);
		if (retval != -ERESTARTSYS)
			return retval;
		if (signal_pending(current))
			return retval;
		schedule();
		/*
		 * Need to reset f_op in case a hangup happened.
		 */
		if (filp->f_op == &hung_up_tty_fops)
			filp->f_op = &tty_fops;
		goto retry_open;
	}

	mutex_lock(&tty_mutex);
	spin_lock_irq(&current->sighand->siglock);
	if (!noctty &&
	    current->signal->leader &&
	    !current->signal->tty &&
	    tty->session == NULL)
		__proc_set_tty(current, tty);
	spin_unlock_irq(&current->sighand->siglock);
	mutex_unlock(&tty_mutex);
	return 0;
}

/* BKL pushdown: scary code avoidance wrapper */
static int tty_open(struct inode *inode, struct file *filp)
{
	int ret;

	lock_kernel();
	ret = __tty_open(inode, filp);
	unlock_kernel();
	return ret;
}





static int tty_release(struct inode *inode, struct file *filp)
{
	lock_kernel();
	tty_release_dev(filp);
	unlock_kernel();
	return 0;
}


static unsigned int tty_poll(struct file *filp, poll_table *wait)
{
	struct tty_struct *tty;
	struct tty_ldisc *ld;
	int ret = 0;

	tty = (struct tty_struct *)filp->private_data;
	if (tty_paranoia_check(tty, filp->f_path.dentry->d_inode, "tty_poll"))
		return 0;

	ld = tty_ldisc_ref_wait(tty);
	if (ld->ops->poll)
		ret = (ld->ops->poll)(tty, filp, wait);
	tty_ldisc_deref(ld);
	return ret;
}

static int tty_fasync(int fd, struct file *filp, int on)
{
	struct tty_struct *tty;
	unsigned long flags;
	int retval = 0;

	lock_kernel();
	tty = (struct tty_struct *)filp->private_data;
	if (tty_paranoia_check(tty, filp->f_path.dentry->d_inode, "tty_fasync"))
		goto out;

	retval = fasync_helper(fd, filp, on, &tty->fasync);
	if (retval <= 0)
		goto out;

	if (on) {
		enum pid_type type;
		struct pid *pid;
		if (!waitqueue_active(&tty->read_wait))
			tty->minimum_to_wake = 1;
		spin_lock_irqsave(&tty->ctrl_lock, flags);
		if (tty->pgrp) {
			pid = tty->pgrp;
			type = PIDTYPE_PGID;
		} else {
			pid = task_pid(current);
			type = PIDTYPE_PID;
		}
		spin_unlock_irqrestore(&tty->ctrl_lock, flags);
		retval = __f_setown(filp, pid, type, 0);
		if (retval)
			goto out;
	} else {
		if (!tty->fasync && !waitqueue_active(&tty->read_wait))
			tty->minimum_to_wake = N_TTY_BUF_SIZE;
	}
	retval = 0;
out:
	unlock_kernel();
	return retval;
}


static int tiocsti(struct tty_struct *tty, char __user *p)
{
	char ch, mbz = 0;
	struct tty_ldisc *ld;

	if ((current->signal->tty != tty) && !capable(CAP_SYS_ADMIN))
		return -EPERM;
	if (get_user(ch, p))
		return -EFAULT;
	tty_audit_tiocsti(tty, ch);
	ld = tty_ldisc_ref_wait(tty);
	ld->ops->receive_buf(tty, &ch, &mbz, 1);
	tty_ldisc_deref(ld);
	return 0;
}


static int tiocgwinsz(struct tty_struct *tty, struct winsize __user *arg)
{
	int err;

	mutex_lock(&tty->termios_mutex);
	err = copy_to_user(arg, &tty->winsize, sizeof(*arg));
	mutex_unlock(&tty->termios_mutex);

	return err ? -EFAULT: 0;
}


int tty_do_resize(struct tty_struct *tty, struct winsize *ws)
{
	struct pid *pgrp;
	unsigned long flags;

	/* Lock the tty */
	mutex_lock(&tty->termios_mutex);
	if (!memcmp(ws, &tty->winsize, sizeof(*ws)))
		goto done;
	/* Get the PID values and reference them so we can
	   avoid holding the tty ctrl lock while sending signals */
	spin_lock_irqsave(&tty->ctrl_lock, flags);
	pgrp = get_pid(tty->pgrp);
	spin_unlock_irqrestore(&tty->ctrl_lock, flags);

	if (pgrp)
		kill_pgrp(pgrp, SIGWINCH, 1);
	put_pid(pgrp);

	tty->winsize = *ws;
done:
	mutex_unlock(&tty->termios_mutex);
	return 0;
}


static int tiocswinsz(struct tty_struct *tty, struct winsize __user *arg)
{
	struct winsize tmp_ws;
	if (copy_from_user(&tmp_ws, arg, sizeof(*arg)))
		return -EFAULT;

	if (tty->ops->resize)
		return tty->ops->resize(tty, &tmp_ws);
	else
		return tty_do_resize(tty, &tmp_ws);
}


static int tioccons(struct file *file)
{
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	if (file->f_op->write == redirected_tty_write) {
		struct file *f;
		spin_lock(&redirect_lock);
		f = redirect;
		redirect = NULL;
		spin_unlock(&redirect_lock);
		if (f)
			fput(f);
		return 0;
	}
	spin_lock(&redirect_lock);
	if (redirect) {
		spin_unlock(&redirect_lock);
		return -EBUSY;
	}
	get_file(file);
	redirect = file;
	spin_unlock(&redirect_lock);
	return 0;
}


static int fionbio(struct file *file, int __user *p)
{
	int nonblock;

	if (get_user(nonblock, p))
		return -EFAULT;

	/* file->f_flags is still BKL protected in the fs layer - vomit */
	lock_kernel();
	if (nonblock)
		file->f_flags |= O_NONBLOCK;
	else
		file->f_flags &= ~O_NONBLOCK;
	unlock_kernel();
	return 0;
}


static int tiocsctty(struct tty_struct *tty, int arg)
{
	int ret = 0;
	if (current->signal->leader && (task_session(current) == tty->session))
		return ret;

	mutex_lock(&tty_mutex);
	/*
	 * The process must be a session leader and
	 * not have a controlling tty already.
	 */
	if (!current->signal->leader || current->signal->tty) {
		ret = -EPERM;
		goto unlock;
	}

	if (tty->session) {
		/*
		 * This tty is already the controlling
		 * tty for another session group!
		 */
		if (arg == 1 && capable(CAP_SYS_ADMIN)) {
			/*
			 * Steal it away
			 */
			read_lock(&tasklist_lock);
			session_clear_tty(tty->session);
			read_unlock(&tasklist_lock);
		} else {
			ret = -EPERM;
			goto unlock;
		}
	}
	proc_set_tty(current, tty);
unlock:
	mutex_unlock(&tty_mutex);
	return ret;
}


struct pid *tty_get_pgrp(struct tty_struct *tty)
{
	unsigned long flags;
	struct pid *pgrp;

	spin_lock_irqsave(&tty->ctrl_lock, flags);
	pgrp = get_pid(tty->pgrp);
	spin_unlock_irqrestore(&tty->ctrl_lock, flags);

	return pgrp;
}
EXPORT_SYMBOL_GPL(tty_get_pgrp);


static int tiocgpgrp(struct tty_struct *tty, struct tty_struct *real_tty, pid_t __user *p)
{
	struct pid *pid;
	int ret;
	/*
	 * (tty == real_tty) is a cheap way of
	 * testing if the tty is NOT a master pty.
	 */
	if (tty == real_tty && current->signal->tty != real_tty)
		return -ENOTTY;
	pid = tty_get_pgrp(real_tty);
	ret =  put_user(pid_vnr(pid), p);
	put_pid(pid);
	return ret;
}


static int tiocspgrp(struct tty_struct *tty, struct tty_struct *real_tty, pid_t __user *p)
{
	struct pid *pgrp;
	pid_t pgrp_nr;
	int retval = tty_check_change(real_tty);
	unsigned long flags;

	if (retval == -EIO)
		return -ENOTTY;
	if (retval)
		return retval;
	if (!current->signal->tty ||
	    (current->signal->tty != real_tty) ||
	    (real_tty->session != task_session(current)))
		return -ENOTTY;
	if (get_user(pgrp_nr, p))
		return -EFAULT;
	if (pgrp_nr < 0)
		return -EINVAL;
	rcu_read_lock();
	pgrp = find_vpid(pgrp_nr);
	retval = -ESRCH;
	if (!pgrp)
		goto out_unlock;
	retval = -EPERM;
	if (session_of_pgrp(pgrp) != task_session(current))
		goto out_unlock;
	retval = 0;
	spin_lock_irqsave(&tty->ctrl_lock, flags);
	put_pid(real_tty->pgrp);
	real_tty->pgrp = get_pid(pgrp);
	spin_unlock_irqrestore(&tty->ctrl_lock, flags);
out_unlock:
	rcu_read_unlock();
	return retval;
}


static int tiocgsid(struct tty_struct *tty, struct tty_struct *real_tty, pid_t __user *p)
{
	/*
	 * (tty == real_tty) is a cheap way of
	 * testing if the tty is NOT a master pty.
	*/
	if (tty == real_tty && current->signal->tty != real_tty)
		return -ENOTTY;
	if (!real_tty->session)
		return -ENOTTY;
	return put_user(pid_vnr(real_tty->session), p);
}


static int tiocsetd(struct tty_struct *tty, int __user *p)
{
	int ldisc;
	int ret;

	if (get_user(ldisc, p))
		return -EFAULT;

	lock_kernel();
	ret = tty_set_ldisc(tty, ldisc);
	unlock_kernel();

	return ret;
}


static int send_break(struct tty_struct *tty, unsigned int duration)
{
	int retval;

	if (tty->ops->break_ctl == NULL)
		return 0;

	if (tty->driver->flags & TTY_DRIVER_HARDWARE_BREAK)
		retval = tty->ops->break_ctl(tty, duration);
	else {
		/* Do the work ourselves */
		if (tty_write_lock(tty, 0) < 0)
			return -EINTR;
		retval = tty->ops->break_ctl(tty, -1);
		if (retval)
			goto out;
		if (!signal_pending(current))
			msleep_interruptible(duration);
		retval = tty->ops->break_ctl(tty, 0);
out:
		tty_write_unlock(tty);
		if (signal_pending(current))
			retval = -EINTR;
	}
	return retval;
}


static int tty_tiocmget(struct tty_struct *tty, struct file *file, int __user *p)
{
	int retval = -EINVAL;

	if (tty->ops->tiocmget) {
		retval = tty->ops->tiocmget(tty, file);

		if (retval >= 0)
			retval = put_user(retval, p);
	}
	return retval;
}


static int tty_tiocmset(struct tty_struct *tty, struct file *file, unsigned int cmd,
	     unsigned __user *p)
{
	int retval;
	unsigned int set, clear, val;

	if (tty->ops->tiocmset == NULL)
		return -EINVAL;

	retval = get_user(val, p);
	if (retval)
		return retval;
	set = clear = 0;
	switch (cmd) {
	case TIOCMBIS:
		set = val;
		break;
	case TIOCMBIC:
		clear = val;
		break;
	case TIOCMSET:
		set = val;
		clear = ~val;
		break;
	}
	set &= TIOCM_DTR|TIOCM_RTS|TIOCM_OUT1|TIOCM_OUT2|TIOCM_LOOP;
	clear &= TIOCM_DTR|TIOCM_RTS|TIOCM_OUT1|TIOCM_OUT2|TIOCM_LOOP;
	return tty->ops->tiocmset(tty, file, set, clear);
}

long tty_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct tty_struct *tty, *real_tty;
	void __user *p = (void __user *)arg;
	int retval;
	struct tty_ldisc *ld;
	struct inode *inode = file->f_dentry->d_inode;

	tty = (struct tty_struct *)file->private_data;
	if (tty_paranoia_check(tty, inode, "tty_ioctl"))
		return -EINVAL;

	real_tty = tty;
	if (tty->driver->type == TTY_DRIVER_TYPE_PTY &&
	    tty->driver->subtype == PTY_TYPE_MASTER)
		real_tty = tty->link;


	/*
	 * Factor out some common prep work
	 */
	switch (cmd) {
	case TIOCSETD:
	case TIOCSBRK:
	case TIOCCBRK:
	case TCSBRK:
	case TCSBRKP:
		retval = tty_check_change(tty);
		if (retval)
			return retval;
		if (cmd != TIOCCBRK) {
			tty_wait_until_sent(tty, 0);
			if (signal_pending(current))
				return -EINTR;
		}
		break;
	}

	/*
	 *	Now do the stuff.
	 */
	switch (cmd) {
	case TIOCSTI:
		return tiocsti(tty, p);
	case TIOCGWINSZ:
		return tiocgwinsz(real_tty, p);
	case TIOCSWINSZ:
		return tiocswinsz(real_tty, p);
	case TIOCCONS:
		return real_tty != tty ? -EINVAL : tioccons(file);
	case FIONBIO:
		return fionbio(file, p);
	case TIOCEXCL:
		set_bit(TTY_EXCLUSIVE, &tty->flags);
		return 0;
	case TIOCNXCL:
		clear_bit(TTY_EXCLUSIVE, &tty->flags);
		return 0;
	case TIOCNOTTY:
		if (current->signal->tty != tty)
			return -ENOTTY;
		no_tty();
		return 0;
	case TIOCSCTTY:
		return tiocsctty(tty, arg);
	case TIOCGPGRP:
		return tiocgpgrp(tty, real_tty, p);
	case TIOCSPGRP:
		return tiocspgrp(tty, real_tty, p);
	case TIOCGSID:
		return tiocgsid(tty, real_tty, p);
	case TIOCGETD:
		return put_user(tty->ldisc.ops->num, (int __user *)p);
	case TIOCSETD:
		return tiocsetd(tty, p);
	/*
	 * Break handling
	 */
	case TIOCSBRK:	/* Turn break on, unconditionally */
		if (tty->ops->break_ctl)
			return tty->ops->break_ctl(tty, -1);
		return 0;
	case TIOCCBRK:	/* Turn break off, unconditionally */
		if (tty->ops->break_ctl)
			return tty->ops->break_ctl(tty, 0);
		return 0;
	case TCSBRK:   /* SVID version: non-zero arg --> no break */
		/* non-zero arg means wait for all output data
		 * to be sent (performed above) but don't send break.
		 * This is used by the tcdrain() termios function.
		 */
		if (!arg)
			return send_break(tty, 250);
		return 0;
	case TCSBRKP:	/* support for POSIX tcsendbreak() */
		return send_break(tty, arg ? arg*100 : 250);

	case TIOCMGET:
		return tty_tiocmget(tty, file, p);
	case TIOCMSET:
	case TIOCMBIC:
	case TIOCMBIS:
		return tty_tiocmset(tty, file, cmd, p);
	case TCFLSH:
		switch (arg) {
		case TCIFLUSH:
		case TCIOFLUSH:
		/* flush tty buffer and allow ldisc to process ioctl */
			tty_buffer_flush(tty);
			break;
		}
		break;
	}
	if (tty->ops->ioctl) {
		retval = (tty->ops->ioctl)(tty, file, cmd, arg);
		if (retval != -ENOIOCTLCMD)
			return retval;
	}
	ld = tty_ldisc_ref_wait(tty);
	retval = -EINVAL;
	if (ld->ops->ioctl) {
		retval = ld->ops->ioctl(tty, file, cmd, arg);
		if (retval == -ENOIOCTLCMD)
			retval = -EINVAL;
	}
	tty_ldisc_deref(ld);
	return retval;
}

#ifdef CONFIG_COMPAT
static long tty_compat_ioctl(struct file *file, unsigned int cmd,
				unsigned long arg)
{
	struct inode *inode = file->f_dentry->d_inode;
	struct tty_struct *tty = file->private_data;
	struct tty_ldisc *ld;
	int retval = -ENOIOCTLCMD;

	if (tty_paranoia_check(tty, inode, "tty_ioctl"))
		return -EINVAL;

	if (tty->ops->compat_ioctl) {
		retval = (tty->ops->compat_ioctl)(tty, file, cmd, arg);
		if (retval != -ENOIOCTLCMD)
			return retval;
	}

	ld = tty_ldisc_ref_wait(tty);
	if (ld->ops->compat_ioctl)
		retval = ld->ops->compat_ioctl(tty, file, cmd, arg);
	tty_ldisc_deref(ld);

	return retval;
}
#endif

void __do_SAK(struct tty_struct *tty)
{
#ifdef TTY_SOFT_SAK
	tty_hangup(tty);
#else
	struct task_struct *g, *p;
	struct pid *session;
	int		i;
	struct file	*filp;
	struct fdtable *fdt;

	if (!tty)
		return;
	session = tty->session;

	tty_ldisc_flush(tty);

	tty_driver_flush_buffer(tty);

	read_lock(&tasklist_lock);
	/* Kill the entire session */
	do_each_pid_task(session, PIDTYPE_SID, p) {
		printk(KERN_NOTICE "SAK: killed process %d"
			" (%s): task_session_nr(p)==tty->session\n",
			task_pid_nr(p), p->comm);
		send_sig(SIGKILL, p, 1);
	} while_each_pid_task(session, PIDTYPE_SID, p);
	/* Now kill any processes that happen to have the
	 * tty open.
	 */
	do_each_thread(g, p) {
		if (p->signal->tty == tty) {
			printk(KERN_NOTICE "SAK: killed process %d"
			    " (%s): task_session_nr(p)==tty->session\n",
			    task_pid_nr(p), p->comm);
			send_sig(SIGKILL, p, 1);
			continue;
		}
		task_lock(p);
		if (p->files) {
			/*
			 * We don't take a ref to the file, so we must
			 * hold ->file_lock instead.
			 */
			spin_lock(&p->files->file_lock);
			fdt = files_fdtable(p->files);
			for (i = 0; i < fdt->max_fds; i++) {
				filp = fcheck_files(p->files, i);
				if (!filp)
					continue;
				if (filp->f_op->read == tty_read &&
				    filp->private_data == tty) {
					printk(KERN_NOTICE "SAK: killed process %d"
					    " (%s): fd#%d opened to the tty\n",
					    task_pid_nr(p), p->comm, i);
					force_sig(SIGKILL, p);
					break;
				}
			}
			spin_unlock(&p->files->file_lock);
		}
		task_unlock(p);
	} while_each_thread(g, p);
	read_unlock(&tasklist_lock);
#endif
}

static void do_SAK_work(struct work_struct *work)
{
	struct tty_struct *tty =
		container_of(work, struct tty_struct, SAK_work);
	__do_SAK(tty);
}

void do_SAK(struct tty_struct *tty)
{
	if (!tty)
		return;
	schedule_work(&tty->SAK_work);
}

EXPORT_SYMBOL(do_SAK);


void initialize_tty_struct(struct tty_struct *tty,
		struct tty_driver *driver, int idx)
{
	memset(tty, 0, sizeof(struct tty_struct));
	kref_init(&tty->kref);
	tty->magic = TTY_MAGIC;
	tty_ldisc_init(tty);
	tty->session = NULL;
	tty->pgrp = NULL;
	tty->overrun_time = jiffies;
	tty->buf.head = tty->buf.tail = NULL;
	tty_buffer_init(tty);
	mutex_init(&tty->termios_mutex);
	init_waitqueue_head(&tty->write_wait);
	init_waitqueue_head(&tty->read_wait);
	INIT_WORK(&tty->hangup_work, do_tty_hangup);
	mutex_init(&tty->atomic_read_lock);
	mutex_init(&tty->atomic_write_lock);
	mutex_init(&tty->output_lock);
	mutex_init(&tty->echo_lock);
	spin_lock_init(&tty->read_lock);
	spin_lock_init(&tty->ctrl_lock);
	INIT_LIST_HEAD(&tty->tty_files);
	INIT_WORK(&tty->SAK_work, do_SAK_work);

	tty->driver = driver;
	tty->ops = driver->ops;
	tty->index = idx;
	tty_line_name(driver, idx, tty->name);
}


int tty_put_char(struct tty_struct *tty, unsigned char ch)
{
	if (tty->ops->put_char)
		return tty->ops->put_char(tty, ch);
	return tty->ops->write(tty, &ch, 1);
}
EXPORT_SYMBOL_GPL(tty_put_char);

struct class *tty_class;


struct device *tty_register_device(struct tty_driver *driver, unsigned index,
				   struct device *device)
{
	char name[64];
	dev_t dev = MKDEV(driver->major, driver->minor_start) + index;

	if (index >= driver->num) {
		printk(KERN_ERR "Attempt to register invalid tty line number "
		       " (%d).\n", index);
		return ERR_PTR(-EINVAL);
	}

	if (driver->type == TTY_DRIVER_TYPE_PTY)
		pty_line_name(driver, index, name);
	else
		tty_line_name(driver, index, name);

	return device_create(tty_class, device, dev, NULL, name);
}
EXPORT_SYMBOL(tty_register_device);


void tty_unregister_device(struct tty_driver *driver, unsigned index)
{
	device_destroy(tty_class,
		MKDEV(driver->major, driver->minor_start) + index);
}
EXPORT_SYMBOL(tty_unregister_device);

struct tty_driver *alloc_tty_driver(int lines)
{
	struct tty_driver *driver;

	driver = kzalloc(sizeof(struct tty_driver), GFP_KERNEL);
	if (driver) {
		kref_init(&driver->kref);
		driver->magic = TTY_DRIVER_MAGIC;
		driver->num = lines;
		/* later we'll move allocation of tables here */
	}
	return driver;
}
EXPORT_SYMBOL(alloc_tty_driver);

static void destruct_tty_driver(struct kref *kref)
{
	struct tty_driver *driver = container_of(kref, struct tty_driver, kref);
	int i;
	struct ktermios *tp;
	void *p;

	if (driver->flags & TTY_DRIVER_INSTALLED) {
		/*
		 * Free the termios and termios_locked structures because
		 * we don't want to get memory leaks when modular tty
		 * drivers are removed from the kernel.
		 */
		for (i = 0; i < driver->num; i++) {
			tp = driver->termios[i];
			if (tp) {
				driver->termios[i] = NULL;
				kfree(tp);
			}
			if (!(driver->flags & TTY_DRIVER_DYNAMIC_DEV))
				tty_unregister_device(driver, i);
		}
		p = driver->ttys;
		proc_tty_unregister_driver(driver);
		driver->ttys = NULL;
		driver->termios = NULL;
		kfree(p);
		cdev_del(&driver->cdev);
	}
	kfree(driver);
}

void tty_driver_kref_put(struct tty_driver *driver)
{
	kref_put(&driver->kref, destruct_tty_driver);
}
EXPORT_SYMBOL(tty_driver_kref_put);

void tty_set_operations(struct tty_driver *driver,
			const struct tty_operations *op)
{
	driver->ops = op;
};
EXPORT_SYMBOL(tty_set_operations);

void put_tty_driver(struct tty_driver *d)
{
	tty_driver_kref_put(d);
}
EXPORT_SYMBOL(put_tty_driver);

int tty_register_driver(struct tty_driver *driver)
{
	int error;
	int i;
	dev_t dev;
	void **p = NULL;

	if (!(driver->flags & TTY_DRIVER_DEVPTS_MEM) && driver->num) {
		p = kzalloc(driver->num * 2 * sizeof(void *), GFP_KERNEL);
		if (!p)
			return -ENOMEM;
	}

	if (!driver->major) {
		error = alloc_chrdev_region(&dev, driver->minor_start,
						driver->num, driver->name);
		if (!error) {
			driver->major = MAJOR(dev);
			driver->minor_start = MINOR(dev);
		}
	} else {
		dev = MKDEV(driver->major, driver->minor_start);
		error = register_chrdev_region(dev, driver->num, driver->name);
	}
	if (error < 0) {
		kfree(p);
		return error;
	}

	if (p) {
		driver->ttys = (struct tty_struct **)p;
		driver->termios = (struct ktermios **)(p + driver->num);
	} else {
		driver->ttys = NULL;
		driver->termios = NULL;
	}

	cdev_init(&driver->cdev, &tty_fops);
	driver->cdev.owner = driver->owner;
	error = cdev_add(&driver->cdev, dev, driver->num);
	if (error) {
		unregister_chrdev_region(dev, driver->num);
		driver->ttys = NULL;
		driver->termios = NULL;
		kfree(p);
		return error;
	}

	mutex_lock(&tty_mutex);
	list_add(&driver->tty_drivers, &tty_drivers);
	mutex_unlock(&tty_mutex);

	if (!(driver->flags & TTY_DRIVER_DYNAMIC_DEV)) {
		for (i = 0; i < driver->num; i++)
		    tty_register_device(driver, i, NULL);
	}
	proc_tty_register_driver(driver);
	driver->flags |= TTY_DRIVER_INSTALLED;
	return 0;
}

EXPORT_SYMBOL(tty_register_driver);

int tty_unregister_driver(struct tty_driver *driver)
{
#if 0
	/* FIXME */
	if (driver->refcount)
		return -EBUSY;
#endif
	unregister_chrdev_region(MKDEV(driver->major, driver->minor_start),
				driver->num);
	mutex_lock(&tty_mutex);
	list_del(&driver->tty_drivers);
	mutex_unlock(&tty_mutex);
	return 0;
}

EXPORT_SYMBOL(tty_unregister_driver);

dev_t tty_devnum(struct tty_struct *tty)
{
	return MKDEV(tty->driver->major, tty->driver->minor_start) + tty->index;
}
EXPORT_SYMBOL(tty_devnum);

void proc_clear_tty(struct task_struct *p)
{
	unsigned long flags;
	struct tty_struct *tty;
	spin_lock_irqsave(&p->sighand->siglock, flags);
	tty = p->signal->tty;
	p->signal->tty = NULL;
	spin_unlock_irqrestore(&p->sighand->siglock, flags);
	tty_kref_put(tty);
}

/* Called under the sighand lock */

static void __proc_set_tty(struct task_struct *tsk, struct tty_struct *tty)
{
	if (tty) {
		unsigned long flags;
		/* We should not have a session or pgrp to put here but.... */
		spin_lock_irqsave(&tty->ctrl_lock, flags);
		put_pid(tty->session);
		put_pid(tty->pgrp);
		tty->pgrp = get_pid(task_pgrp(tsk));
		spin_unlock_irqrestore(&tty->ctrl_lock, flags);
		tty->session = get_pid(task_session(tsk));
		if (tsk->signal->tty) {
			printk(KERN_DEBUG "tty not NULL!!\n");
			tty_kref_put(tsk->signal->tty);
		}
	}
	put_pid(tsk->signal->tty_old_pgrp);
	tsk->signal->tty = tty_kref_get(tty);
	tsk->signal->tty_old_pgrp = NULL;
}

static void proc_set_tty(struct task_struct *tsk, struct tty_struct *tty)
{
	spin_lock_irq(&tsk->sighand->siglock);
	__proc_set_tty(tsk, tty);
	spin_unlock_irq(&tsk->sighand->siglock);
}

struct tty_struct *get_current_tty(void)
{
	struct tty_struct *tty;
	unsigned long flags;

	spin_lock_irqsave(&current->sighand->siglock, flags);
	tty = tty_kref_get(current->signal->tty);
	spin_unlock_irqrestore(&current->sighand->siglock, flags);
	return tty;
}
EXPORT_SYMBOL_GPL(get_current_tty);

void tty_default_fops(struct file_operations *fops)
{
	*fops = tty_fops;
}

void __init console_init(void)
{
	initcall_t *call;

	/* Setup the default TTY line discipline. */
	tty_ldisc_begin();

	/*
	 * set up the console device so that later boot sequences can
	 * inform about problems etc..
	 */
	call = __con_initcall_start;
	while (call < __con_initcall_end) {
		(*call)();
		call++;
	}
}

static int __init tty_class_init(void)
{
	tty_class = class_create(THIS_MODULE, "tty");
	if (IS_ERR(tty_class))
		return PTR_ERR(tty_class);
	return 0;
}

postcore_initcall(tty_class_init);

/* 3/2004 jmc: why do these devices exist? */

static struct cdev tty_cdev, console_cdev;

static int __init tty_init(void)
{
	cdev_init(&tty_cdev, &tty_fops);
	if (cdev_add(&tty_cdev, MKDEV(TTYAUX_MAJOR, 0), 1) ||
	    register_chrdev_region(MKDEV(TTYAUX_MAJOR, 0), 1, "/dev/tty") < 0)
		panic("Couldn't register /dev/tty driver\n");
	device_create(tty_class, NULL, MKDEV(TTYAUX_MAJOR, 0), NULL,
			      "tty");

	cdev_init(&console_cdev, &console_fops);
	if (cdev_add(&console_cdev, MKDEV(TTYAUX_MAJOR, 1), 1) ||
	    register_chrdev_region(MKDEV(TTYAUX_MAJOR, 1), 1, "/dev/console") < 0)
		panic("Couldn't register /dev/console driver\n");
	device_create(tty_class, NULL, MKDEV(TTYAUX_MAJOR, 1), NULL,
			      "console");

#ifdef CONFIG_VT
	vty_init(&console_fops);
#endif
	return 0;
}
module_init(tty_init);
