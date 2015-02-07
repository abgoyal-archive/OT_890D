
#ifndef __XFS_QUOTA_H__
#define __XFS_QUOTA_H__

#define XFS_DQUOT_MAGIC		0x4451		/* 'DQ' */
#define XFS_DQUOT_VERSION	(u_int8_t)0x01	/* latest version number */

typedef __uint32_t	xfs_dqid_t;

typedef __uint64_t	xfs_qcnt_t;
typedef __uint16_t	xfs_qwarncnt_t;

typedef struct	xfs_disk_dquot {
	__be16		d_magic;	/* dquot magic = XFS_DQUOT_MAGIC */
	__u8		d_version;	/* dquot version */
	__u8		d_flags;	/* XFS_DQ_USER/PROJ/GROUP */
	__be32		d_id;		/* user,project,group id */
	__be64		d_blk_hardlimit;/* absolute limit on disk blks */
	__be64		d_blk_softlimit;/* preferred limit on disk blks */
	__be64		d_ino_hardlimit;/* maximum # allocated inodes */
	__be64		d_ino_softlimit;/* preferred inode limit */
	__be64		d_bcount;	/* disk blocks owned by the user */
	__be64		d_icount;	/* inodes owned by the user */
	__be32		d_itimer;	/* zero if within inode limits if not,
					   this is when we refuse service */
	__be32		d_btimer;	/* similar to above; for disk blocks */
	__be16		d_iwarns;	/* warnings issued wrt num inodes */
	__be16		d_bwarns;	/* warnings issued wrt disk blocks */
	__be32		d_pad0;		/* 64 bit align */
	__be64		d_rtb_hardlimit;/* absolute limit on realtime blks */
	__be64		d_rtb_softlimit;/* preferred limit on RT disk blks */
	__be64		d_rtbcount;	/* realtime blocks owned */
	__be32		d_rtbtimer;	/* similar to above; for RT disk blocks */
	__be16		d_rtbwarns;	/* warnings issued wrt RT disk blocks */
	__be16		d_pad;
} xfs_disk_dquot_t;

typedef struct xfs_dqblk {
	xfs_disk_dquot_t  dd_diskdq;	/* portion that lives incore as well */
	char		  dd_fill[32];	/* filling for posterity */
} xfs_dqblk_t;

#define XFS_DQ_USER		0x0001		/* a user quota */
#define XFS_DQ_PROJ		0x0002		/* project quota */
#define XFS_DQ_GROUP		0x0004		/* a group quota */
#define XFS_DQ_DIRTY		0x0008		/* dquot is dirty */
#define XFS_DQ_WANT		0x0010		/* for lookup/reclaim race */
#define XFS_DQ_INACTIVE		0x0020		/* dq off mplist & hashlist */

#define XFS_DQ_ALLTYPES		(XFS_DQ_USER|XFS_DQ_PROJ|XFS_DQ_GROUP)

#define XFS_DQUOT_LOGRES(mp)	(sizeof(xfs_disk_dquot_t) * 3)



typedef struct xfs_dq_logformat {
	__uint16_t		qlf_type;      /* dquot log item type */
	__uint16_t		qlf_size;      /* size of this item */
	xfs_dqid_t		qlf_id;	       /* usr/grp/proj id : 32 bits */
	__int64_t		qlf_blkno;     /* blkno of dquot buffer */
	__int32_t		qlf_len;       /* len of dquot buffer */
	__uint32_t		qlf_boffset;   /* off of dquot in buffer */
} xfs_dq_logformat_t;

typedef struct xfs_qoff_logformat {
	unsigned short		qf_type;	/* quotaoff log item type */
	unsigned short		qf_size;	/* size of this item */
	unsigned int		qf_flags;	/* USR and/or GRP */
	char			qf_pad[12];	/* padding for future */
} xfs_qoff_logformat_t;


#define XFS_UQUOTA_ACCT	0x0001  /* user quota accounting ON */
#define XFS_UQUOTA_ENFD	0x0002  /* user quota limits enforced */
#define XFS_UQUOTA_CHKD	0x0004  /* quotacheck run on usr quotas */
#define XFS_PQUOTA_ACCT	0x0008  /* project quota accounting ON */
#define XFS_OQUOTA_ENFD	0x0010  /* other (grp/prj) quota limits enforced */
#define XFS_OQUOTA_CHKD	0x0020  /* quotacheck run on other (grp/prj) quotas */
#define XFS_GQUOTA_ACCT	0x0040  /* group quota accounting ON */

#define XFS_ALL_QUOTA_ACCT	\
		(XFS_UQUOTA_ACCT | XFS_GQUOTA_ACCT | XFS_PQUOTA_ACCT)
#define XFS_ALL_QUOTA_ENFD	(XFS_UQUOTA_ENFD | XFS_OQUOTA_ENFD)
#define XFS_ALL_QUOTA_CHKD	(XFS_UQUOTA_CHKD | XFS_OQUOTA_CHKD)

#define XFS_IS_QUOTA_RUNNING(mp)	((mp)->m_qflags & XFS_ALL_QUOTA_ACCT)
#define XFS_IS_UQUOTA_RUNNING(mp)	((mp)->m_qflags & XFS_UQUOTA_ACCT)
#define XFS_IS_PQUOTA_RUNNING(mp)	((mp)->m_qflags & XFS_PQUOTA_ACCT)
#define XFS_IS_GQUOTA_RUNNING(mp)	((mp)->m_qflags & XFS_GQUOTA_ACCT)
#define XFS_IS_UQUOTA_ENFORCED(mp)	((mp)->m_qflags & XFS_UQUOTA_ENFD)
#define XFS_IS_OQUOTA_ENFORCED(mp)	((mp)->m_qflags & XFS_OQUOTA_ENFD)

#define XFS_UQUOTA_ACTIVE	0x0100  /* uquotas are being turned off */
#define XFS_PQUOTA_ACTIVE	0x0200  /* pquotas are being turned off */
#define XFS_GQUOTA_ACTIVE	0x0400  /* gquotas are being turned off */

#define XFS_IS_QUOTA_ON(mp)	((mp)->m_qflags & (XFS_UQUOTA_ACTIVE | \
						   XFS_GQUOTA_ACTIVE | \
						   XFS_PQUOTA_ACTIVE))
#define XFS_IS_OQUOTA_ON(mp)	((mp)->m_qflags & (XFS_GQUOTA_ACTIVE | \
						   XFS_PQUOTA_ACTIVE))
#define XFS_IS_UQUOTA_ON(mp)	((mp)->m_qflags & XFS_UQUOTA_ACTIVE)
#define XFS_IS_GQUOTA_ON(mp)	((mp)->m_qflags & XFS_GQUOTA_ACTIVE)
#define XFS_IS_PQUOTA_ON(mp)	((mp)->m_qflags & XFS_PQUOTA_ACTIVE)

#define XFS_QMOPT_DQLOCK	0x0000001 /* dqlock */
#define XFS_QMOPT_DQALLOC	0x0000002 /* alloc dquot ondisk if needed */
#define XFS_QMOPT_UQUOTA	0x0000004 /* user dquot requested */
#define XFS_QMOPT_PQUOTA	0x0000008 /* project dquot requested */
#define XFS_QMOPT_FORCE_RES	0x0000010 /* ignore quota limits */
#define XFS_QMOPT_DQSUSER	0x0000020 /* don't cache super users dquot */
#define XFS_QMOPT_SBVERSION	0x0000040 /* change superblock version num */
#define XFS_QMOPT_QUOTAOFF	0x0000080 /* quotas are being turned off */
#define XFS_QMOPT_UMOUNTING	0x0000100 /* filesys is being unmounted */
#define XFS_QMOPT_DOLOG		0x0000200 /* log buf changes (in quotacheck) */
#define XFS_QMOPT_DOWARN        0x0000400 /* increase warning cnt if needed */
#define XFS_QMOPT_ILOCKED	0x0000800 /* inode is already locked (excl) */
#define XFS_QMOPT_DQREPAIR	0x0001000 /* repair dquot if damaged */
#define XFS_QMOPT_GQUOTA	0x0002000 /* group dquot requested */
#define XFS_QMOPT_ENOSPC	0x0004000 /* enospc instead of edquot (prj) */

#define XFS_QMOPT_RES_REGBLKS	0x0010000
#define XFS_QMOPT_RES_RTBLKS	0x0020000
#define XFS_QMOPT_BCOUNT	0x0040000
#define XFS_QMOPT_ICOUNT	0x0080000
#define XFS_QMOPT_RTBCOUNT	0x0100000
#define XFS_QMOPT_DELBCOUNT	0x0200000
#define XFS_QMOPT_DELRTBCOUNT	0x0400000
#define XFS_QMOPT_RES_INOS	0x0800000

#define XFS_QMOPT_SYNC		0x1000000
#define XFS_QMOPT_ASYNC		0x2000000
#define XFS_QMOPT_DELWRI	0x4000000

#define XFS_QMOPT_INHERIT	0x8000000

#define XFS_TRANS_DQ_RES_BLKS	XFS_QMOPT_RES_REGBLKS
#define XFS_TRANS_DQ_RES_RTBLKS	XFS_QMOPT_RES_RTBLKS
#define XFS_TRANS_DQ_RES_INOS	XFS_QMOPT_RES_INOS
#define XFS_TRANS_DQ_BCOUNT	XFS_QMOPT_BCOUNT
#define XFS_TRANS_DQ_DELBCOUNT	XFS_QMOPT_DELBCOUNT
#define XFS_TRANS_DQ_ICOUNT	XFS_QMOPT_ICOUNT
#define XFS_TRANS_DQ_RTBCOUNT	XFS_QMOPT_RTBCOUNT
#define XFS_TRANS_DQ_DELRTBCOUNT XFS_QMOPT_DELRTBCOUNT


#define XFS_QMOPT_QUOTALL	\
		(XFS_QMOPT_UQUOTA | XFS_QMOPT_PQUOTA | XFS_QMOPT_GQUOTA)
#define XFS_QMOPT_RESBLK_MASK	(XFS_QMOPT_RES_REGBLKS | XFS_QMOPT_RES_RTBLKS)

#ifdef __KERNEL__
#define XFS_NOT_DQATTACHED(mp, ip) ((XFS_IS_UQUOTA_ON(mp) &&\
				     (ip)->i_udquot == NULL) || \
				    (XFS_IS_OQUOTA_ON(mp) && \
				     (ip)->i_gdquot == NULL))

#define XFS_QM_NEED_QUOTACHECK(mp) \
	((XFS_IS_UQUOTA_ON(mp) && \
		(mp->m_sb.sb_qflags & XFS_UQUOTA_CHKD) == 0) || \
	 (XFS_IS_GQUOTA_ON(mp) && \
		((mp->m_sb.sb_qflags & XFS_OQUOTA_CHKD) == 0 || \
		 (mp->m_sb.sb_qflags & XFS_PQUOTA_ACCT))) || \
	 (XFS_IS_PQUOTA_ON(mp) && \
		((mp->m_sb.sb_qflags & XFS_OQUOTA_CHKD) == 0 || \
		 (mp->m_sb.sb_qflags & XFS_GQUOTA_ACCT))))

#define XFS_MOUNT_QUOTA_SET1	(XFS_UQUOTA_ACCT|XFS_UQUOTA_ENFD|\
				 XFS_UQUOTA_CHKD|XFS_PQUOTA_ACCT|\
				 XFS_OQUOTA_ENFD|XFS_OQUOTA_CHKD)

#define XFS_MOUNT_QUOTA_SET2	(XFS_UQUOTA_ACCT|XFS_UQUOTA_ENFD|\
				 XFS_UQUOTA_CHKD|XFS_GQUOTA_ACCT|\
				 XFS_OQUOTA_ENFD|XFS_OQUOTA_CHKD)

#define XFS_MOUNT_QUOTA_ALL	(XFS_UQUOTA_ACCT|XFS_UQUOTA_ENFD|\
				 XFS_UQUOTA_CHKD|XFS_PQUOTA_ACCT|\
				 XFS_OQUOTA_ENFD|XFS_OQUOTA_CHKD|\
				 XFS_GQUOTA_ACCT)


typedef struct xfs_dqtrx {
	struct xfs_dquot *qt_dquot;	  /* the dquot this refers to */
	ulong		qt_blk_res;	  /* blks reserved on a dquot */
	ulong		qt_blk_res_used;  /* blks used from the reservation */
	ulong		qt_ino_res;	  /* inode reserved on a dquot */
	ulong		qt_ino_res_used;  /* inodes used from the reservation */
	long		qt_bcount_delta;  /* dquot blk count changes */
	long		qt_delbcnt_delta; /* delayed dquot blk count changes */
	long		qt_icount_delta;  /* dquot inode count changes */
	ulong		qt_rtblk_res;	  /* # blks reserved on a dquot */
	ulong		qt_rtblk_res_used;/* # blks used from reservation */
	long		qt_rtbcount_delta;/* dquot realtime blk changes */
	long		qt_delrtb_delta;  /* delayed RT blk count changes */
} xfs_dqtrx_t;

typedef void	(*qo_dup_dqinfo_t)(struct xfs_trans *, struct xfs_trans *);
typedef void	(*qo_mod_dquot_byino_t)(struct xfs_trans *,
				struct xfs_inode *, uint, long);
typedef void	(*qo_free_dqinfo_t)(struct xfs_trans *);
typedef void	(*qo_apply_dquot_deltas_t)(struct xfs_trans *);
typedef void	(*qo_unreserve_and_mod_dquots_t)(struct xfs_trans *);
typedef int	(*qo_reserve_quota_nblks_t)(
				struct xfs_trans *, struct xfs_mount *,
				struct xfs_inode *, long, long, uint);
typedef int	(*qo_reserve_quota_bydquots_t)(
				struct xfs_trans *, struct xfs_mount *,
				struct xfs_dquot *, struct xfs_dquot *,
				long, long, uint);
typedef struct xfs_dqtrxops {
	qo_dup_dqinfo_t			qo_dup_dqinfo;
	qo_free_dqinfo_t		qo_free_dqinfo;
	qo_mod_dquot_byino_t		qo_mod_dquot_byino;
	qo_apply_dquot_deltas_t		qo_apply_dquot_deltas;
	qo_reserve_quota_nblks_t	qo_reserve_quota_nblks;
	qo_reserve_quota_bydquots_t	qo_reserve_quota_bydquots;
	qo_unreserve_and_mod_dquots_t	qo_unreserve_and_mod_dquots;
} xfs_dqtrxops_t;

#define XFS_DQTRXOP(mp, tp, op, args...) \
		((mp)->m_qm_ops->xfs_dqtrxops ? \
		((mp)->m_qm_ops->xfs_dqtrxops->op)(tp, ## args) : 0)

#define XFS_DQTRXOP_VOID(mp, tp, op, args...) \
		((mp)->m_qm_ops->xfs_dqtrxops ? \
		((mp)->m_qm_ops->xfs_dqtrxops->op)(tp, ## args) : (void)0)

#define XFS_TRANS_DUP_DQINFO(mp, otp, ntp) \
	XFS_DQTRXOP_VOID(mp, otp, qo_dup_dqinfo, ntp)
#define XFS_TRANS_FREE_DQINFO(mp, tp) \
	XFS_DQTRXOP_VOID(mp, tp, qo_free_dqinfo)
#define XFS_TRANS_MOD_DQUOT_BYINO(mp, tp, ip, field, delta) \
	XFS_DQTRXOP_VOID(mp, tp, qo_mod_dquot_byino, ip, field, delta)
#define XFS_TRANS_APPLY_DQUOT_DELTAS(mp, tp) \
	XFS_DQTRXOP_VOID(mp, tp, qo_apply_dquot_deltas)
#define XFS_TRANS_RESERVE_QUOTA_NBLKS(mp, tp, ip, nblks, ninos, fl) \
	XFS_DQTRXOP(mp, tp, qo_reserve_quota_nblks, mp, ip, nblks, ninos, fl)
#define XFS_TRANS_RESERVE_QUOTA_BYDQUOTS(mp, tp, ud, gd, nb, ni, fl) \
	XFS_DQTRXOP(mp, tp, qo_reserve_quota_bydquots, mp, ud, gd, nb, ni, fl)
#define XFS_TRANS_UNRESERVE_AND_MOD_DQUOTS(mp, tp) \
	XFS_DQTRXOP_VOID(mp, tp, qo_unreserve_and_mod_dquots)

#define XFS_TRANS_UNRESERVE_QUOTA_NBLKS(mp, tp, ip, nblks, ninos, flags) \
	XFS_TRANS_RESERVE_QUOTA_NBLKS(mp, tp, ip, -(nblks), -(ninos), flags)
#define XFS_TRANS_RESERVE_QUOTA(mp, tp, ud, gd, nb, ni, f) \
	XFS_TRANS_RESERVE_QUOTA_BYDQUOTS(mp, tp, ud, gd, nb, ni, \
				f | XFS_QMOPT_RES_REGBLKS)
#define XFS_TRANS_UNRESERVE_QUOTA(mp, tp, ud, gd, nb, ni, f) \
	XFS_TRANS_RESERVE_QUOTA_BYDQUOTS(mp, tp, ud, gd, -(nb), -(ni), \
				f | XFS_QMOPT_RES_REGBLKS)

extern int xfs_qm_dqcheck(xfs_disk_dquot_t *, xfs_dqid_t, uint, uint, char *);
extern int xfs_mount_reset_sbqflags(struct xfs_mount *);

extern struct xfs_qmops xfs_qmcore_xfs;

#endif	/* __KERNEL__ */

#endif	/* __XFS_QUOTA_H__ */
