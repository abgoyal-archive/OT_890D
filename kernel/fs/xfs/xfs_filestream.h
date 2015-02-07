
#ifndef __XFS_FILESTREAM_H__
#define __XFS_FILESTREAM_H__

#ifdef __KERNEL__

struct xfs_mount;
struct xfs_inode;
struct xfs_perag;
struct xfs_bmalloca;

#ifdef XFS_FILESTREAMS_TRACE
#define XFS_FSTRM_KTRACE_INFO		1
#define XFS_FSTRM_KTRACE_AGSCAN		2
#define XFS_FSTRM_KTRACE_AGPICK1	3
#define XFS_FSTRM_KTRACE_AGPICK2	4
#define XFS_FSTRM_KTRACE_UPDATE		5
#define XFS_FSTRM_KTRACE_FREE		6
#define	XFS_FSTRM_KTRACE_ITEM_LOOKUP	7
#define	XFS_FSTRM_KTRACE_ASSOCIATE	8
#define	XFS_FSTRM_KTRACE_MOVEAG		9
#define	XFS_FSTRM_KTRACE_ORPHAN		10

#define XFS_FSTRM_KTRACE_SIZE	16384
extern ktrace_t *xfs_filestreams_trace_buf;

#endif

STATIC_INLINE int
xfs_filestream_peek_ag(
	xfs_mount_t	*mp,
	xfs_agnumber_t	agno)
{
	return atomic_read(&mp->m_perag[agno].pagf_fstrms);
}

STATIC_INLINE int
xfs_filestream_get_ag(
	xfs_mount_t	*mp,
	xfs_agnumber_t	agno)
{
	return atomic_inc_return(&mp->m_perag[agno].pagf_fstrms);
}

STATIC_INLINE int
xfs_filestream_put_ag(
	xfs_mount_t	*mp,
	xfs_agnumber_t	agno)
{
	return atomic_dec_return(&mp->m_perag[agno].pagf_fstrms);
}

/* allocation selection flags */
typedef enum xfs_fstrm_alloc {
	XFS_PICK_USERDATA = 1,
	XFS_PICK_LOWSPACE = 2,
} xfs_fstrm_alloc_t;

/* prototypes for filestream.c */
int xfs_filestream_init(void);
void xfs_filestream_uninit(void);
int xfs_filestream_mount(struct xfs_mount *mp);
void xfs_filestream_unmount(struct xfs_mount *mp);
void xfs_filestream_flush(struct xfs_mount *mp);
xfs_agnumber_t xfs_filestream_lookup_ag(struct xfs_inode *ip);
int xfs_filestream_associate(struct xfs_inode *dip, struct xfs_inode *ip);
void xfs_filestream_deassociate(struct xfs_inode *ip);
int xfs_filestream_new_ag(struct xfs_bmalloca *ap, xfs_agnumber_t *agp);


/* filestreams for the inode? */
STATIC_INLINE int
xfs_inode_is_filestream(
	struct xfs_inode	*ip)
{
	return (ip->i_mount->m_flags & XFS_MOUNT_FILESTREAMS) ||
		xfs_iflags_test(ip, XFS_IFILESTREAM) ||
		(ip->i_d.di_flags & XFS_DIFLAG_FILESTREAM);
}

#endif /* __KERNEL__ */

#endif /* __XFS_FILESTREAM_H__ */
