
#ifndef __XFS_VNODE_H__
#define __XFS_VNODE_H__

#include "xfs_fs.h"

struct file;
struct xfs_inode;
struct xfs_iomap;
struct attrlist_cursor_kern;

#define	VN_INACTIVE_CACHE	0
#define	VN_INACTIVE_NOCACHE	1

#define IO_ISAIO	0x00001		/* don't wait for completion */
#define IO_ISDIRECT	0x00004		/* bypass page cache */
#define IO_INVIS	0x00020		/* don't update inode timestamps */

#define FLUSH_SYNC		1	/* wait for flush to complete	*/

#define FI_NONE			0	/* none */
#define FI_REMAPF		1	/* Do a remapf prior to the operation */
#define FI_REMAPF_LOCKED	2	/* Do a remapf prior to the operation.
					   Prevent VM access to the pages until
					   the operation completes. */

static inline int VN_BAD(struct inode *vp)
{
	return is_bad_inode(vp);
}

static inline void vn_atime_to_bstime(struct inode *vp, xfs_bstime_t *bs_atime)
{
	bs_atime->tv_sec = vp->i_atime.tv_sec;
	bs_atime->tv_nsec = vp->i_atime.tv_nsec;
}

static inline void vn_atime_to_timespec(struct inode *vp, struct timespec *ts)
{
	*ts = vp->i_atime;
}

static inline void vn_atime_to_time_t(struct inode *vp, time_t *tt)
{
	*tt = vp->i_atime.tv_sec;
}

#define VN_MAPPED(vp)	mapping_mapped(vp->i_mapping)
#define VN_CACHED(vp)	(vp->i_mapping->nrpages)
#define VN_DIRTY(vp)	mapping_tagged(vp->i_mapping, \
					PAGECACHE_TAG_DIRTY)


#endif	/* __XFS_VNODE_H__ */
