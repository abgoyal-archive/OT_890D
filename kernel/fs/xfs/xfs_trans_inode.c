
#include "xfs.h"
#include "xfs_fs.h"
#include "xfs_types.h"
#include "xfs_bit.h"
#include "xfs_log.h"
#include "xfs_inum.h"
#include "xfs_trans.h"
#include "xfs_sb.h"
#include "xfs_ag.h"
#include "xfs_dir2.h"
#include "xfs_dmapi.h"
#include "xfs_mount.h"
#include "xfs_bmap_btree.h"
#include "xfs_alloc_btree.h"
#include "xfs_ialloc_btree.h"
#include "xfs_dir2_sf.h"
#include "xfs_attr_sf.h"
#include "xfs_dinode.h"
#include "xfs_inode.h"
#include "xfs_btree.h"
#include "xfs_ialloc.h"
#include "xfs_trans_priv.h"
#include "xfs_inode_item.h"

#ifdef XFS_TRANS_DEBUG
STATIC void
xfs_trans_inode_broot_debug(
	xfs_inode_t	*ip);
#else
#define	xfs_trans_inode_broot_debug(ip)
#endif


int
xfs_trans_iget(
	xfs_mount_t	*mp,
	xfs_trans_t	*tp,
	xfs_ino_t	ino,
	uint		flags,
	uint		lock_flags,
	xfs_inode_t	**ipp)
{
	int			error;
	xfs_inode_t		*ip;

	/*
	 * If the transaction pointer is NULL, just call the normal
	 * xfs_iget().
	 */
	if (tp == NULL)
		return xfs_iget(mp, NULL, ino, flags, lock_flags, ipp, 0);

	/*
	 * If we find the inode in core with this transaction
	 * pointer in its i_transp field, then we know we already
	 * have it locked.  In this case we just increment the lock
	 * recursion count and return the inode to the caller.
	 * Assert that the inode is already locked in the mode requested
	 * by the caller.  We cannot do lock promotions yet, so
	 * die if someone gets this wrong.
	 */
	if ((ip = xfs_inode_incore(tp->t_mountp, ino, tp)) != NULL) {
		/*
		 * Make sure that the inode lock is held EXCL and
		 * that the io lock is never upgraded when the inode
		 * is already a part of the transaction.
		 */
		ASSERT(ip->i_itemp != NULL);
		ASSERT(lock_flags & XFS_ILOCK_EXCL);
		ASSERT(xfs_isilocked(ip, XFS_ILOCK_EXCL));
		ASSERT((!(lock_flags & XFS_IOLOCK_EXCL)) ||
		       xfs_isilocked(ip, XFS_IOLOCK_EXCL));
		ASSERT((!(lock_flags & XFS_IOLOCK_EXCL)) ||
		       (ip->i_itemp->ili_flags & XFS_ILI_IOLOCKED_EXCL));
		ASSERT((!(lock_flags & XFS_IOLOCK_SHARED)) ||
		       xfs_isilocked(ip, XFS_IOLOCK_EXCL|XFS_IOLOCK_SHARED));
		ASSERT((!(lock_flags & XFS_IOLOCK_SHARED)) ||
		       (ip->i_itemp->ili_flags & XFS_ILI_IOLOCKED_ANY));

		if (lock_flags & (XFS_IOLOCK_SHARED | XFS_IOLOCK_EXCL)) {
			ip->i_itemp->ili_iolock_recur++;
		}
		if (lock_flags & XFS_ILOCK_EXCL) {
			ip->i_itemp->ili_ilock_recur++;
		}
		*ipp = ip;
		return 0;
	}

	ASSERT(lock_flags & XFS_ILOCK_EXCL);
	error = xfs_iget(tp->t_mountp, tp, ino, flags, lock_flags, &ip, 0);
	if (error) {
		return error;
	}
	ASSERT(ip != NULL);

	xfs_trans_ijoin(tp, ip, lock_flags);
	*ipp = ip;
	return 0;
}

void
xfs_trans_ijoin(
	xfs_trans_t	*tp,
	xfs_inode_t	*ip,
	uint		lock_flags)
{
	xfs_inode_log_item_t	*iip;

	ASSERT(ip->i_transp == NULL);
	ASSERT(xfs_isilocked(ip, XFS_ILOCK_EXCL));
	ASSERT(lock_flags & XFS_ILOCK_EXCL);
	if (ip->i_itemp == NULL)
		xfs_inode_item_init(ip, ip->i_mount);
	iip = ip->i_itemp;
	ASSERT(iip->ili_flags == 0);
	ASSERT(iip->ili_ilock_recur == 0);
	ASSERT(iip->ili_iolock_recur == 0);

	/*
	 * Get a log_item_desc to point at the new item.
	 */
	(void) xfs_trans_add_item(tp, (xfs_log_item_t*)(iip));

	xfs_trans_inode_broot_debug(ip);

	/*
	 * If the IO lock is already held, mark that in the inode log item.
	 */
	if (lock_flags & XFS_IOLOCK_EXCL) {
		iip->ili_flags |= XFS_ILI_IOLOCKED_EXCL;
	} else if (lock_flags & XFS_IOLOCK_SHARED) {
		iip->ili_flags |= XFS_ILI_IOLOCKED_SHARED;
	}

	/*
	 * Initialize i_transp so we can find it with xfs_inode_incore()
	 * in xfs_trans_iget() above.
	 */
	ip->i_transp = tp;
}



/*ARGSUSED*/
void
xfs_trans_ihold(
	xfs_trans_t	*tp,
	xfs_inode_t	*ip)
{
	ASSERT(ip->i_transp == tp);
	ASSERT(ip->i_itemp != NULL);
	ASSERT(xfs_isilocked(ip, XFS_ILOCK_EXCL));

	ip->i_itemp->ili_flags |= XFS_ILI_HOLD;
}


void
xfs_trans_log_inode(
	xfs_trans_t	*tp,
	xfs_inode_t	*ip,
	uint		flags)
{
	xfs_log_item_desc_t	*lidp;

	ASSERT(ip->i_transp == tp);
	ASSERT(ip->i_itemp != NULL);
	ASSERT(xfs_isilocked(ip, XFS_ILOCK_EXCL));

	lidp = xfs_trans_find_item(tp, (xfs_log_item_t*)(ip->i_itemp));
	ASSERT(lidp != NULL);

	tp->t_flags |= XFS_TRANS_DIRTY;
	lidp->lid_flags |= XFS_LID_DIRTY;

	/*
	 * Always OR in the bits from the ili_last_fields field.
	 * This is to coordinate with the xfs_iflush() and xfs_iflush_done()
	 * routines in the eventual clearing of the ilf_fields bits.
	 * See the big comment in xfs_iflush() for an explanation of
	 * this coordination mechanism.
	 */
	flags |= ip->i_itemp->ili_last_fields;
	ip->i_itemp->ili_format.ilf_fields |= flags;
}

#ifdef XFS_TRANS_DEBUG
STATIC void
xfs_trans_inode_broot_debug(
	xfs_inode_t	*ip)
{
	xfs_inode_log_item_t	*iip;

	ASSERT(ip->i_itemp != NULL);
	iip = ip->i_itemp;
	if (iip->ili_root_size != 0) {
		ASSERT(iip->ili_orig_root != NULL);
		kmem_free(iip->ili_orig_root);
		iip->ili_root_size = 0;
		iip->ili_orig_root = NULL;
	}
	if (ip->i_d.di_format == XFS_DINODE_FMT_BTREE) {
		ASSERT((ip->i_df.if_broot != NULL) &&
		       (ip->i_df.if_broot_bytes > 0));
		iip->ili_root_size = ip->i_df.if_broot_bytes;
		iip->ili_orig_root =
			(char*)kmem_alloc(iip->ili_root_size, KM_SLEEP);
		memcpy(iip->ili_orig_root, (char*)(ip->i_df.if_broot),
		      iip->ili_root_size);
	}
}
#endif
