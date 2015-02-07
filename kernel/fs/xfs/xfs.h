
#ifndef __XFS_H__
#define __XFS_H__

#ifdef CONFIG_XFS_DEBUG
#define STATIC
#define DEBUG 1
#define XFS_BUF_LOCK_TRACKING 1
/* #define QUOTADEBUG 1 */
#endif

#ifdef CONFIG_XFS_TRACE
#define XFS_ALLOC_TRACE 1
#define XFS_ATTR_TRACE 1
#define XFS_BLI_TRACE 1
#define XFS_BMAP_TRACE 1
#define XFS_BTREE_TRACE 1
#define XFS_DIR2_TRACE 1
#define XFS_DQUOT_TRACE 1
#define XFS_ILOCK_TRACE 1
#define XFS_LOG_TRACE 1
#define XFS_RW_TRACE 1
#define XFS_BUF_TRACE 1
#define XFS_INODE_TRACE 1
#define XFS_FILESTREAMS_TRACE 1
#endif

#include <linux-2.6/xfs_linux.h>
#endif	/* __XFS_H__ */
