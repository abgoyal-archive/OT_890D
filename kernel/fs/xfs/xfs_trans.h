
#ifndef	__XFS_TRANS_H__
#define	__XFS_TRANS_H__

struct xfs_log_item;

typedef struct xfs_trans_header {
	uint		th_magic;		/* magic number */
	uint		th_type;		/* transaction type */
	__int32_t	th_tid;			/* transaction id (unused) */
	uint		th_num_items;		/* num items logged by trans */
} xfs_trans_header_t;

#define	XFS_TRANS_HEADER_MAGIC	0x5452414e	/* TRAN */

#define	XFS_LI_EFI		0x1236
#define	XFS_LI_EFD		0x1237
#define	XFS_LI_IUNLINK		0x1238
#define	XFS_LI_INODE		0x123b	/* aligned ino chunks, var-size ibufs */
#define	XFS_LI_BUF		0x123c	/* v2 bufs, variable sized inode bufs */
#define	XFS_LI_DQUOT		0x123d
#define	XFS_LI_QUOTAOFF		0x123e

#define XFS_TRANS_SETATTR_NOT_SIZE	1
#define XFS_TRANS_SETATTR_SIZE		2
#define XFS_TRANS_INACTIVE		3
#define XFS_TRANS_CREATE		4
#define XFS_TRANS_CREATE_TRUNC		5
#define XFS_TRANS_TRUNCATE_FILE		6
#define XFS_TRANS_REMOVE		7
#define XFS_TRANS_LINK			8
#define XFS_TRANS_RENAME		9
#define XFS_TRANS_MKDIR			10
#define XFS_TRANS_RMDIR			11
#define XFS_TRANS_SYMLINK		12
#define XFS_TRANS_SET_DMATTRS		13
#define XFS_TRANS_GROWFS		14
#define XFS_TRANS_STRAT_WRITE		15
#define XFS_TRANS_DIOSTRAT		16
#define	XFS_TRANS_WRITE_SYNC		17
#define	XFS_TRANS_WRITEID		18
#define	XFS_TRANS_ADDAFORK		19
#define	XFS_TRANS_ATTRINVAL		20
#define	XFS_TRANS_ATRUNCATE		21
#define	XFS_TRANS_ATTR_SET		22
#define	XFS_TRANS_ATTR_RM		23
#define	XFS_TRANS_ATTR_FLAG		24
#define	XFS_TRANS_CLEAR_AGI_BUCKET	25
#define XFS_TRANS_QM_SBCHANGE		26
#define XFS_TRANS_DUMMY1		27
#define XFS_TRANS_DUMMY2		28
#define XFS_TRANS_QM_QUOTAOFF		29
#define XFS_TRANS_QM_DQALLOC		30
#define XFS_TRANS_QM_SETQLIM		31
#define XFS_TRANS_QM_DQCLUSTER		32
#define XFS_TRANS_QM_QINOCREATE		33
#define XFS_TRANS_QM_QUOTAOFF_END	34
#define XFS_TRANS_SB_UNIT		35
#define XFS_TRANS_FSYNC_TS		36
#define	XFS_TRANS_GROWFSRT_ALLOC	37
#define	XFS_TRANS_GROWFSRT_ZERO		38
#define	XFS_TRANS_GROWFSRT_FREE		39
#define	XFS_TRANS_SWAPEXT		40
#define	XFS_TRANS_SB_COUNT		41
#define	XFS_TRANS_TYPE_MAX		41
/* new transaction types need to be reflected in xfs_logprint(8) */

typedef struct xfs_log_item_desc {
	struct xfs_log_item	*lid_item;
	ushort		lid_size;
	unsigned char	lid_flags;
	unsigned char	lid_index;
} xfs_log_item_desc_t;

#define XFS_LID_DIRTY		0x1
#define XFS_LID_PINNED		0x2
#define XFS_LID_BUF_STALE	0x8

#define	XFS_LIC_NUM_SLOTS	15
typedef struct xfs_log_item_chunk {
	struct xfs_log_item_chunk	*lic_next;
	ushort				lic_free;
	ushort				lic_unused;
	xfs_log_item_desc_t		lic_descs[XFS_LIC_NUM_SLOTS];
} xfs_log_item_chunk_t;

#define	XFS_LIC_MAX_SLOT	(XFS_LIC_NUM_SLOTS - 1)
#define	XFS_LIC_FREEMASK	((1 << XFS_LIC_NUM_SLOTS) - 1)


static inline void xfs_lic_init(xfs_log_item_chunk_t *cp)
{
	cp->lic_free = XFS_LIC_FREEMASK;
}

static inline void xfs_lic_init_slot(xfs_log_item_chunk_t *cp, int slot)
{
	cp->lic_descs[slot].lid_index = (unsigned char)(slot);
}

static inline int xfs_lic_vacancy(xfs_log_item_chunk_t *cp)
{
	return cp->lic_free & XFS_LIC_FREEMASK;
}

static inline void xfs_lic_all_free(xfs_log_item_chunk_t *cp)
{
	cp->lic_free = XFS_LIC_FREEMASK;
}

static inline int xfs_lic_are_all_free(xfs_log_item_chunk_t *cp)
{
	return ((cp->lic_free & XFS_LIC_FREEMASK) == XFS_LIC_FREEMASK);
}

static inline int xfs_lic_isfree(xfs_log_item_chunk_t *cp, int slot)
{
	return (cp->lic_free & (1 << slot));
}

static inline void xfs_lic_claim(xfs_log_item_chunk_t *cp, int slot)
{
	cp->lic_free &= ~(1 << slot);
}

static inline void xfs_lic_relse(xfs_log_item_chunk_t *cp, int slot)
{
	cp->lic_free |= 1 << slot;
}

static inline xfs_log_item_desc_t *
xfs_lic_slot(xfs_log_item_chunk_t *cp, int slot)
{
	return &(cp->lic_descs[slot]);
}

static inline int xfs_lic_desc_to_slot(xfs_log_item_desc_t *dp)
{
	return (uint)dp->lid_index;
}

static inline xfs_log_item_chunk_t *
xfs_lic_desc_to_chunk(xfs_log_item_desc_t *dp)
{
	return (xfs_log_item_chunk_t*) \
		(((xfs_caddr_t)((dp) - (dp)->lid_index)) - \
		(xfs_caddr_t)(((xfs_log_item_chunk_t*)0)->lic_descs));
}

#define	XFS_TRANS_MAGIC		0x5452414E	/* 'TRAN' */
#define	XFS_TRANS_DIRTY		0x01	/* something needs to be logged */
#define	XFS_TRANS_SB_DIRTY	0x02	/* superblock is modified */
#define	XFS_TRANS_PERM_LOG_RES	0x04	/* xact took a permanent log res */
#define	XFS_TRANS_SYNC		0x08	/* make commit synchronous */
#define XFS_TRANS_DQ_DIRTY	0x10	/* at least one dquot in trx dirty */
#define XFS_TRANS_RESERVE	0x20    /* OK to use reserved data blocks */

#define	XFS_TRANS_NOSLEEP		0x1
#define	XFS_TRANS_WAIT			0x2
#define	XFS_TRANS_RELEASE_LOG_RES	0x4
#define	XFS_TRANS_ABORT			0x8

#define	XFS_TRANS_SB_ICOUNT		0x00000001
#define	XFS_TRANS_SB_IFREE		0x00000002
#define	XFS_TRANS_SB_FDBLOCKS		0x00000004
#define	XFS_TRANS_SB_RES_FDBLOCKS	0x00000008
#define	XFS_TRANS_SB_FREXTENTS		0x00000010
#define	XFS_TRANS_SB_RES_FREXTENTS	0x00000020
#define	XFS_TRANS_SB_DBLOCKS		0x00000040
#define	XFS_TRANS_SB_AGCOUNT		0x00000080
#define	XFS_TRANS_SB_IMAXPCT		0x00000100
#define	XFS_TRANS_SB_REXTSIZE		0x00000200
#define	XFS_TRANS_SB_RBMBLOCKS		0x00000400
#define	XFS_TRANS_SB_RBLOCKS		0x00000800
#define	XFS_TRANS_SB_REXTENTS		0x00001000
#define	XFS_TRANS_SB_REXTSLOG		0x00002000



#define	XFS_ALLOCFREE_LOG_RES(mp,nx) \
	((nx) * (2 * XFS_FSB_TO_B((mp), 2 * XFS_AG_MAXLEVELS(mp) - 1)))
#define	XFS_ALLOCFREE_LOG_COUNT(mp,nx) \
	((nx) * (2 * (2 * XFS_AG_MAXLEVELS(mp) - 1)))

#define	XFS_DIROP_LOG_RES(mp)	\
	(XFS_FSB_TO_B(mp, XFS_DAENTER_BLOCKS(mp, XFS_DATA_FORK)) + \
	 (XFS_FSB_TO_B(mp, XFS_DAENTER_BMAPS(mp, XFS_DATA_FORK) + 1)))
#define	XFS_DIROP_LOG_COUNT(mp)	\
	(XFS_DAENTER_BLOCKS(mp, XFS_DATA_FORK) + \
	 XFS_DAENTER_BMAPS(mp, XFS_DATA_FORK) + 1)

#define XFS_CALC_WRITE_LOG_RES(mp) \
	(MAX( \
	 ((mp)->m_sb.sb_inodesize + \
	  XFS_FSB_TO_B((mp), XFS_BM_MAXLEVELS(mp, XFS_DATA_FORK)) + \
	  (2 * (mp)->m_sb.sb_sectsize) + \
	  (mp)->m_sb.sb_sectsize + \
	  XFS_ALLOCFREE_LOG_RES(mp, 2) + \
	  (128 * (4 + XFS_BM_MAXLEVELS(mp, XFS_DATA_FORK) + XFS_ALLOCFREE_LOG_COUNT(mp, 2)))),\
	 ((2 * (mp)->m_sb.sb_sectsize) + \
	  (2 * (mp)->m_sb.sb_sectsize) + \
	  (mp)->m_sb.sb_sectsize + \
	  XFS_ALLOCFREE_LOG_RES(mp, 2) + \
	  (128 * (5 + XFS_ALLOCFREE_LOG_COUNT(mp, 2))))))

#define	XFS_WRITE_LOG_RES(mp)	((mp)->m_reservations.tr_write)

#define	XFS_CALC_ITRUNCATE_LOG_RES(mp) \
	(MAX( \
	 ((mp)->m_sb.sb_inodesize + \
	  XFS_FSB_TO_B((mp), XFS_BM_MAXLEVELS(mp, XFS_DATA_FORK) + 1) + \
	  (128 * (2 + XFS_BM_MAXLEVELS(mp, XFS_DATA_FORK)))), \
	 ((4 * (mp)->m_sb.sb_sectsize) + \
	  (4 * (mp)->m_sb.sb_sectsize) + \
	  (mp)->m_sb.sb_sectsize + \
	  XFS_ALLOCFREE_LOG_RES(mp, 4) + \
	  (128 * (9 + XFS_ALLOCFREE_LOG_COUNT(mp, 4))) + \
	  (128 * 5) + \
	  XFS_ALLOCFREE_LOG_RES(mp, 1) + \
	   (128 * (2 + XFS_IALLOC_BLOCKS(mp) + XFS_IN_MAXLEVELS(mp) + \
	    XFS_ALLOCFREE_LOG_COUNT(mp, 1))))))

#define	XFS_ITRUNCATE_LOG_RES(mp)   ((mp)->m_reservations.tr_itruncate)

#define	XFS_CALC_RENAME_LOG_RES(mp) \
	(MAX( \
	 ((4 * (mp)->m_sb.sb_inodesize) + \
	  (2 * XFS_DIROP_LOG_RES(mp)) + \
	  (128 * (4 + 2 * XFS_DIROP_LOG_COUNT(mp)))), \
	 ((3 * (mp)->m_sb.sb_sectsize) + \
	  (3 * (mp)->m_sb.sb_sectsize) + \
	  (mp)->m_sb.sb_sectsize + \
	  XFS_ALLOCFREE_LOG_RES(mp, 3) + \
	  (128 * (7 + XFS_ALLOCFREE_LOG_COUNT(mp, 3))))))

#define	XFS_RENAME_LOG_RES(mp)	((mp)->m_reservations.tr_rename)

#define	XFS_CALC_LINK_LOG_RES(mp) \
	(MAX( \
	 ((mp)->m_sb.sb_inodesize + \
	  (mp)->m_sb.sb_inodesize + \
	  XFS_DIROP_LOG_RES(mp) + \
	  (128 * (2 + XFS_DIROP_LOG_COUNT(mp)))), \
	 ((mp)->m_sb.sb_sectsize + \
	  (mp)->m_sb.sb_sectsize + \
	  (mp)->m_sb.sb_sectsize + \
	  XFS_ALLOCFREE_LOG_RES(mp, 1) + \
	  (128 * (3 + XFS_ALLOCFREE_LOG_COUNT(mp, 1))))))

#define	XFS_LINK_LOG_RES(mp)	((mp)->m_reservations.tr_link)

#define	XFS_CALC_REMOVE_LOG_RES(mp)	\
	(MAX( \
	 ((mp)->m_sb.sb_inodesize + \
	  (mp)->m_sb.sb_inodesize + \
	  XFS_DIROP_LOG_RES(mp) + \
	  (128 * (2 + XFS_DIROP_LOG_COUNT(mp)))), \
	 ((2 * (mp)->m_sb.sb_sectsize) + \
	  (2 * (mp)->m_sb.sb_sectsize) + \
	  (mp)->m_sb.sb_sectsize + \
	  XFS_ALLOCFREE_LOG_RES(mp, 2) + \
	  (128 * (5 + XFS_ALLOCFREE_LOG_COUNT(mp, 2))))))

#define	XFS_REMOVE_LOG_RES(mp)	((mp)->m_reservations.tr_remove)

#define	XFS_CALC_SYMLINK_LOG_RES(mp)		\
	(MAX( \
	 ((mp)->m_sb.sb_inodesize + \
	  (mp)->m_sb.sb_inodesize + \
	  XFS_FSB_TO_B(mp, 1) + \
	  XFS_DIROP_LOG_RES(mp) + \
	  1024 + \
	  (128 * (4 + XFS_DIROP_LOG_COUNT(mp)))), \
	 (2 * (mp)->m_sb.sb_sectsize + \
	  XFS_FSB_TO_B((mp), XFS_IALLOC_BLOCKS((mp))) + \
	  XFS_FSB_TO_B((mp), XFS_IN_MAXLEVELS(mp)) + \
	  XFS_ALLOCFREE_LOG_RES(mp, 1) + \
	  (128 * (2 + XFS_IALLOC_BLOCKS(mp) + XFS_IN_MAXLEVELS(mp) + \
	   XFS_ALLOCFREE_LOG_COUNT(mp, 1))))))

#define	XFS_SYMLINK_LOG_RES(mp)	((mp)->m_reservations.tr_symlink)

#define	XFS_CALC_CREATE_LOG_RES(mp)		\
	(MAX( \
	 ((mp)->m_sb.sb_inodesize + \
	  (mp)->m_sb.sb_inodesize + \
	  (mp)->m_sb.sb_sectsize + \
	  XFS_FSB_TO_B(mp, 1) + \
	  XFS_DIROP_LOG_RES(mp) + \
	  (128 * (3 + XFS_DIROP_LOG_COUNT(mp)))), \
	 (3 * (mp)->m_sb.sb_sectsize + \
	  XFS_FSB_TO_B((mp), XFS_IALLOC_BLOCKS((mp))) + \
	  XFS_FSB_TO_B((mp), XFS_IN_MAXLEVELS(mp)) + \
	  XFS_ALLOCFREE_LOG_RES(mp, 1) + \
	  (128 * (2 + XFS_IALLOC_BLOCKS(mp) + XFS_IN_MAXLEVELS(mp) + \
	   XFS_ALLOCFREE_LOG_COUNT(mp, 1))))))

#define	XFS_CREATE_LOG_RES(mp)	((mp)->m_reservations.tr_create)

#define	XFS_CALC_MKDIR_LOG_RES(mp)	XFS_CALC_CREATE_LOG_RES(mp)

#define	XFS_MKDIR_LOG_RES(mp)	((mp)->m_reservations.tr_mkdir)

#define	XFS_CALC_IFREE_LOG_RES(mp) \
	((mp)->m_sb.sb_inodesize + \
	 (mp)->m_sb.sb_sectsize + \
	 (mp)->m_sb.sb_sectsize + \
	 XFS_FSB_TO_B((mp), 1) + \
	 MAX((__uint16_t)XFS_FSB_TO_B((mp), 1), XFS_INODE_CLUSTER_SIZE(mp)) + \
	 (128 * 5) + \
	  XFS_ALLOCFREE_LOG_RES(mp, 1) + \
	  (128 * (2 + XFS_IALLOC_BLOCKS(mp) + XFS_IN_MAXLEVELS(mp) + \
	   XFS_ALLOCFREE_LOG_COUNT(mp, 1))))


#define	XFS_IFREE_LOG_RES(mp)	((mp)->m_reservations.tr_ifree)

#define	XFS_CALC_ICHANGE_LOG_RES(mp)	((mp)->m_sb.sb_inodesize + \
					 (mp)->m_sb.sb_sectsize + 512)

#define	XFS_ICHANGE_LOG_RES(mp)	((mp)->m_reservations.tr_ichange)

#define	XFS_CALC_GROWDATA_LOG_RES(mp) \
	((mp)->m_sb.sb_sectsize * 3 + \
	 XFS_ALLOCFREE_LOG_RES(mp, 1) + \
	 (128 * (3 + XFS_ALLOCFREE_LOG_COUNT(mp, 1))))

#define	XFS_GROWDATA_LOG_RES(mp)    ((mp)->m_reservations.tr_growdata)

#define	XFS_CALC_GROWRTALLOC_LOG_RES(mp) \
	(2 * (mp)->m_sb.sb_sectsize + \
	 XFS_FSB_TO_B((mp), XFS_BM_MAXLEVELS(mp, XFS_DATA_FORK)) + \
	 (mp)->m_sb.sb_inodesize + \
	 XFS_ALLOCFREE_LOG_RES(mp, 1) + \
	 (128 * \
	  (3 + XFS_BM_MAXLEVELS(mp, XFS_DATA_FORK) + \
	   XFS_ALLOCFREE_LOG_COUNT(mp, 1))))

#define	XFS_GROWRTALLOC_LOG_RES(mp)	((mp)->m_reservations.tr_growrtalloc)

#define	XFS_CALC_GROWRTZERO_LOG_RES(mp) \
	((mp)->m_sb.sb_blocksize + 128)

#define	XFS_GROWRTZERO_LOG_RES(mp)	((mp)->m_reservations.tr_growrtzero)

#define	XFS_CALC_GROWRTFREE_LOG_RES(mp) \
	((mp)->m_sb.sb_sectsize + \
	 2 * (mp)->m_sb.sb_inodesize + \
	 (mp)->m_sb.sb_blocksize + \
	 (mp)->m_rsumsize + \
	 (128 * 5))

#define	XFS_GROWRTFREE_LOG_RES(mp)	((mp)->m_reservations.tr_growrtfree)

#define	XFS_CALC_SWRITE_LOG_RES(mp) \
	((mp)->m_sb.sb_inodesize + 128)

#define	XFS_SWRITE_LOG_RES(mp)	((mp)->m_reservations.tr_swrite)

#define XFS_FSYNC_TS_LOG_RES(mp)        ((mp)->m_reservations.tr_swrite)

#define	XFS_CALC_WRITEID_LOG_RES(mp) \
	((mp)->m_sb.sb_inodesize + 128)

#define	XFS_WRITEID_LOG_RES(mp)	((mp)->m_reservations.tr_swrite)

#define	XFS_CALC_ADDAFORK_LOG_RES(mp)	\
	((mp)->m_sb.sb_inodesize + \
	 (mp)->m_sb.sb_sectsize * 2 + \
	 (mp)->m_dirblksize + \
	 XFS_FSB_TO_B(mp, (XFS_DAENTER_BMAP1B(mp, XFS_DATA_FORK) + 1)) + \
	 XFS_ALLOCFREE_LOG_RES(mp, 1) + \
	 (128 * (4 + (XFS_DAENTER_BMAP1B(mp, XFS_DATA_FORK) + 1) + \
		 XFS_ALLOCFREE_LOG_COUNT(mp, 1))))

#define	XFS_ADDAFORK_LOG_RES(mp)	((mp)->m_reservations.tr_addafork)

#define	XFS_CALC_ATTRINVAL_LOG_RES(mp)	\
	(MAX( \
	 ((mp)->m_sb.sb_inodesize + \
	  XFS_FSB_TO_B((mp), XFS_BM_MAXLEVELS(mp, XFS_ATTR_FORK)) + \
	  (128 * (1 + XFS_BM_MAXLEVELS(mp, XFS_ATTR_FORK)))), \
	 ((4 * (mp)->m_sb.sb_sectsize) + \
	  (4 * (mp)->m_sb.sb_sectsize) + \
	  (mp)->m_sb.sb_sectsize + \
	  XFS_ALLOCFREE_LOG_RES(mp, 4) + \
	  (128 * (9 + XFS_ALLOCFREE_LOG_COUNT(mp, 4))))))

#define	XFS_ATTRINVAL_LOG_RES(mp)	((mp)->m_reservations.tr_attrinval)

#define	XFS_CALC_ATTRSET_LOG_RES(mp)	\
	((mp)->m_sb.sb_inodesize + \
	 (mp)->m_sb.sb_sectsize + \
	  XFS_FSB_TO_B((mp), XFS_DA_NODE_MAXDEPTH) + \
	  (128 * (2 + XFS_DA_NODE_MAXDEPTH)))

#define	XFS_ATTRSET_LOG_RES(mp, ext)	\
	((mp)->m_reservations.tr_attrset + \
	 (ext * (mp)->m_sb.sb_sectsize) + \
	 (ext * XFS_FSB_TO_B((mp), XFS_BM_MAXLEVELS(mp, XFS_ATTR_FORK))) + \
	 (128 * (ext + (ext * XFS_BM_MAXLEVELS(mp, XFS_ATTR_FORK)))))

#define	XFS_CALC_ATTRRM_LOG_RES(mp)	\
	(MAX( \
	  ((mp)->m_sb.sb_inodesize + \
	  XFS_FSB_TO_B((mp), XFS_DA_NODE_MAXDEPTH) + \
	  XFS_FSB_TO_B((mp), XFS_BM_MAXLEVELS(mp, XFS_ATTR_FORK)) + \
	  (128 * (1 + XFS_DA_NODE_MAXDEPTH + XFS_BM_MAXLEVELS(mp, XFS_DATA_FORK)))), \
	 ((2 * (mp)->m_sb.sb_sectsize) + \
	  (2 * (mp)->m_sb.sb_sectsize) + \
	  (mp)->m_sb.sb_sectsize + \
	  XFS_ALLOCFREE_LOG_RES(mp, 2) + \
	  (128 * (5 + XFS_ALLOCFREE_LOG_COUNT(mp, 2))))))

#define	XFS_ATTRRM_LOG_RES(mp)	((mp)->m_reservations.tr_attrrm)

#define	XFS_CALC_CLEAR_AGI_BUCKET_LOG_RES(mp) \
	((mp)->m_sb.sb_sectsize + 128)

#define	XFS_CLEAR_AGI_BUCKET_LOG_RES(mp)  ((mp)->m_reservations.tr_clearagi)


#define	XFS_DEFAULT_LOG_COUNT		1
#define	XFS_DEFAULT_PERM_LOG_COUNT	2
#define	XFS_ITRUNCATE_LOG_COUNT		2
#define XFS_INACTIVE_LOG_COUNT		2
#define	XFS_CREATE_LOG_COUNT		2
#define	XFS_MKDIR_LOG_COUNT		3
#define	XFS_SYMLINK_LOG_COUNT		3
#define	XFS_REMOVE_LOG_COUNT		2
#define	XFS_LINK_LOG_COUNT		2
#define	XFS_RENAME_LOG_COUNT		2
#define	XFS_WRITE_LOG_COUNT		2
#define	XFS_ADDAFORK_LOG_COUNT		2
#define	XFS_ATTRINVAL_LOG_COUNT		1
#define	XFS_ATTRSET_LOG_COUNT		3
#define	XFS_ATTRRM_LOG_COUNT		3

#define	XFS_AGF_REF		4
#define	XFS_AGI_REF		4
#define	XFS_AGFL_REF		3
#define	XFS_INO_BTREE_REF	3
#define	XFS_ALLOC_BTREE_REF	2
#define	XFS_BMAP_BTREE_REF	2
#define	XFS_DIR_BTREE_REF	2
#define	XFS_ATTR_BTREE_REF	1
#define	XFS_INO_REF		1
#define	XFS_DQUOT_REF		1

#ifdef __KERNEL__

struct xfs_buf;
struct xfs_buftarg;
struct xfs_efd_log_item;
struct xfs_efi_log_item;
struct xfs_inode;
struct xfs_item_ops;
struct xfs_log_iovec;
struct xfs_log_item_desc;
struct xfs_mount;
struct xfs_trans;
struct xfs_dquot_acct;

typedef struct xfs_log_item {
	struct list_head		li_ail;		/* AIL pointers */
	xfs_lsn_t			li_lsn;		/* last on-disk lsn */
	struct xfs_log_item_desc	*li_desc;	/* ptr to current desc*/
	struct xfs_mount		*li_mountp;	/* ptr to fs mount */
	struct xfs_ail			*li_ailp;	/* ptr to AIL */
	uint				li_type;	/* item type */
	uint				li_flags;	/* misc flags */
	struct xfs_log_item		*li_bio_list;	/* buffer item list */
	void				(*li_cb)(struct xfs_buf *,
						 struct xfs_log_item *);
							/* buffer item iodone */
							/* callback func */
	struct xfs_item_ops		*li_ops;	/* function list */
} xfs_log_item_t;

#define	XFS_LI_IN_AIL	0x1
#define XFS_LI_ABORTED	0x2

typedef struct xfs_item_ops {
	uint (*iop_size)(xfs_log_item_t *);
	void (*iop_format)(xfs_log_item_t *, struct xfs_log_iovec *);
	void (*iop_pin)(xfs_log_item_t *);
	void (*iop_unpin)(xfs_log_item_t *, int);
	void (*iop_unpin_remove)(xfs_log_item_t *, struct xfs_trans *);
	uint (*iop_trylock)(xfs_log_item_t *);
	void (*iop_unlock)(xfs_log_item_t *);
	xfs_lsn_t (*iop_committed)(xfs_log_item_t *, xfs_lsn_t);
	void (*iop_push)(xfs_log_item_t *);
	void (*iop_pushbuf)(xfs_log_item_t *);
	void (*iop_committing)(xfs_log_item_t *, xfs_lsn_t);
} xfs_item_ops_t;

#define IOP_SIZE(ip)		(*(ip)->li_ops->iop_size)(ip)
#define IOP_FORMAT(ip,vp)	(*(ip)->li_ops->iop_format)(ip, vp)
#define IOP_PIN(ip)		(*(ip)->li_ops->iop_pin)(ip)
#define IOP_UNPIN(ip, flags)	(*(ip)->li_ops->iop_unpin)(ip, flags)
#define IOP_UNPIN_REMOVE(ip,tp) (*(ip)->li_ops->iop_unpin_remove)(ip, tp)
#define IOP_TRYLOCK(ip)		(*(ip)->li_ops->iop_trylock)(ip)
#define IOP_UNLOCK(ip)		(*(ip)->li_ops->iop_unlock)(ip)
#define IOP_COMMITTED(ip, lsn)	(*(ip)->li_ops->iop_committed)(ip, lsn)
#define IOP_PUSH(ip)		(*(ip)->li_ops->iop_push)(ip)
#define IOP_PUSHBUF(ip)		(*(ip)->li_ops->iop_pushbuf)(ip)
#define IOP_COMMITTING(ip, lsn) (*(ip)->li_ops->iop_committing)(ip, lsn)

#define	XFS_ITEM_SUCCESS	0
#define	XFS_ITEM_PINNED		1
#define	XFS_ITEM_LOCKED		2
#define	XFS_ITEM_FLUSHING	3
#define XFS_ITEM_PUSHBUF	4


typedef struct xfs_log_busy_slot {
	xfs_agnumber_t		lbc_ag;
	ushort			lbc_idx;	/* index in perag.busy[] */
} xfs_log_busy_slot_t;

#define XFS_LBC_NUM_SLOTS	31
typedef struct xfs_log_busy_chunk {
	struct xfs_log_busy_chunk	*lbc_next;
	uint				lbc_free;	/* free slots bitmask */
	ushort				lbc_unused;	/* first unused */
	xfs_log_busy_slot_t		lbc_busy[XFS_LBC_NUM_SLOTS];
} xfs_log_busy_chunk_t;

#define	XFS_LBC_MAX_SLOT	(XFS_LBC_NUM_SLOTS - 1)
#define	XFS_LBC_FREEMASK	((1U << XFS_LBC_NUM_SLOTS) - 1)

#define	XFS_LBC_INIT(cp)	((cp)->lbc_free = XFS_LBC_FREEMASK)
#define	XFS_LBC_CLAIM(cp, slot)	((cp)->lbc_free &= ~(1 << (slot)))
#define	XFS_LBC_SLOT(cp, slot)	(&((cp)->lbc_busy[(slot)]))
#define	XFS_LBC_VACANCY(cp)	(((cp)->lbc_free) & XFS_LBC_FREEMASK)
#define	XFS_LBC_ISFREE(cp, slot) ((cp)->lbc_free & (1 << (slot)))

typedef void (*xfs_trans_callback_t)(struct xfs_trans *, void *);

typedef struct xfs_trans {
	unsigned int		t_magic;	/* magic number */
	xfs_log_callback_t	t_logcb;	/* log callback struct */
	unsigned int		t_type;		/* transaction type */
	unsigned int		t_log_res;	/* amt of log space resvd */
	unsigned int		t_log_count;	/* count for perm log res */
	unsigned int		t_blk_res;	/* # of blocks resvd */
	unsigned int		t_blk_res_used;	/* # of resvd blocks used */
	unsigned int		t_rtx_res;	/* # of rt extents resvd */
	unsigned int		t_rtx_res_used;	/* # of resvd rt extents used */
	xfs_log_ticket_t	t_ticket;	/* log mgr ticket */
	xfs_lsn_t		t_lsn;		/* log seq num of start of
						 * transaction. */
	xfs_lsn_t		t_commit_lsn;	/* log seq num of end of
						 * transaction. */
	struct xfs_mount	*t_mountp;	/* ptr to fs mount struct */
	struct xfs_dquot_acct   *t_dqinfo;	/* acctg info for dquots */
	xfs_trans_callback_t	t_callback;	/* transaction callback */
	void			*t_callarg;	/* callback arg */
	unsigned int		t_flags;	/* misc flags */
	int64_t			t_icount_delta;	/* superblock icount change */
	int64_t			t_ifree_delta;	/* superblock ifree change */
	int64_t			t_fdblocks_delta; /* superblock fdblocks chg */
	int64_t			t_res_fdblocks_delta; /* on-disk only chg */
	int64_t			t_frextents_delta;/* superblock freextents chg*/
	int64_t			t_res_frextents_delta; /* on-disk only chg */
#ifdef DEBUG
	int64_t			t_ag_freeblks_delta; /* debugging counter */
	int64_t			t_ag_flist_delta; /* debugging counter */
	int64_t			t_ag_btree_delta; /* debugging counter */
#endif
	int64_t			t_dblocks_delta;/* superblock dblocks change */
	int64_t			t_agcount_delta;/* superblock agcount change */
	int64_t			t_imaxpct_delta;/* superblock imaxpct change */
	int64_t			t_rextsize_delta;/* superblock rextsize chg */
	int64_t			t_rbmblocks_delta;/* superblock rbmblocks chg */
	int64_t			t_rblocks_delta;/* superblock rblocks change */
	int64_t			t_rextents_delta;/* superblocks rextents chg */
	int64_t			t_rextslog_delta;/* superblocks rextslog chg */
	unsigned int		t_items_free;	/* log item descs free */
	xfs_log_item_chunk_t	t_items;	/* first log item desc chunk */
	xfs_trans_header_t	t_header;	/* header for in-log trans */
	unsigned int		t_busy_free;	/* busy descs free */
	xfs_log_busy_chunk_t	t_busy;		/* busy/async free blocks */
	unsigned long		t_pflags;	/* saved process flags state */
} xfs_trans_t;

#define	xfs_trans_get_log_res(tp)	((tp)->t_log_res)
#define	xfs_trans_get_log_count(tp)	((tp)->t_log_count)
#define	xfs_trans_get_block_res(tp)	((tp)->t_blk_res)
#define	xfs_trans_set_sync(tp)		((tp)->t_flags |= XFS_TRANS_SYNC)

#ifdef DEBUG
#define	xfs_trans_agblocks_delta(tp, d)	((tp)->t_ag_freeblks_delta += (int64_t)d)
#define	xfs_trans_agflist_delta(tp, d)	((tp)->t_ag_flist_delta += (int64_t)d)
#define	xfs_trans_agbtree_delta(tp, d)	((tp)->t_ag_btree_delta += (int64_t)d)
#else
#define	xfs_trans_agblocks_delta(tp, d)
#define	xfs_trans_agflist_delta(tp, d)
#define	xfs_trans_agbtree_delta(tp, d)
#endif

xfs_trans_t	*xfs_trans_alloc(struct xfs_mount *, uint);
xfs_trans_t	*_xfs_trans_alloc(struct xfs_mount *, uint);
xfs_trans_t	*xfs_trans_dup(xfs_trans_t *);
int		xfs_trans_reserve(xfs_trans_t *, uint, uint, uint,
				  uint, uint);
void		xfs_trans_mod_sb(xfs_trans_t *, uint, int64_t);
struct xfs_buf	*xfs_trans_get_buf(xfs_trans_t *, struct xfs_buftarg *, xfs_daddr_t,
				   int, uint);
int		xfs_trans_read_buf(struct xfs_mount *, xfs_trans_t *,
				   struct xfs_buftarg *, xfs_daddr_t, int, uint,
				   struct xfs_buf **);
struct xfs_buf	*xfs_trans_getsb(xfs_trans_t *, struct xfs_mount *, int);

void		xfs_trans_brelse(xfs_trans_t *, struct xfs_buf *);
void		xfs_trans_bjoin(xfs_trans_t *, struct xfs_buf *);
void		xfs_trans_bhold(xfs_trans_t *, struct xfs_buf *);
void		xfs_trans_bhold_release(xfs_trans_t *, struct xfs_buf *);
void		xfs_trans_binval(xfs_trans_t *, struct xfs_buf *);
void		xfs_trans_inode_buf(xfs_trans_t *, struct xfs_buf *);
void		xfs_trans_stale_inode_buf(xfs_trans_t *, struct xfs_buf *);
void		xfs_trans_dquot_buf(xfs_trans_t *, struct xfs_buf *, uint);
void		xfs_trans_inode_alloc_buf(xfs_trans_t *, struct xfs_buf *);
int		xfs_trans_iget(struct xfs_mount *, xfs_trans_t *,
			       xfs_ino_t , uint, uint, struct xfs_inode **);
void		xfs_trans_ijoin(xfs_trans_t *, struct xfs_inode *, uint);
void		xfs_trans_ihold(xfs_trans_t *, struct xfs_inode *);
void		xfs_trans_log_buf(xfs_trans_t *, struct xfs_buf *, uint, uint);
void		xfs_trans_log_inode(xfs_trans_t *, struct xfs_inode *, uint);
struct xfs_efi_log_item	*xfs_trans_get_efi(xfs_trans_t *, uint);
void		xfs_efi_release(struct xfs_efi_log_item *, uint);
void		xfs_trans_log_efi_extent(xfs_trans_t *,
					 struct xfs_efi_log_item *,
					 xfs_fsblock_t,
					 xfs_extlen_t);
struct xfs_efd_log_item	*xfs_trans_get_efd(xfs_trans_t *,
				  struct xfs_efi_log_item *,
				  uint);
void		xfs_trans_log_efd_extent(xfs_trans_t *,
					 struct xfs_efd_log_item *,
					 xfs_fsblock_t,
					 xfs_extlen_t);
int		_xfs_trans_commit(xfs_trans_t *,
				  uint flags,
				  int *);
#define xfs_trans_commit(tp, flags)	_xfs_trans_commit(tp, flags, NULL)
void		xfs_trans_cancel(xfs_trans_t *, int);
int		xfs_trans_ail_init(struct xfs_mount *);
void		xfs_trans_ail_destroy(struct xfs_mount *);
xfs_log_busy_slot_t *xfs_trans_add_busy(xfs_trans_t *tp,
					xfs_agnumber_t ag,
					xfs_extlen_t idx);

extern kmem_zone_t	*xfs_trans_zone;

#endif	/* __KERNEL__ */

void		xfs_trans_init(struct xfs_mount *);
int		xfs_trans_roll(struct xfs_trans **, struct xfs_inode *);

#endif	/* __XFS_TRANS_H__ */
