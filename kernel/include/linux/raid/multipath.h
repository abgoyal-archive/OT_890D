
#ifndef _MULTIPATH_H
#define _MULTIPATH_H

#include <linux/raid/md.h>

struct multipath_info {
	mdk_rdev_t	*rdev;
};

struct multipath_private_data {
	mddev_t			*mddev;
	struct multipath_info	*multipaths;
	int			raid_disks;
	int			working_disks;
	spinlock_t		device_lock;
	struct list_head	retry_list;

	mempool_t		*pool;
};

typedef struct multipath_private_data multipath_conf_t;

#define mddev_to_conf(mddev) ((multipath_conf_t *) mddev->private)


struct multipath_bh {
	mddev_t			*mddev;
	struct bio		*master_bio;
	struct bio		bio;
	int			path;
	struct list_head	retry_list;
};
#endif
