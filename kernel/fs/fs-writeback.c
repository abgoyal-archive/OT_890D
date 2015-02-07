

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/writeback.h>
#include <linux/blkdev.h>
#include <linux/backing-dev.h>
#include <linux/buffer_head.h>
#include "internal.h"


static int writeback_acquire(struct backing_dev_info *bdi)
{
	return !test_and_set_bit(BDI_pdflush, &bdi->state);
}

int writeback_in_progress(struct backing_dev_info *bdi)
{
	return test_bit(BDI_pdflush, &bdi->state);
}

static void writeback_release(struct backing_dev_info *bdi)
{
	BUG_ON(!writeback_in_progress(bdi));
	clear_bit(BDI_pdflush, &bdi->state);
}

void __mark_inode_dirty(struct inode *inode, int flags)
{
	struct super_block *sb = inode->i_sb;

	/*
	 * Don't do this for I_DIRTY_PAGES - that doesn't actually
	 * dirty the inode itself
	 */
	if (flags & (I_DIRTY_SYNC | I_DIRTY_DATASYNC)) {
		if (sb->s_op->dirty_inode)
			sb->s_op->dirty_inode(inode);
	}

	/*
	 * make sure that changes are seen by all cpus before we test i_state
	 * -- mikulas
	 */
	smp_mb();

	/* avoid the locking if we can */
	if ((inode->i_state & flags) == flags)
		return;

	if (unlikely(block_dump > 1)) {
		struct dentry *dentry = NULL;
		const char *name = "?";

		if (!list_empty(&inode->i_dentry)) {
			dentry = list_entry(inode->i_dentry.next,
					    struct dentry, d_alias);
			if (dentry && dentry->d_name.name)
				name = (const char *) dentry->d_name.name;
		}

		if (inode->i_ino || strcmp(inode->i_sb->s_id, "bdev"))
			printk(KERN_DEBUG
			       "%s(%d): dirtied inode %lu (%s) on %s\n",
			       current->comm, task_pid_nr(current), inode->i_ino,
			       name, inode->i_sb->s_id);
	}

	spin_lock(&inode_lock);
	if ((inode->i_state & flags) != flags) {
		const int was_dirty = inode->i_state & I_DIRTY;

		inode->i_state |= flags;

		/*
		 * If the inode is being synced, just update its dirty state.
		 * The unlocker will place the inode on the appropriate
		 * superblock list, based upon its state.
		 */
		if (inode->i_state & I_SYNC)
			goto out;

		/*
		 * Only add valid (hashed) inodes to the superblock's
		 * dirty list.  Add blockdev inodes as well.
		 */
		if (!S_ISBLK(inode->i_mode)) {
			if (hlist_unhashed(&inode->i_hash))
				goto out;
		}
		if (inode->i_state & (I_FREEING|I_CLEAR))
			goto out;

		/*
		 * If the inode was already on s_dirty/s_io/s_more_io, don't
		 * reposition it (that would break s_dirty time-ordering).
		 */
		if (!was_dirty) {
			inode->dirtied_when = jiffies;
			list_move(&inode->i_list, &sb->s_dirty);
		}
	}
out:
	spin_unlock(&inode_lock);
}

EXPORT_SYMBOL(__mark_inode_dirty);

static int write_inode(struct inode *inode, int sync)
{
	if (inode->i_sb->s_op->write_inode && !is_bad_inode(inode))
		return inode->i_sb->s_op->write_inode(inode, sync);
	return 0;
}

static void redirty_tail(struct inode *inode)
{
	struct super_block *sb = inode->i_sb;

	if (!list_empty(&sb->s_dirty)) {
		struct inode *tail_inode;

		tail_inode = list_entry(sb->s_dirty.next, struct inode, i_list);
		if (!time_after_eq(inode->dirtied_when,
				tail_inode->dirtied_when))
			inode->dirtied_when = jiffies;
	}
	list_move(&inode->i_list, &sb->s_dirty);
}

static void requeue_io(struct inode *inode)
{
	list_move(&inode->i_list, &inode->i_sb->s_more_io);
}

static void inode_sync_complete(struct inode *inode)
{
	/*
	 * Prevent speculative execution through spin_unlock(&inode_lock);
	 */
	smp_mb();
	wake_up_bit(&inode->i_state, __I_SYNC);
}

static void move_expired_inodes(struct list_head *delaying_queue,
			       struct list_head *dispatch_queue,
				unsigned long *older_than_this)
{
	while (!list_empty(delaying_queue)) {
		struct inode *inode = list_entry(delaying_queue->prev,
						struct inode, i_list);
		if (older_than_this &&
			time_after(inode->dirtied_when, *older_than_this))
			break;
		list_move(&inode->i_list, dispatch_queue);
	}
}

static void queue_io(struct super_block *sb,
				unsigned long *older_than_this)
{
	list_splice_init(&sb->s_more_io, sb->s_io.prev);
	move_expired_inodes(&sb->s_dirty, &sb->s_io, older_than_this);
}

int sb_has_dirty_inodes(struct super_block *sb)
{
	return !list_empty(&sb->s_dirty) ||
	       !list_empty(&sb->s_io) ||
	       !list_empty(&sb->s_more_io);
}
EXPORT_SYMBOL(sb_has_dirty_inodes);

static int
__sync_single_inode(struct inode *inode, struct writeback_control *wbc)
{
	unsigned dirty;
	struct address_space *mapping = inode->i_mapping;
	int wait = wbc->sync_mode == WB_SYNC_ALL;
	int ret;

	BUG_ON(inode->i_state & I_SYNC);
	WARN_ON(inode->i_state & I_NEW);

	/* Set I_SYNC, reset I_DIRTY */
	dirty = inode->i_state & I_DIRTY;
	inode->i_state |= I_SYNC;
	inode->i_state &= ~I_DIRTY;

	spin_unlock(&inode_lock);

	ret = do_writepages(mapping, wbc);

	/* Don't write the inode if only I_DIRTY_PAGES was set */
	if (dirty & (I_DIRTY_SYNC | I_DIRTY_DATASYNC)) {
		int err = write_inode(inode, wait);
		if (ret == 0)
			ret = err;
	}

	if (wait) {
		int err = filemap_fdatawait(mapping);
		if (ret == 0)
			ret = err;
	}

	spin_lock(&inode_lock);
	WARN_ON(inode->i_state & I_NEW);
	inode->i_state &= ~I_SYNC;
	if (!(inode->i_state & I_FREEING)) {
		if (!(inode->i_state & I_DIRTY) &&
		    mapping_tagged(mapping, PAGECACHE_TAG_DIRTY)) {
			/*
			 * We didn't write back all the pages.  nfs_writepages()
			 * sometimes bales out without doing anything. Redirty
			 * the inode; Move it from s_io onto s_more_io/s_dirty.
			 */
			/*
			 * akpm: if the caller was the kupdate function we put
			 * this inode at the head of s_dirty so it gets first
			 * consideration.  Otherwise, move it to the tail, for
			 * the reasons described there.  I'm not really sure
			 * how much sense this makes.  Presumably I had a good
			 * reasons for doing it this way, and I'd rather not
			 * muck with it at present.
			 */
			if (wbc->for_kupdate) {
				/*
				 * For the kupdate function we move the inode
				 * to s_more_io so it will get more writeout as
				 * soon as the queue becomes uncongested.
				 */
				inode->i_state |= I_DIRTY_PAGES;
				if (wbc->nr_to_write <= 0) {
					/*
					 * slice used up: queue for next turn
					 */
					requeue_io(inode);
				} else {
					/*
					 * somehow blocked: retry later
					 */
					redirty_tail(inode);
				}
			} else {
				/*
				 * Otherwise fully redirty the inode so that
				 * other inodes on this superblock will get some
				 * writeout.  Otherwise heavy writing to one
				 * file would indefinitely suspend writeout of
				 * all the other files.
				 */
				inode->i_state |= I_DIRTY_PAGES;
				redirty_tail(inode);
			}
		} else if (inode->i_state & I_DIRTY) {
			/*
			 * Someone redirtied the inode while were writing back
			 * the pages.
			 */
			redirty_tail(inode);
		} else if (atomic_read(&inode->i_count)) {
			/*
			 * The inode is clean, inuse
			 */
			list_move(&inode->i_list, &inode_in_use);
		} else {
			/*
			 * The inode is clean, unused
			 */
			list_move(&inode->i_list, &inode_unused);
		}
	}
	inode_sync_complete(inode);
	return ret;
}

static int
__writeback_single_inode(struct inode *inode, struct writeback_control *wbc)
{
	wait_queue_head_t *wqh;

	if (!atomic_read(&inode->i_count))
		WARN_ON(!(inode->i_state & (I_WILL_FREE|I_FREEING)));
	else
		WARN_ON(inode->i_state & I_WILL_FREE);

	if ((wbc->sync_mode != WB_SYNC_ALL) && (inode->i_state & I_SYNC)) {
		/*
		 * We're skipping this inode because it's locked, and we're not
		 * doing writeback-for-data-integrity.  Move it to s_more_io so
		 * that writeback can proceed with the other inodes on s_io.
		 * We'll have another go at writing back this inode when we
		 * completed a full scan of s_io.
		 */
		requeue_io(inode);
		return 0;
	}

	/*
	 * It's a data-integrity sync.  We must wait.
	 */
	if (inode->i_state & I_SYNC) {
		DEFINE_WAIT_BIT(wq, &inode->i_state, __I_SYNC);

		wqh = bit_waitqueue(&inode->i_state, __I_SYNC);
		do {
			spin_unlock(&inode_lock);
			__wait_on_bit(wqh, &wq, inode_wait,
							TASK_UNINTERRUPTIBLE);
			spin_lock(&inode_lock);
		} while (inode->i_state & I_SYNC);
	}
	return __sync_single_inode(inode, wbc);
}

void generic_sync_sb_inodes(struct super_block *sb,
				struct writeback_control *wbc)
{
	const unsigned long start = jiffies;	/* livelock avoidance */
	int sync = wbc->sync_mode == WB_SYNC_ALL;

	spin_lock(&inode_lock);
	if (!wbc->for_kupdate || list_empty(&sb->s_io))
		queue_io(sb, wbc->older_than_this);

	while (!list_empty(&sb->s_io)) {
		struct inode *inode = list_entry(sb->s_io.prev,
						struct inode, i_list);
		struct address_space *mapping = inode->i_mapping;
		struct backing_dev_info *bdi = mapping->backing_dev_info;
		long pages_skipped;

		if (!bdi_cap_writeback_dirty(bdi)) {
			redirty_tail(inode);
			if (sb_is_blkdev_sb(sb)) {
				/*
				 * Dirty memory-backed blockdev: the ramdisk
				 * driver does this.  Skip just this inode
				 */
				continue;
			}
			/*
			 * Dirty memory-backed inode against a filesystem other
			 * than the kernel-internal bdev filesystem.  Skip the
			 * entire superblock.
			 */
			break;
		}

		if (inode->i_state & I_NEW) {
			requeue_io(inode);
			continue;
		}

		if (wbc->nonblocking && bdi_write_congested(bdi)) {
			wbc->encountered_congestion = 1;
			if (!sb_is_blkdev_sb(sb))
				break;		/* Skip a congested fs */
			requeue_io(inode);
			continue;		/* Skip a congested blockdev */
		}

		if (wbc->bdi && bdi != wbc->bdi) {
			if (!sb_is_blkdev_sb(sb))
				break;		/* fs has the wrong queue */
			requeue_io(inode);
			continue;		/* blockdev has wrong queue */
		}

		/* Was this inode dirtied after sync_sb_inodes was called? */
		if (time_after(inode->dirtied_when, start))
			break;

		/* Is another pdflush already flushing this queue? */
		if (current_is_pdflush() && !writeback_acquire(bdi))
			break;

		BUG_ON(inode->i_state & I_FREEING);
		__iget(inode);
		pages_skipped = wbc->pages_skipped;
		__writeback_single_inode(inode, wbc);
		if (current_is_pdflush())
			writeback_release(bdi);
		if (wbc->pages_skipped != pages_skipped) {
			/*
			 * writeback is not making progress due to locked
			 * buffers.  Skip this inode for now.
			 */
			redirty_tail(inode);
		}
		spin_unlock(&inode_lock);
		iput(inode);
		cond_resched();
		spin_lock(&inode_lock);
		if (wbc->nr_to_write <= 0) {
			wbc->more_io = 1;
			break;
		}
		if (!list_empty(&sb->s_more_io))
			wbc->more_io = 1;
	}

	if (sync) {
		struct inode *inode, *old_inode = NULL;

		/*
		 * Data integrity sync. Must wait for all pages under writeback,
		 * because there may have been pages dirtied before our sync
		 * call, but which had writeout started before we write it out.
		 * In which case, the inode may not be on the dirty list, but
		 * we still have to wait for that writeout.
		 */
		list_for_each_entry(inode, &sb->s_inodes, i_sb_list) {
			struct address_space *mapping;

			if (inode->i_state & (I_FREEING|I_WILL_FREE|I_NEW))
				continue;
			mapping = inode->i_mapping;
			if (mapping->nrpages == 0)
				continue;
			__iget(inode);
			spin_unlock(&inode_lock);
			/*
			 * We hold a reference to 'inode' so it couldn't have
			 * been removed from s_inodes list while we dropped the
			 * inode_lock.  We cannot iput the inode now as we can
			 * be holding the last reference and we cannot iput it
			 * under inode_lock. So we keep the reference and iput
			 * it later.
			 */
			iput(old_inode);
			old_inode = inode;

			filemap_fdatawait(mapping);

			cond_resched();

			spin_lock(&inode_lock);
		}
		spin_unlock(&inode_lock);
		iput(old_inode);
	} else
		spin_unlock(&inode_lock);

	return;		/* Leave any unwritten inodes on s_io */
}
EXPORT_SYMBOL_GPL(generic_sync_sb_inodes);

static void sync_sb_inodes(struct super_block *sb,
				struct writeback_control *wbc)
{
	generic_sync_sb_inodes(sb, wbc);
}

void
writeback_inodes(struct writeback_control *wbc)
{
	struct super_block *sb;

	might_sleep();
	spin_lock(&sb_lock);
restart:
	list_for_each_entry_reverse(sb, &super_blocks, s_list) {
		if (sb_has_dirty_inodes(sb)) {
			/* we're making our own get_super here */
			sb->s_count++;
			spin_unlock(&sb_lock);
			/*
			 * If we can't get the readlock, there's no sense in
			 * waiting around, most of the time the FS is going to
			 * be unmounted by the time it is released.
			 */
			if (down_read_trylock(&sb->s_umount)) {
				if (sb->s_root)
					sync_sb_inodes(sb, wbc);
				up_read(&sb->s_umount);
			}
			spin_lock(&sb_lock);
			if (__put_super_and_need_restart(sb))
				goto restart;
		}
		if (wbc->nr_to_write <= 0)
			break;
	}
	spin_unlock(&sb_lock);
}

void sync_inodes_sb(struct super_block *sb, int wait)
{
	struct writeback_control wbc = {
		.sync_mode	= wait ? WB_SYNC_ALL : WB_SYNC_NONE,
		.range_start	= 0,
		.range_end	= LLONG_MAX,
	};

	if (!wait) {
		unsigned long nr_dirty = global_page_state(NR_FILE_DIRTY);
		unsigned long nr_unstable = global_page_state(NR_UNSTABLE_NFS);

		wbc.nr_to_write = nr_dirty + nr_unstable +
			(inodes_stat.nr_inodes - inodes_stat.nr_unused);
	} else
		wbc.nr_to_write = LONG_MAX; /* doesn't actually matter */

	sync_sb_inodes(sb, &wbc);
}

static void __sync_inodes(int wait)
{
	struct super_block *sb;

	spin_lock(&sb_lock);
restart:
	list_for_each_entry(sb, &super_blocks, s_list) {
		sb->s_count++;
		spin_unlock(&sb_lock);
		down_read(&sb->s_umount);
		if (sb->s_root) {
			sync_inodes_sb(sb, wait);
			sync_blockdev(sb->s_bdev);
		}
		up_read(&sb->s_umount);
		spin_lock(&sb_lock);
		if (__put_super_and_need_restart(sb))
			goto restart;
	}
	spin_unlock(&sb_lock);
}

void sync_inodes(int wait)
{
	__sync_inodes(0);

	if (wait)
		__sync_inodes(1);
}

int write_inode_now(struct inode *inode, int sync)
{
	int ret;
	struct writeback_control wbc = {
		.nr_to_write = LONG_MAX,
		.sync_mode = sync ? WB_SYNC_ALL : WB_SYNC_NONE,
		.range_start = 0,
		.range_end = LLONG_MAX,
	};

	if (!mapping_cap_writeback_dirty(inode->i_mapping))
		wbc.nr_to_write = 0;

	might_sleep();
	spin_lock(&inode_lock);
	ret = __writeback_single_inode(inode, &wbc);
	spin_unlock(&inode_lock);
	if (sync)
		inode_sync_wait(inode);
	return ret;
}
EXPORT_SYMBOL(write_inode_now);

int sync_inode(struct inode *inode, struct writeback_control *wbc)
{
	int ret;

	spin_lock(&inode_lock);
	ret = __writeback_single_inode(inode, wbc);
	spin_unlock(&inode_lock);
	return ret;
}
EXPORT_SYMBOL(sync_inode);


int generic_osync_inode(struct inode *inode, struct address_space *mapping, int what)
{
	int err = 0;
	int need_write_inode_now = 0;
	int err2;

	if (what & OSYNC_DATA)
		err = filemap_fdatawrite(mapping);
	if (what & (OSYNC_METADATA|OSYNC_DATA)) {
		err2 = sync_mapping_buffers(mapping);
		if (!err)
			err = err2;
	}
	if (what & OSYNC_DATA) {
		err2 = filemap_fdatawait(mapping);
		if (!err)
			err = err2;
	}

	spin_lock(&inode_lock);
	if ((inode->i_state & I_DIRTY) &&
	    ((what & OSYNC_INODE) || (inode->i_state & I_DIRTY_DATASYNC)))
		need_write_inode_now = 1;
	spin_unlock(&inode_lock);

	if (need_write_inode_now) {
		err2 = write_inode_now(inode, 1);
		if (!err)
			err = err2;
	}
	else
		inode_sync_wait(inode);

	return err;
}
EXPORT_SYMBOL(generic_osync_inode);
