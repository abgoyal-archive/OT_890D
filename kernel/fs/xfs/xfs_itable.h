
#ifndef __XFS_ITABLE_H__
#define	__XFS_ITABLE_H__

typedef int (*bulkstat_one_pf)(struct xfs_mount	*mp,
			       xfs_ino_t	ino,
			       void		__user *buffer,
			       int		ubsize,
			       void		*private_data,
			       xfs_daddr_t	bno,
			       int		*ubused,
			       void		*dip,
			       int		*stat);

#define BULKSTAT_RV_NOTHING	0
#define BULKSTAT_RV_DIDONE	1
#define BULKSTAT_RV_GIVEUP	2

#define BULKSTAT_FG_IGET	0x1	/* Go through the buffer cache */
#define BULKSTAT_FG_QUICK	0x2	/* No iget, walk the dinode cluster */
#define BULKSTAT_FG_INLINE	0x4	/* No iget if inline attrs */

int					/* error status */
xfs_bulkstat(
	xfs_mount_t	*mp,		/* mount point for filesystem */
	xfs_ino_t	*lastino,	/* last inode returned */
	int		*count,		/* size of buffer/count returned */
	bulkstat_one_pf formatter,	/* func that'd fill a single buf */
	void		*private_data,	/* private data for formatter */
	size_t		statstruct_size,/* sizeof struct that we're filling */
	char		__user *ubuffer,/* buffer with inode stats */
	int		flags,		/* flag to control access method */
	int		*done);		/* 1 if there are more stats to get */

int
xfs_bulkstat_single(
	xfs_mount_t		*mp,
	xfs_ino_t		*lastinop,
	char			__user *buffer,
	int			*done);

typedef int (*bulkstat_one_fmt_pf)(  /* used size in bytes or negative error */
	void			__user *ubuffer, /* buffer to write to */
	int			ubsize,		 /* remaining user buffer sz */
	int			*ubused,	 /* bytes used by formatter */
	const xfs_bstat_t	*buffer);        /* buffer to read from */

int
xfs_bulkstat_one_int(
	xfs_mount_t		*mp,
	xfs_ino_t		ino,
	void			__user *buffer,
	int			ubsize,
	bulkstat_one_fmt_pf	formatter,
	xfs_daddr_t		bno,
	int			*ubused,
	void			*dibuff,
	int			*stat);

int
xfs_bulkstat_one(
	xfs_mount_t		*mp,
	xfs_ino_t		ino,
	void			__user *buffer,
	int			ubsize,
	void			*private_data,
	xfs_daddr_t		bno,
	int			*ubused,
	void			*dibuff,
	int			*stat);

int
xfs_internal_inum(
	xfs_mount_t		*mp,
	xfs_ino_t		ino);

typedef int (*inumbers_fmt_pf)(
	void			__user *ubuffer, /* buffer to write to */
	const xfs_inogrp_t	*buffer,	/* buffer to read from */
	long			count,		/* # of elements to read */
	long			*written);	/* # of bytes written */

int
xfs_inumbers_fmt(
	void			__user *ubuffer, /* buffer to write to */
	const xfs_inogrp_t	*buffer,	/* buffer to read from */
	long			count,		/* # of elements to read */
	long			*written);	/* # of bytes written */

int					/* error status */
xfs_inumbers(
	xfs_mount_t		*mp,	/* mount point for filesystem */
	xfs_ino_t		*last,	/* last inode returned */
	int			*count,	/* size of buffer/count returned */
	void			__user *buffer, /* buffer with inode info */
	inumbers_fmt_pf		formatter);

#endif	/* __XFS_ITABLE_H__ */
