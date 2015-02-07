

#include <linux/time.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/writeback.h>
#include <linux/jbd2.h>
#include <linux/blkdev.h>
#include <linux/marker.h>
#include "ext4.h"
#include "ext4_jbd2.h"


int ext4_sync_file(struct file *file, struct dentry *dentry, int datasync)
{
	struct inode *inode = dentry->d_inode;
	journal_t *journal = EXT4_SB(inode->i_sb)->s_journal;
	int ret = 0;

	J_ASSERT(ext4_journal_current_handle() == NULL);

	trace_mark(ext4_sync_file, "dev %s datasync %d ino %ld parent %ld",
		   inode->i_sb->s_id, datasync, inode->i_ino,
		   dentry->d_parent->d_inode->i_ino);

	/*
	 * data=writeback:
	 *  The caller's filemap_fdatawrite()/wait will sync the data.
	 *  sync_inode() will sync the metadata
	 *
	 * data=ordered:
	 *  The caller's filemap_fdatawrite() will write the data and
	 *  sync_inode() will write the inode if it is dirty.  Then the caller's
	 *  filemap_fdatawait() will wait on the pages.
	 *
	 * data=journal:
	 *  filemap_fdatawrite won't do anything (the buffers are clean).
	 *  ext4_force_commit will write the file data into the journal and
	 *  will wait on that.
	 *  filemap_fdatawait() will encounter a ton of newly-dirtied pages
	 *  (they were dirtied by commit).  But that's OK - the blocks are
	 *  safe in-journal, which is all fsync() needs to ensure.
	 */
	if (ext4_should_journal_data(inode)) {
		ret = ext4_force_commit(inode->i_sb);
		goto out;
	}

	if (datasync && !(inode->i_state & I_DIRTY_DATASYNC))
		goto out;

	/*
	 * The VFS has written the file data.  If the inode is unaltered
	 * then we need not start a commit.
	 */
	if (inode->i_state & (I_DIRTY_SYNC|I_DIRTY_DATASYNC)) {
		struct writeback_control wbc = {
			.sync_mode = WB_SYNC_ALL,
			.nr_to_write = 0, /* sys_fsync did this */
		};
		ret = sync_inode(inode, &wbc);
		if (journal && (journal->j_flags & JBD2_BARRIER))
			blkdev_issue_flush(inode->i_sb->s_bdev, NULL);
	}
out:
	return ret;
}
