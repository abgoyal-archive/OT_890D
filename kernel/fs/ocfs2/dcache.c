

#include <linux/fs.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/namei.h>

#define MLOG_MASK_PREFIX ML_DCACHE
#include <cluster/masklog.h>

#include "ocfs2.h"

#include "alloc.h"
#include "dcache.h"
#include "dlmglue.h"
#include "file.h"
#include "inode.h"
#include "super.h"


static int ocfs2_dentry_revalidate(struct dentry *dentry,
				   struct nameidata *nd)
{
	struct inode *inode = dentry->d_inode;
	int ret = 0;    /* if all else fails, just return false */
	struct ocfs2_super *osb = OCFS2_SB(dentry->d_sb);

	mlog_entry("(0x%p, '%.*s')\n", dentry,
		   dentry->d_name.len, dentry->d_name.name);

	/* Never trust a negative dentry - force a new lookup. */
	if (inode == NULL) {
		mlog(0, "negative dentry: %.*s\n", dentry->d_name.len,
		     dentry->d_name.name);
		goto bail;
	}

	BUG_ON(!osb);

	if (inode == osb->root_inode || is_bad_inode(inode))
		goto bail;

	spin_lock(&OCFS2_I(inode)->ip_lock);
	/* did we or someone else delete this inode? */
	if (OCFS2_I(inode)->ip_flags & OCFS2_INODE_DELETED) {
		spin_unlock(&OCFS2_I(inode)->ip_lock);
		mlog(0, "inode (%llu) deleted, returning false\n",
		     (unsigned long long)OCFS2_I(inode)->ip_blkno);
		goto bail;
	}
	spin_unlock(&OCFS2_I(inode)->ip_lock);

	/*
	 * We don't need a cluster lock to test this because once an
	 * inode nlink hits zero, it never goes back.
	 */
	if (inode->i_nlink == 0) {
		mlog(0, "Inode %llu orphaned, returning false "
		     "dir = %d\n",
		     (unsigned long long)OCFS2_I(inode)->ip_blkno,
		     S_ISDIR(inode->i_mode));
		goto bail;
	}

	ret = 1;

bail:
	mlog_exit(ret);

	return ret;
}

static int ocfs2_match_dentry(struct dentry *dentry,
			      u64 parent_blkno,
			      int skip_unhashed)
{
	struct inode *parent;

	/*
	 * ocfs2_lookup() does a d_splice_alias() _before_ attaching
	 * to the lock data, so we skip those here, otherwise
	 * ocfs2_dentry_attach_lock() will get its original dentry
	 * back.
	 */
	if (!dentry->d_fsdata)
		return 0;

	if (!dentry->d_parent)
		return 0;

	if (skip_unhashed && d_unhashed(dentry))
		return 0;

	parent = dentry->d_parent->d_inode;
	/* Negative parent dentry? */
	if (!parent)
		return 0;

	/* Name is in a different directory. */
	if (OCFS2_I(parent)->ip_blkno != parent_blkno)
		return 0;

	return 1;
}

struct dentry *ocfs2_find_local_alias(struct inode *inode,
				      u64 parent_blkno,
				      int skip_unhashed)
{
	struct list_head *p;
	struct dentry *dentry = NULL;

	spin_lock(&dcache_lock);

	list_for_each(p, &inode->i_dentry) {
		dentry = list_entry(p, struct dentry, d_alias);

		if (ocfs2_match_dentry(dentry, parent_blkno, skip_unhashed)) {
			mlog(0, "dentry found: %.*s\n",
			     dentry->d_name.len, dentry->d_name.name);

			dget_locked(dentry);
			break;
		}

		dentry = NULL;
	}

	spin_unlock(&dcache_lock);

	return dentry;
}

DEFINE_SPINLOCK(dentry_attach_lock);

int ocfs2_dentry_attach_lock(struct dentry *dentry,
			     struct inode *inode,
			     u64 parent_blkno)
{
	int ret;
	struct dentry *alias;
	struct ocfs2_dentry_lock *dl = dentry->d_fsdata;

	mlog(0, "Attach \"%.*s\", parent %llu, fsdata: %p\n",
	     dentry->d_name.len, dentry->d_name.name,
	     (unsigned long long)parent_blkno, dl);

	/*
	 * Negative dentry. We ignore these for now.
	 *
	 * XXX: Could we can improve ocfs2_dentry_revalidate() by
	 * tracking these?
	 */
	if (!inode)
		return 0;

	if (dl) {
		mlog_bug_on_msg(dl->dl_parent_blkno != parent_blkno,
				" \"%.*s\": old parent: %llu, new: %llu\n",
				dentry->d_name.len, dentry->d_name.name,
				(unsigned long long)parent_blkno,
				(unsigned long long)dl->dl_parent_blkno);
		return 0;
	}

	alias = ocfs2_find_local_alias(inode, parent_blkno, 0);
	if (alias) {
		/*
		 * Great, an alias exists, which means we must have a
		 * dentry lock already. We can just grab the lock off
		 * the alias and add it to the list.
		 *
		 * We're depending here on the fact that this dentry
		 * was found and exists in the dcache and so must have
		 * a reference to the dentry_lock because we can't
		 * race creates. Final dput() cannot happen on it
		 * since we have it pinned, so our reference is safe.
		 */
		dl = alias->d_fsdata;
		mlog_bug_on_msg(!dl, "parent %llu, ino %llu\n",
				(unsigned long long)parent_blkno,
				(unsigned long long)OCFS2_I(inode)->ip_blkno);

		mlog_bug_on_msg(dl->dl_parent_blkno != parent_blkno,
				" \"%.*s\": old parent: %llu, new: %llu\n",
				dentry->d_name.len, dentry->d_name.name,
				(unsigned long long)parent_blkno,
				(unsigned long long)dl->dl_parent_blkno);

		mlog(0, "Found: %s\n", dl->dl_lockres.l_name);

		goto out_attach;
	}

	/*
	 * There are no other aliases
	 */
	dl = kmalloc(sizeof(*dl), GFP_NOFS);
	if (!dl) {
		ret = -ENOMEM;
		mlog_errno(ret);
		return ret;
	}

	dl->dl_count = 0;
	/*
	 * Does this have to happen below, for all attaches, in case
	 * the struct inode gets blown away by the downconvert thread?
	 */
	dl->dl_inode = igrab(inode);
	dl->dl_parent_blkno = parent_blkno;
	ocfs2_dentry_lock_res_init(dl, parent_blkno, inode);

out_attach:
	spin_lock(&dentry_attach_lock);
	dentry->d_fsdata = dl;
	dl->dl_count++;
	spin_unlock(&dentry_attach_lock);

	/*
	 * This actually gets us our PRMODE level lock. From now on,
	 * we'll have a notification if one of these names is
	 * destroyed on another node.
	 */
	ret = ocfs2_dentry_lock(dentry, 0);
	if (!ret)
		ocfs2_dentry_unlock(dentry, 0);
	else
		mlog_errno(ret);

	dput(alias);

	return ret;
}

static DEFINE_SPINLOCK(dentry_list_lock);

#define DL_INODE_DROP_COUNT 64

/* Drop inode references from dentry locks */
void ocfs2_drop_dl_inodes(struct work_struct *work)
{
	struct ocfs2_super *osb = container_of(work, struct ocfs2_super,
					       dentry_lock_work);
	struct ocfs2_dentry_lock *dl;
	int drop_count = DL_INODE_DROP_COUNT;

	spin_lock(&dentry_list_lock);
	while (osb->dentry_lock_list && drop_count--) {
		dl = osb->dentry_lock_list;
		osb->dentry_lock_list = dl->dl_next;
		spin_unlock(&dentry_list_lock);
		iput(dl->dl_inode);
		kfree(dl);
		spin_lock(&dentry_list_lock);
	}
	if (osb->dentry_lock_list)
		queue_work(ocfs2_wq, &osb->dentry_lock_work);
	spin_unlock(&dentry_list_lock);
}

static void ocfs2_drop_dentry_lock(struct ocfs2_super *osb,
				   struct ocfs2_dentry_lock *dl)
{
	ocfs2_simple_drop_lockres(osb, &dl->dl_lockres);
	ocfs2_lock_res_free(&dl->dl_lockres);

	/* We leave dropping of inode reference to ocfs2_wq as that can
	 * possibly lead to inode deletion which gets tricky */
	spin_lock(&dentry_list_lock);
	if (!osb->dentry_lock_list)
		queue_work(ocfs2_wq, &osb->dentry_lock_work);
	dl->dl_next = osb->dentry_lock_list;
	osb->dentry_lock_list = dl;
	spin_unlock(&dentry_list_lock);
}

void ocfs2_dentry_lock_put(struct ocfs2_super *osb,
			   struct ocfs2_dentry_lock *dl)
{
	int unlock;

	BUG_ON(dl->dl_count == 0);

	spin_lock(&dentry_attach_lock);
	dl->dl_count--;
	unlock = !dl->dl_count;
	spin_unlock(&dentry_attach_lock);

	if (unlock)
		ocfs2_drop_dentry_lock(osb, dl);
}

static void ocfs2_dentry_iput(struct dentry *dentry, struct inode *inode)
{
	struct ocfs2_dentry_lock *dl = dentry->d_fsdata;

	if (!dl) {
		/*
		 * No dentry lock is ok if we're disconnected or
		 * unhashed.
		 */
		if (!(dentry->d_flags & DCACHE_DISCONNECTED) &&
		    !d_unhashed(dentry)) {
			unsigned long long ino = 0ULL;
			if (inode)
				ino = (unsigned long long)OCFS2_I(inode)->ip_blkno;
			mlog(ML_ERROR, "Dentry is missing cluster lock. "
			     "inode: %llu, d_flags: 0x%x, d_name: %.*s\n",
			     ino, dentry->d_flags, dentry->d_name.len,
			     dentry->d_name.name);
		}

		goto out;
	}

	mlog_bug_on_msg(dl->dl_count == 0, "dentry: %.*s, count: %u\n",
			dentry->d_name.len, dentry->d_name.name,
			dl->dl_count);

	ocfs2_dentry_lock_put(OCFS2_SB(dentry->d_sb), dl);

out:
	iput(inode);
}

void ocfs2_dentry_move(struct dentry *dentry, struct dentry *target,
		       struct inode *old_dir, struct inode *new_dir)
{
	int ret;
	struct ocfs2_super *osb = OCFS2_SB(old_dir->i_sb);
	struct inode *inode = dentry->d_inode;

	/*
	 * Move within the same directory, so the actual lock info won't
	 * change.
	 *
	 * XXX: Is there any advantage to dropping the lock here?
	 */
	if (old_dir == new_dir)
		goto out_move;

	ocfs2_dentry_lock_put(osb, dentry->d_fsdata);

	dentry->d_fsdata = NULL;
	ret = ocfs2_dentry_attach_lock(dentry, inode, OCFS2_I(new_dir)->ip_blkno);
	if (ret)
		mlog_errno(ret);

out_move:
	d_move(dentry, target);
}

struct dentry_operations ocfs2_dentry_ops = {
	.d_revalidate		= ocfs2_dentry_revalidate,
	.d_iput			= ocfs2_dentry_iput,
};
