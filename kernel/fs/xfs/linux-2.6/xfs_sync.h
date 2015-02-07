
#ifndef XFS_SYNC_H
#define XFS_SYNC_H 1

struct xfs_mount;

typedef struct bhv_vfs_sync_work {
	struct list_head	w_list;
	struct xfs_mount	*w_mount;
	void			*w_data;	/* syncer routine argument */
	void			(*w_syncer)(struct xfs_mount *, void *);
} bhv_vfs_sync_work_t;

#define SYNC_ATTR		0x0001	/* sync attributes */
#define SYNC_DELWRI		0x0002	/* look at delayed writes */
#define SYNC_WAIT		0x0004	/* wait for i/o to complete */
#define SYNC_BDFLUSH		0x0008	/* BDFLUSH is calling -- don't block */
#define SYNC_IOWAIT		0x0010  /* wait for all I/O to complete */

int xfs_syncd_init(struct xfs_mount *mp);
void xfs_syncd_stop(struct xfs_mount *mp);

int xfs_sync_inodes(struct xfs_mount *mp, int flags);
int xfs_sync_fsdata(struct xfs_mount *mp, int flags);

int xfs_quiesce_data(struct xfs_mount *mp);
void xfs_quiesce_attr(struct xfs_mount *mp);

void xfs_flush_inode(struct xfs_inode *ip);
void xfs_flush_device(struct xfs_inode *ip);

int xfs_reclaim_inode(struct xfs_inode *ip, int locked, int sync_mode);
int xfs_reclaim_inodes(struct xfs_mount *mp, int noblock, int mode);

void xfs_inode_set_reclaim_tag(struct xfs_inode *ip);
void xfs_inode_clear_reclaim_tag(struct xfs_inode *ip);
void __xfs_inode_clear_reclaim_tag(struct xfs_mount *mp, struct xfs_perag *pag,
				struct xfs_inode *ip);
#endif
