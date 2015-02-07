
#ifndef __XFS_QM_H__
#define __XFS_QM_H__

#include "xfs_dquot_item.h"
#include "xfs_dquot.h"
#include "xfs_quota_priv.h"
#include "xfs_qm_stats.h"

struct xfs_qm;
struct xfs_inode;

extern uint		ndquot;
extern mutex_t		xfs_Gqm_lock;
extern struct xfs_qm	*xfs_Gqm;
extern kmem_zone_t	*qm_dqzone;
extern kmem_zone_t	*qm_dqtrxzone;

#define XFS_QM_SYNC_MAX_RESTARTS	7

#define XFS_QM_RECLAIM_MAX_RESTARTS	4

#define XFS_QM_DQFREE_RATIO		2

#define XFS_QM_HASHSIZE_LOW		(PAGE_SIZE / sizeof(xfs_dqhash_t))
#define XFS_QM_HASHSIZE_HIGH		((PAGE_SIZE * 4) / sizeof(xfs_dqhash_t))

#define XFS_DQUOT_CLUSTER_SIZE_FSB	(xfs_filblks_t)1
#define XFS_QM_MAX_DQCLUSTER_LOGSZ	3

typedef xfs_dqhash_t	xfs_dqlist_t;
typedef struct xfs_frlist {
       struct xfs_dquot *qh_next;
       struct xfs_dquot *qh_prev;
       mutex_t		 qh_lock;
       uint		 qh_version;
       uint		 qh_nelems;
} xfs_frlist_t;

typedef struct xfs_qm {
	xfs_dqlist_t	*qm_usr_dqhtable;/* udquot hash table */
	xfs_dqlist_t	*qm_grp_dqhtable;/* gdquot hash table */
	uint		 qm_dqhashmask;	 /* # buckets in dq hashtab - 1 */
	xfs_frlist_t	 qm_dqfreelist;	 /* freelist of dquots */
	atomic_t	 qm_totaldquots; /* total incore dquots */
	uint		 qm_nrefs;	 /* file systems with quota on */
	int		 qm_dqfree_ratio;/* ratio of free to inuse dquots */
	kmem_zone_t	*qm_dqzone;	 /* dquot mem-alloc zone */
	kmem_zone_t	*qm_dqtrxzone;	 /* t_dqinfo of transactions */
} xfs_qm_t;

typedef struct xfs_quotainfo {
	xfs_inode_t	*qi_uquotaip;	 /* user quota inode */
	xfs_inode_t	*qi_gquotaip;	 /* group quota inode */
	xfs_dqlist_t	 qi_dqlist;	 /* all dquots in filesys */
	int		 qi_dqreclaims;	 /* a change here indicates
					    a removal in the dqlist */
	time_t		 qi_btimelimit;	 /* limit for blks timer */
	time_t		 qi_itimelimit;	 /* limit for inodes timer */
	time_t		 qi_rtbtimelimit;/* limit for rt blks timer */
	xfs_qwarncnt_t	 qi_bwarnlimit;	 /* limit for blks warnings */
	xfs_qwarncnt_t	 qi_iwarnlimit;	 /* limit for inodes warnings */
	xfs_qwarncnt_t	 qi_rtbwarnlimit;/* limit for rt blks warnings */
	mutex_t		 qi_quotaofflock;/* to serialize quotaoff */
	xfs_filblks_t	 qi_dqchunklen;	 /* # BBs in a chunk of dqs */
	uint		 qi_dqperchunk;	 /* # ondisk dqs in above chunk */
	xfs_qcnt_t	 qi_bhardlimit;	 /* default data blk hard limit */
	xfs_qcnt_t	 qi_bsoftlimit;	 /* default data blk soft limit */
	xfs_qcnt_t	 qi_ihardlimit;	 /* default inode count hard limit */
	xfs_qcnt_t	 qi_isoftlimit;	 /* default inode count soft limit */
	xfs_qcnt_t	 qi_rtbhardlimit;/* default realtime blk hard limit */
	xfs_qcnt_t	 qi_rtbsoftlimit;/* default realtime blk soft limit */
} xfs_quotainfo_t;


extern xfs_dqtrxops_t	xfs_trans_dquot_ops;

extern void	xfs_trans_mod_dquot(xfs_trans_t *, xfs_dquot_t *, uint, long);
extern int	xfs_trans_reserve_quota_bydquots(xfs_trans_t *, xfs_mount_t *,
			xfs_dquot_t *, xfs_dquot_t *, long, long, uint);
extern void	xfs_trans_dqjoin(xfs_trans_t *, xfs_dquot_t *);
extern void	xfs_trans_log_dquot(xfs_trans_t *, xfs_dquot_t *);

#define XFS_QM_TRANS_MAXDQS		2
typedef struct xfs_dquot_acct {
	xfs_dqtrx_t	dqa_usrdquots[XFS_QM_TRANS_MAXDQS];
	xfs_dqtrx_t	dqa_grpdquots[XFS_QM_TRANS_MAXDQS];
} xfs_dquot_acct_t;

#define XFS_QM_BTIMELIMIT	(7 * 24*60*60)          /* 1 week */
#define XFS_QM_RTBTIMELIMIT	(7 * 24*60*60)          /* 1 week */
#define XFS_QM_ITIMELIMIT	(7 * 24*60*60)          /* 1 week */

#define XFS_QM_BWARNLIMIT	5
#define XFS_QM_IWARNLIMIT	5
#define XFS_QM_RTBWARNLIMIT	5

#define XFS_QM_LOCK(xqm)	(mutex_lock(&xqm##_lock))
#define XFS_QM_UNLOCK(xqm)	(mutex_unlock(&xqm##_lock))
#define XFS_QM_HOLD(xqm)	((xqm)->qm_nrefs++)
#define XFS_QM_RELE(xqm)	((xqm)->qm_nrefs--)

extern void		xfs_qm_destroy_quotainfo(xfs_mount_t *);
extern void		xfs_qm_mount_quotas(xfs_mount_t *);
extern int		xfs_qm_quotacheck(xfs_mount_t *);
extern void		xfs_qm_unmount_quotadestroy(xfs_mount_t *);
extern void		xfs_qm_unmount_quotas(xfs_mount_t *);
extern int		xfs_qm_write_sb_changes(xfs_mount_t *, __int64_t);
extern int		xfs_qm_sync(xfs_mount_t *, int);

/* dquot stuff */
extern boolean_t	xfs_qm_dqalloc_incore(xfs_dquot_t **);
extern int		xfs_qm_dqattach(xfs_inode_t *, uint);
extern void		xfs_qm_dqdetach(xfs_inode_t *);
extern int		xfs_qm_dqpurge_all(xfs_mount_t *, uint);
extern void		xfs_qm_dqrele_all_inodes(xfs_mount_t *, uint);

/* vop stuff */
extern int		xfs_qm_vop_dqalloc(xfs_mount_t *, xfs_inode_t *,
					uid_t, gid_t, prid_t, uint,
					xfs_dquot_t **, xfs_dquot_t **);
extern void		xfs_qm_vop_dqattach_and_dqmod_newinode(
					xfs_trans_t *, xfs_inode_t *,
					xfs_dquot_t *, xfs_dquot_t *);
extern int		xfs_qm_vop_rename_dqattach(xfs_inode_t **);
extern xfs_dquot_t *	xfs_qm_vop_chown(xfs_trans_t *, xfs_inode_t *,
					xfs_dquot_t **, xfs_dquot_t *);
extern int		xfs_qm_vop_chown_reserve(xfs_trans_t *, xfs_inode_t *,
					xfs_dquot_t *, xfs_dquot_t *, uint);

/* list stuff */
extern void		xfs_qm_freelist_append(xfs_frlist_t *, xfs_dquot_t *);
extern void		xfs_qm_freelist_unlink(xfs_dquot_t *);
extern int		xfs_qm_freelist_lock_nowait(xfs_qm_t *);

/* system call interface */
extern int		xfs_qm_quotactl(struct xfs_mount *, int, int,
				xfs_caddr_t);

#ifdef DEBUG
extern int		xfs_qm_internalqcheck(xfs_mount_t *);
#else
#define xfs_qm_internalqcheck(mp)	(0)
#endif

#endif /* __XFS_QM_H__ */