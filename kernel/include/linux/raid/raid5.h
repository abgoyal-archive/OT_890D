
#ifndef _RAID5_H
#define _RAID5_H

#include <linux/raid/md.h>
#include <linux/raid/xor.h>


enum check_states {
	check_state_idle = 0,
	check_state_run, /* parity check */
	check_state_check_result,
	check_state_compute_run, /* parity repair */
	check_state_compute_result,
};

enum reconstruct_states {
	reconstruct_state_idle = 0,
	reconstruct_state_prexor_drain_run,	/* prexor-write */
	reconstruct_state_drain_run,		/* write */
	reconstruct_state_run,			/* expand */
	reconstruct_state_prexor_drain_result,
	reconstruct_state_drain_result,
	reconstruct_state_result,
};

struct stripe_head {
	struct hlist_node	hash;
	struct list_head	lru;			/* inactive_list or handle_list */
	struct raid5_private_data	*raid_conf;
	sector_t		sector;			/* sector of this row */
	int			pd_idx;			/* parity disk index */
	unsigned long		state;			/* state flags */
	atomic_t		count;			/* nr of active thread/requests */
	spinlock_t		lock;
	int			bm_seq;	/* sequence number for bitmap flushes */
	int			disks;			/* disks in stripe */
	enum check_states	check_state;
	enum reconstruct_states reconstruct_state;
	/* stripe_operations
	 * @target - STRIPE_OP_COMPUTE_BLK target
	 */
	struct stripe_operations {
		int		   target;
		u32		   zero_sum_result;
	} ops;
	struct r5dev {
		struct bio	req;
		struct bio_vec	vec;
		struct page	*page;
		struct bio	*toread, *read, *towrite, *written;
		sector_t	sector;			/* sector of this page */
		unsigned long	flags;
	} dev[1]; /* allocated with extra space depending of RAID geometry */
};

struct stripe_head_state {
	int syncing, expanding, expanded;
	int locked, uptodate, to_read, to_write, failed, written;
	int to_fill, compute, req_compute, non_overwrite;
	int failed_num;
	unsigned long ops_request;
};

/* r6_state - extra state data only relevant to r6 */
struct r6_state {
	int p_failed, q_failed, qd_idx, failed_num[2];
};

/* Flags */
#define	R5_UPTODATE	0	/* page contains current data */
#define	R5_LOCKED	1	/* IO has been submitted on "req" */
#define	R5_OVERWRITE	2	/* towrite covers whole page */
/* and some that are internal to handle_stripe */
#define	R5_Insync	3	/* rdev && rdev->in_sync at start */
#define	R5_Wantread	4	/* want to schedule a read */
#define	R5_Wantwrite	5
#define	R5_Overlap	7	/* There is a pending overlapping request on this block */
#define	R5_ReadError	8	/* seen a read error here recently */
#define	R5_ReWrite	9	/* have tried to over-write the readerror */

#define	R5_Expanded	10	/* This block now has post-expand data */
#define	R5_Wantcompute	11 /* compute_block in progress treat as
				    * uptodate
				    */
#define	R5_Wantfill	12 /* dev->toread contains a bio that needs
				    * filling
				    */
#define R5_Wantdrain	13 /* dev->towrite needs to be drained */
#define RECONSTRUCT_WRITE	1
#define READ_MODIFY_WRITE	2
/* not a write method, but a compute_parity mode */
#define	CHECK_PARITY		3

#define STRIPE_HANDLE		2
#define	STRIPE_SYNCING		3
#define	STRIPE_INSYNC		4
#define	STRIPE_PREREAD_ACTIVE	5
#define	STRIPE_DELAYED		6
#define	STRIPE_DEGRADED		7
#define	STRIPE_BIT_DELAY	8
#define	STRIPE_EXPANDING	9
#define	STRIPE_EXPAND_SOURCE	10
#define	STRIPE_EXPAND_READY	11
#define	STRIPE_IO_STARTED	12 /* do not count towards 'bypass_count' */
#define	STRIPE_FULL_WRITE	13 /* all blocks are set to be overwritten */
#define	STRIPE_BIOFILL_RUN	14
#define	STRIPE_COMPUTE_RUN	15
#define STRIPE_OP_BIOFILL	0
#define STRIPE_OP_COMPUTE_BLK	1
#define STRIPE_OP_PREXOR	2
#define STRIPE_OP_BIODRAIN	3
#define STRIPE_OP_POSTXOR	4
#define STRIPE_OP_CHECK	5

 

struct disk_info {
	mdk_rdev_t	*rdev;
};

struct raid5_private_data {
	struct hlist_head	*stripe_hashtbl;
	mddev_t			*mddev;
	struct disk_info	*spare;
	int			chunk_size, level, algorithm;
	int			max_degraded;
	int			raid_disks;
	int			max_nr_stripes;

	/* used during an expand */
	sector_t		expand_progress;	/* MaxSector when no expand happening */
	sector_t		expand_lo; /* from here up to expand_progress it out-of-bounds
					    * as we haven't flushed the metadata yet
					    */
	int			previous_raid_disks;

	struct list_head	handle_list; /* stripes needing handling */
	struct list_head	hold_list; /* preread ready stripes */
	struct list_head	delayed_list; /* stripes that have plugged requests */
	struct list_head	bitmap_list; /* stripes delaying awaiting bitmap update */
	struct bio		*retry_read_aligned; /* currently retrying aligned bios   */
	struct bio		*retry_read_aligned_list; /* aligned bios retry list  */
	atomic_t		preread_active_stripes; /* stripes with scheduled io */
	atomic_t		active_aligned_reads;
	atomic_t		pending_full_writes; /* full write backlog */
	int			bypass_count; /* bypassed prereads */
	int			bypass_threshold; /* preread nice */
	struct list_head	*last_hold; /* detect hold_list promotions */

	atomic_t		reshape_stripes; /* stripes with pending writes for reshape */
	/* unfortunately we need two cache names as we temporarily have
	 * two caches.
	 */
	int			active_name;
	char			cache_name[2][20];
	struct kmem_cache		*slab_cache; /* for allocating stripes */

	int			seq_flush, seq_write;
	int			quiesce;

	int			fullsync;  /* set to 1 if a full sync is needed,
					    * (fresh device added).
					    * Cleared when a sync completes.
					    */

	struct page 		*spare_page; /* Used when checking P/Q in raid6 */

	/*
	 * Free stripes pool
	 */
	atomic_t		active_stripes;
	struct list_head	inactive_list;
	wait_queue_head_t	wait_for_stripe;
	wait_queue_head_t	wait_for_overlap;
	int			inactive_blocked;	/* release of inactive stripes blocked,
							 * waiting for 25% to be free
							 */
	int			pool_size; /* number of disks in stripeheads in pool */
	spinlock_t		device_lock;
	struct disk_info	*disks;
};

typedef struct raid5_private_data raid5_conf_t;

#define mddev_to_conf(mddev) ((raid5_conf_t *) mddev->private)

#define ALGORITHM_LEFT_ASYMMETRIC	0
#define ALGORITHM_RIGHT_ASYMMETRIC	1
#define ALGORITHM_LEFT_SYMMETRIC	2
#define ALGORITHM_RIGHT_SYMMETRIC	3

#endif
