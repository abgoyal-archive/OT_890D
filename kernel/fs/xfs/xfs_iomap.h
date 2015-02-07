
#ifndef __XFS_IOMAP_H__
#define __XFS_IOMAP_H__

#define IOMAP_DADDR_NULL ((xfs_daddr_t) (-1LL))


typedef enum {				/* iomap_flags values */
	IOMAP_READ =		0,	/* mapping for a read */
	IOMAP_HOLE =		0x02,	/* mapping covers a hole  */
	IOMAP_DELAY =		0x04,	/* mapping covers delalloc region  */
	IOMAP_REALTIME =	0x10,	/* mapping on the realtime device  */
	IOMAP_UNWRITTEN =	0x20,	/* mapping covers allocated */
					/* but uninitialized file data  */
	IOMAP_NEW =		0x40	/* just allocate */
} iomap_flags_t;

typedef enum {
	/* base extent manipulation calls */
	BMAPI_READ = (1 << 0),		/* read extents */
	BMAPI_WRITE = (1 << 1),		/* create extents */
	BMAPI_ALLOCATE = (1 << 2),	/* delayed allocate to real extents */
	/* modifiers */
	BMAPI_IGNSTATE = (1 << 4),	/* ignore unwritten state on read */
	BMAPI_DIRECT = (1 << 5),	/* direct instead of buffered write */
	BMAPI_MMAP = (1 << 6),		/* allocate for mmap write */
	BMAPI_SYNC = (1 << 7),		/* sync write to flush delalloc space */
	BMAPI_TRYLOCK = (1 << 8),	/* non-blocking request */
} bmapi_flags_t;



typedef struct xfs_iomap {
	xfs_daddr_t		iomap_bn;	/* first 512b blk of mapping */
	xfs_buftarg_t		*iomap_target;
	xfs_off_t		iomap_offset;	/* offset of mapping, bytes */
	xfs_off_t		iomap_bsize;	/* size of mapping, bytes */
	xfs_off_t		iomap_delta;	/* offset into mapping, bytes */
	iomap_flags_t		iomap_flags;
} xfs_iomap_t;

struct xfs_inode;
struct xfs_bmbt_irec;

extern int xfs_iomap(struct xfs_inode *, xfs_off_t, ssize_t, int,
		     struct xfs_iomap *, int *);
extern int xfs_iomap_write_direct(struct xfs_inode *, xfs_off_t, size_t,
				  int, struct xfs_bmbt_irec *, int *, int);
extern int xfs_iomap_write_delay(struct xfs_inode *, xfs_off_t, size_t, int,
				 struct xfs_bmbt_irec *, int *);
extern int xfs_iomap_write_allocate(struct xfs_inode *, xfs_off_t, size_t,
				struct xfs_bmbt_irec *, int *);
extern int xfs_iomap_write_unwritten(struct xfs_inode *, xfs_off_t, size_t);

#endif /* __XFS_IOMAP_H__*/
