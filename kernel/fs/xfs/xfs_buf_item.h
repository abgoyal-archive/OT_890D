
#ifndef	__XFS_BUF_ITEM_H__
#define	__XFS_BUF_ITEM_H__

extern kmem_zone_t	*xfs_buf_item_zone;

typedef struct xfs_buf_log_format_t {
	unsigned short	blf_type;	/* buf log item type indicator */
	unsigned short	blf_size;	/* size of this item */
	ushort		blf_flags;	/* misc state */
	ushort		blf_len;	/* number of blocks in this buf */
	__int64_t	blf_blkno;	/* starting blkno of this buf */
	unsigned int	blf_map_size;	/* size of data bitmap in words */
	unsigned int	blf_data_map[1];/* variable size bitmap of */
					/*   regions of buffer in this item */
} xfs_buf_log_format_t;

#define	XFS_BLI_INODE_BUF	0x1
#define	XFS_BLI_CANCEL		0x2
#define	XFS_BLI_UDQUOT_BUF	0x4
#define XFS_BLI_PDQUOT_BUF	0x8
#define	XFS_BLI_GDQUOT_BUF	0x10

#define	XFS_BLI_CHUNK		128
#define	XFS_BLI_SHIFT		7
#define	BIT_TO_WORD_SHIFT	5
#define	NBWORD			(NBBY * sizeof(unsigned int))

#define	XFS_BLI_HOLD		0x01
#define	XFS_BLI_DIRTY		0x02
#define	XFS_BLI_STALE		0x04
#define	XFS_BLI_LOGGED		0x08
#define	XFS_BLI_INODE_ALLOC_BUF	0x10
#define XFS_BLI_STALE_INODE	0x20


#ifdef __KERNEL__

struct xfs_buf;
struct ktrace;
struct xfs_mount;
struct xfs_buf_log_item;

#if defined(XFS_BLI_TRACE)
#define	XFS_BLI_TRACE_SIZE	32

void	xfs_buf_item_trace(char *, struct xfs_buf_log_item *);
#else
#define	xfs_buf_item_trace(id, bip)
#endif

typedef struct xfs_buf_log_item {
	xfs_log_item_t		bli_item;	/* common item structure */
	struct xfs_buf		*bli_buf;	/* real buffer pointer */
	unsigned int		bli_flags;	/* misc flags */
	unsigned int		bli_recur;	/* lock recursion count */
	atomic_t		bli_refcount;	/* cnt of tp refs */
#ifdef XFS_BLI_TRACE
	struct ktrace		*bli_trace;	/* event trace buf */
#endif
#ifdef XFS_TRANS_DEBUG
	char			*bli_orig;	/* original buffer copy */
	char			*bli_logged;	/* bytes logged (bitmap) */
#endif
	xfs_buf_log_format_t	bli_format;	/* in-log header */
} xfs_buf_log_item_t;

typedef struct xfs_buf_cancel {
	xfs_daddr_t		bc_blkno;
	uint			bc_len;
	int			bc_refcount;
	struct xfs_buf_cancel	*bc_next;
} xfs_buf_cancel_t;

void	xfs_buf_item_init(struct xfs_buf *, struct xfs_mount *);
void	xfs_buf_item_relse(struct xfs_buf *);
void	xfs_buf_item_log(xfs_buf_log_item_t *, uint, uint);
uint	xfs_buf_item_dirty(xfs_buf_log_item_t *);
void	xfs_buf_attach_iodone(struct xfs_buf *,
			      void(*)(struct xfs_buf *, xfs_log_item_t *),
			      xfs_log_item_t *);
void	xfs_buf_iodone_callbacks(struct xfs_buf *);
void	xfs_buf_iodone(struct xfs_buf *, xfs_buf_log_item_t *);

#ifdef XFS_TRANS_DEBUG
void
xfs_buf_item_flush_log_debug(
	struct xfs_buf *bp,
	uint	first,
	uint	last);
#else
#define	xfs_buf_item_flush_log_debug(bp, first, last)
#endif

#endif	/* __KERNEL__ */

#endif	/* __XFS_BUF_ITEM_H__ */
