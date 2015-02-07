

#include <linux/kthread.h>
#include <linux/raid/md.h>
#include <linux/raid/bitmap.h>
#include <linux/sysctl.h>
#include <linux/buffer_head.h> /* for invalidate_bdev */
#include <linux/poll.h>
#include <linux/ctype.h>
#include <linux/hdreg.h>
#include <linux/proc_fs.h>
#include <linux/random.h>
#include <linux/reboot.h>
#include <linux/file.h>
#include <linux/delay.h>

#define MAJOR_NR MD_MAJOR

/* 63 partitions with the alternate major number (mdp) */
#define MdpMinorShift 6

#define DEBUG 0
#define dprintk(x...) ((void)(DEBUG && printk(x)))


#ifndef MODULE
static void autostart_arrays(int part);
#endif

static LIST_HEAD(pers_list);
static DEFINE_SPINLOCK(pers_lock);

static void md_print_devices(void);

static DECLARE_WAIT_QUEUE_HEAD(resync_wait);

#define MD_BUG(x...) { printk("md: bug in file %s, line %d\n", __FILE__, __LINE__); md_print_devices(); }


static int sysctl_speed_limit_min = 1000;
static int sysctl_speed_limit_max = 200000;
static inline int speed_min(mddev_t *mddev)
{
	return mddev->sync_speed_min ?
		mddev->sync_speed_min : sysctl_speed_limit_min;
}

static inline int speed_max(mddev_t *mddev)
{
	return mddev->sync_speed_max ?
		mddev->sync_speed_max : sysctl_speed_limit_max;
}

static struct ctl_table_header *raid_table_header;

static ctl_table raid_table[] = {
	{
		.ctl_name	= DEV_RAID_SPEED_LIMIT_MIN,
		.procname	= "speed_limit_min",
		.data		= &sysctl_speed_limit_min,
		.maxlen		= sizeof(int),
		.mode		= S_IRUGO|S_IWUSR,
		.proc_handler	= &proc_dointvec,
	},
	{
		.ctl_name	= DEV_RAID_SPEED_LIMIT_MAX,
		.procname	= "speed_limit_max",
		.data		= &sysctl_speed_limit_max,
		.maxlen		= sizeof(int),
		.mode		= S_IRUGO|S_IWUSR,
		.proc_handler	= &proc_dointvec,
	},
	{ .ctl_name = 0 }
};

static ctl_table raid_dir_table[] = {
	{
		.ctl_name	= DEV_RAID,
		.procname	= "raid",
		.maxlen		= 0,
		.mode		= S_IRUGO|S_IXUGO,
		.child		= raid_table,
	},
	{ .ctl_name = 0 }
};

static ctl_table raid_root_table[] = {
	{
		.ctl_name	= CTL_DEV,
		.procname	= "dev",
		.maxlen		= 0,
		.mode		= 0555,
		.child		= raid_dir_table,
	},
	{ .ctl_name = 0 }
};

static struct block_device_operations md_fops;

static int start_readonly;

static DECLARE_WAIT_QUEUE_HEAD(md_event_waiters);
static atomic_t md_event_count;
void md_new_event(mddev_t *mddev)
{
	atomic_inc(&md_event_count);
	wake_up(&md_event_waiters);
}
EXPORT_SYMBOL_GPL(md_new_event);

static void md_new_event_inintr(mddev_t *mddev)
{
	atomic_inc(&md_event_count);
	wake_up(&md_event_waiters);
}

static LIST_HEAD(all_mddevs);
static DEFINE_SPINLOCK(all_mddevs_lock);


#define for_each_mddev(mddev,tmp)					\
									\
	for (({ spin_lock(&all_mddevs_lock); 				\
		tmp = all_mddevs.next;					\
		mddev = NULL;});					\
	     ({ if (tmp != &all_mddevs)					\
			mddev_get(list_entry(tmp, mddev_t, all_mddevs));\
		spin_unlock(&all_mddevs_lock);				\
		if (mddev) mddev_put(mddev);				\
		mddev = list_entry(tmp, mddev_t, all_mddevs);		\
		tmp != &all_mddevs;});					\
	     ({ spin_lock(&all_mddevs_lock);				\
		tmp = tmp->next;})					\
		)


static int md_fail_request(struct request_queue *q, struct bio *bio)
{
	bio_io_error(bio);
	return 0;
}

static inline mddev_t *mddev_get(mddev_t *mddev)
{
	atomic_inc(&mddev->active);
	return mddev;
}

static void mddev_delayed_delete(struct work_struct *ws);

static void mddev_put(mddev_t *mddev)
{
	if (!atomic_dec_and_lock(&mddev->active, &all_mddevs_lock))
		return;
	if (!mddev->raid_disks && list_empty(&mddev->disks) &&
	    !mddev->hold_active) {
		list_del(&mddev->all_mddevs);
		if (mddev->gendisk) {
			/* we did a probe so need to clean up.
			 * Call schedule_work inside the spinlock
			 * so that flush_scheduled_work() after
			 * mddev_find will succeed in waiting for the
			 * work to be done.
			 */
			INIT_WORK(&mddev->del_work, mddev_delayed_delete);
			schedule_work(&mddev->del_work);
		} else
			kfree(mddev);
	}
	spin_unlock(&all_mddevs_lock);
}

static mddev_t * mddev_find(dev_t unit)
{
	mddev_t *mddev, *new = NULL;

 retry:
	spin_lock(&all_mddevs_lock);

	if (unit) {
		list_for_each_entry(mddev, &all_mddevs, all_mddevs)
			if (mddev->unit == unit) {
				mddev_get(mddev);
				spin_unlock(&all_mddevs_lock);
				kfree(new);
				return mddev;
			}

		if (new) {
			list_add(&new->all_mddevs, &all_mddevs);
			spin_unlock(&all_mddevs_lock);
			new->hold_active = UNTIL_IOCTL;
			return new;
		}
	} else if (new) {
		/* find an unused unit number */
		static int next_minor = 512;
		int start = next_minor;
		int is_free = 0;
		int dev = 0;
		while (!is_free) {
			dev = MKDEV(MD_MAJOR, next_minor);
			next_minor++;
			if (next_minor > MINORMASK)
				next_minor = 0;
			if (next_minor == start) {
				/* Oh dear, all in use. */
				spin_unlock(&all_mddevs_lock);
				kfree(new);
				return NULL;
			}
				
			is_free = 1;
			list_for_each_entry(mddev, &all_mddevs, all_mddevs)
				if (mddev->unit == dev) {
					is_free = 0;
					break;
				}
		}
		new->unit = dev;
		new->md_minor = MINOR(dev);
		new->hold_active = UNTIL_STOP;
		list_add(&new->all_mddevs, &all_mddevs);
		spin_unlock(&all_mddevs_lock);
		return new;
	}
	spin_unlock(&all_mddevs_lock);

	new = kzalloc(sizeof(*new), GFP_KERNEL);
	if (!new)
		return NULL;

	new->unit = unit;
	if (MAJOR(unit) == MD_MAJOR)
		new->md_minor = MINOR(unit);
	else
		new->md_minor = MINOR(unit) >> MdpMinorShift;

	mutex_init(&new->reconfig_mutex);
	INIT_LIST_HEAD(&new->disks);
	INIT_LIST_HEAD(&new->all_mddevs);
	init_timer(&new->safemode_timer);
	atomic_set(&new->active, 1);
	atomic_set(&new->openers, 0);
	spin_lock_init(&new->write_lock);
	init_waitqueue_head(&new->sb_wait);
	init_waitqueue_head(&new->recovery_wait);
	new->reshape_position = MaxSector;
	new->resync_min = 0;
	new->resync_max = MaxSector;
	new->level = LEVEL_NONE;

	goto retry;
}

static inline int mddev_lock(mddev_t * mddev)
{
	return mutex_lock_interruptible(&mddev->reconfig_mutex);
}

static inline int mddev_trylock(mddev_t * mddev)
{
	return mutex_trylock(&mddev->reconfig_mutex);
}

static inline void mddev_unlock(mddev_t * mddev)
{
	mutex_unlock(&mddev->reconfig_mutex);

	md_wakeup_thread(mddev->thread);
}

static mdk_rdev_t * find_rdev_nr(mddev_t *mddev, int nr)
{
	mdk_rdev_t *rdev;

	list_for_each_entry(rdev, &mddev->disks, same_set)
		if (rdev->desc_nr == nr)
			return rdev;

	return NULL;
}

static mdk_rdev_t * find_rdev(mddev_t * mddev, dev_t dev)
{
	mdk_rdev_t *rdev;

	list_for_each_entry(rdev, &mddev->disks, same_set)
		if (rdev->bdev->bd_dev == dev)
			return rdev;

	return NULL;
}

static struct mdk_personality *find_pers(int level, char *clevel)
{
	struct mdk_personality *pers;
	list_for_each_entry(pers, &pers_list, list) {
		if (level != LEVEL_NONE && pers->level == level)
			return pers;
		if (strcmp(pers->name, clevel)==0)
			return pers;
	}
	return NULL;
}

/* return the offset of the super block in 512byte sectors */
static inline sector_t calc_dev_sboffset(struct block_device *bdev)
{
	sector_t num_sectors = bdev->bd_inode->i_size / 512;
	return MD_NEW_SIZE_SECTORS(num_sectors);
}

static sector_t calc_num_sectors(mdk_rdev_t *rdev, unsigned chunk_size)
{
	sector_t num_sectors = rdev->sb_start;

	if (chunk_size)
		num_sectors &= ~((sector_t)chunk_size/512 - 1);
	return num_sectors;
}

static int alloc_disk_sb(mdk_rdev_t * rdev)
{
	if (rdev->sb_page)
		MD_BUG();

	rdev->sb_page = alloc_page(GFP_KERNEL);
	if (!rdev->sb_page) {
		printk(KERN_ALERT "md: out of memory.\n");
		return -ENOMEM;
	}

	return 0;
}

static void free_disk_sb(mdk_rdev_t * rdev)
{
	if (rdev->sb_page) {
		put_page(rdev->sb_page);
		rdev->sb_loaded = 0;
		rdev->sb_page = NULL;
		rdev->sb_start = 0;
		rdev->size = 0;
	}
}


static void super_written(struct bio *bio, int error)
{
	mdk_rdev_t *rdev = bio->bi_private;
	mddev_t *mddev = rdev->mddev;

	if (error || !test_bit(BIO_UPTODATE, &bio->bi_flags)) {
		printk("md: super_written gets error=%d, uptodate=%d\n",
		       error, test_bit(BIO_UPTODATE, &bio->bi_flags));
		WARN_ON(test_bit(BIO_UPTODATE, &bio->bi_flags));
		md_error(mddev, rdev);
	}

	if (atomic_dec_and_test(&mddev->pending_writes))
		wake_up(&mddev->sb_wait);
	bio_put(bio);
}

static void super_written_barrier(struct bio *bio, int error)
{
	struct bio *bio2 = bio->bi_private;
	mdk_rdev_t *rdev = bio2->bi_private;
	mddev_t *mddev = rdev->mddev;

	if (!test_bit(BIO_UPTODATE, &bio->bi_flags) &&
	    error == -EOPNOTSUPP) {
		unsigned long flags;
		/* barriers don't appear to be supported :-( */
		set_bit(BarriersNotsupp, &rdev->flags);
		mddev->barriers_work = 0;
		spin_lock_irqsave(&mddev->write_lock, flags);
		bio2->bi_next = mddev->biolist;
		mddev->biolist = bio2;
		spin_unlock_irqrestore(&mddev->write_lock, flags);
		wake_up(&mddev->sb_wait);
		bio_put(bio);
	} else {
		bio_put(bio2);
		bio->bi_private = rdev;
		super_written(bio, error);
	}
}

void md_super_write(mddev_t *mddev, mdk_rdev_t *rdev,
		   sector_t sector, int size, struct page *page)
{
	/* write first size bytes of page to sector of rdev
	 * Increment mddev->pending_writes before returning
	 * and decrement it on completion, waking up sb_wait
	 * if zero is reached.
	 * If an error occurred, call md_error
	 *
	 * As we might need to resubmit the request if BIO_RW_BARRIER
	 * causes ENOTSUPP, we allocate a spare bio...
	 */
	struct bio *bio = bio_alloc(GFP_NOIO, 1);
	int rw = (1<<BIO_RW) | (1<<BIO_RW_SYNCIO) | (1<<BIO_RW_UNPLUG);

	bio->bi_bdev = rdev->bdev;
	bio->bi_sector = sector;
	bio_add_page(bio, page, size, 0);
	bio->bi_private = rdev;
	bio->bi_end_io = super_written;
	bio->bi_rw = rw;

	atomic_inc(&mddev->pending_writes);
	if (!test_bit(BarriersNotsupp, &rdev->flags)) {
		struct bio *rbio;
		rw |= (1<<BIO_RW_BARRIER);
		rbio = bio_clone(bio, GFP_NOIO);
		rbio->bi_private = bio;
		rbio->bi_end_io = super_written_barrier;
		submit_bio(rw, rbio);
	} else
		submit_bio(rw, bio);
}

void md_super_wait(mddev_t *mddev)
{
	/* wait for all superblock writes that were scheduled to complete.
	 * if any had to be retried (due to BARRIER problems), retry them
	 */
	DEFINE_WAIT(wq);
	for(;;) {
		prepare_to_wait(&mddev->sb_wait, &wq, TASK_UNINTERRUPTIBLE);
		if (atomic_read(&mddev->pending_writes)==0)
			break;
		while (mddev->biolist) {
			struct bio *bio;
			spin_lock_irq(&mddev->write_lock);
			bio = mddev->biolist;
			mddev->biolist = bio->bi_next ;
			bio->bi_next = NULL;
			spin_unlock_irq(&mddev->write_lock);
			submit_bio(bio->bi_rw, bio);
		}
		schedule();
	}
	finish_wait(&mddev->sb_wait, &wq);
}

static void bi_complete(struct bio *bio, int error)
{
	complete((struct completion*)bio->bi_private);
}

int sync_page_io(struct block_device *bdev, sector_t sector, int size,
		   struct page *page, int rw)
{
	struct bio *bio = bio_alloc(GFP_NOIO, 1);
	struct completion event;
	int ret;

	rw |= (1 << BIO_RW_SYNCIO) | (1 << BIO_RW_UNPLUG);

	bio->bi_bdev = bdev;
	bio->bi_sector = sector;
	bio_add_page(bio, page, size, 0);
	init_completion(&event);
	bio->bi_private = &event;
	bio->bi_end_io = bi_complete;
	submit_bio(rw, bio);
	wait_for_completion(&event);

	ret = test_bit(BIO_UPTODATE, &bio->bi_flags);
	bio_put(bio);
	return ret;
}
EXPORT_SYMBOL_GPL(sync_page_io);

static int read_disk_sb(mdk_rdev_t * rdev, int size)
{
	char b[BDEVNAME_SIZE];
	if (!rdev->sb_page) {
		MD_BUG();
		return -EINVAL;
	}
	if (rdev->sb_loaded)
		return 0;


	if (!sync_page_io(rdev->bdev, rdev->sb_start, size, rdev->sb_page, READ))
		goto fail;
	rdev->sb_loaded = 1;
	return 0;

fail:
	printk(KERN_WARNING "md: disabled device %s, could not read superblock.\n",
		bdevname(rdev->bdev,b));
	return -EINVAL;
}

static int uuid_equal(mdp_super_t *sb1, mdp_super_t *sb2)
{
	return 	sb1->set_uuid0 == sb2->set_uuid0 &&
		sb1->set_uuid1 == sb2->set_uuid1 &&
		sb1->set_uuid2 == sb2->set_uuid2 &&
		sb1->set_uuid3 == sb2->set_uuid3;
}

static int sb_equal(mdp_super_t *sb1, mdp_super_t *sb2)
{
	int ret;
	mdp_super_t *tmp1, *tmp2;

	tmp1 = kmalloc(sizeof(*tmp1),GFP_KERNEL);
	tmp2 = kmalloc(sizeof(*tmp2),GFP_KERNEL);

	if (!tmp1 || !tmp2) {
		ret = 0;
		printk(KERN_INFO "md.c sb_equal(): failed to allocate memory!\n");
		goto abort;
	}

	*tmp1 = *sb1;
	*tmp2 = *sb2;

	/*
	 * nr_disks is not constant
	 */
	tmp1->nr_disks = 0;
	tmp2->nr_disks = 0;

	ret = (memcmp(tmp1, tmp2, MD_SB_GENERIC_CONSTANT_WORDS * 4) == 0);
abort:
	kfree(tmp1);
	kfree(tmp2);
	return ret;
}


static u32 md_csum_fold(u32 csum)
{
	csum = (csum & 0xffff) + (csum >> 16);
	return (csum & 0xffff) + (csum >> 16);
}

static unsigned int calc_sb_csum(mdp_super_t * sb)
{
	u64 newcsum = 0;
	u32 *sb32 = (u32*)sb;
	int i;
	unsigned int disk_csum, csum;

	disk_csum = sb->sb_csum;
	sb->sb_csum = 0;

	for (i = 0; i < MD_SB_BYTES/4 ; i++)
		newcsum += sb32[i];
	csum = (newcsum & 0xffffffff) + (newcsum>>32);


#ifdef CONFIG_ALPHA
	/* This used to use csum_partial, which was wrong for several
	 * reasons including that different results are returned on
	 * different architectures.  It isn't critical that we get exactly
	 * the same return value as before (we always csum_fold before
	 * testing, and that removes any differences).  However as we
	 * know that csum_partial always returned a 16bit value on
	 * alphas, do a fold to maximise conformity to previous behaviour.
	 */
	sb->sb_csum = md_csum_fold(disk_csum);
#else
	sb->sb_csum = disk_csum;
#endif
	return csum;
}



struct super_type  {
	char		    *name;
	struct module	    *owner;
	int		    (*load_super)(mdk_rdev_t *rdev, mdk_rdev_t *refdev,
					  int minor_version);
	int		    (*validate_super)(mddev_t *mddev, mdk_rdev_t *rdev);
	void		    (*sync_super)(mddev_t *mddev, mdk_rdev_t *rdev);
	unsigned long long  (*rdev_size_change)(mdk_rdev_t *rdev,
						sector_t num_sectors);
};

static int super_90_load(mdk_rdev_t *rdev, mdk_rdev_t *refdev, int minor_version)
{
	char b[BDEVNAME_SIZE], b2[BDEVNAME_SIZE];
	mdp_super_t *sb;
	int ret;

	/*
	 * Calculate the position of the superblock (512byte sectors),
	 * it's at the end of the disk.
	 *
	 * It also happens to be a multiple of 4Kb.
	 */
	rdev->sb_start = calc_dev_sboffset(rdev->bdev);

	ret = read_disk_sb(rdev, MD_SB_BYTES);
	if (ret) return ret;

	ret = -EINVAL;

	bdevname(rdev->bdev, b);
	sb = (mdp_super_t*)page_address(rdev->sb_page);

	if (sb->md_magic != MD_SB_MAGIC) {
		printk(KERN_ERR "md: invalid raid superblock magic on %s\n",
		       b);
		goto abort;
	}

	if (sb->major_version != 0 ||
	    sb->minor_version < 90 ||
	    sb->minor_version > 91) {
		printk(KERN_WARNING "Bad version number %d.%d on %s\n",
			sb->major_version, sb->minor_version,
			b);
		goto abort;
	}

	if (sb->raid_disks <= 0)
		goto abort;

	if (md_csum_fold(calc_sb_csum(sb)) != md_csum_fold(sb->sb_csum)) {
		printk(KERN_WARNING "md: invalid superblock checksum on %s\n",
			b);
		goto abort;
	}

	rdev->preferred_minor = sb->md_minor;
	rdev->data_offset = 0;
	rdev->sb_size = MD_SB_BYTES;

	if (sb->state & (1<<MD_SB_BITMAP_PRESENT)) {
		if (sb->level != 1 && sb->level != 4
		    && sb->level != 5 && sb->level != 6
		    && sb->level != 10) {
			/* FIXME use a better test */
			printk(KERN_WARNING
			       "md: bitmaps not supported for this level.\n");
			goto abort;
		}
	}

	if (sb->level == LEVEL_MULTIPATH)
		rdev->desc_nr = -1;
	else
		rdev->desc_nr = sb->this_disk.number;

	if (!refdev) {
		ret = 1;
	} else {
		__u64 ev1, ev2;
		mdp_super_t *refsb = (mdp_super_t*)page_address(refdev->sb_page);
		if (!uuid_equal(refsb, sb)) {
			printk(KERN_WARNING "md: %s has different UUID to %s\n",
				b, bdevname(refdev->bdev,b2));
			goto abort;
		}
		if (!sb_equal(refsb, sb)) {
			printk(KERN_WARNING "md: %s has same UUID"
			       " but different superblock to %s\n",
			       b, bdevname(refdev->bdev, b2));
			goto abort;
		}
		ev1 = md_event(sb);
		ev2 = md_event(refsb);
		if (ev1 > ev2)
			ret = 1;
		else 
			ret = 0;
	}
	rdev->size = calc_num_sectors(rdev, sb->chunk_size) / 2;

	if (rdev->size < sb->size && sb->level > 1)
		/* "this cannot possibly happen" ... */
		ret = -EINVAL;

 abort:
	return ret;
}

static int super_90_validate(mddev_t *mddev, mdk_rdev_t *rdev)
{
	mdp_disk_t *desc;
	mdp_super_t *sb = (mdp_super_t *)page_address(rdev->sb_page);
	__u64 ev1 = md_event(sb);

	rdev->raid_disk = -1;
	clear_bit(Faulty, &rdev->flags);
	clear_bit(In_sync, &rdev->flags);
	clear_bit(WriteMostly, &rdev->flags);
	clear_bit(BarriersNotsupp, &rdev->flags);

	if (mddev->raid_disks == 0) {
		mddev->major_version = 0;
		mddev->minor_version = sb->minor_version;
		mddev->patch_version = sb->patch_version;
		mddev->external = 0;
		mddev->chunk_size = sb->chunk_size;
		mddev->ctime = sb->ctime;
		mddev->utime = sb->utime;
		mddev->level = sb->level;
		mddev->clevel[0] = 0;
		mddev->layout = sb->layout;
		mddev->raid_disks = sb->raid_disks;
		mddev->size = sb->size;
		mddev->events = ev1;
		mddev->bitmap_offset = 0;
		mddev->default_bitmap_offset = MD_SB_BYTES >> 9;

		if (mddev->minor_version >= 91) {
			mddev->reshape_position = sb->reshape_position;
			mddev->delta_disks = sb->delta_disks;
			mddev->new_level = sb->new_level;
			mddev->new_layout = sb->new_layout;
			mddev->new_chunk = sb->new_chunk;
		} else {
			mddev->reshape_position = MaxSector;
			mddev->delta_disks = 0;
			mddev->new_level = mddev->level;
			mddev->new_layout = mddev->layout;
			mddev->new_chunk = mddev->chunk_size;
		}

		if (sb->state & (1<<MD_SB_CLEAN))
			mddev->recovery_cp = MaxSector;
		else {
			if (sb->events_hi == sb->cp_events_hi && 
				sb->events_lo == sb->cp_events_lo) {
				mddev->recovery_cp = sb->recovery_cp;
			} else
				mddev->recovery_cp = 0;
		}

		memcpy(mddev->uuid+0, &sb->set_uuid0, 4);
		memcpy(mddev->uuid+4, &sb->set_uuid1, 4);
		memcpy(mddev->uuid+8, &sb->set_uuid2, 4);
		memcpy(mddev->uuid+12,&sb->set_uuid3, 4);

		mddev->max_disks = MD_SB_DISKS;

		if (sb->state & (1<<MD_SB_BITMAP_PRESENT) &&
		    mddev->bitmap_file == NULL)
			mddev->bitmap_offset = mddev->default_bitmap_offset;

	} else if (mddev->pers == NULL) {
		/* Insist on good event counter while assembling */
		++ev1;
		if (ev1 < mddev->events) 
			return -EINVAL;
	} else if (mddev->bitmap) {
		/* if adding to array with a bitmap, then we can accept an
		 * older device ... but not too old.
		 */
		if (ev1 < mddev->bitmap->events_cleared)
			return 0;
	} else {
		if (ev1 < mddev->events)
			/* just a hot-add of a new device, leave raid_disk at -1 */
			return 0;
	}

	if (mddev->level != LEVEL_MULTIPATH) {
		desc = sb->disks + rdev->desc_nr;

		if (desc->state & (1<<MD_DISK_FAULTY))
			set_bit(Faulty, &rdev->flags);
		else if (desc->state & (1<<MD_DISK_SYNC) /* &&
			    desc->raid_disk < mddev->raid_disks */) {
			set_bit(In_sync, &rdev->flags);
			rdev->raid_disk = desc->raid_disk;
		}
		if (desc->state & (1<<MD_DISK_WRITEMOSTLY))
			set_bit(WriteMostly, &rdev->flags);
	} else /* MULTIPATH are always insync */
		set_bit(In_sync, &rdev->flags);
	return 0;
}

static void super_90_sync(mddev_t *mddev, mdk_rdev_t *rdev)
{
	mdp_super_t *sb;
	mdk_rdev_t *rdev2;
	int next_spare = mddev->raid_disks;


	/* make rdev->sb match mddev data..
	 *
	 * 1/ zero out disks
	 * 2/ Add info for each disk, keeping track of highest desc_nr (next_spare);
	 * 3/ any empty disks < next_spare become removed
	 *
	 * disks[0] gets initialised to REMOVED because
	 * we cannot be sure from other fields if it has
	 * been initialised or not.
	 */
	int i;
	int active=0, working=0,failed=0,spare=0,nr_disks=0;

	rdev->sb_size = MD_SB_BYTES;

	sb = (mdp_super_t*)page_address(rdev->sb_page);

	memset(sb, 0, sizeof(*sb));

	sb->md_magic = MD_SB_MAGIC;
	sb->major_version = mddev->major_version;
	sb->patch_version = mddev->patch_version;
	sb->gvalid_words  = 0; /* ignored */
	memcpy(&sb->set_uuid0, mddev->uuid+0, 4);
	memcpy(&sb->set_uuid1, mddev->uuid+4, 4);
	memcpy(&sb->set_uuid2, mddev->uuid+8, 4);
	memcpy(&sb->set_uuid3, mddev->uuid+12,4);

	sb->ctime = mddev->ctime;
	sb->level = mddev->level;
	sb->size  = mddev->size;
	sb->raid_disks = mddev->raid_disks;
	sb->md_minor = mddev->md_minor;
	sb->not_persistent = 0;
	sb->utime = mddev->utime;
	sb->state = 0;
	sb->events_hi = (mddev->events>>32);
	sb->events_lo = (u32)mddev->events;

	if (mddev->reshape_position == MaxSector)
		sb->minor_version = 90;
	else {
		sb->minor_version = 91;
		sb->reshape_position = mddev->reshape_position;
		sb->new_level = mddev->new_level;
		sb->delta_disks = mddev->delta_disks;
		sb->new_layout = mddev->new_layout;
		sb->new_chunk = mddev->new_chunk;
	}
	mddev->minor_version = sb->minor_version;
	if (mddev->in_sync)
	{
		sb->recovery_cp = mddev->recovery_cp;
		sb->cp_events_hi = (mddev->events>>32);
		sb->cp_events_lo = (u32)mddev->events;
		if (mddev->recovery_cp == MaxSector)
			sb->state = (1<< MD_SB_CLEAN);
	} else
		sb->recovery_cp = 0;

	sb->layout = mddev->layout;
	sb->chunk_size = mddev->chunk_size;

	if (mddev->bitmap && mddev->bitmap_file == NULL)
		sb->state |= (1<<MD_SB_BITMAP_PRESENT);

	sb->disks[0].state = (1<<MD_DISK_REMOVED);
	list_for_each_entry(rdev2, &mddev->disks, same_set) {
		mdp_disk_t *d;
		int desc_nr;
		if (rdev2->raid_disk >= 0 && test_bit(In_sync, &rdev2->flags)
		    && !test_bit(Faulty, &rdev2->flags))
			desc_nr = rdev2->raid_disk;
		else
			desc_nr = next_spare++;
		rdev2->desc_nr = desc_nr;
		d = &sb->disks[rdev2->desc_nr];
		nr_disks++;
		d->number = rdev2->desc_nr;
		d->major = MAJOR(rdev2->bdev->bd_dev);
		d->minor = MINOR(rdev2->bdev->bd_dev);
		if (rdev2->raid_disk >= 0 && test_bit(In_sync, &rdev2->flags)
		    && !test_bit(Faulty, &rdev2->flags))
			d->raid_disk = rdev2->raid_disk;
		else
			d->raid_disk = rdev2->desc_nr; /* compatibility */
		if (test_bit(Faulty, &rdev2->flags))
			d->state = (1<<MD_DISK_FAULTY);
		else if (test_bit(In_sync, &rdev2->flags)) {
			d->state = (1<<MD_DISK_ACTIVE);
			d->state |= (1<<MD_DISK_SYNC);
			active++;
			working++;
		} else {
			d->state = 0;
			spare++;
			working++;
		}
		if (test_bit(WriteMostly, &rdev2->flags))
			d->state |= (1<<MD_DISK_WRITEMOSTLY);
	}
	/* now set the "removed" and "faulty" bits on any missing devices */
	for (i=0 ; i < mddev->raid_disks ; i++) {
		mdp_disk_t *d = &sb->disks[i];
		if (d->state == 0 && d->number == 0) {
			d->number = i;
			d->raid_disk = i;
			d->state = (1<<MD_DISK_REMOVED);
			d->state |= (1<<MD_DISK_FAULTY);
			failed++;
		}
	}
	sb->nr_disks = nr_disks;
	sb->active_disks = active;
	sb->working_disks = working;
	sb->failed_disks = failed;
	sb->spare_disks = spare;

	sb->this_disk = sb->disks[rdev->desc_nr];
	sb->sb_csum = calc_sb_csum(sb);
}

static unsigned long long
super_90_rdev_size_change(mdk_rdev_t *rdev, sector_t num_sectors)
{
	if (num_sectors && num_sectors < rdev->mddev->size * 2)
		return 0; /* component must fit device */
	if (rdev->mddev->bitmap_offset)
		return 0; /* can't move bitmap */
	rdev->sb_start = calc_dev_sboffset(rdev->bdev);
	if (!num_sectors || num_sectors > rdev->sb_start)
		num_sectors = rdev->sb_start;
	md_super_write(rdev->mddev, rdev, rdev->sb_start, rdev->sb_size,
		       rdev->sb_page);
	md_super_wait(rdev->mddev);
	return num_sectors / 2; /* kB for sysfs */
}



static __le32 calc_sb_1_csum(struct mdp_superblock_1 * sb)
{
	__le32 disk_csum;
	u32 csum;
	unsigned long long newcsum;
	int size = 256 + le32_to_cpu(sb->max_dev)*2;
	__le32 *isuper = (__le32*)sb;
	int i;

	disk_csum = sb->sb_csum;
	sb->sb_csum = 0;
	newcsum = 0;
	for (i=0; size>=4; size -= 4 )
		newcsum += le32_to_cpu(*isuper++);

	if (size == 2)
		newcsum += le16_to_cpu(*(__le16*) isuper);

	csum = (newcsum & 0xffffffff) + (newcsum >> 32);
	sb->sb_csum = disk_csum;
	return cpu_to_le32(csum);
}

static int super_1_load(mdk_rdev_t *rdev, mdk_rdev_t *refdev, int minor_version)
{
	struct mdp_superblock_1 *sb;
	int ret;
	sector_t sb_start;
	char b[BDEVNAME_SIZE], b2[BDEVNAME_SIZE];
	int bmask;

	/*
	 * Calculate the position of the superblock in 512byte sectors.
	 * It is always aligned to a 4K boundary and
	 * depeding on minor_version, it can be:
	 * 0: At least 8K, but less than 12K, from end of device
	 * 1: At start of device
	 * 2: 4K from start of device.
	 */
	switch(minor_version) {
	case 0:
		sb_start = rdev->bdev->bd_inode->i_size >> 9;
		sb_start -= 8*2;
		sb_start &= ~(sector_t)(4*2-1);
		break;
	case 1:
		sb_start = 0;
		break;
	case 2:
		sb_start = 8;
		break;
	default:
		return -EINVAL;
	}
	rdev->sb_start = sb_start;

	/* superblock is rarely larger than 1K, but it can be larger,
	 * and it is safe to read 4k, so we do that
	 */
	ret = read_disk_sb(rdev, 4096);
	if (ret) return ret;


	sb = (struct mdp_superblock_1*)page_address(rdev->sb_page);

	if (sb->magic != cpu_to_le32(MD_SB_MAGIC) ||
	    sb->major_version != cpu_to_le32(1) ||
	    le32_to_cpu(sb->max_dev) > (4096-256)/2 ||
	    le64_to_cpu(sb->super_offset) != rdev->sb_start ||
	    (le32_to_cpu(sb->feature_map) & ~MD_FEATURE_ALL) != 0)
		return -EINVAL;

	if (calc_sb_1_csum(sb) != sb->sb_csum) {
		printk("md: invalid superblock checksum on %s\n",
			bdevname(rdev->bdev,b));
		return -EINVAL;
	}
	if (le64_to_cpu(sb->data_size) < 10) {
		printk("md: data_size too small on %s\n",
		       bdevname(rdev->bdev,b));
		return -EINVAL;
	}
	if ((le32_to_cpu(sb->feature_map) & MD_FEATURE_BITMAP_OFFSET)) {
		if (sb->level != cpu_to_le32(1) &&
		    sb->level != cpu_to_le32(4) &&
		    sb->level != cpu_to_le32(5) &&
		    sb->level != cpu_to_le32(6) &&
		    sb->level != cpu_to_le32(10)) {
			printk(KERN_WARNING
			       "md: bitmaps not supported for this level.\n");
			return -EINVAL;
		}
	}

	rdev->preferred_minor = 0xffff;
	rdev->data_offset = le64_to_cpu(sb->data_offset);
	atomic_set(&rdev->corrected_errors, le32_to_cpu(sb->cnt_corrected_read));

	rdev->sb_size = le32_to_cpu(sb->max_dev) * 2 + 256;
	bmask = queue_hardsect_size(rdev->bdev->bd_disk->queue)-1;
	if (rdev->sb_size & bmask)
		rdev->sb_size = (rdev->sb_size | bmask) + 1;

	if (minor_version
	    && rdev->data_offset < sb_start + (rdev->sb_size/512))
		return -EINVAL;

	if (sb->level == cpu_to_le32(LEVEL_MULTIPATH))
		rdev->desc_nr = -1;
	else
		rdev->desc_nr = le32_to_cpu(sb->dev_number);

	if (!refdev) {
		ret = 1;
	} else {
		__u64 ev1, ev2;
		struct mdp_superblock_1 *refsb = 
			(struct mdp_superblock_1*)page_address(refdev->sb_page);

		if (memcmp(sb->set_uuid, refsb->set_uuid, 16) != 0 ||
		    sb->level != refsb->level ||
		    sb->layout != refsb->layout ||
		    sb->chunksize != refsb->chunksize) {
			printk(KERN_WARNING "md: %s has strangely different"
				" superblock to %s\n",
				bdevname(rdev->bdev,b),
				bdevname(refdev->bdev,b2));
			return -EINVAL;
		}
		ev1 = le64_to_cpu(sb->events);
		ev2 = le64_to_cpu(refsb->events);

		if (ev1 > ev2)
			ret = 1;
		else
			ret = 0;
	}
	if (minor_version)
		rdev->size = ((rdev->bdev->bd_inode->i_size>>9) - le64_to_cpu(sb->data_offset)) / 2;
	else
		rdev->size = rdev->sb_start / 2;
	if (rdev->size < le64_to_cpu(sb->data_size)/2)
		return -EINVAL;
	rdev->size = le64_to_cpu(sb->data_size)/2;
	if (le32_to_cpu(sb->chunksize))
		rdev->size &= ~((sector_t)le32_to_cpu(sb->chunksize)/2 - 1);

	if (le64_to_cpu(sb->size) > rdev->size*2)
		return -EINVAL;
	return ret;
}

static int super_1_validate(mddev_t *mddev, mdk_rdev_t *rdev)
{
	struct mdp_superblock_1 *sb = (struct mdp_superblock_1*)page_address(rdev->sb_page);
	__u64 ev1 = le64_to_cpu(sb->events);

	rdev->raid_disk = -1;
	clear_bit(Faulty, &rdev->flags);
	clear_bit(In_sync, &rdev->flags);
	clear_bit(WriteMostly, &rdev->flags);
	clear_bit(BarriersNotsupp, &rdev->flags);

	if (mddev->raid_disks == 0) {
		mddev->major_version = 1;
		mddev->patch_version = 0;
		mddev->external = 0;
		mddev->chunk_size = le32_to_cpu(sb->chunksize) << 9;
		mddev->ctime = le64_to_cpu(sb->ctime) & ((1ULL << 32)-1);
		mddev->utime = le64_to_cpu(sb->utime) & ((1ULL << 32)-1);
		mddev->level = le32_to_cpu(sb->level);
		mddev->clevel[0] = 0;
		mddev->layout = le32_to_cpu(sb->layout);
		mddev->raid_disks = le32_to_cpu(sb->raid_disks);
		mddev->size = le64_to_cpu(sb->size)/2;
		mddev->events = ev1;
		mddev->bitmap_offset = 0;
		mddev->default_bitmap_offset = 1024 >> 9;
		
		mddev->recovery_cp = le64_to_cpu(sb->resync_offset);
		memcpy(mddev->uuid, sb->set_uuid, 16);

		mddev->max_disks =  (4096-256)/2;

		if ((le32_to_cpu(sb->feature_map) & MD_FEATURE_BITMAP_OFFSET) &&
		    mddev->bitmap_file == NULL )
			mddev->bitmap_offset = (__s32)le32_to_cpu(sb->bitmap_offset);

		if ((le32_to_cpu(sb->feature_map) & MD_FEATURE_RESHAPE_ACTIVE)) {
			mddev->reshape_position = le64_to_cpu(sb->reshape_position);
			mddev->delta_disks = le32_to_cpu(sb->delta_disks);
			mddev->new_level = le32_to_cpu(sb->new_level);
			mddev->new_layout = le32_to_cpu(sb->new_layout);
			mddev->new_chunk = le32_to_cpu(sb->new_chunk)<<9;
		} else {
			mddev->reshape_position = MaxSector;
			mddev->delta_disks = 0;
			mddev->new_level = mddev->level;
			mddev->new_layout = mddev->layout;
			mddev->new_chunk = mddev->chunk_size;
		}

	} else if (mddev->pers == NULL) {
		/* Insist of good event counter while assembling */
		++ev1;
		if (ev1 < mddev->events)
			return -EINVAL;
	} else if (mddev->bitmap) {
		/* If adding to array with a bitmap, then we can accept an
		 * older device, but not too old.
		 */
		if (ev1 < mddev->bitmap->events_cleared)
			return 0;
	} else {
		if (ev1 < mddev->events)
			/* just a hot-add of a new device, leave raid_disk at -1 */
			return 0;
	}
	if (mddev->level != LEVEL_MULTIPATH) {
		int role;
		role = le16_to_cpu(sb->dev_roles[rdev->desc_nr]);
		switch(role) {
		case 0xffff: /* spare */
			break;
		case 0xfffe: /* faulty */
			set_bit(Faulty, &rdev->flags);
			break;
		default:
			if ((le32_to_cpu(sb->feature_map) &
			     MD_FEATURE_RECOVERY_OFFSET))
				rdev->recovery_offset = le64_to_cpu(sb->recovery_offset);
			else
				set_bit(In_sync, &rdev->flags);
			rdev->raid_disk = role;
			break;
		}
		if (sb->devflags & WriteMostly1)
			set_bit(WriteMostly, &rdev->flags);
	} else /* MULTIPATH are always insync */
		set_bit(In_sync, &rdev->flags);

	return 0;
}

static void super_1_sync(mddev_t *mddev, mdk_rdev_t *rdev)
{
	struct mdp_superblock_1 *sb;
	mdk_rdev_t *rdev2;
	int max_dev, i;
	/* make rdev->sb match mddev and rdev data. */

	sb = (struct mdp_superblock_1*)page_address(rdev->sb_page);

	sb->feature_map = 0;
	sb->pad0 = 0;
	sb->recovery_offset = cpu_to_le64(0);
	memset(sb->pad1, 0, sizeof(sb->pad1));
	memset(sb->pad2, 0, sizeof(sb->pad2));
	memset(sb->pad3, 0, sizeof(sb->pad3));

	sb->utime = cpu_to_le64((__u64)mddev->utime);
	sb->events = cpu_to_le64(mddev->events);
	if (mddev->in_sync)
		sb->resync_offset = cpu_to_le64(mddev->recovery_cp);
	else
		sb->resync_offset = cpu_to_le64(0);

	sb->cnt_corrected_read = cpu_to_le32(atomic_read(&rdev->corrected_errors));

	sb->raid_disks = cpu_to_le32(mddev->raid_disks);
	sb->size = cpu_to_le64(mddev->size<<1);

	if (mddev->bitmap && mddev->bitmap_file == NULL) {
		sb->bitmap_offset = cpu_to_le32((__u32)mddev->bitmap_offset);
		sb->feature_map = cpu_to_le32(MD_FEATURE_BITMAP_OFFSET);
	}

	if (rdev->raid_disk >= 0 &&
	    !test_bit(In_sync, &rdev->flags) &&
	    rdev->recovery_offset > 0) {
		sb->feature_map |= cpu_to_le32(MD_FEATURE_RECOVERY_OFFSET);
		sb->recovery_offset = cpu_to_le64(rdev->recovery_offset);
	}

	if (mddev->reshape_position != MaxSector) {
		sb->feature_map |= cpu_to_le32(MD_FEATURE_RESHAPE_ACTIVE);
		sb->reshape_position = cpu_to_le64(mddev->reshape_position);
		sb->new_layout = cpu_to_le32(mddev->new_layout);
		sb->delta_disks = cpu_to_le32(mddev->delta_disks);
		sb->new_level = cpu_to_le32(mddev->new_level);
		sb->new_chunk = cpu_to_le32(mddev->new_chunk>>9);
	}

	max_dev = 0;
	list_for_each_entry(rdev2, &mddev->disks, same_set)
		if (rdev2->desc_nr+1 > max_dev)
			max_dev = rdev2->desc_nr+1;

	if (max_dev > le32_to_cpu(sb->max_dev))
		sb->max_dev = cpu_to_le32(max_dev);
	for (i=0; i<max_dev;i++)
		sb->dev_roles[i] = cpu_to_le16(0xfffe);
	
	list_for_each_entry(rdev2, &mddev->disks, same_set) {
		i = rdev2->desc_nr;
		if (test_bit(Faulty, &rdev2->flags))
			sb->dev_roles[i] = cpu_to_le16(0xfffe);
		else if (test_bit(In_sync, &rdev2->flags))
			sb->dev_roles[i] = cpu_to_le16(rdev2->raid_disk);
		else if (rdev2->raid_disk >= 0 && rdev2->recovery_offset > 0)
			sb->dev_roles[i] = cpu_to_le16(rdev2->raid_disk);
		else
			sb->dev_roles[i] = cpu_to_le16(0xffff);
	}

	sb->sb_csum = calc_sb_1_csum(sb);
}

static unsigned long long
super_1_rdev_size_change(mdk_rdev_t *rdev, sector_t num_sectors)
{
	struct mdp_superblock_1 *sb;
	sector_t max_sectors;
	if (num_sectors && num_sectors < rdev->mddev->size * 2)
		return 0; /* component must fit device */
	if (rdev->sb_start < rdev->data_offset) {
		/* minor versions 1 and 2; superblock before data */
		max_sectors = rdev->bdev->bd_inode->i_size >> 9;
		max_sectors -= rdev->data_offset;
		if (!num_sectors || num_sectors > max_sectors)
			num_sectors = max_sectors;
	} else if (rdev->mddev->bitmap_offset) {
		/* minor version 0 with bitmap we can't move */
		return 0;
	} else {
		/* minor version 0; superblock after data */
		sector_t sb_start;
		sb_start = (rdev->bdev->bd_inode->i_size >> 9) - 8*2;
		sb_start &= ~(sector_t)(4*2 - 1);
		max_sectors = rdev->size * 2 + sb_start - rdev->sb_start;
		if (!num_sectors || num_sectors > max_sectors)
			num_sectors = max_sectors;
		rdev->sb_start = sb_start;
	}
	sb = (struct mdp_superblock_1 *) page_address(rdev->sb_page);
	sb->data_size = cpu_to_le64(num_sectors);
	sb->super_offset = rdev->sb_start;
	sb->sb_csum = calc_sb_1_csum(sb);
	md_super_write(rdev->mddev, rdev, rdev->sb_start, rdev->sb_size,
		       rdev->sb_page);
	md_super_wait(rdev->mddev);
	return num_sectors / 2; /* kB for sysfs */
}

static struct super_type super_types[] = {
	[0] = {
		.name	= "0.90.0",
		.owner	= THIS_MODULE,
		.load_super	    = super_90_load,
		.validate_super	    = super_90_validate,
		.sync_super	    = super_90_sync,
		.rdev_size_change   = super_90_rdev_size_change,
	},
	[1] = {
		.name	= "md-1",
		.owner	= THIS_MODULE,
		.load_super	    = super_1_load,
		.validate_super	    = super_1_validate,
		.sync_super	    = super_1_sync,
		.rdev_size_change   = super_1_rdev_size_change,
	},
};

static int match_mddev_units(mddev_t *mddev1, mddev_t *mddev2)
{
	mdk_rdev_t *rdev, *rdev2;

	rcu_read_lock();
	rdev_for_each_rcu(rdev, mddev1)
		rdev_for_each_rcu(rdev2, mddev2)
			if (rdev->bdev->bd_contains ==
			    rdev2->bdev->bd_contains) {
				rcu_read_unlock();
				return 1;
			}
	rcu_read_unlock();
	return 0;
}

static LIST_HEAD(pending_raid_disks);

static int bind_rdev_to_array(mdk_rdev_t * rdev, mddev_t * mddev)
{
	char b[BDEVNAME_SIZE];
	struct kobject *ko;
	char *s;
	int err;

	if (rdev->mddev) {
		MD_BUG();
		return -EINVAL;
	}

	/* prevent duplicates */
	if (find_rdev(mddev, rdev->bdev->bd_dev))
		return -EEXIST;

	/* make sure rdev->size exceeds mddev->size */
	if (rdev->size && (mddev->size == 0 || rdev->size < mddev->size)) {
		if (mddev->pers) {
			/* Cannot change size, so fail
			 * If mddev->level <= 0, then we don't care
			 * about aligning sizes (e.g. linear)
			 */
			if (mddev->level > 0)
				return -ENOSPC;
		} else
			mddev->size = rdev->size;
	}

	/* Verify rdev->desc_nr is unique.
	 * If it is -1, assign a free number, else
	 * check number is not in use
	 */
	if (rdev->desc_nr < 0) {
		int choice = 0;
		if (mddev->pers) choice = mddev->raid_disks;
		while (find_rdev_nr(mddev, choice))
			choice++;
		rdev->desc_nr = choice;
	} else {
		if (find_rdev_nr(mddev, rdev->desc_nr))
			return -EBUSY;
	}
	if (mddev->max_disks && rdev->desc_nr >= mddev->max_disks) {
		printk(KERN_WARNING "md: %s: array is limited to %d devices\n",
		       mdname(mddev), mddev->max_disks);
		return -EBUSY;
	}
	bdevname(rdev->bdev,b);
	while ( (s=strchr(b, '/')) != NULL)
		*s = '!';

	rdev->mddev = mddev;
	printk(KERN_INFO "md: bind<%s>\n", b);

	if ((err = kobject_add(&rdev->kobj, &mddev->kobj, "dev-%s", b)))
		goto fail;

	ko = &part_to_dev(rdev->bdev->bd_part)->kobj;
	if ((err = sysfs_create_link(&rdev->kobj, ko, "block"))) {
		kobject_del(&rdev->kobj);
		goto fail;
	}
	rdev->sysfs_state = sysfs_get_dirent(rdev->kobj.sd, "state");

	list_add_rcu(&rdev->same_set, &mddev->disks);
	bd_claim_by_disk(rdev->bdev, rdev->bdev->bd_holder, mddev->gendisk);

	/* May as well allow recovery to be retried once */
	mddev->recovery_disabled = 0;
	return 0;

 fail:
	printk(KERN_WARNING "md: failed to register dev-%s for %s\n",
	       b, mdname(mddev));
	return err;
}

static void md_delayed_delete(struct work_struct *ws)
{
	mdk_rdev_t *rdev = container_of(ws, mdk_rdev_t, del_work);
	kobject_del(&rdev->kobj);
	kobject_put(&rdev->kobj);
}

static void unbind_rdev_from_array(mdk_rdev_t * rdev)
{
	char b[BDEVNAME_SIZE];
	if (!rdev->mddev) {
		MD_BUG();
		return;
	}
	bd_release_from_disk(rdev->bdev, rdev->mddev->gendisk);
	list_del_rcu(&rdev->same_set);
	printk(KERN_INFO "md: unbind<%s>\n", bdevname(rdev->bdev,b));
	rdev->mddev = NULL;
	sysfs_remove_link(&rdev->kobj, "block");
	sysfs_put(rdev->sysfs_state);
	rdev->sysfs_state = NULL;
	/* We need to delay this, otherwise we can deadlock when
	 * writing to 'remove' to "dev/state".  We also need
	 * to delay it due to rcu usage.
	 */
	synchronize_rcu();
	INIT_WORK(&rdev->del_work, md_delayed_delete);
	kobject_get(&rdev->kobj);
	schedule_work(&rdev->del_work);
}

static int lock_rdev(mdk_rdev_t *rdev, dev_t dev, int shared)
{
	int err = 0;
	struct block_device *bdev;
	char b[BDEVNAME_SIZE];

	bdev = open_by_devnum(dev, FMODE_READ|FMODE_WRITE);
	if (IS_ERR(bdev)) {
		printk(KERN_ERR "md: could not open %s.\n",
			__bdevname(dev, b));
		return PTR_ERR(bdev);
	}
	err = bd_claim(bdev, shared ? (mdk_rdev_t *)lock_rdev : rdev);
	if (err) {
		printk(KERN_ERR "md: could not bd_claim %s.\n",
			bdevname(bdev, b));
		blkdev_put(bdev, FMODE_READ|FMODE_WRITE);
		return err;
	}
	if (!shared)
		set_bit(AllReserved, &rdev->flags);
	rdev->bdev = bdev;
	return err;
}

static void unlock_rdev(mdk_rdev_t *rdev)
{
	struct block_device *bdev = rdev->bdev;
	rdev->bdev = NULL;
	if (!bdev)
		MD_BUG();
	bd_release(bdev);
	blkdev_put(bdev, FMODE_READ|FMODE_WRITE);
}

void md_autodetect_dev(dev_t dev);

static void export_rdev(mdk_rdev_t * rdev)
{
	char b[BDEVNAME_SIZE];
	printk(KERN_INFO "md: export_rdev(%s)\n",
		bdevname(rdev->bdev,b));
	if (rdev->mddev)
		MD_BUG();
	free_disk_sb(rdev);
#ifndef MODULE
	if (test_bit(AutoDetected, &rdev->flags))
		md_autodetect_dev(rdev->bdev->bd_dev);
#endif
	unlock_rdev(rdev);
	kobject_put(&rdev->kobj);
}

static void kick_rdev_from_array(mdk_rdev_t * rdev)
{
	unbind_rdev_from_array(rdev);
	export_rdev(rdev);
}

static void export_array(mddev_t *mddev)
{
	mdk_rdev_t *rdev, *tmp;

	rdev_for_each(rdev, tmp, mddev) {
		if (!rdev->mddev) {
			MD_BUG();
			continue;
		}
		kick_rdev_from_array(rdev);
	}
	if (!list_empty(&mddev->disks))
		MD_BUG();
	mddev->raid_disks = 0;
	mddev->major_version = 0;
}

static void print_desc(mdp_disk_t *desc)
{
	printk(" DISK<N:%d,(%d,%d),R:%d,S:%d>\n", desc->number,
		desc->major,desc->minor,desc->raid_disk,desc->state);
}

static void print_sb_90(mdp_super_t *sb)
{
	int i;

	printk(KERN_INFO 
		"md:  SB: (V:%d.%d.%d) ID:<%08x.%08x.%08x.%08x> CT:%08x\n",
		sb->major_version, sb->minor_version, sb->patch_version,
		sb->set_uuid0, sb->set_uuid1, sb->set_uuid2, sb->set_uuid3,
		sb->ctime);
	printk(KERN_INFO "md:     L%d S%08d ND:%d RD:%d md%d LO:%d CS:%d\n",
		sb->level, sb->size, sb->nr_disks, sb->raid_disks,
		sb->md_minor, sb->layout, sb->chunk_size);
	printk(KERN_INFO "md:     UT:%08x ST:%d AD:%d WD:%d"
		" FD:%d SD:%d CSUM:%08x E:%08lx\n",
		sb->utime, sb->state, sb->active_disks, sb->working_disks,
		sb->failed_disks, sb->spare_disks,
		sb->sb_csum, (unsigned long)sb->events_lo);

	printk(KERN_INFO);
	for (i = 0; i < MD_SB_DISKS; i++) {
		mdp_disk_t *desc;

		desc = sb->disks + i;
		if (desc->number || desc->major || desc->minor ||
		    desc->raid_disk || (desc->state && (desc->state != 4))) {
			printk("     D %2d: ", i);
			print_desc(desc);
		}
	}
	printk(KERN_INFO "md:     THIS: ");
	print_desc(&sb->this_disk);
}

static void print_sb_1(struct mdp_superblock_1 *sb)
{
	__u8 *uuid;

	uuid = sb->set_uuid;
	printk(KERN_INFO "md:  SB: (V:%u) (F:0x%08x) Array-ID:<%02x%02x%02x%02x"
			":%02x%02x:%02x%02x:%02x%02x:%02x%02x%02x%02x%02x%02x>\n"
	       KERN_INFO "md:    Name: \"%s\" CT:%llu\n",
		le32_to_cpu(sb->major_version),
		le32_to_cpu(sb->feature_map),
		uuid[0], uuid[1], uuid[2], uuid[3],
		uuid[4], uuid[5], uuid[6], uuid[7],
		uuid[8], uuid[9], uuid[10], uuid[11],
		uuid[12], uuid[13], uuid[14], uuid[15],
		sb->set_name,
		(unsigned long long)le64_to_cpu(sb->ctime)
		       & MD_SUPERBLOCK_1_TIME_SEC_MASK);

	uuid = sb->device_uuid;
	printk(KERN_INFO "md:       L%u SZ%llu RD:%u LO:%u CS:%u DO:%llu DS:%llu SO:%llu"
			" RO:%llu\n"
	       KERN_INFO "md:     Dev:%08x UUID: %02x%02x%02x%02x:%02x%02x:%02x%02x:%02x%02x"
			":%02x%02x%02x%02x%02x%02x\n"
	       KERN_INFO "md:       (F:0x%08x) UT:%llu Events:%llu ResyncOffset:%llu CSUM:0x%08x\n"
	       KERN_INFO "md:         (MaxDev:%u) \n",
		le32_to_cpu(sb->level),
		(unsigned long long)le64_to_cpu(sb->size),
		le32_to_cpu(sb->raid_disks),
		le32_to_cpu(sb->layout),
		le32_to_cpu(sb->chunksize),
		(unsigned long long)le64_to_cpu(sb->data_offset),
		(unsigned long long)le64_to_cpu(sb->data_size),
		(unsigned long long)le64_to_cpu(sb->super_offset),
		(unsigned long long)le64_to_cpu(sb->recovery_offset),
		le32_to_cpu(sb->dev_number),
		uuid[0], uuid[1], uuid[2], uuid[3],
		uuid[4], uuid[5], uuid[6], uuid[7],
		uuid[8], uuid[9], uuid[10], uuid[11],
		uuid[12], uuid[13], uuid[14], uuid[15],
		sb->devflags,
		(unsigned long long)le64_to_cpu(sb->utime) & MD_SUPERBLOCK_1_TIME_SEC_MASK,
		(unsigned long long)le64_to_cpu(sb->events),
		(unsigned long long)le64_to_cpu(sb->resync_offset),
		le32_to_cpu(sb->sb_csum),
		le32_to_cpu(sb->max_dev)
		);
}

static void print_rdev(mdk_rdev_t *rdev, int major_version)
{
	char b[BDEVNAME_SIZE];
	printk(KERN_INFO "md: rdev %s, SZ:%08llu F:%d S:%d DN:%u\n",
		bdevname(rdev->bdev,b), (unsigned long long)rdev->size,
	        test_bit(Faulty, &rdev->flags), test_bit(In_sync, &rdev->flags),
	        rdev->desc_nr);
	if (rdev->sb_loaded) {
		printk(KERN_INFO "md: rdev superblock (MJ:%d):\n", major_version);
		switch (major_version) {
		case 0:
			print_sb_90((mdp_super_t*)page_address(rdev->sb_page));
			break;
		case 1:
			print_sb_1((struct mdp_superblock_1 *)page_address(rdev->sb_page));
			break;
		}
	} else
		printk(KERN_INFO "md: no rdev superblock!\n");
}

static void md_print_devices(void)
{
	struct list_head *tmp;
	mdk_rdev_t *rdev;
	mddev_t *mddev;
	char b[BDEVNAME_SIZE];

	printk("\n");
	printk("md:	**********************************\n");
	printk("md:	* <COMPLETE RAID STATE PRINTOUT> *\n");
	printk("md:	**********************************\n");
	for_each_mddev(mddev, tmp) {

		if (mddev->bitmap)
			bitmap_print_sb(mddev->bitmap);
		else
			printk("%s: ", mdname(mddev));
		list_for_each_entry(rdev, &mddev->disks, same_set)
			printk("<%s>", bdevname(rdev->bdev,b));
		printk("\n");

		list_for_each_entry(rdev, &mddev->disks, same_set)
			print_rdev(rdev, mddev->major_version);
	}
	printk("md:	**********************************\n");
	printk("\n");
}


static void sync_sbs(mddev_t * mddev, int nospares)
{
	/* Update each superblock (in-memory image), but
	 * if we are allowed to, skip spares which already
	 * have the right event counter, or have one earlier
	 * (which would mean they aren't being marked as dirty
	 * with the rest of the array)
	 */
	mdk_rdev_t *rdev;

	list_for_each_entry(rdev, &mddev->disks, same_set) {
		if (rdev->sb_events == mddev->events ||
		    (nospares &&
		     rdev->raid_disk < 0 &&
		     (rdev->sb_events&1)==0 &&
		     rdev->sb_events+1 == mddev->events)) {
			/* Don't update this superblock */
			rdev->sb_loaded = 2;
		} else {
			super_types[mddev->major_version].
				sync_super(mddev, rdev);
			rdev->sb_loaded = 1;
		}
	}
}

static void md_update_sb(mddev_t * mddev, int force_change)
{
	mdk_rdev_t *rdev;
	int sync_req;
	int nospares = 0;

	if (mddev->external)
		return;
repeat:
	spin_lock_irq(&mddev->write_lock);

	set_bit(MD_CHANGE_PENDING, &mddev->flags);
	if (test_and_clear_bit(MD_CHANGE_DEVS, &mddev->flags))
		force_change = 1;
	if (test_and_clear_bit(MD_CHANGE_CLEAN, &mddev->flags))
		/* just a clean<-> dirty transition, possibly leave spares alone,
		 * though if events isn't the right even/odd, we will have to do
		 * spares after all
		 */
		nospares = 1;
	if (force_change)
		nospares = 0;
	if (mddev->degraded)
		/* If the array is degraded, then skipping spares is both
		 * dangerous and fairly pointless.
		 * Dangerous because a device that was removed from the array
		 * might have a event_count that still looks up-to-date,
		 * so it can be re-added without a resync.
		 * Pointless because if there are any spares to skip,
		 * then a recovery will happen and soon that array won't
		 * be degraded any more and the spare can go back to sleep then.
		 */
		nospares = 0;

	sync_req = mddev->in_sync;
	mddev->utime = get_seconds();

	/* If this is just a dirty<->clean transition, and the array is clean
	 * and 'events' is odd, we can roll back to the previous clean state */
	if (nospares
	    && (mddev->in_sync && mddev->recovery_cp == MaxSector)
	    && (mddev->events & 1)
	    && mddev->events != 1)
		mddev->events--;
	else {
		/* otherwise we have to go forward and ... */
		mddev->events ++;
		if (!mddev->in_sync || mddev->recovery_cp != MaxSector) { /* not clean */
			/* .. if the array isn't clean, insist on an odd 'events' */
			if ((mddev->events&1)==0) {
				mddev->events++;
				nospares = 0;
			}
		} else {
			/* otherwise insist on an even 'events' (for clean states) */
			if ((mddev->events&1)) {
				mddev->events++;
				nospares = 0;
			}
		}
	}

	if (!mddev->events) {
		/*
		 * oops, this 64-bit counter should never wrap.
		 * Either we are in around ~1 trillion A.C., assuming
		 * 1 reboot per second, or we have a bug:
		 */
		MD_BUG();
		mddev->events --;
	}

	/*
	 * do not write anything to disk if using
	 * nonpersistent superblocks
	 */
	if (!mddev->persistent) {
		if (!mddev->external)
			clear_bit(MD_CHANGE_PENDING, &mddev->flags);

		spin_unlock_irq(&mddev->write_lock);
		wake_up(&mddev->sb_wait);
		return;
	}
	sync_sbs(mddev, nospares);
	spin_unlock_irq(&mddev->write_lock);

	dprintk(KERN_INFO 
		"md: updating %s RAID superblock on device (in sync %d)\n",
		mdname(mddev),mddev->in_sync);

	bitmap_update_sb(mddev->bitmap);
	list_for_each_entry(rdev, &mddev->disks, same_set) {
		char b[BDEVNAME_SIZE];
		dprintk(KERN_INFO "md: ");
		if (rdev->sb_loaded != 1)
			continue; /* no noise on spare devices */
		if (test_bit(Faulty, &rdev->flags))
			dprintk("(skipping faulty ");

		dprintk("%s ", bdevname(rdev->bdev,b));
		if (!test_bit(Faulty, &rdev->flags)) {
			md_super_write(mddev,rdev,
				       rdev->sb_start, rdev->sb_size,
				       rdev->sb_page);
			dprintk(KERN_INFO "(write) %s's sb offset: %llu\n",
				bdevname(rdev->bdev,b),
				(unsigned long long)rdev->sb_start);
			rdev->sb_events = mddev->events;

		} else
			dprintk(")\n");
		if (mddev->level == LEVEL_MULTIPATH)
			/* only need to write one superblock... */
			break;
	}
	md_super_wait(mddev);
	/* if there was a failure, MD_CHANGE_DEVS was set, and we re-write super */

	spin_lock_irq(&mddev->write_lock);
	if (mddev->in_sync != sync_req ||
	    test_bit(MD_CHANGE_DEVS, &mddev->flags)) {
		/* have to write it out again */
		spin_unlock_irq(&mddev->write_lock);
		goto repeat;
	}
	clear_bit(MD_CHANGE_PENDING, &mddev->flags);
	spin_unlock_irq(&mddev->write_lock);
	wake_up(&mddev->sb_wait);

}

static int cmd_match(const char *cmd, const char *str)
{
	/* See if cmd, written into a sysfs file, matches
	 * str.  They must either be the same, or cmd can
	 * have a trailing newline
	 */
	while (*cmd && *str && *cmd == *str) {
		cmd++;
		str++;
	}
	if (*cmd == '\n')
		cmd++;
	if (*str || *cmd)
		return 0;
	return 1;
}

struct rdev_sysfs_entry {
	struct attribute attr;
	ssize_t (*show)(mdk_rdev_t *, char *);
	ssize_t (*store)(mdk_rdev_t *, const char *, size_t);
};

static ssize_t
state_show(mdk_rdev_t *rdev, char *page)
{
	char *sep = "";
	size_t len = 0;

	if (test_bit(Faulty, &rdev->flags)) {
		len+= sprintf(page+len, "%sfaulty",sep);
		sep = ",";
	}
	if (test_bit(In_sync, &rdev->flags)) {
		len += sprintf(page+len, "%sin_sync",sep);
		sep = ",";
	}
	if (test_bit(WriteMostly, &rdev->flags)) {
		len += sprintf(page+len, "%swrite_mostly",sep);
		sep = ",";
	}
	if (test_bit(Blocked, &rdev->flags)) {
		len += sprintf(page+len, "%sblocked", sep);
		sep = ",";
	}
	if (!test_bit(Faulty, &rdev->flags) &&
	    !test_bit(In_sync, &rdev->flags)) {
		len += sprintf(page+len, "%sspare", sep);
		sep = ",";
	}
	return len+sprintf(page+len, "\n");
}

static ssize_t
state_store(mdk_rdev_t *rdev, const char *buf, size_t len)
{
	/* can write
	 *  faulty  - simulates and error
	 *  remove  - disconnects the device
	 *  writemostly - sets write_mostly
	 *  -writemostly - clears write_mostly
	 *  blocked - sets the Blocked flag
	 *  -blocked - clears the Blocked flag
	 */
	int err = -EINVAL;
	if (cmd_match(buf, "faulty") && rdev->mddev->pers) {
		md_error(rdev->mddev, rdev);
		err = 0;
	} else if (cmd_match(buf, "remove")) {
		if (rdev->raid_disk >= 0)
			err = -EBUSY;
		else {
			mddev_t *mddev = rdev->mddev;
			kick_rdev_from_array(rdev);
			if (mddev->pers)
				md_update_sb(mddev, 1);
			md_new_event(mddev);
			err = 0;
		}
	} else if (cmd_match(buf, "writemostly")) {
		set_bit(WriteMostly, &rdev->flags);
		err = 0;
	} else if (cmd_match(buf, "-writemostly")) {
		clear_bit(WriteMostly, &rdev->flags);
		err = 0;
	} else if (cmd_match(buf, "blocked")) {
		set_bit(Blocked, &rdev->flags);
		err = 0;
	} else if (cmd_match(buf, "-blocked")) {
		clear_bit(Blocked, &rdev->flags);
		wake_up(&rdev->blocked_wait);
		set_bit(MD_RECOVERY_NEEDED, &rdev->mddev->recovery);
		md_wakeup_thread(rdev->mddev->thread);

		err = 0;
	}
	if (!err && rdev->sysfs_state)
		sysfs_notify_dirent(rdev->sysfs_state);
	return err ? err : len;
}
static struct rdev_sysfs_entry rdev_state =
__ATTR(state, S_IRUGO|S_IWUSR, state_show, state_store);

static ssize_t
errors_show(mdk_rdev_t *rdev, char *page)
{
	return sprintf(page, "%d\n", atomic_read(&rdev->corrected_errors));
}

static ssize_t
errors_store(mdk_rdev_t *rdev, const char *buf, size_t len)
{
	char *e;
	unsigned long n = simple_strtoul(buf, &e, 10);
	if (*buf && (*e == 0 || *e == '\n')) {
		atomic_set(&rdev->corrected_errors, n);
		return len;
	}
	return -EINVAL;
}
static struct rdev_sysfs_entry rdev_errors =
__ATTR(errors, S_IRUGO|S_IWUSR, errors_show, errors_store);

static ssize_t
slot_show(mdk_rdev_t *rdev, char *page)
{
	if (rdev->raid_disk < 0)
		return sprintf(page, "none\n");
	else
		return sprintf(page, "%d\n", rdev->raid_disk);
}

static ssize_t
slot_store(mdk_rdev_t *rdev, const char *buf, size_t len)
{
	char *e;
	int err;
	char nm[20];
	int slot = simple_strtoul(buf, &e, 10);
	if (strncmp(buf, "none", 4)==0)
		slot = -1;
	else if (e==buf || (*e && *e!= '\n'))
		return -EINVAL;
	if (rdev->mddev->pers && slot == -1) {
		/* Setting 'slot' on an active array requires also
		 * updating the 'rd%d' link, and communicating
		 * with the personality with ->hot_*_disk.
		 * For now we only support removing
		 * failed/spare devices.  This normally happens automatically,
		 * but not when the metadata is externally managed.
		 */
		if (rdev->raid_disk == -1)
			return -EEXIST;
		/* personality does all needed checks */
		if (rdev->mddev->pers->hot_add_disk == NULL)
			return -EINVAL;
		err = rdev->mddev->pers->
			hot_remove_disk(rdev->mddev, rdev->raid_disk);
		if (err)
			return err;
		sprintf(nm, "rd%d", rdev->raid_disk);
		sysfs_remove_link(&rdev->mddev->kobj, nm);
		set_bit(MD_RECOVERY_NEEDED, &rdev->mddev->recovery);
		md_wakeup_thread(rdev->mddev->thread);
	} else if (rdev->mddev->pers) {
		mdk_rdev_t *rdev2;
		/* Activating a spare .. or possibly reactivating
		 * if we every get bitmaps working here.
		 */

		if (rdev->raid_disk != -1)
			return -EBUSY;

		if (rdev->mddev->pers->hot_add_disk == NULL)
			return -EINVAL;

		list_for_each_entry(rdev2, &rdev->mddev->disks, same_set)
			if (rdev2->raid_disk == slot)
				return -EEXIST;

		rdev->raid_disk = slot;
		if (test_bit(In_sync, &rdev->flags))
			rdev->saved_raid_disk = slot;
		else
			rdev->saved_raid_disk = -1;
		err = rdev->mddev->pers->
			hot_add_disk(rdev->mddev, rdev);
		if (err) {
			rdev->raid_disk = -1;
			return err;
		} else
			sysfs_notify_dirent(rdev->sysfs_state);
		sprintf(nm, "rd%d", rdev->raid_disk);
		if (sysfs_create_link(&rdev->mddev->kobj, &rdev->kobj, nm))
			printk(KERN_WARNING
			       "md: cannot register "
			       "%s for %s\n",
			       nm, mdname(rdev->mddev));

		/* don't wakeup anyone, leave that to userspace. */
	} else {
		if (slot >= rdev->mddev->raid_disks)
			return -ENOSPC;
		rdev->raid_disk = slot;
		/* assume it is working */
		clear_bit(Faulty, &rdev->flags);
		clear_bit(WriteMostly, &rdev->flags);
		set_bit(In_sync, &rdev->flags);
		sysfs_notify_dirent(rdev->sysfs_state);
	}
	return len;
}


static struct rdev_sysfs_entry rdev_slot =
__ATTR(slot, S_IRUGO|S_IWUSR, slot_show, slot_store);

static ssize_t
offset_show(mdk_rdev_t *rdev, char *page)
{
	return sprintf(page, "%llu\n", (unsigned long long)rdev->data_offset);
}

static ssize_t
offset_store(mdk_rdev_t *rdev, const char *buf, size_t len)
{
	char *e;
	unsigned long long offset = simple_strtoull(buf, &e, 10);
	if (e==buf || (*e && *e != '\n'))
		return -EINVAL;
	if (rdev->mddev->pers && rdev->raid_disk >= 0)
		return -EBUSY;
	if (rdev->size && rdev->mddev->external)
		/* Must set offset before size, so overlap checks
		 * can be sane */
		return -EBUSY;
	rdev->data_offset = offset;
	return len;
}

static struct rdev_sysfs_entry rdev_offset =
__ATTR(offset, S_IRUGO|S_IWUSR, offset_show, offset_store);

static ssize_t
rdev_size_show(mdk_rdev_t *rdev, char *page)
{
	return sprintf(page, "%llu\n", (unsigned long long)rdev->size);
}

static int overlaps(sector_t s1, sector_t l1, sector_t s2, sector_t l2)
{
	/* check if two start/length pairs overlap */
	if (s1+l1 <= s2)
		return 0;
	if (s2+l2 <= s1)
		return 0;
	return 1;
}

static ssize_t
rdev_size_store(mdk_rdev_t *rdev, const char *buf, size_t len)
{
	unsigned long long size;
	unsigned long long oldsize = rdev->size;
	mddev_t *my_mddev = rdev->mddev;

	if (strict_strtoull(buf, 10, &size) < 0)
		return -EINVAL;
	if (my_mddev->pers && rdev->raid_disk >= 0) {
		if (my_mddev->persistent) {
			size = super_types[my_mddev->major_version].
				rdev_size_change(rdev, size * 2);
			if (!size)
				return -EBUSY;
		} else if (!size) {
			size = (rdev->bdev->bd_inode->i_size >> 10);
			size -= rdev->data_offset/2;
		}
	}
	if (size < my_mddev->size)
		return -EINVAL; /* component must fit device */

	rdev->size = size;
	if (size > oldsize && my_mddev->external) {
		/* need to check that all other rdevs with the same ->bdev
		 * do not overlap.  We need to unlock the mddev to avoid
		 * a deadlock.  We have already changed rdev->size, and if
		 * we have to change it back, we will have the lock again.
		 */
		mddev_t *mddev;
		int overlap = 0;
		struct list_head *tmp;

		mddev_unlock(my_mddev);
		for_each_mddev(mddev, tmp) {
			mdk_rdev_t *rdev2;

			mddev_lock(mddev);
			list_for_each_entry(rdev2, &mddev->disks, same_set)
				if (test_bit(AllReserved, &rdev2->flags) ||
				    (rdev->bdev == rdev2->bdev &&
				     rdev != rdev2 &&
				     overlaps(rdev->data_offset, rdev->size * 2,
					      rdev2->data_offset,
					      rdev2->size * 2))) {
					overlap = 1;
					break;
				}
			mddev_unlock(mddev);
			if (overlap) {
				mddev_put(mddev);
				break;
			}
		}
		mddev_lock(my_mddev);
		if (overlap) {
			/* Someone else could have slipped in a size
			 * change here, but doing so is just silly.
			 * We put oldsize back because we *know* it is
			 * safe, and trust userspace not to race with
			 * itself
			 */
			rdev->size = oldsize;
			return -EBUSY;
		}
	}
	return len;
}

static struct rdev_sysfs_entry rdev_size =
__ATTR(size, S_IRUGO|S_IWUSR, rdev_size_show, rdev_size_store);

static struct attribute *rdev_default_attrs[] = {
	&rdev_state.attr,
	&rdev_errors.attr,
	&rdev_slot.attr,
	&rdev_offset.attr,
	&rdev_size.attr,
	NULL,
};
static ssize_t
rdev_attr_show(struct kobject *kobj, struct attribute *attr, char *page)
{
	struct rdev_sysfs_entry *entry = container_of(attr, struct rdev_sysfs_entry, attr);
	mdk_rdev_t *rdev = container_of(kobj, mdk_rdev_t, kobj);
	mddev_t *mddev = rdev->mddev;
	ssize_t rv;

	if (!entry->show)
		return -EIO;

	rv = mddev ? mddev_lock(mddev) : -EBUSY;
	if (!rv) {
		if (rdev->mddev == NULL)
			rv = -EBUSY;
		else
			rv = entry->show(rdev, page);
		mddev_unlock(mddev);
	}
	return rv;
}

static ssize_t
rdev_attr_store(struct kobject *kobj, struct attribute *attr,
	      const char *page, size_t length)
{
	struct rdev_sysfs_entry *entry = container_of(attr, struct rdev_sysfs_entry, attr);
	mdk_rdev_t *rdev = container_of(kobj, mdk_rdev_t, kobj);
	ssize_t rv;
	mddev_t *mddev = rdev->mddev;

	if (!entry->store)
		return -EIO;
	if (!capable(CAP_SYS_ADMIN))
		return -EACCES;
	rv = mddev ? mddev_lock(mddev): -EBUSY;
	if (!rv) {
		if (rdev->mddev == NULL)
			rv = -EBUSY;
		else
			rv = entry->store(rdev, page, length);
		mddev_unlock(mddev);
	}
	return rv;
}

static void rdev_free(struct kobject *ko)
{
	mdk_rdev_t *rdev = container_of(ko, mdk_rdev_t, kobj);
	kfree(rdev);
}
static struct sysfs_ops rdev_sysfs_ops = {
	.show		= rdev_attr_show,
	.store		= rdev_attr_store,
};
static struct kobj_type rdev_ktype = {
	.release	= rdev_free,
	.sysfs_ops	= &rdev_sysfs_ops,
	.default_attrs	= rdev_default_attrs,
};

static mdk_rdev_t *md_import_device(dev_t newdev, int super_format, int super_minor)
{
	char b[BDEVNAME_SIZE];
	int err;
	mdk_rdev_t *rdev;
	sector_t size;

	rdev = kzalloc(sizeof(*rdev), GFP_KERNEL);
	if (!rdev) {
		printk(KERN_ERR "md: could not alloc mem for new device!\n");
		return ERR_PTR(-ENOMEM);
	}

	if ((err = alloc_disk_sb(rdev)))
		goto abort_free;

	err = lock_rdev(rdev, newdev, super_format == -2);
	if (err)
		goto abort_free;

	kobject_init(&rdev->kobj, &rdev_ktype);

	rdev->desc_nr = -1;
	rdev->saved_raid_disk = -1;
	rdev->raid_disk = -1;
	rdev->flags = 0;
	rdev->data_offset = 0;
	rdev->sb_events = 0;
	atomic_set(&rdev->nr_pending, 0);
	atomic_set(&rdev->read_errors, 0);
	atomic_set(&rdev->corrected_errors, 0);

	size = rdev->bdev->bd_inode->i_size >> BLOCK_SIZE_BITS;
	if (!size) {
		printk(KERN_WARNING 
			"md: %s has zero or unknown size, marking faulty!\n",
			bdevname(rdev->bdev,b));
		err = -EINVAL;
		goto abort_free;
	}

	if (super_format >= 0) {
		err = super_types[super_format].
			load_super(rdev, NULL, super_minor);
		if (err == -EINVAL) {
			printk(KERN_WARNING
				"md: %s does not have a valid v%d.%d "
			       "superblock, not importing!\n",
				bdevname(rdev->bdev,b),
			       super_format, super_minor);
			goto abort_free;
		}
		if (err < 0) {
			printk(KERN_WARNING 
				"md: could not read %s's sb, not importing!\n",
				bdevname(rdev->bdev,b));
			goto abort_free;
		}
	}

	INIT_LIST_HEAD(&rdev->same_set);
	init_waitqueue_head(&rdev->blocked_wait);

	return rdev;

abort_free:
	if (rdev->sb_page) {
		if (rdev->bdev)
			unlock_rdev(rdev);
		free_disk_sb(rdev);
	}
	kfree(rdev);
	return ERR_PTR(err);
}



static void analyze_sbs(mddev_t * mddev)
{
	int i;
	mdk_rdev_t *rdev, *freshest, *tmp;
	char b[BDEVNAME_SIZE];

	freshest = NULL;
	rdev_for_each(rdev, tmp, mddev)
		switch (super_types[mddev->major_version].
			load_super(rdev, freshest, mddev->minor_version)) {
		case 1:
			freshest = rdev;
			break;
		case 0:
			break;
		default:
			printk( KERN_ERR \
				"md: fatal superblock inconsistency in %s"
				" -- removing from array\n", 
				bdevname(rdev->bdev,b));
			kick_rdev_from_array(rdev);
		}


	super_types[mddev->major_version].
		validate_super(mddev, freshest);

	i = 0;
	rdev_for_each(rdev, tmp, mddev) {
		if (rdev->desc_nr >= mddev->max_disks ||
		    i > mddev->max_disks) {
			printk(KERN_WARNING
			       "md: %s: %s: only %d devices permitted\n",
			       mdname(mddev), bdevname(rdev->bdev, b),
			       mddev->max_disks);
			kick_rdev_from_array(rdev);
			continue;
		}
		if (rdev != freshest)
			if (super_types[mddev->major_version].
			    validate_super(mddev, rdev)) {
				printk(KERN_WARNING "md: kicking non-fresh %s"
					" from array!\n",
					bdevname(rdev->bdev,b));
				kick_rdev_from_array(rdev);
				continue;
			}
		if (mddev->level == LEVEL_MULTIPATH) {
			rdev->desc_nr = i++;
			rdev->raid_disk = rdev->desc_nr;
			set_bit(In_sync, &rdev->flags);
		} else if (rdev->raid_disk >= mddev->raid_disks) {
			rdev->raid_disk = -1;
			clear_bit(In_sync, &rdev->flags);
		}
	}



	if (mddev->recovery_cp != MaxSector &&
	    mddev->level >= 1)
		printk(KERN_ERR "md: %s: raid array is not clean"
		       " -- starting background reconstruction\n",
		       mdname(mddev));

}

static void md_safemode_timeout(unsigned long data);

static ssize_t
safe_delay_show(mddev_t *mddev, char *page)
{
	int msec = (mddev->safemode_delay*1000)/HZ;
	return sprintf(page, "%d.%03d\n", msec/1000, msec%1000);
}
static ssize_t
safe_delay_store(mddev_t *mddev, const char *cbuf, size_t len)
{
	int scale=1;
	int dot=0;
	int i;
	unsigned long msec;
	char buf[30];

	/* remove a period, and count digits after it */
	if (len >= sizeof(buf))
		return -EINVAL;
	strlcpy(buf, cbuf, sizeof(buf));
	for (i=0; i<len; i++) {
		if (dot) {
			if (isdigit(buf[i])) {
				buf[i-1] = buf[i];
				scale *= 10;
			}
			buf[i] = 0;
		} else if (buf[i] == '.') {
			dot=1;
			buf[i] = 0;
		}
	}
	if (strict_strtoul(buf, 10, &msec) < 0)
		return -EINVAL;
	msec = (msec * 1000) / scale;
	if (msec == 0)
		mddev->safemode_delay = 0;
	else {
		unsigned long old_delay = mddev->safemode_delay;
		mddev->safemode_delay = (msec*HZ)/1000;
		if (mddev->safemode_delay == 0)
			mddev->safemode_delay = 1;
		if (mddev->safemode_delay < old_delay)
			md_safemode_timeout((unsigned long)mddev);
	}
	return len;
}
static struct md_sysfs_entry md_safe_delay =
__ATTR(safe_mode_delay, S_IRUGO|S_IWUSR,safe_delay_show, safe_delay_store);

static ssize_t
level_show(mddev_t *mddev, char *page)
{
	struct mdk_personality *p = mddev->pers;
	if (p)
		return sprintf(page, "%s\n", p->name);
	else if (mddev->clevel[0])
		return sprintf(page, "%s\n", mddev->clevel);
	else if (mddev->level != LEVEL_NONE)
		return sprintf(page, "%d\n", mddev->level);
	else
		return 0;
}

static ssize_t
level_store(mddev_t *mddev, const char *buf, size_t len)
{
	ssize_t rv = len;
	if (mddev->pers)
		return -EBUSY;
	if (len == 0)
		return 0;
	if (len >= sizeof(mddev->clevel))
		return -ENOSPC;
	strncpy(mddev->clevel, buf, len);
	if (mddev->clevel[len-1] == '\n')
		len--;
	mddev->clevel[len] = 0;
	mddev->level = LEVEL_NONE;
	return rv;
}

static struct md_sysfs_entry md_level =
__ATTR(level, S_IRUGO|S_IWUSR, level_show, level_store);


static ssize_t
layout_show(mddev_t *mddev, char *page)
{
	/* just a number, not meaningful for all levels */
	if (mddev->reshape_position != MaxSector &&
	    mddev->layout != mddev->new_layout)
		return sprintf(page, "%d (%d)\n",
			       mddev->new_layout, mddev->layout);
	return sprintf(page, "%d\n", mddev->layout);
}

static ssize_t
layout_store(mddev_t *mddev, const char *buf, size_t len)
{
	char *e;
	unsigned long n = simple_strtoul(buf, &e, 10);

	if (!*buf || (*e && *e != '\n'))
		return -EINVAL;

	if (mddev->pers)
		return -EBUSY;
	if (mddev->reshape_position != MaxSector)
		mddev->new_layout = n;
	else
		mddev->layout = n;
	return len;
}
static struct md_sysfs_entry md_layout =
__ATTR(layout, S_IRUGO|S_IWUSR, layout_show, layout_store);


static ssize_t
raid_disks_show(mddev_t *mddev, char *page)
{
	if (mddev->raid_disks == 0)
		return 0;
	if (mddev->reshape_position != MaxSector &&
	    mddev->delta_disks != 0)
		return sprintf(page, "%d (%d)\n", mddev->raid_disks,
			       mddev->raid_disks - mddev->delta_disks);
	return sprintf(page, "%d\n", mddev->raid_disks);
}

static int update_raid_disks(mddev_t *mddev, int raid_disks);

static ssize_t
raid_disks_store(mddev_t *mddev, const char *buf, size_t len)
{
	char *e;
	int rv = 0;
	unsigned long n = simple_strtoul(buf, &e, 10);

	if (!*buf || (*e && *e != '\n'))
		return -EINVAL;

	if (mddev->pers)
		rv = update_raid_disks(mddev, n);
	else if (mddev->reshape_position != MaxSector) {
		int olddisks = mddev->raid_disks - mddev->delta_disks;
		mddev->delta_disks = n - olddisks;
		mddev->raid_disks = n;
	} else
		mddev->raid_disks = n;
	return rv ? rv : len;
}
static struct md_sysfs_entry md_raid_disks =
__ATTR(raid_disks, S_IRUGO|S_IWUSR, raid_disks_show, raid_disks_store);

static ssize_t
chunk_size_show(mddev_t *mddev, char *page)
{
	if (mddev->reshape_position != MaxSector &&
	    mddev->chunk_size != mddev->new_chunk)
		return sprintf(page, "%d (%d)\n", mddev->new_chunk,
			       mddev->chunk_size);
	return sprintf(page, "%d\n", mddev->chunk_size);
}

static ssize_t
chunk_size_store(mddev_t *mddev, const char *buf, size_t len)
{
	/* can only set chunk_size if array is not yet active */
	char *e;
	unsigned long n = simple_strtoul(buf, &e, 10);

	if (!*buf || (*e && *e != '\n'))
		return -EINVAL;

	if (mddev->pers)
		return -EBUSY;
	else if (mddev->reshape_position != MaxSector)
		mddev->new_chunk = n;
	else
		mddev->chunk_size = n;
	return len;
}
static struct md_sysfs_entry md_chunk_size =
__ATTR(chunk_size, S_IRUGO|S_IWUSR, chunk_size_show, chunk_size_store);

static ssize_t
resync_start_show(mddev_t *mddev, char *page)
{
	return sprintf(page, "%llu\n", (unsigned long long)mddev->recovery_cp);
}

static ssize_t
resync_start_store(mddev_t *mddev, const char *buf, size_t len)
{
	char *e;
	unsigned long long n = simple_strtoull(buf, &e, 10);

	if (mddev->pers)
		return -EBUSY;
	if (!*buf || (*e && *e != '\n'))
		return -EINVAL;

	mddev->recovery_cp = n;
	return len;
}
static struct md_sysfs_entry md_resync_start =
__ATTR(resync_start, S_IRUGO|S_IWUSR, resync_start_show, resync_start_store);

enum array_state { clear, inactive, suspended, readonly, read_auto, clean, active,
		   write_pending, active_idle, bad_word};
static char *array_states[] = {
	"clear", "inactive", "suspended", "readonly", "read-auto", "clean", "active",
	"write-pending", "active-idle", NULL };

static int match_word(const char *word, char **list)
{
	int n;
	for (n=0; list[n]; n++)
		if (cmd_match(word, list[n]))
			break;
	return n;
}

static ssize_t
array_state_show(mddev_t *mddev, char *page)
{
	enum array_state st = inactive;

	if (mddev->pers)
		switch(mddev->ro) {
		case 1:
			st = readonly;
			break;
		case 2:
			st = read_auto;
			break;
		case 0:
			if (mddev->in_sync)
				st = clean;
			else if (test_bit(MD_CHANGE_CLEAN, &mddev->flags))
				st = write_pending;
			else if (mddev->safemode)
				st = active_idle;
			else
				st = active;
		}
	else {
		if (list_empty(&mddev->disks) &&
		    mddev->raid_disks == 0 &&
		    mddev->size == 0)
			st = clear;
		else
			st = inactive;
	}
	return sprintf(page, "%s\n", array_states[st]);
}

static int do_md_stop(mddev_t * mddev, int ro, int is_open);
static int do_md_run(mddev_t * mddev);
static int restart_array(mddev_t *mddev);

static ssize_t
array_state_store(mddev_t *mddev, const char *buf, size_t len)
{
	int err = -EINVAL;
	enum array_state st = match_word(buf, array_states);
	switch(st) {
	case bad_word:
		break;
	case clear:
		/* stopping an active array */
		if (atomic_read(&mddev->openers) > 0)
			return -EBUSY;
		err = do_md_stop(mddev, 0, 0);
		break;
	case inactive:
		/* stopping an active array */
		if (mddev->pers) {
			if (atomic_read(&mddev->openers) > 0)
				return -EBUSY;
			err = do_md_stop(mddev, 2, 0);
		} else
			err = 0; /* already inactive */
		break;
	case suspended:
		break; /* not supported yet */
	case readonly:
		if (mddev->pers)
			err = do_md_stop(mddev, 1, 0);
		else {
			mddev->ro = 1;
			set_disk_ro(mddev->gendisk, 1);
			err = do_md_run(mddev);
		}
		break;
	case read_auto:
		if (mddev->pers) {
			if (mddev->ro == 0)
				err = do_md_stop(mddev, 1, 0);
			else if (mddev->ro == 1)
				err = restart_array(mddev);
			if (err == 0) {
				mddev->ro = 2;
				set_disk_ro(mddev->gendisk, 0);
			}
		} else {
			mddev->ro = 2;
			err = do_md_run(mddev);
		}
		break;
	case clean:
		if (mddev->pers) {
			restart_array(mddev);
			spin_lock_irq(&mddev->write_lock);
			if (atomic_read(&mddev->writes_pending) == 0) {
				if (mddev->in_sync == 0) {
					mddev->in_sync = 1;
					if (mddev->safemode == 1)
						mddev->safemode = 0;
					if (mddev->persistent)
						set_bit(MD_CHANGE_CLEAN,
							&mddev->flags);
				}
				err = 0;
			} else
				err = -EBUSY;
			spin_unlock_irq(&mddev->write_lock);
		} else {
			mddev->ro = 0;
			mddev->recovery_cp = MaxSector;
			err = do_md_run(mddev);
		}
		break;
	case active:
		if (mddev->pers) {
			restart_array(mddev);
			if (mddev->external)
				clear_bit(MD_CHANGE_CLEAN, &mddev->flags);
			wake_up(&mddev->sb_wait);
			err = 0;
		} else {
			mddev->ro = 0;
			set_disk_ro(mddev->gendisk, 0);
			err = do_md_run(mddev);
		}
		break;
	case write_pending:
	case active_idle:
		/* these cannot be set */
		break;
	}
	if (err)
		return err;
	else {
		sysfs_notify_dirent(mddev->sysfs_state);
		return len;
	}
}
static struct md_sysfs_entry md_array_state =
__ATTR(array_state, S_IRUGO|S_IWUSR, array_state_show, array_state_store);

static ssize_t
null_show(mddev_t *mddev, char *page)
{
	return -EINVAL;
}

static ssize_t
new_dev_store(mddev_t *mddev, const char *buf, size_t len)
{
	/* buf must be %d:%d\n? giving major and minor numbers */
	/* The new device is added to the array.
	 * If the array has a persistent superblock, we read the
	 * superblock to initialise info and check validity.
	 * Otherwise, only checking done is that in bind_rdev_to_array,
	 * which mainly checks size.
	 */
	char *e;
	int major = simple_strtoul(buf, &e, 10);
	int minor;
	dev_t dev;
	mdk_rdev_t *rdev;
	int err;

	if (!*buf || *e != ':' || !e[1] || e[1] == '\n')
		return -EINVAL;
	minor = simple_strtoul(e+1, &e, 10);
	if (*e && *e != '\n')
		return -EINVAL;
	dev = MKDEV(major, minor);
	if (major != MAJOR(dev) ||
	    minor != MINOR(dev))
		return -EOVERFLOW;


	if (mddev->persistent) {
		rdev = md_import_device(dev, mddev->major_version,
					mddev->minor_version);
		if (!IS_ERR(rdev) && !list_empty(&mddev->disks)) {
			mdk_rdev_t *rdev0 = list_entry(mddev->disks.next,
						       mdk_rdev_t, same_set);
			err = super_types[mddev->major_version]
				.load_super(rdev, rdev0, mddev->minor_version);
			if (err < 0)
				goto out;
		}
	} else if (mddev->external)
		rdev = md_import_device(dev, -2, -1);
	else
		rdev = md_import_device(dev, -1, -1);

	if (IS_ERR(rdev))
		return PTR_ERR(rdev);
	err = bind_rdev_to_array(rdev, mddev);
 out:
	if (err)
		export_rdev(rdev);
	return err ? err : len;
}

static struct md_sysfs_entry md_new_device =
__ATTR(new_dev, S_IWUSR, null_show, new_dev_store);

static ssize_t
bitmap_store(mddev_t *mddev, const char *buf, size_t len)
{
	char *end;
	unsigned long chunk, end_chunk;

	if (!mddev->bitmap)
		goto out;
	/* buf should be <chunk> <chunk> ... or <chunk>-<chunk> ... (range) */
	while (*buf) {
		chunk = end_chunk = simple_strtoul(buf, &end, 0);
		if (buf == end) break;
		if (*end == '-') { /* range */
			buf = end + 1;
			end_chunk = simple_strtoul(buf, &end, 0);
			if (buf == end) break;
		}
		if (*end && !isspace(*end)) break;
		bitmap_dirty_bits(mddev->bitmap, chunk, end_chunk);
		buf = end;
		while (isspace(*buf)) buf++;
	}
	bitmap_unplug(mddev->bitmap); /* flush the bits to disk */
out:
	return len;
}

static struct md_sysfs_entry md_bitmap =
__ATTR(bitmap_set_bits, S_IWUSR, null_show, bitmap_store);

static ssize_t
size_show(mddev_t *mddev, char *page)
{
	return sprintf(page, "%llu\n", (unsigned long long)mddev->size);
}

static int update_size(mddev_t *mddev, sector_t num_sectors);

static ssize_t
size_store(mddev_t *mddev, const char *buf, size_t len)
{
	/* If array is inactive, we can reduce the component size, but
	 * not increase it (except from 0).
	 * If array is active, we can try an on-line resize
	 */
	char *e;
	int err = 0;
	unsigned long long size = simple_strtoull(buf, &e, 10);
	if (!*buf || *buf == '\n' ||
	    (*e && *e != '\n'))
		return -EINVAL;

	if (mddev->pers) {
		err = update_size(mddev, size * 2);
		md_update_sb(mddev, 1);
	} else {
		if (mddev->size == 0 ||
		    mddev->size > size)
			mddev->size = size;
		else
			err = -ENOSPC;
	}
	return err ? err : len;
}

static struct md_sysfs_entry md_size =
__ATTR(component_size, S_IRUGO|S_IWUSR, size_show, size_store);


static ssize_t
metadata_show(mddev_t *mddev, char *page)
{
	if (mddev->persistent)
		return sprintf(page, "%d.%d\n",
			       mddev->major_version, mddev->minor_version);
	else if (mddev->external)
		return sprintf(page, "external:%s\n", mddev->metadata_type);
	else
		return sprintf(page, "none\n");
}

static ssize_t
metadata_store(mddev_t *mddev, const char *buf, size_t len)
{
	int major, minor;
	char *e;
	/* Changing the details of 'external' metadata is
	 * always permitted.  Otherwise there must be
	 * no devices attached to the array.
	 */
	if (mddev->external && strncmp(buf, "external:", 9) == 0)
		;
	else if (!list_empty(&mddev->disks))
		return -EBUSY;

	if (cmd_match(buf, "none")) {
		mddev->persistent = 0;
		mddev->external = 0;
		mddev->major_version = 0;
		mddev->minor_version = 90;
		return len;
	}
	if (strncmp(buf, "external:", 9) == 0) {
		size_t namelen = len-9;
		if (namelen >= sizeof(mddev->metadata_type))
			namelen = sizeof(mddev->metadata_type)-1;
		strncpy(mddev->metadata_type, buf+9, namelen);
		mddev->metadata_type[namelen] = 0;
		if (namelen && mddev->metadata_type[namelen-1] == '\n')
			mddev->metadata_type[--namelen] = 0;
		mddev->persistent = 0;
		mddev->external = 1;
		mddev->major_version = 0;
		mddev->minor_version = 90;
		return len;
	}
	major = simple_strtoul(buf, &e, 10);
	if (e==buf || *e != '.')
		return -EINVAL;
	buf = e+1;
	minor = simple_strtoul(buf, &e, 10);
	if (e==buf || (*e && *e != '\n') )
		return -EINVAL;
	if (major >= ARRAY_SIZE(super_types) || super_types[major].name == NULL)
		return -ENOENT;
	mddev->major_version = major;
	mddev->minor_version = minor;
	mddev->persistent = 1;
	mddev->external = 0;
	return len;
}

static struct md_sysfs_entry md_metadata =
__ATTR(metadata_version, S_IRUGO|S_IWUSR, metadata_show, metadata_store);

static ssize_t
action_show(mddev_t *mddev, char *page)
{
	char *type = "idle";
	if (test_bit(MD_RECOVERY_RUNNING, &mddev->recovery) ||
	    (!mddev->ro && test_bit(MD_RECOVERY_NEEDED, &mddev->recovery))) {
		if (test_bit(MD_RECOVERY_RESHAPE, &mddev->recovery))
			type = "reshape";
		else if (test_bit(MD_RECOVERY_SYNC, &mddev->recovery)) {
			if (!test_bit(MD_RECOVERY_REQUESTED, &mddev->recovery))
				type = "resync";
			else if (test_bit(MD_RECOVERY_CHECK, &mddev->recovery))
				type = "check";
			else
				type = "repair";
		} else if (test_bit(MD_RECOVERY_RECOVER, &mddev->recovery))
			type = "recover";
	}
	return sprintf(page, "%s\n", type);
}

static ssize_t
action_store(mddev_t *mddev, const char *page, size_t len)
{
	if (!mddev->pers || !mddev->pers->sync_request)
		return -EINVAL;

	if (cmd_match(page, "idle")) {
		if (mddev->sync_thread) {
			set_bit(MD_RECOVERY_INTR, &mddev->recovery);
			md_unregister_thread(mddev->sync_thread);
			mddev->sync_thread = NULL;
			mddev->recovery = 0;
		}
	} else if (test_bit(MD_RECOVERY_RUNNING, &mddev->recovery) ||
		   test_bit(MD_RECOVERY_NEEDED, &mddev->recovery))
		return -EBUSY;
	else if (cmd_match(page, "resync"))
		set_bit(MD_RECOVERY_NEEDED, &mddev->recovery);
	else if (cmd_match(page, "recover")) {
		set_bit(MD_RECOVERY_RECOVER, &mddev->recovery);
		set_bit(MD_RECOVERY_NEEDED, &mddev->recovery);
	} else if (cmd_match(page, "reshape")) {
		int err;
		if (mddev->pers->start_reshape == NULL)
			return -EINVAL;
		err = mddev->pers->start_reshape(mddev);
		if (err)
			return err;
		sysfs_notify(&mddev->kobj, NULL, "degraded");
	} else {
		if (cmd_match(page, "check"))
			set_bit(MD_RECOVERY_CHECK, &mddev->recovery);
		else if (!cmd_match(page, "repair"))
			return -EINVAL;
		set_bit(MD_RECOVERY_REQUESTED, &mddev->recovery);
		set_bit(MD_RECOVERY_SYNC, &mddev->recovery);
	}
	set_bit(MD_RECOVERY_NEEDED, &mddev->recovery);
	md_wakeup_thread(mddev->thread);
	sysfs_notify_dirent(mddev->sysfs_action);
	return len;
}

static ssize_t
mismatch_cnt_show(mddev_t *mddev, char *page)
{
	return sprintf(page, "%llu\n",
		       (unsigned long long) mddev->resync_mismatches);
}

static struct md_sysfs_entry md_scan_mode =
__ATTR(sync_action, S_IRUGO|S_IWUSR, action_show, action_store);


static struct md_sysfs_entry md_mismatches = __ATTR_RO(mismatch_cnt);

static ssize_t
sync_min_show(mddev_t *mddev, char *page)
{
	return sprintf(page, "%d (%s)\n", speed_min(mddev),
		       mddev->sync_speed_min ? "local": "system");
}

static ssize_t
sync_min_store(mddev_t *mddev, const char *buf, size_t len)
{
	int min;
	char *e;
	if (strncmp(buf, "system", 6)==0) {
		mddev->sync_speed_min = 0;
		return len;
	}
	min = simple_strtoul(buf, &e, 10);
	if (buf == e || (*e && *e != '\n') || min <= 0)
		return -EINVAL;
	mddev->sync_speed_min = min;
	return len;
}

static struct md_sysfs_entry md_sync_min =
__ATTR(sync_speed_min, S_IRUGO|S_IWUSR, sync_min_show, sync_min_store);

static ssize_t
sync_max_show(mddev_t *mddev, char *page)
{
	return sprintf(page, "%d (%s)\n", speed_max(mddev),
		       mddev->sync_speed_max ? "local": "system");
}

static ssize_t
sync_max_store(mddev_t *mddev, const char *buf, size_t len)
{
	int max;
	char *e;
	if (strncmp(buf, "system", 6)==0) {
		mddev->sync_speed_max = 0;
		return len;
	}
	max = simple_strtoul(buf, &e, 10);
	if (buf == e || (*e && *e != '\n') || max <= 0)
		return -EINVAL;
	mddev->sync_speed_max = max;
	return len;
}

static struct md_sysfs_entry md_sync_max =
__ATTR(sync_speed_max, S_IRUGO|S_IWUSR, sync_max_show, sync_max_store);

static ssize_t
degraded_show(mddev_t *mddev, char *page)
{
	return sprintf(page, "%d\n", mddev->degraded);
}
static struct md_sysfs_entry md_degraded = __ATTR_RO(degraded);

static ssize_t
sync_force_parallel_show(mddev_t *mddev, char *page)
{
	return sprintf(page, "%d\n", mddev->parallel_resync);
}

static ssize_t
sync_force_parallel_store(mddev_t *mddev, const char *buf, size_t len)
{
	long n;

	if (strict_strtol(buf, 10, &n))
		return -EINVAL;

	if (n != 0 && n != 1)
		return -EINVAL;

	mddev->parallel_resync = n;

	if (mddev->sync_thread)
		wake_up(&resync_wait);

	return len;
}

/* force parallel resync, even with shared block devices */
static struct md_sysfs_entry md_sync_force_parallel =
__ATTR(sync_force_parallel, S_IRUGO|S_IWUSR,
       sync_force_parallel_show, sync_force_parallel_store);

static ssize_t
sync_speed_show(mddev_t *mddev, char *page)
{
	unsigned long resync, dt, db;
	resync = mddev->curr_mark_cnt - atomic_read(&mddev->recovery_active);
	dt = (jiffies - mddev->resync_mark) / HZ;
	if (!dt) dt++;
	db = resync - mddev->resync_mark_cnt;
	return sprintf(page, "%lu\n", db/dt/2); /* K/sec */
}

static struct md_sysfs_entry md_sync_speed = __ATTR_RO(sync_speed);

static ssize_t
sync_completed_show(mddev_t *mddev, char *page)
{
	unsigned long max_blocks, resync;

	if (test_bit(MD_RECOVERY_SYNC, &mddev->recovery))
		max_blocks = mddev->resync_max_sectors;
	else
		max_blocks = mddev->size << 1;

	resync = (mddev->curr_resync - atomic_read(&mddev->recovery_active));
	return sprintf(page, "%lu / %lu\n", resync, max_blocks);
}

static struct md_sysfs_entry md_sync_completed = __ATTR_RO(sync_completed);

static ssize_t
min_sync_show(mddev_t *mddev, char *page)
{
	return sprintf(page, "%llu\n",
		       (unsigned long long)mddev->resync_min);
}
static ssize_t
min_sync_store(mddev_t *mddev, const char *buf, size_t len)
{
	unsigned long long min;
	if (strict_strtoull(buf, 10, &min))
		return -EINVAL;
	if (min > mddev->resync_max)
		return -EINVAL;
	if (test_bit(MD_RECOVERY_RUNNING, &mddev->recovery))
		return -EBUSY;

	/* Must be a multiple of chunk_size */
	if (mddev->chunk_size) {
		if (min & (sector_t)((mddev->chunk_size>>9)-1))
			return -EINVAL;
	}
	mddev->resync_min = min;

	return len;
}

static struct md_sysfs_entry md_min_sync =
__ATTR(sync_min, S_IRUGO|S_IWUSR, min_sync_show, min_sync_store);

static ssize_t
max_sync_show(mddev_t *mddev, char *page)
{
	if (mddev->resync_max == MaxSector)
		return sprintf(page, "max\n");
	else
		return sprintf(page, "%llu\n",
			       (unsigned long long)mddev->resync_max);
}
static ssize_t
max_sync_store(mddev_t *mddev, const char *buf, size_t len)
{
	if (strncmp(buf, "max", 3) == 0)
		mddev->resync_max = MaxSector;
	else {
		unsigned long long max;
		if (strict_strtoull(buf, 10, &max))
			return -EINVAL;
		if (max < mddev->resync_min)
			return -EINVAL;
		if (max < mddev->resync_max &&
		    test_bit(MD_RECOVERY_RUNNING, &mddev->recovery))
			return -EBUSY;

		/* Must be a multiple of chunk_size */
		if (mddev->chunk_size) {
			if (max & (sector_t)((mddev->chunk_size>>9)-1))
				return -EINVAL;
		}
		mddev->resync_max = max;
	}
	wake_up(&mddev->recovery_wait);
	return len;
}

static struct md_sysfs_entry md_max_sync =
__ATTR(sync_max, S_IRUGO|S_IWUSR, max_sync_show, max_sync_store);

static ssize_t
suspend_lo_show(mddev_t *mddev, char *page)
{
	return sprintf(page, "%llu\n", (unsigned long long)mddev->suspend_lo);
}

static ssize_t
suspend_lo_store(mddev_t *mddev, const char *buf, size_t len)
{
	char *e;
	unsigned long long new = simple_strtoull(buf, &e, 10);

	if (mddev->pers->quiesce == NULL)
		return -EINVAL;
	if (buf == e || (*e && *e != '\n'))
		return -EINVAL;
	if (new >= mddev->suspend_hi ||
	    (new > mddev->suspend_lo && new < mddev->suspend_hi)) {
		mddev->suspend_lo = new;
		mddev->pers->quiesce(mddev, 2);
		return len;
	} else
		return -EINVAL;
}
static struct md_sysfs_entry md_suspend_lo =
__ATTR(suspend_lo, S_IRUGO|S_IWUSR, suspend_lo_show, suspend_lo_store);


static ssize_t
suspend_hi_show(mddev_t *mddev, char *page)
{
	return sprintf(page, "%llu\n", (unsigned long long)mddev->suspend_hi);
}

static ssize_t
suspend_hi_store(mddev_t *mddev, const char *buf, size_t len)
{
	char *e;
	unsigned long long new = simple_strtoull(buf, &e, 10);

	if (mddev->pers->quiesce == NULL)
		return -EINVAL;
	if (buf == e || (*e && *e != '\n'))
		return -EINVAL;
	if ((new <= mddev->suspend_lo && mddev->suspend_lo >= mddev->suspend_hi) ||
	    (new > mddev->suspend_lo && new > mddev->suspend_hi)) {
		mddev->suspend_hi = new;
		mddev->pers->quiesce(mddev, 1);
		mddev->pers->quiesce(mddev, 0);
		return len;
	} else
		return -EINVAL;
}
static struct md_sysfs_entry md_suspend_hi =
__ATTR(suspend_hi, S_IRUGO|S_IWUSR, suspend_hi_show, suspend_hi_store);

static ssize_t
reshape_position_show(mddev_t *mddev, char *page)
{
	if (mddev->reshape_position != MaxSector)
		return sprintf(page, "%llu\n",
			       (unsigned long long)mddev->reshape_position);
	strcpy(page, "none\n");
	return 5;
}

static ssize_t
reshape_position_store(mddev_t *mddev, const char *buf, size_t len)
{
	char *e;
	unsigned long long new = simple_strtoull(buf, &e, 10);
	if (mddev->pers)
		return -EBUSY;
	if (buf == e || (*e && *e != '\n'))
		return -EINVAL;
	mddev->reshape_position = new;
	mddev->delta_disks = 0;
	mddev->new_level = mddev->level;
	mddev->new_layout = mddev->layout;
	mddev->new_chunk = mddev->chunk_size;
	return len;
}

static struct md_sysfs_entry md_reshape_position =
__ATTR(reshape_position, S_IRUGO|S_IWUSR, reshape_position_show,
       reshape_position_store);


static struct attribute *md_default_attrs[] = {
	&md_level.attr,
	&md_layout.attr,
	&md_raid_disks.attr,
	&md_chunk_size.attr,
	&md_size.attr,
	&md_resync_start.attr,
	&md_metadata.attr,
	&md_new_device.attr,
	&md_safe_delay.attr,
	&md_array_state.attr,
	&md_reshape_position.attr,
	NULL,
};

static struct attribute *md_redundancy_attrs[] = {
	&md_scan_mode.attr,
	&md_mismatches.attr,
	&md_sync_min.attr,
	&md_sync_max.attr,
	&md_sync_speed.attr,
	&md_sync_force_parallel.attr,
	&md_sync_completed.attr,
	&md_min_sync.attr,
	&md_max_sync.attr,
	&md_suspend_lo.attr,
	&md_suspend_hi.attr,
	&md_bitmap.attr,
	&md_degraded.attr,
	NULL,
};
static struct attribute_group md_redundancy_group = {
	.name = NULL,
	.attrs = md_redundancy_attrs,
};


static ssize_t
md_attr_show(struct kobject *kobj, struct attribute *attr, char *page)
{
	struct md_sysfs_entry *entry = container_of(attr, struct md_sysfs_entry, attr);
	mddev_t *mddev = container_of(kobj, struct mddev_s, kobj);
	ssize_t rv;

	if (!entry->show)
		return -EIO;
	rv = mddev_lock(mddev);
	if (!rv) {
		rv = entry->show(mddev, page);
		mddev_unlock(mddev);
	}
	return rv;
}

static ssize_t
md_attr_store(struct kobject *kobj, struct attribute *attr,
	      const char *page, size_t length)
{
	struct md_sysfs_entry *entry = container_of(attr, struct md_sysfs_entry, attr);
	mddev_t *mddev = container_of(kobj, struct mddev_s, kobj);
	ssize_t rv;

	if (!entry->store)
		return -EIO;
	if (!capable(CAP_SYS_ADMIN))
		return -EACCES;
	rv = mddev_lock(mddev);
	if (mddev->hold_active == UNTIL_IOCTL)
		mddev->hold_active = 0;
	if (!rv) {
		rv = entry->store(mddev, page, length);
		mddev_unlock(mddev);
	}
	return rv;
}

static void md_free(struct kobject *ko)
{
	mddev_t *mddev = container_of(ko, mddev_t, kobj);

	if (mddev->sysfs_state)
		sysfs_put(mddev->sysfs_state);

	if (mddev->gendisk) {
		del_gendisk(mddev->gendisk);
		put_disk(mddev->gendisk);
	}
	if (mddev->queue)
		blk_cleanup_queue(mddev->queue);

	kfree(mddev);
}

static struct sysfs_ops md_sysfs_ops = {
	.show	= md_attr_show,
	.store	= md_attr_store,
};
static struct kobj_type md_ktype = {
	.release	= md_free,
	.sysfs_ops	= &md_sysfs_ops,
	.default_attrs	= md_default_attrs,
};

int mdp_major = 0;

static void mddev_delayed_delete(struct work_struct *ws)
{
	mddev_t *mddev = container_of(ws, mddev_t, del_work);

	if (mddev->private == &md_redundancy_group) {
		sysfs_remove_group(&mddev->kobj, &md_redundancy_group);
		if (mddev->sysfs_action)
			sysfs_put(mddev->sysfs_action);
		mddev->sysfs_action = NULL;
		mddev->private = NULL;
	}
	kobject_del(&mddev->kobj);
	kobject_put(&mddev->kobj);
}

static int md_alloc(dev_t dev, char *name)
{
	static DEFINE_MUTEX(disks_mutex);
	mddev_t *mddev = mddev_find(dev);
	struct gendisk *disk;
	int partitioned;
	int shift;
	int unit;
	int error;

	if (!mddev)
		return -ENODEV;

	partitioned = (MAJOR(mddev->unit) != MD_MAJOR);
	shift = partitioned ? MdpMinorShift : 0;
	unit = MINOR(mddev->unit) >> shift;

	/* wait for any previous instance if this device
	 * to be completed removed (mddev_delayed_delete).
	 */
	flush_scheduled_work();

	mutex_lock(&disks_mutex);
	if (mddev->gendisk) {
		mutex_unlock(&disks_mutex);
		mddev_put(mddev);
		return -EEXIST;
	}

	if (name) {
		/* Need to ensure that 'name' is not a duplicate.
		 */
		mddev_t *mddev2;
		spin_lock(&all_mddevs_lock);

		list_for_each_entry(mddev2, &all_mddevs, all_mddevs)
			if (mddev2->gendisk &&
			    strcmp(mddev2->gendisk->disk_name, name) == 0) {
				spin_unlock(&all_mddevs_lock);
				return -EEXIST;
			}
		spin_unlock(&all_mddevs_lock);
	}

	mddev->queue = blk_alloc_queue(GFP_KERNEL);
	if (!mddev->queue) {
		mutex_unlock(&disks_mutex);
		mddev_put(mddev);
		return -ENOMEM;
	}
	/* Can be unlocked because the queue is new: no concurrency */
	queue_flag_set_unlocked(QUEUE_FLAG_CLUSTER, mddev->queue);

	blk_queue_make_request(mddev->queue, md_fail_request);

	disk = alloc_disk(1 << shift);
	if (!disk) {
		mutex_unlock(&disks_mutex);
		blk_cleanup_queue(mddev->queue);
		mddev->queue = NULL;
		mddev_put(mddev);
		return -ENOMEM;
	}
	disk->major = MAJOR(mddev->unit);
	disk->first_minor = unit << shift;
	if (name)
		strcpy(disk->disk_name, name);
	else if (partitioned)
		sprintf(disk->disk_name, "md_d%d", unit);
	else
		sprintf(disk->disk_name, "md%d", unit);
	disk->fops = &md_fops;
	disk->private_data = mddev;
	disk->queue = mddev->queue;
	/* Allow extended partitions.  This makes the
	 * 'mdp' device redundant, but we can't really
	 * remove it now.
	 */
	disk->flags |= GENHD_FL_EXT_DEVT;
	add_disk(disk);
	mddev->gendisk = disk;
	error = kobject_init_and_add(&mddev->kobj, &md_ktype,
				     &disk_to_dev(disk)->kobj, "%s", "md");
	mutex_unlock(&disks_mutex);
	if (error)
		printk(KERN_WARNING "md: cannot register %s/md - name in use\n",
		       disk->disk_name);
	else {
		kobject_uevent(&mddev->kobj, KOBJ_ADD);
		mddev->sysfs_state = sysfs_get_dirent(mddev->kobj.sd, "array_state");
	}
	mddev_put(mddev);
	return 0;
}

static struct kobject *md_probe(dev_t dev, int *part, void *data)
{
	md_alloc(dev, NULL);
	return NULL;
}

static int add_named_array(const char *val, struct kernel_param *kp)
{
	/* val must be "md_*" where * is not all digits.
	 * We allocate an array with a large free minor number, and
	 * set the name to val.  val must not already be an active name.
	 */
	int len = strlen(val);
	char buf[DISK_NAME_LEN];

	while (len && val[len-1] == '\n')
		len--;
	if (len >= DISK_NAME_LEN)
		return -E2BIG;
	strlcpy(buf, val, len+1);
	if (strncmp(buf, "md_", 3) != 0)
		return -EINVAL;
	return md_alloc(0, buf);
}

static void md_safemode_timeout(unsigned long data)
{
	mddev_t *mddev = (mddev_t *) data;

	if (!atomic_read(&mddev->writes_pending)) {
		mddev->safemode = 1;
		if (mddev->external)
			sysfs_notify_dirent(mddev->sysfs_state);
	}
	md_wakeup_thread(mddev->thread);
}

static int start_dirty_degraded;

static int do_md_run(mddev_t * mddev)
{
	int err;
	int chunk_size;
	mdk_rdev_t *rdev;
	struct gendisk *disk;
	struct mdk_personality *pers;
	char b[BDEVNAME_SIZE];

	if (list_empty(&mddev->disks))
		/* cannot run an array with no devices.. */
		return -EINVAL;

	if (mddev->pers)
		return -EBUSY;

	/*
	 * Analyze all RAID superblock(s)
	 */
	if (!mddev->raid_disks) {
		if (!mddev->persistent)
			return -EINVAL;
		analyze_sbs(mddev);
	}

	chunk_size = mddev->chunk_size;

	if (chunk_size) {
		if (chunk_size > MAX_CHUNK_SIZE) {
			printk(KERN_ERR "too big chunk_size: %d > %d\n",
				chunk_size, MAX_CHUNK_SIZE);
			return -EINVAL;
		}
		/*
		 * chunk-size has to be a power of 2
		 */
		if ( (1 << ffz(~chunk_size)) != chunk_size) {
			printk(KERN_ERR "chunk_size of %d not valid\n", chunk_size);
			return -EINVAL;
		}

		/* devices must have minimum size of one chunk */
		list_for_each_entry(rdev, &mddev->disks, same_set) {
			if (test_bit(Faulty, &rdev->flags))
				continue;
			if (rdev->size < chunk_size / 1024) {
				printk(KERN_WARNING
					"md: Dev %s smaller than chunk_size:"
					" %lluk < %dk\n",
					bdevname(rdev->bdev,b),
					(unsigned long long)rdev->size,
					chunk_size / 1024);
				return -EINVAL;
			}
		}
	}

	if (mddev->level != LEVEL_NONE)
		request_module("md-level-%d", mddev->level);
	else if (mddev->clevel[0])
		request_module("md-%s", mddev->clevel);

	/*
	 * Drop all container device buffers, from now on
	 * the only valid external interface is through the md
	 * device.
	 */
	list_for_each_entry(rdev, &mddev->disks, same_set) {
		if (test_bit(Faulty, &rdev->flags))
			continue;
		sync_blockdev(rdev->bdev);
		invalidate_bdev(rdev->bdev);

		/* perform some consistency tests on the device.
		 * We don't want the data to overlap the metadata,
		 * Internal Bitmap issues has handled elsewhere.
		 */
		if (rdev->data_offset < rdev->sb_start) {
			if (mddev->size &&
			    rdev->data_offset + mddev->size*2
			    > rdev->sb_start) {
				printk("md: %s: data overlaps metadata\n",
				       mdname(mddev));
				return -EINVAL;
			}
		} else {
			if (rdev->sb_start + rdev->sb_size/512
			    > rdev->data_offset) {
				printk("md: %s: metadata overlaps data\n",
				       mdname(mddev));
				return -EINVAL;
			}
		}
		sysfs_notify_dirent(rdev->sysfs_state);
	}

	md_probe(mddev->unit, NULL, NULL);
	disk = mddev->gendisk;
	if (!disk)
		return -ENOMEM;

	spin_lock(&pers_lock);
	pers = find_pers(mddev->level, mddev->clevel);
	if (!pers || !try_module_get(pers->owner)) {
		spin_unlock(&pers_lock);
		if (mddev->level != LEVEL_NONE)
			printk(KERN_WARNING "md: personality for level %d is not loaded!\n",
			       mddev->level);
		else
			printk(KERN_WARNING "md: personality for level %s is not loaded!\n",
			       mddev->clevel);
		return -EINVAL;
	}
	mddev->pers = pers;
	spin_unlock(&pers_lock);
	mddev->level = pers->level;
	strlcpy(mddev->clevel, pers->name, sizeof(mddev->clevel));

	if (mddev->reshape_position != MaxSector &&
	    pers->start_reshape == NULL) {
		/* This personality cannot handle reshaping... */
		mddev->pers = NULL;
		module_put(pers->owner);
		return -EINVAL;
	}

	if (pers->sync_request) {
		/* Warn if this is a potentially silly
		 * configuration.
		 */
		char b[BDEVNAME_SIZE], b2[BDEVNAME_SIZE];
		mdk_rdev_t *rdev2;
		int warned = 0;

		list_for_each_entry(rdev, &mddev->disks, same_set)
			list_for_each_entry(rdev2, &mddev->disks, same_set) {
				if (rdev < rdev2 &&
				    rdev->bdev->bd_contains ==
				    rdev2->bdev->bd_contains) {
					printk(KERN_WARNING
					       "%s: WARNING: %s appears to be"
					       " on the same physical disk as"
					       " %s.\n",
					       mdname(mddev),
					       bdevname(rdev->bdev,b),
					       bdevname(rdev2->bdev,b2));
					warned = 1;
				}
			}

		if (warned)
			printk(KERN_WARNING
			       "True protection against single-disk"
			       " failure might be compromised.\n");
	}

	mddev->recovery = 0;
	mddev->resync_max_sectors = mddev->size << 1; /* may be over-ridden by personality */
	mddev->barriers_work = 1;
	mddev->ok_start_degraded = start_dirty_degraded;

	if (start_readonly)
		mddev->ro = 2; /* read-only, but switch on first write */

	err = mddev->pers->run(mddev);
	if (err)
		printk(KERN_ERR "md: pers->run() failed ...\n");
	else if (mddev->pers->sync_request) {
		err = bitmap_create(mddev);
		if (err) {
			printk(KERN_ERR "%s: failed to create bitmap (%d)\n",
			       mdname(mddev), err);
			mddev->pers->stop(mddev);
		}
	}
	if (err) {
		module_put(mddev->pers->owner);
		mddev->pers = NULL;
		bitmap_destroy(mddev);
		return err;
	}
	if (mddev->pers->sync_request) {
		if (sysfs_create_group(&mddev->kobj, &md_redundancy_group))
			printk(KERN_WARNING
			       "md: cannot register extra attributes for %s\n",
			       mdname(mddev));
		mddev->sysfs_action = sysfs_get_dirent(mddev->kobj.sd, "sync_action");
	} else if (mddev->ro == 2) /* auto-readonly not meaningful */
		mddev->ro = 0;

 	atomic_set(&mddev->writes_pending,0);
	mddev->safemode = 0;
	mddev->safemode_timer.function = md_safemode_timeout;
	mddev->safemode_timer.data = (unsigned long) mddev;
	mddev->safemode_delay = (200 * HZ)/1000 +1; /* 200 msec delay */
	mddev->in_sync = 1;

	list_for_each_entry(rdev, &mddev->disks, same_set)
		if (rdev->raid_disk >= 0) {
			char nm[20];
			sprintf(nm, "rd%d", rdev->raid_disk);
			if (sysfs_create_link(&mddev->kobj, &rdev->kobj, nm))
				printk("md: cannot register %s for %s\n",
				       nm, mdname(mddev));
		}
	
	set_bit(MD_RECOVERY_NEEDED, &mddev->recovery);
	
	if (mddev->flags)
		md_update_sb(mddev, 0);

	set_capacity(disk, mddev->array_sectors);

	/* If we call blk_queue_make_request here, it will
	 * re-initialise max_sectors etc which may have been
	 * refined inside -> run.  So just set the bits we need to set.
	 * Most initialisation happended when we called
	 * blk_queue_make_request(..., md_fail_request)
	 * earlier.
	 */
	mddev->queue->queuedata = mddev;
	mddev->queue->make_request_fn = mddev->pers->make_request;

	/* If there is a partially-recovered drive we need to
	 * start recovery here.  If we leave it to md_check_recovery,
	 * it will remove the drives and not do the right thing
	 */
	if (mddev->degraded && !mddev->sync_thread) {
		int spares = 0;
		list_for_each_entry(rdev, &mddev->disks, same_set)
			if (rdev->raid_disk >= 0 &&
			    !test_bit(In_sync, &rdev->flags) &&
			    !test_bit(Faulty, &rdev->flags))
				/* complete an interrupted recovery */
				spares++;
		if (spares && mddev->pers->sync_request) {
			mddev->recovery = 0;
			set_bit(MD_RECOVERY_RUNNING, &mddev->recovery);
			mddev->sync_thread = md_register_thread(md_do_sync,
								mddev,
								"%s_resync");
			if (!mddev->sync_thread) {
				printk(KERN_ERR "%s: could not start resync"
				       " thread...\n",
				       mdname(mddev));
				/* leave the spares where they are, it shouldn't hurt */
				mddev->recovery = 0;
			}
		}
	}
	md_wakeup_thread(mddev->thread);
	md_wakeup_thread(mddev->sync_thread); /* possibly kick off a reshape */

	mddev->changed = 1;
	md_new_event(mddev);
	sysfs_notify_dirent(mddev->sysfs_state);
	if (mddev->sysfs_action)
		sysfs_notify_dirent(mddev->sysfs_action);
	sysfs_notify(&mddev->kobj, NULL, "degraded");
	kobject_uevent(&disk_to_dev(mddev->gendisk)->kobj, KOBJ_CHANGE);
	return 0;
}

static int restart_array(mddev_t *mddev)
{
	struct gendisk *disk = mddev->gendisk;

	/* Complain if it has no devices */
	if (list_empty(&mddev->disks))
		return -ENXIO;
	if (!mddev->pers)
		return -EINVAL;
	if (!mddev->ro)
		return -EBUSY;
	mddev->safemode = 0;
	mddev->ro = 0;
	set_disk_ro(disk, 0);
	printk(KERN_INFO "md: %s switched to read-write mode.\n",
		mdname(mddev));
	/* Kick recovery or resync if necessary */
	set_bit(MD_RECOVERY_NEEDED, &mddev->recovery);
	md_wakeup_thread(mddev->thread);
	md_wakeup_thread(mddev->sync_thread);
	sysfs_notify_dirent(mddev->sysfs_state);
	return 0;
}

static int deny_bitmap_write_access(struct file * file)
{
	struct inode *inode = file->f_mapping->host;

	spin_lock(&inode->i_lock);
	if (atomic_read(&inode->i_writecount) > 1) {
		spin_unlock(&inode->i_lock);
		return -ETXTBSY;
	}
	atomic_set(&inode->i_writecount, -1);
	spin_unlock(&inode->i_lock);

	return 0;
}

static void restore_bitmap_write_access(struct file *file)
{
	struct inode *inode = file->f_mapping->host;

	spin_lock(&inode->i_lock);
	atomic_set(&inode->i_writecount, 1);
	spin_unlock(&inode->i_lock);
}

static int do_md_stop(mddev_t * mddev, int mode, int is_open)
{
	int err = 0;
	struct gendisk *disk = mddev->gendisk;

	if (atomic_read(&mddev->openers) > is_open) {
		printk("md: %s still in use.\n",mdname(mddev));
		return -EBUSY;
	}

	if (mddev->pers) {

		if (mddev->sync_thread) {
			set_bit(MD_RECOVERY_FROZEN, &mddev->recovery);
			set_bit(MD_RECOVERY_INTR, &mddev->recovery);
			md_unregister_thread(mddev->sync_thread);
			mddev->sync_thread = NULL;
		}

		del_timer_sync(&mddev->safemode_timer);

		switch(mode) {
		case 1: /* readonly */
			err  = -ENXIO;
			if (mddev->ro==1)
				goto out;
			mddev->ro = 1;
			break;
		case 0: /* disassemble */
		case 2: /* stop */
			bitmap_flush(mddev);
			md_super_wait(mddev);
			if (mddev->ro)
				set_disk_ro(disk, 0);
			blk_queue_make_request(mddev->queue, md_fail_request);
			mddev->pers->stop(mddev);
			mddev->queue->merge_bvec_fn = NULL;
			mddev->queue->unplug_fn = NULL;
			mddev->queue->backing_dev_info.congested_fn = NULL;
			module_put(mddev->pers->owner);
			if (mddev->pers->sync_request)
				mddev->private = &md_redundancy_group;
			mddev->pers = NULL;
			/* tell userspace to handle 'inactive' */
			sysfs_notify_dirent(mddev->sysfs_state);

			set_capacity(disk, 0);
			mddev->changed = 1;

			if (mddev->ro)
				mddev->ro = 0;
		}
		if (!mddev->in_sync || mddev->flags) {
			/* mark array as shutdown cleanly */
			mddev->in_sync = 1;
			md_update_sb(mddev, 1);
		}
		if (mode == 1)
			set_disk_ro(disk, 1);
		clear_bit(MD_RECOVERY_FROZEN, &mddev->recovery);
	}

	/*
	 * Free resources if final stop
	 */
	if (mode == 0) {
		mdk_rdev_t *rdev;

		printk(KERN_INFO "md: %s stopped.\n", mdname(mddev));

		bitmap_destroy(mddev);
		if (mddev->bitmap_file) {
			restore_bitmap_write_access(mddev->bitmap_file);
			fput(mddev->bitmap_file);
			mddev->bitmap_file = NULL;
		}
		mddev->bitmap_offset = 0;

		list_for_each_entry(rdev, &mddev->disks, same_set)
			if (rdev->raid_disk >= 0) {
				char nm[20];
				sprintf(nm, "rd%d", rdev->raid_disk);
				sysfs_remove_link(&mddev->kobj, nm);
			}

		/* make sure all md_delayed_delete calls have finished */
		flush_scheduled_work();

		export_array(mddev);

		mddev->array_sectors = 0;
		mddev->size = 0;
		mddev->raid_disks = 0;
		mddev->recovery_cp = 0;
		mddev->resync_min = 0;
		mddev->resync_max = MaxSector;
		mddev->reshape_position = MaxSector;
		mddev->external = 0;
		mddev->persistent = 0;
		mddev->level = LEVEL_NONE;
		mddev->clevel[0] = 0;
		mddev->flags = 0;
		mddev->ro = 0;
		mddev->metadata_type[0] = 0;
		mddev->chunk_size = 0;
		mddev->ctime = mddev->utime = 0;
		mddev->layout = 0;
		mddev->max_disks = 0;
		mddev->events = 0;
		mddev->delta_disks = 0;
		mddev->new_level = LEVEL_NONE;
		mddev->new_layout = 0;
		mddev->new_chunk = 0;
		mddev->curr_resync = 0;
		mddev->resync_mismatches = 0;
		mddev->suspend_lo = mddev->suspend_hi = 0;
		mddev->sync_speed_min = mddev->sync_speed_max = 0;
		mddev->recovery = 0;
		mddev->in_sync = 0;
		mddev->changed = 0;
		mddev->degraded = 0;
		mddev->barriers_work = 0;
		mddev->safemode = 0;
		kobject_uevent(&disk_to_dev(mddev->gendisk)->kobj, KOBJ_CHANGE);
		if (mddev->hold_active == UNTIL_STOP)
			mddev->hold_active = 0;

	} else if (mddev->pers)
		printk(KERN_INFO "md: %s switched to read-only mode.\n",
			mdname(mddev));
	err = 0;
	md_new_event(mddev);
	sysfs_notify_dirent(mddev->sysfs_state);
out:
	return err;
}

#ifndef MODULE
static void autorun_array(mddev_t *mddev)
{
	mdk_rdev_t *rdev;
	int err;

	if (list_empty(&mddev->disks))
		return;

	printk(KERN_INFO "md: running: ");

	list_for_each_entry(rdev, &mddev->disks, same_set) {
		char b[BDEVNAME_SIZE];
		printk("<%s>", bdevname(rdev->bdev,b));
	}
	printk("\n");

	err = do_md_run(mddev);
	if (err) {
		printk(KERN_WARNING "md: do_md_run() returned %d\n", err);
		do_md_stop(mddev, 0, 0);
	}
}

static void autorun_devices(int part)
{
	mdk_rdev_t *rdev0, *rdev, *tmp;
	mddev_t *mddev;
	char b[BDEVNAME_SIZE];

	printk(KERN_INFO "md: autorun ...\n");
	while (!list_empty(&pending_raid_disks)) {
		int unit;
		dev_t dev;
		LIST_HEAD(candidates);
		rdev0 = list_entry(pending_raid_disks.next,
					 mdk_rdev_t, same_set);

		printk(KERN_INFO "md: considering %s ...\n",
			bdevname(rdev0->bdev,b));
		INIT_LIST_HEAD(&candidates);
		rdev_for_each_list(rdev, tmp, &pending_raid_disks)
			if (super_90_load(rdev, rdev0, 0) >= 0) {
				printk(KERN_INFO "md:  adding %s ...\n",
					bdevname(rdev->bdev,b));
				list_move(&rdev->same_set, &candidates);
			}
		/*
		 * now we have a set of devices, with all of them having
		 * mostly sane superblocks. It's time to allocate the
		 * mddev.
		 */
		if (part) {
			dev = MKDEV(mdp_major,
				    rdev0->preferred_minor << MdpMinorShift);
			unit = MINOR(dev) >> MdpMinorShift;
		} else {
			dev = MKDEV(MD_MAJOR, rdev0->preferred_minor);
			unit = MINOR(dev);
		}
		if (rdev0->preferred_minor != unit) {
			printk(KERN_INFO "md: unit number in %s is bad: %d\n",
			       bdevname(rdev0->bdev, b), rdev0->preferred_minor);
			break;
		}

		md_probe(dev, NULL, NULL);
		mddev = mddev_find(dev);
		if (!mddev || !mddev->gendisk) {
			if (mddev)
				mddev_put(mddev);
			printk(KERN_ERR
				"md: cannot allocate memory for md drive.\n");
			break;
		}
		if (mddev_lock(mddev)) 
			printk(KERN_WARNING "md: %s locked, cannot run\n",
			       mdname(mddev));
		else if (mddev->raid_disks || mddev->major_version
			 || !list_empty(&mddev->disks)) {
			printk(KERN_WARNING 
				"md: %s already running, cannot run %s\n",
				mdname(mddev), bdevname(rdev0->bdev,b));
			mddev_unlock(mddev);
		} else {
			printk(KERN_INFO "md: created %s\n", mdname(mddev));
			mddev->persistent = 1;
			rdev_for_each_list(rdev, tmp, &candidates) {
				list_del_init(&rdev->same_set);
				if (bind_rdev_to_array(rdev, mddev))
					export_rdev(rdev);
			}
			autorun_array(mddev);
			mddev_unlock(mddev);
		}
		/* on success, candidates will be empty, on error
		 * it won't...
		 */
		rdev_for_each_list(rdev, tmp, &candidates) {
			list_del_init(&rdev->same_set);
			export_rdev(rdev);
		}
		mddev_put(mddev);
	}
	printk(KERN_INFO "md: ... autorun DONE.\n");
}
#endif /* !MODULE */

static int get_version(void __user * arg)
{
	mdu_version_t ver;

	ver.major = MD_MAJOR_VERSION;
	ver.minor = MD_MINOR_VERSION;
	ver.patchlevel = MD_PATCHLEVEL_VERSION;

	if (copy_to_user(arg, &ver, sizeof(ver)))
		return -EFAULT;

	return 0;
}

static int get_array_info(mddev_t * mddev, void __user * arg)
{
	mdu_array_info_t info;
	int nr,working,active,failed,spare;
	mdk_rdev_t *rdev;

	nr=working=active=failed=spare=0;
	list_for_each_entry(rdev, &mddev->disks, same_set) {
		nr++;
		if (test_bit(Faulty, &rdev->flags))
			failed++;
		else {
			working++;
			if (test_bit(In_sync, &rdev->flags))
				active++;	
			else
				spare++;
		}
	}

	info.major_version = mddev->major_version;
	info.minor_version = mddev->minor_version;
	info.patch_version = MD_PATCHLEVEL_VERSION;
	info.ctime         = mddev->ctime;
	info.level         = mddev->level;
	info.size          = mddev->size;
	if (info.size != mddev->size) /* overflow */
		info.size = -1;
	info.nr_disks      = nr;
	info.raid_disks    = mddev->raid_disks;
	info.md_minor      = mddev->md_minor;
	info.not_persistent= !mddev->persistent;

	info.utime         = mddev->utime;
	info.state         = 0;
	if (mddev->in_sync)
		info.state = (1<<MD_SB_CLEAN);
	if (mddev->bitmap && mddev->bitmap_offset)
		info.state = (1<<MD_SB_BITMAP_PRESENT);
	info.active_disks  = active;
	info.working_disks = working;
	info.failed_disks  = failed;
	info.spare_disks   = spare;

	info.layout        = mddev->layout;
	info.chunk_size    = mddev->chunk_size;

	if (copy_to_user(arg, &info, sizeof(info)))
		return -EFAULT;

	return 0;
}

static int get_bitmap_file(mddev_t * mddev, void __user * arg)
{
	mdu_bitmap_file_t *file = NULL; /* too big for stack allocation */
	char *ptr, *buf = NULL;
	int err = -ENOMEM;

	if (md_allow_write(mddev))
		file = kmalloc(sizeof(*file), GFP_NOIO);
	else
		file = kmalloc(sizeof(*file), GFP_KERNEL);

	if (!file)
		goto out;

	/* bitmap disabled, zero the first byte and copy out */
	if (!mddev->bitmap || !mddev->bitmap->file) {
		file->pathname[0] = '\0';
		goto copy_out;
	}

	buf = kmalloc(sizeof(file->pathname), GFP_KERNEL);
	if (!buf)
		goto out;

	ptr = d_path(&mddev->bitmap->file->f_path, buf, sizeof(file->pathname));
	if (IS_ERR(ptr))
		goto out;

	strcpy(file->pathname, ptr);

copy_out:
	err = 0;
	if (copy_to_user(arg, file, sizeof(*file)))
		err = -EFAULT;
out:
	kfree(buf);
	kfree(file);
	return err;
}

static int get_disk_info(mddev_t * mddev, void __user * arg)
{
	mdu_disk_info_t info;
	mdk_rdev_t *rdev;

	if (copy_from_user(&info, arg, sizeof(info)))
		return -EFAULT;

	rdev = find_rdev_nr(mddev, info.number);
	if (rdev) {
		info.major = MAJOR(rdev->bdev->bd_dev);
		info.minor = MINOR(rdev->bdev->bd_dev);
		info.raid_disk = rdev->raid_disk;
		info.state = 0;
		if (test_bit(Faulty, &rdev->flags))
			info.state |= (1<<MD_DISK_FAULTY);
		else if (test_bit(In_sync, &rdev->flags)) {
			info.state |= (1<<MD_DISK_ACTIVE);
			info.state |= (1<<MD_DISK_SYNC);
		}
		if (test_bit(WriteMostly, &rdev->flags))
			info.state |= (1<<MD_DISK_WRITEMOSTLY);
	} else {
		info.major = info.minor = 0;
		info.raid_disk = -1;
		info.state = (1<<MD_DISK_REMOVED);
	}

	if (copy_to_user(arg, &info, sizeof(info)))
		return -EFAULT;

	return 0;
}

static int add_new_disk(mddev_t * mddev, mdu_disk_info_t *info)
{
	char b[BDEVNAME_SIZE], b2[BDEVNAME_SIZE];
	mdk_rdev_t *rdev;
	dev_t dev = MKDEV(info->major,info->minor);

	if (info->major != MAJOR(dev) || info->minor != MINOR(dev))
		return -EOVERFLOW;

	if (!mddev->raid_disks) {
		int err;
		/* expecting a device which has a superblock */
		rdev = md_import_device(dev, mddev->major_version, mddev->minor_version);
		if (IS_ERR(rdev)) {
			printk(KERN_WARNING 
				"md: md_import_device returned %ld\n",
				PTR_ERR(rdev));
			return PTR_ERR(rdev);
		}
		if (!list_empty(&mddev->disks)) {
			mdk_rdev_t *rdev0 = list_entry(mddev->disks.next,
							mdk_rdev_t, same_set);
			int err = super_types[mddev->major_version]
				.load_super(rdev, rdev0, mddev->minor_version);
			if (err < 0) {
				printk(KERN_WARNING 
					"md: %s has different UUID to %s\n",
					bdevname(rdev->bdev,b), 
					bdevname(rdev0->bdev,b2));
				export_rdev(rdev);
				return -EINVAL;
			}
		}
		err = bind_rdev_to_array(rdev, mddev);
		if (err)
			export_rdev(rdev);
		return err;
	}

	/*
	 * add_new_disk can be used once the array is assembled
	 * to add "hot spares".  They must already have a superblock
	 * written
	 */
	if (mddev->pers) {
		int err;
		if (!mddev->pers->hot_add_disk) {
			printk(KERN_WARNING 
				"%s: personality does not support diskops!\n",
			       mdname(mddev));
			return -EINVAL;
		}
		if (mddev->persistent)
			rdev = md_import_device(dev, mddev->major_version,
						mddev->minor_version);
		else
			rdev = md_import_device(dev, -1, -1);
		if (IS_ERR(rdev)) {
			printk(KERN_WARNING 
				"md: md_import_device returned %ld\n",
				PTR_ERR(rdev));
			return PTR_ERR(rdev);
		}
		/* set save_raid_disk if appropriate */
		if (!mddev->persistent) {
			if (info->state & (1<<MD_DISK_SYNC)  &&
			    info->raid_disk < mddev->raid_disks)
				rdev->raid_disk = info->raid_disk;
			else
				rdev->raid_disk = -1;
		} else
			super_types[mddev->major_version].
				validate_super(mddev, rdev);
		rdev->saved_raid_disk = rdev->raid_disk;

		clear_bit(In_sync, &rdev->flags); /* just to be sure */
		if (info->state & (1<<MD_DISK_WRITEMOSTLY))
			set_bit(WriteMostly, &rdev->flags);

		rdev->raid_disk = -1;
		err = bind_rdev_to_array(rdev, mddev);
		if (!err && !mddev->pers->hot_remove_disk) {
			/* If there is hot_add_disk but no hot_remove_disk
			 * then added disks for geometry changes,
			 * and should be added immediately.
			 */
			super_types[mddev->major_version].
				validate_super(mddev, rdev);
			err = mddev->pers->hot_add_disk(mddev, rdev);
			if (err)
				unbind_rdev_from_array(rdev);
		}
		if (err)
			export_rdev(rdev);
		else
			sysfs_notify_dirent(rdev->sysfs_state);

		md_update_sb(mddev, 1);
		if (mddev->degraded)
			set_bit(MD_RECOVERY_RECOVER, &mddev->recovery);
		set_bit(MD_RECOVERY_NEEDED, &mddev->recovery);
		md_wakeup_thread(mddev->thread);
		return err;
	}

	/* otherwise, add_new_disk is only allowed
	 * for major_version==0 superblocks
	 */
	if (mddev->major_version != 0) {
		printk(KERN_WARNING "%s: ADD_NEW_DISK not supported\n",
		       mdname(mddev));
		return -EINVAL;
	}

	if (!(info->state & (1<<MD_DISK_FAULTY))) {
		int err;
		rdev = md_import_device(dev, -1, 0);
		if (IS_ERR(rdev)) {
			printk(KERN_WARNING 
				"md: error, md_import_device() returned %ld\n",
				PTR_ERR(rdev));
			return PTR_ERR(rdev);
		}
		rdev->desc_nr = info->number;
		if (info->raid_disk < mddev->raid_disks)
			rdev->raid_disk = info->raid_disk;
		else
			rdev->raid_disk = -1;

		if (rdev->raid_disk < mddev->raid_disks)
			if (info->state & (1<<MD_DISK_SYNC))
				set_bit(In_sync, &rdev->flags);

		if (info->state & (1<<MD_DISK_WRITEMOSTLY))
			set_bit(WriteMostly, &rdev->flags);

		if (!mddev->persistent) {
			printk(KERN_INFO "md: nonpersistent superblock ...\n");
			rdev->sb_start = rdev->bdev->bd_inode->i_size / 512;
		} else 
			rdev->sb_start = calc_dev_sboffset(rdev->bdev);
		rdev->size = calc_num_sectors(rdev, mddev->chunk_size) / 2;

		err = bind_rdev_to_array(rdev, mddev);
		if (err) {
			export_rdev(rdev);
			return err;
		}
	}

	return 0;
}

static int hot_remove_disk(mddev_t * mddev, dev_t dev)
{
	char b[BDEVNAME_SIZE];
	mdk_rdev_t *rdev;

	rdev = find_rdev(mddev, dev);
	if (!rdev)
		return -ENXIO;

	if (rdev->raid_disk >= 0)
		goto busy;

	kick_rdev_from_array(rdev);
	md_update_sb(mddev, 1);
	md_new_event(mddev);

	return 0;
busy:
	printk(KERN_WARNING "md: cannot remove active disk %s from %s ...\n",
		bdevname(rdev->bdev,b), mdname(mddev));
	return -EBUSY;
}

static int hot_add_disk(mddev_t * mddev, dev_t dev)
{
	char b[BDEVNAME_SIZE];
	int err;
	mdk_rdev_t *rdev;

	if (!mddev->pers)
		return -ENODEV;

	if (mddev->major_version != 0) {
		printk(KERN_WARNING "%s: HOT_ADD may only be used with"
			" version-0 superblocks.\n",
			mdname(mddev));
		return -EINVAL;
	}
	if (!mddev->pers->hot_add_disk) {
		printk(KERN_WARNING 
			"%s: personality does not support diskops!\n",
			mdname(mddev));
		return -EINVAL;
	}

	rdev = md_import_device(dev, -1, 0);
	if (IS_ERR(rdev)) {
		printk(KERN_WARNING 
			"md: error, md_import_device() returned %ld\n",
			PTR_ERR(rdev));
		return -EINVAL;
	}

	if (mddev->persistent)
		rdev->sb_start = calc_dev_sboffset(rdev->bdev);
	else
		rdev->sb_start = rdev->bdev->bd_inode->i_size / 512;

	rdev->size = calc_num_sectors(rdev, mddev->chunk_size) / 2;

	if (test_bit(Faulty, &rdev->flags)) {
		printk(KERN_WARNING 
			"md: can not hot-add faulty %s disk to %s!\n",
			bdevname(rdev->bdev,b), mdname(mddev));
		err = -EINVAL;
		goto abort_export;
	}
	clear_bit(In_sync, &rdev->flags);
	rdev->desc_nr = -1;
	rdev->saved_raid_disk = -1;
	err = bind_rdev_to_array(rdev, mddev);
	if (err)
		goto abort_export;

	/*
	 * The rest should better be atomic, we can have disk failures
	 * noticed in interrupt contexts ...
	 */

	rdev->raid_disk = -1;

	md_update_sb(mddev, 1);

	/*
	 * Kick recovery, maybe this spare has to be added to the
	 * array immediately.
	 */
	set_bit(MD_RECOVERY_NEEDED, &mddev->recovery);
	md_wakeup_thread(mddev->thread);
	md_new_event(mddev);
	return 0;

abort_export:
	export_rdev(rdev);
	return err;
}

static int set_bitmap_file(mddev_t *mddev, int fd)
{
	int err;

	if (mddev->pers) {
		if (!mddev->pers->quiesce)
			return -EBUSY;
		if (mddev->recovery || mddev->sync_thread)
			return -EBUSY;
		/* we should be able to change the bitmap.. */
	}


	if (fd >= 0) {
		if (mddev->bitmap)
			return -EEXIST; /* cannot add when bitmap is present */
		mddev->bitmap_file = fget(fd);

		if (mddev->bitmap_file == NULL) {
			printk(KERN_ERR "%s: error: failed to get bitmap file\n",
			       mdname(mddev));
			return -EBADF;
		}

		err = deny_bitmap_write_access(mddev->bitmap_file);
		if (err) {
			printk(KERN_ERR "%s: error: bitmap file is already in use\n",
			       mdname(mddev));
			fput(mddev->bitmap_file);
			mddev->bitmap_file = NULL;
			return err;
		}
		mddev->bitmap_offset = 0; /* file overrides offset */
	} else if (mddev->bitmap == NULL)
		return -ENOENT; /* cannot remove what isn't there */
	err = 0;
	if (mddev->pers) {
		mddev->pers->quiesce(mddev, 1);
		if (fd >= 0)
			err = bitmap_create(mddev);
		if (fd < 0 || err) {
			bitmap_destroy(mddev);
			fd = -1; /* make sure to put the file */
		}
		mddev->pers->quiesce(mddev, 0);
	}
	if (fd < 0) {
		if (mddev->bitmap_file) {
			restore_bitmap_write_access(mddev->bitmap_file);
			fput(mddev->bitmap_file);
		}
		mddev->bitmap_file = NULL;
	}

	return err;
}

static int set_array_info(mddev_t * mddev, mdu_array_info_t *info)
{

	if (info->raid_disks == 0) {
		/* just setting version number for superblock loading */
		if (info->major_version < 0 ||
		    info->major_version >= ARRAY_SIZE(super_types) ||
		    super_types[info->major_version].name == NULL) {
			/* maybe try to auto-load a module? */
			printk(KERN_INFO 
				"md: superblock version %d not known\n",
				info->major_version);
			return -EINVAL;
		}
		mddev->major_version = info->major_version;
		mddev->minor_version = info->minor_version;
		mddev->patch_version = info->patch_version;
		mddev->persistent = !info->not_persistent;
		return 0;
	}
	mddev->major_version = MD_MAJOR_VERSION;
	mddev->minor_version = MD_MINOR_VERSION;
	mddev->patch_version = MD_PATCHLEVEL_VERSION;
	mddev->ctime         = get_seconds();

	mddev->level         = info->level;
	mddev->clevel[0]     = 0;
	mddev->size          = info->size;
	mddev->raid_disks    = info->raid_disks;
	/* don't set md_minor, it is determined by which /dev/md* was
	 * openned
	 */
	if (info->state & (1<<MD_SB_CLEAN))
		mddev->recovery_cp = MaxSector;
	else
		mddev->recovery_cp = 0;
	mddev->persistent    = ! info->not_persistent;
	mddev->external	     = 0;

	mddev->layout        = info->layout;
	mddev->chunk_size    = info->chunk_size;

	mddev->max_disks     = MD_SB_DISKS;

	if (mddev->persistent)
		mddev->flags         = 0;
	set_bit(MD_CHANGE_DEVS, &mddev->flags);

	mddev->default_bitmap_offset = MD_SB_BYTES >> 9;
	mddev->bitmap_offset = 0;

	mddev->reshape_position = MaxSector;

	/*
	 * Generate a 128 bit UUID
	 */
	get_random_bytes(mddev->uuid, 16);

	mddev->new_level = mddev->level;
	mddev->new_chunk = mddev->chunk_size;
	mddev->new_layout = mddev->layout;
	mddev->delta_disks = 0;

	return 0;
}

static int update_size(mddev_t *mddev, sector_t num_sectors)
{
	mdk_rdev_t *rdev;
	int rv;
	int fit = (num_sectors == 0);

	if (mddev->pers->resize == NULL)
		return -EINVAL;
	/* The "num_sectors" is the number of sectors of each device that
	 * is used.  This can only make sense for arrays with redundancy.
	 * linear and raid0 always use whatever space is available. We can only
	 * consider changing this number if no resync or reconstruction is
	 * happening, and if the new size is acceptable. It must fit before the
	 * sb_start or, if that is <data_offset, it must fit before the size
	 * of each device.  If num_sectors is zero, we find the largest size
	 * that fits.

	 */
	if (mddev->sync_thread)
		return -EBUSY;
	if (mddev->bitmap)
		/* Sorry, cannot grow a bitmap yet, just remove it,
		 * grow, and re-add.
		 */
		return -EBUSY;
	list_for_each_entry(rdev, &mddev->disks, same_set) {
		sector_t avail;
		avail = rdev->size * 2;

		if (fit && (num_sectors == 0 || num_sectors > avail))
			num_sectors = avail;
		if (avail < num_sectors)
			return -ENOSPC;
	}
	rv = mddev->pers->resize(mddev, num_sectors);
	if (!rv) {
		struct block_device *bdev;

		bdev = bdget_disk(mddev->gendisk, 0);
		if (bdev) {
			mutex_lock(&bdev->bd_inode->i_mutex);
			i_size_write(bdev->bd_inode,
				     (loff_t)mddev->array_sectors << 9);
			mutex_unlock(&bdev->bd_inode->i_mutex);
			bdput(bdev);
		}
	}
	return rv;
}

static int update_raid_disks(mddev_t *mddev, int raid_disks)
{
	int rv;
	/* change the number of raid disks */
	if (mddev->pers->check_reshape == NULL)
		return -EINVAL;
	if (raid_disks <= 0 ||
	    raid_disks >= mddev->max_disks)
		return -EINVAL;
	if (mddev->sync_thread || mddev->reshape_position != MaxSector)
		return -EBUSY;
	mddev->delta_disks = raid_disks - mddev->raid_disks;

	rv = mddev->pers->check_reshape(mddev);
	return rv;
}


static int update_array_info(mddev_t *mddev, mdu_array_info_t *info)
{
	int rv = 0;
	int cnt = 0;
	int state = 0;

	/* calculate expected state,ignoring low bits */
	if (mddev->bitmap && mddev->bitmap_offset)
		state |= (1 << MD_SB_BITMAP_PRESENT);

	if (mddev->major_version != info->major_version ||
	    mddev->minor_version != info->minor_version ||
/*	    mddev->patch_version != info->patch_version || */
	    mddev->ctime         != info->ctime         ||
	    mddev->level         != info->level         ||
/*	    mddev->layout        != info->layout        || */
	    !mddev->persistent	 != info->not_persistent||
	    mddev->chunk_size    != info->chunk_size    ||
	    /* ignore bottom 8 bits of state, and allow SB_BITMAP_PRESENT to change */
	    ((state^info->state) & 0xfffffe00)
		)
		return -EINVAL;
	/* Check there is only one change */
	if (info->size >= 0 && mddev->size != info->size) cnt++;
	if (mddev->raid_disks != info->raid_disks) cnt++;
	if (mddev->layout != info->layout) cnt++;
	if ((state ^ info->state) & (1<<MD_SB_BITMAP_PRESENT)) cnt++;
	if (cnt == 0) return 0;
	if (cnt > 1) return -EINVAL;

	if (mddev->layout != info->layout) {
		/* Change layout
		 * we don't need to do anything at the md level, the
		 * personality will take care of it all.
		 */
		if (mddev->pers->reconfig == NULL)
			return -EINVAL;
		else
			return mddev->pers->reconfig(mddev, info->layout, -1);
	}
	if (info->size >= 0 && mddev->size != info->size)
		rv = update_size(mddev, (sector_t)info->size * 2);

	if (mddev->raid_disks    != info->raid_disks)
		rv = update_raid_disks(mddev, info->raid_disks);

	if ((state ^ info->state) & (1<<MD_SB_BITMAP_PRESENT)) {
		if (mddev->pers->quiesce == NULL)
			return -EINVAL;
		if (mddev->recovery || mddev->sync_thread)
			return -EBUSY;
		if (info->state & (1<<MD_SB_BITMAP_PRESENT)) {
			/* add the bitmap */
			if (mddev->bitmap)
				return -EEXIST;
			if (mddev->default_bitmap_offset == 0)
				return -EINVAL;
			mddev->bitmap_offset = mddev->default_bitmap_offset;
			mddev->pers->quiesce(mddev, 1);
			rv = bitmap_create(mddev);
			if (rv)
				bitmap_destroy(mddev);
			mddev->pers->quiesce(mddev, 0);
		} else {
			/* remove the bitmap */
			if (!mddev->bitmap)
				return -ENOENT;
			if (mddev->bitmap->file)
				return -EINVAL;
			mddev->pers->quiesce(mddev, 1);
			bitmap_destroy(mddev);
			mddev->pers->quiesce(mddev, 0);
			mddev->bitmap_offset = 0;
		}
	}
	md_update_sb(mddev, 1);
	return rv;
}

static int set_disk_faulty(mddev_t *mddev, dev_t dev)
{
	mdk_rdev_t *rdev;

	if (mddev->pers == NULL)
		return -ENODEV;

	rdev = find_rdev(mddev, dev);
	if (!rdev)
		return -ENODEV;

	md_error(mddev, rdev);
	return 0;
}

static int md_getgeo(struct block_device *bdev, struct hd_geometry *geo)
{
	mddev_t *mddev = bdev->bd_disk->private_data;

	geo->heads = 2;
	geo->sectors = 4;
	geo->cylinders = get_capacity(mddev->gendisk) / 8;
	return 0;
}

static int md_ioctl(struct block_device *bdev, fmode_t mode,
			unsigned int cmd, unsigned long arg)
{
	int err = 0;
	void __user *argp = (void __user *)arg;
	mddev_t *mddev = NULL;

	if (!capable(CAP_SYS_ADMIN))
		return -EACCES;

	/*
	 * Commands dealing with the RAID driver but not any
	 * particular array:
	 */
	switch (cmd)
	{
		case RAID_VERSION:
			err = get_version(argp);
			goto done;

		case PRINT_RAID_DEBUG:
			err = 0;
			md_print_devices();
			goto done;

#ifndef MODULE
		case RAID_AUTORUN:
			err = 0;
			autostart_arrays(arg);
			goto done;
#endif
		default:;
	}

	/*
	 * Commands creating/starting a new array:
	 */

	mddev = bdev->bd_disk->private_data;

	if (!mddev) {
		BUG();
		goto abort;
	}

	err = mddev_lock(mddev);
	if (err) {
		printk(KERN_INFO 
			"md: ioctl lock interrupted, reason %d, cmd %d\n",
			err, cmd);
		goto abort;
	}

	switch (cmd)
	{
		case SET_ARRAY_INFO:
			{
				mdu_array_info_t info;
				if (!arg)
					memset(&info, 0, sizeof(info));
				else if (copy_from_user(&info, argp, sizeof(info))) {
					err = -EFAULT;
					goto abort_unlock;
				}
				if (mddev->pers) {
					err = update_array_info(mddev, &info);
					if (err) {
						printk(KERN_WARNING "md: couldn't update"
						       " array info. %d\n", err);
						goto abort_unlock;
					}
					goto done_unlock;
				}
				if (!list_empty(&mddev->disks)) {
					printk(KERN_WARNING
					       "md: array %s already has disks!\n",
					       mdname(mddev));
					err = -EBUSY;
					goto abort_unlock;
				}
				if (mddev->raid_disks) {
					printk(KERN_WARNING
					       "md: array %s already initialised!\n",
					       mdname(mddev));
					err = -EBUSY;
					goto abort_unlock;
				}
				err = set_array_info(mddev, &info);
				if (err) {
					printk(KERN_WARNING "md: couldn't set"
					       " array info. %d\n", err);
					goto abort_unlock;
				}
			}
			goto done_unlock;

		default:;
	}

	/*
	 * Commands querying/configuring an existing array:
	 */
	/* if we are not initialised yet, only ADD_NEW_DISK, STOP_ARRAY,
	 * RUN_ARRAY, and GET_ and SET_BITMAP_FILE are allowed */
	if ((!mddev->raid_disks && !mddev->external)
	    && cmd != ADD_NEW_DISK && cmd != STOP_ARRAY
	    && cmd != RUN_ARRAY && cmd != SET_BITMAP_FILE
	    && cmd != GET_BITMAP_FILE) {
		err = -ENODEV;
		goto abort_unlock;
	}

	/*
	 * Commands even a read-only array can execute:
	 */
	switch (cmd)
	{
		case GET_ARRAY_INFO:
			err = get_array_info(mddev, argp);
			goto done_unlock;

		case GET_BITMAP_FILE:
			err = get_bitmap_file(mddev, argp);
			goto done_unlock;

		case GET_DISK_INFO:
			err = get_disk_info(mddev, argp);
			goto done_unlock;

		case RESTART_ARRAY_RW:
			err = restart_array(mddev);
			goto done_unlock;

		case STOP_ARRAY:
			err = do_md_stop(mddev, 0, 1);
			goto done_unlock;

		case STOP_ARRAY_RO:
			err = do_md_stop(mddev, 1, 1);
			goto done_unlock;

	}

	/*
	 * The remaining ioctls are changing the state of the
	 * superblock, so we do not allow them on read-only arrays.
	 * However non-MD ioctls (e.g. get-size) will still come through
	 * here and hit the 'default' below, so only disallow
	 * 'md' ioctls, and switch to rw mode if started auto-readonly.
	 */
	if (_IOC_TYPE(cmd) == MD_MAJOR && mddev->ro && mddev->pers) {
		if (mddev->ro == 2) {
			mddev->ro = 0;
			sysfs_notify_dirent(mddev->sysfs_state);
			set_bit(MD_RECOVERY_NEEDED, &mddev->recovery);
			md_wakeup_thread(mddev->thread);
		} else {
			err = -EROFS;
			goto abort_unlock;
		}
	}

	switch (cmd)
	{
		case ADD_NEW_DISK:
		{
			mdu_disk_info_t info;
			if (copy_from_user(&info, argp, sizeof(info)))
				err = -EFAULT;
			else
				err = add_new_disk(mddev, &info);
			goto done_unlock;
		}

		case HOT_REMOVE_DISK:
			err = hot_remove_disk(mddev, new_decode_dev(arg));
			goto done_unlock;

		case HOT_ADD_DISK:
			err = hot_add_disk(mddev, new_decode_dev(arg));
			goto done_unlock;

		case SET_DISK_FAULTY:
			err = set_disk_faulty(mddev, new_decode_dev(arg));
			goto done_unlock;

		case RUN_ARRAY:
			err = do_md_run(mddev);
			goto done_unlock;

		case SET_BITMAP_FILE:
			err = set_bitmap_file(mddev, (int)arg);
			goto done_unlock;

		default:
			err = -EINVAL;
			goto abort_unlock;
	}

done_unlock:
abort_unlock:
	if (mddev->hold_active == UNTIL_IOCTL &&
	    err != -EINVAL)
		mddev->hold_active = 0;
	mddev_unlock(mddev);

	return err;
done:
	if (err)
		MD_BUG();
abort:
	return err;
}

static int md_open(struct block_device *bdev, fmode_t mode)
{
	/*
	 * Succeed if we can lock the mddev, which confirms that
	 * it isn't being stopped right now.
	 */
	mddev_t *mddev = mddev_find(bdev->bd_dev);
	int err;

	if (mddev->gendisk != bdev->bd_disk) {
		/* we are racing with mddev_put which is discarding this
		 * bd_disk.
		 */
		mddev_put(mddev);
		/* Wait until bdev->bd_disk is definitely gone */
		flush_scheduled_work();
		/* Then retry the open from the top */
		return -ERESTARTSYS;
	}
	BUG_ON(mddev != bdev->bd_disk->private_data);

	if ((err = mutex_lock_interruptible_nested(&mddev->reconfig_mutex, 1)))
		goto out;

	err = 0;
	atomic_inc(&mddev->openers);
	mddev_unlock(mddev);

	check_disk_change(bdev);
 out:
	return err;
}

static int md_release(struct gendisk *disk, fmode_t mode)
{
 	mddev_t *mddev = disk->private_data;

	BUG_ON(!mddev);
	atomic_dec(&mddev->openers);
	mddev_put(mddev);

	return 0;
}

static int md_media_changed(struct gendisk *disk)
{
	mddev_t *mddev = disk->private_data;

	return mddev->changed;
}

static int md_revalidate(struct gendisk *disk)
{
	mddev_t *mddev = disk->private_data;

	mddev->changed = 0;
	return 0;
}
static struct block_device_operations md_fops =
{
	.owner		= THIS_MODULE,
	.open		= md_open,
	.release	= md_release,
	.locked_ioctl	= md_ioctl,
	.getgeo		= md_getgeo,
	.media_changed	= md_media_changed,
	.revalidate_disk= md_revalidate,
};

static int md_thread(void * arg)
{
	mdk_thread_t *thread = arg;

	/*
	 * md_thread is a 'system-thread', it's priority should be very
	 * high. We avoid resource deadlocks individually in each
	 * raid personality. (RAID5 does preallocation) We also use RR and
	 * the very same RT priority as kswapd, thus we will never get
	 * into a priority inversion deadlock.
	 *
	 * we definitely have to have equal or higher priority than
	 * bdflush, otherwise bdflush will deadlock if there are too
	 * many dirty RAID5 blocks.
	 */

	allow_signal(SIGKILL);
	while (!kthread_should_stop()) {

		/* We need to wait INTERRUPTIBLE so that
		 * we don't add to the load-average.
		 * That means we need to be sure no signals are
		 * pending
		 */
		if (signal_pending(current))
			flush_signals(current);

		wait_event_interruptible_timeout
			(thread->wqueue,
			 test_bit(THREAD_WAKEUP, &thread->flags)
			 || kthread_should_stop(),
			 thread->timeout);

		clear_bit(THREAD_WAKEUP, &thread->flags);

		thread->run(thread->mddev);
	}

	return 0;
}

void md_wakeup_thread(mdk_thread_t *thread)
{
	if (thread) {
		dprintk("md: waking up MD thread %s.\n", thread->tsk->comm);
		set_bit(THREAD_WAKEUP, &thread->flags);
		wake_up(&thread->wqueue);
	}
}

mdk_thread_t *md_register_thread(void (*run) (mddev_t *), mddev_t *mddev,
				 const char *name)
{
	mdk_thread_t *thread;

	thread = kzalloc(sizeof(mdk_thread_t), GFP_KERNEL);
	if (!thread)
		return NULL;

	init_waitqueue_head(&thread->wqueue);

	thread->run = run;
	thread->mddev = mddev;
	thread->timeout = MAX_SCHEDULE_TIMEOUT;
	thread->tsk = kthread_run(md_thread, thread, name, mdname(thread->mddev));
	if (IS_ERR(thread->tsk)) {
		kfree(thread);
		return NULL;
	}
	return thread;
}

void md_unregister_thread(mdk_thread_t *thread)
{
	dprintk("interrupting MD-thread pid %d\n", task_pid_nr(thread->tsk));

	kthread_stop(thread->tsk);
	kfree(thread);
}

void md_error(mddev_t *mddev, mdk_rdev_t *rdev)
{
	if (!mddev) {
		MD_BUG();
		return;
	}

	if (!rdev || test_bit(Faulty, &rdev->flags))
		return;

	if (mddev->external)
		set_bit(Blocked, &rdev->flags);
	if (!mddev->pers)
		return;
	if (!mddev->pers->error_handler)
		return;
	mddev->pers->error_handler(mddev,rdev);
	if (mddev->degraded)
		set_bit(MD_RECOVERY_RECOVER, &mddev->recovery);
	set_bit(StateChanged, &rdev->flags);
	set_bit(MD_RECOVERY_INTR, &mddev->recovery);
	set_bit(MD_RECOVERY_NEEDED, &mddev->recovery);
	md_wakeup_thread(mddev->thread);
	md_new_event_inintr(mddev);
}

/* seq_file implementation /proc/mdstat */

static void status_unused(struct seq_file *seq)
{
	int i = 0;
	mdk_rdev_t *rdev;

	seq_printf(seq, "unused devices: ");

	list_for_each_entry(rdev, &pending_raid_disks, same_set) {
		char b[BDEVNAME_SIZE];
		i++;
		seq_printf(seq, "%s ",
			      bdevname(rdev->bdev,b));
	}
	if (!i)
		seq_printf(seq, "<none>");

	seq_printf(seq, "\n");
}


static void status_resync(struct seq_file *seq, mddev_t * mddev)
{
	sector_t max_blocks, resync, res;
	unsigned long dt, db, rt;
	int scale;
	unsigned int per_milli;

	resync = (mddev->curr_resync - atomic_read(&mddev->recovery_active))/2;

	if (test_bit(MD_RECOVERY_SYNC, &mddev->recovery))
		max_blocks = mddev->resync_max_sectors >> 1;
	else
		max_blocks = mddev->size;

	/*
	 * Should not happen.
	 */
	if (!max_blocks) {
		MD_BUG();
		return;
	}
	/* Pick 'scale' such that (resync>>scale)*1000 will fit
	 * in a sector_t, and (max_blocks>>scale) will fit in a
	 * u32, as those are the requirements for sector_div.
	 * Thus 'scale' must be at least 10
	 */
	scale = 10;
	if (sizeof(sector_t) > sizeof(unsigned long)) {
		while ( max_blocks/2 > (1ULL<<(scale+32)))
			scale++;
	}
	res = (resync>>scale)*1000;
	sector_div(res, (u32)((max_blocks>>scale)+1));

	per_milli = res;
	{
		int i, x = per_milli/50, y = 20-x;
		seq_printf(seq, "[");
		for (i = 0; i < x; i++)
			seq_printf(seq, "=");
		seq_printf(seq, ">");
		for (i = 0; i < y; i++)
			seq_printf(seq, ".");
		seq_printf(seq, "] ");
	}
	seq_printf(seq, " %s =%3u.%u%% (%llu/%llu)",
		   (test_bit(MD_RECOVERY_RESHAPE, &mddev->recovery)?
		    "reshape" :
		    (test_bit(MD_RECOVERY_CHECK, &mddev->recovery)?
		     "check" :
		     (test_bit(MD_RECOVERY_SYNC, &mddev->recovery) ?
		      "resync" : "recovery"))),
		   per_milli/10, per_milli % 10,
		   (unsigned long long) resync,
		   (unsigned long long) max_blocks);

	/*
	 * We do not want to overflow, so the order of operands and
	 * the * 100 / 100 trick are important. We do a +1 to be
	 * safe against division by zero. We only estimate anyway.
	 *
	 * dt: time from mark until now
	 * db: blocks written from mark until now
	 * rt: remaining time
	 */
	dt = ((jiffies - mddev->resync_mark) / HZ);
	if (!dt) dt++;
	db = (mddev->curr_mark_cnt - atomic_read(&mddev->recovery_active))
		- mddev->resync_mark_cnt;
	rt = (dt * ((unsigned long)(max_blocks-resync) / (db/2/100+1)))/100;

	seq_printf(seq, " finish=%lu.%lumin", rt / 60, (rt % 60)/6);

	seq_printf(seq, " speed=%ldK/sec", db/2/dt);
}

static void *md_seq_start(struct seq_file *seq, loff_t *pos)
{
	struct list_head *tmp;
	loff_t l = *pos;
	mddev_t *mddev;

	if (l >= 0x10000)
		return NULL;
	if (!l--)
		/* header */
		return (void*)1;

	spin_lock(&all_mddevs_lock);
	list_for_each(tmp,&all_mddevs)
		if (!l--) {
			mddev = list_entry(tmp, mddev_t, all_mddevs);
			mddev_get(mddev);
			spin_unlock(&all_mddevs_lock);
			return mddev;
		}
	spin_unlock(&all_mddevs_lock);
	if (!l--)
		return (void*)2;/* tail */
	return NULL;
}

static void *md_seq_next(struct seq_file *seq, void *v, loff_t *pos)
{
	struct list_head *tmp;
	mddev_t *next_mddev, *mddev = v;
	
	++*pos;
	if (v == (void*)2)
		return NULL;

	spin_lock(&all_mddevs_lock);
	if (v == (void*)1)
		tmp = all_mddevs.next;
	else
		tmp = mddev->all_mddevs.next;
	if (tmp != &all_mddevs)
		next_mddev = mddev_get(list_entry(tmp,mddev_t,all_mddevs));
	else {
		next_mddev = (void*)2;
		*pos = 0x10000;
	}		
	spin_unlock(&all_mddevs_lock);

	if (v != (void*)1)
		mddev_put(mddev);
	return next_mddev;

}

static void md_seq_stop(struct seq_file *seq, void *v)
{
	mddev_t *mddev = v;

	if (mddev && v != (void*)1 && v != (void*)2)
		mddev_put(mddev);
}

struct mdstat_info {
	int event;
};

static int md_seq_show(struct seq_file *seq, void *v)
{
	mddev_t *mddev = v;
	sector_t size;
	mdk_rdev_t *rdev;
	struct mdstat_info *mi = seq->private;
	struct bitmap *bitmap;

	if (v == (void*)1) {
		struct mdk_personality *pers;
		seq_printf(seq, "Personalities : ");
		spin_lock(&pers_lock);
		list_for_each_entry(pers, &pers_list, list)
			seq_printf(seq, "[%s] ", pers->name);

		spin_unlock(&pers_lock);
		seq_printf(seq, "\n");
		mi->event = atomic_read(&md_event_count);
		return 0;
	}
	if (v == (void*)2) {
		status_unused(seq);
		return 0;
	}

	if (mddev_lock(mddev) < 0)
		return -EINTR;

	if (mddev->pers || mddev->raid_disks || !list_empty(&mddev->disks)) {
		seq_printf(seq, "%s : %sactive", mdname(mddev),
						mddev->pers ? "" : "in");
		if (mddev->pers) {
			if (mddev->ro==1)
				seq_printf(seq, " (read-only)");
			if (mddev->ro==2)
				seq_printf(seq, " (auto-read-only)");
			seq_printf(seq, " %s", mddev->pers->name);
		}

		size = 0;
		list_for_each_entry(rdev, &mddev->disks, same_set) {
			char b[BDEVNAME_SIZE];
			seq_printf(seq, " %s[%d]",
				bdevname(rdev->bdev,b), rdev->desc_nr);
			if (test_bit(WriteMostly, &rdev->flags))
				seq_printf(seq, "(W)");
			if (test_bit(Faulty, &rdev->flags)) {
				seq_printf(seq, "(F)");
				continue;
			} else if (rdev->raid_disk < 0)
				seq_printf(seq, "(S)"); /* spare */
			size += rdev->size;
		}

		if (!list_empty(&mddev->disks)) {
			if (mddev->pers)
				seq_printf(seq, "\n      %llu blocks",
					   (unsigned long long)
					   mddev->array_sectors / 2);
			else
				seq_printf(seq, "\n      %llu blocks",
					   (unsigned long long)size);
		}
		if (mddev->persistent) {
			if (mddev->major_version != 0 ||
			    mddev->minor_version != 90) {
				seq_printf(seq," super %d.%d",
					   mddev->major_version,
					   mddev->minor_version);
			}
		} else if (mddev->external)
			seq_printf(seq, " super external:%s",
				   mddev->metadata_type);
		else
			seq_printf(seq, " super non-persistent");

		if (mddev->pers) {
			mddev->pers->status(seq, mddev);
	 		seq_printf(seq, "\n      ");
			if (mddev->pers->sync_request) {
				if (mddev->curr_resync > 2) {
					status_resync(seq, mddev);
					seq_printf(seq, "\n      ");
				} else if (mddev->curr_resync == 1 || mddev->curr_resync == 2)
					seq_printf(seq, "\tresync=DELAYED\n      ");
				else if (mddev->recovery_cp < MaxSector)
					seq_printf(seq, "\tresync=PENDING\n      ");
			}
		} else
			seq_printf(seq, "\n       ");

		if ((bitmap = mddev->bitmap)) {
			unsigned long chunk_kb;
			unsigned long flags;
			spin_lock_irqsave(&bitmap->lock, flags);
			chunk_kb = bitmap->chunksize >> 10;
			seq_printf(seq, "bitmap: %lu/%lu pages [%luKB], "
				"%lu%s chunk",
				bitmap->pages - bitmap->missing_pages,
				bitmap->pages,
				(bitmap->pages - bitmap->missing_pages)
					<< (PAGE_SHIFT - 10),
				chunk_kb ? chunk_kb : bitmap->chunksize,
				chunk_kb ? "KB" : "B");
			if (bitmap->file) {
				seq_printf(seq, ", file: ");
				seq_path(seq, &bitmap->file->f_path, " \t\n");
			}

			seq_printf(seq, "\n");
			spin_unlock_irqrestore(&bitmap->lock, flags);
		}

		seq_printf(seq, "\n");
	}
	mddev_unlock(mddev);
	
	return 0;
}

static struct seq_operations md_seq_ops = {
	.start  = md_seq_start,
	.next   = md_seq_next,
	.stop   = md_seq_stop,
	.show   = md_seq_show,
};

static int md_seq_open(struct inode *inode, struct file *file)
{
	int error;
	struct mdstat_info *mi = kmalloc(sizeof(*mi), GFP_KERNEL);
	if (mi == NULL)
		return -ENOMEM;

	error = seq_open(file, &md_seq_ops);
	if (error)
		kfree(mi);
	else {
		struct seq_file *p = file->private_data;
		p->private = mi;
		mi->event = atomic_read(&md_event_count);
	}
	return error;
}

static unsigned int mdstat_poll(struct file *filp, poll_table *wait)
{
	struct seq_file *m = filp->private_data;
	struct mdstat_info *mi = m->private;
	int mask;

	poll_wait(filp, &md_event_waiters, wait);

	/* always allow read */
	mask = POLLIN | POLLRDNORM;

	if (mi->event != atomic_read(&md_event_count))
		mask |= POLLERR | POLLPRI;
	return mask;
}

static const struct file_operations md_seq_fops = {
	.owner		= THIS_MODULE,
	.open           = md_seq_open,
	.read           = seq_read,
	.llseek         = seq_lseek,
	.release	= seq_release_private,
	.poll		= mdstat_poll,
};

int register_md_personality(struct mdk_personality *p)
{
	spin_lock(&pers_lock);
	list_add_tail(&p->list, &pers_list);
	printk(KERN_INFO "md: %s personality registered for level %d\n", p->name, p->level);
	spin_unlock(&pers_lock);
	return 0;
}

int unregister_md_personality(struct mdk_personality *p)
{
	printk(KERN_INFO "md: %s personality unregistered\n", p->name);
	spin_lock(&pers_lock);
	list_del_init(&p->list);
	spin_unlock(&pers_lock);
	return 0;
}

static int is_mddev_idle(mddev_t *mddev)
{
	mdk_rdev_t * rdev;
	int idle;
	long curr_events;

	idle = 1;
	rcu_read_lock();
	rdev_for_each_rcu(rdev, mddev) {
		struct gendisk *disk = rdev->bdev->bd_contains->bd_disk;
		curr_events = part_stat_read(&disk->part0, sectors[0]) +
				part_stat_read(&disk->part0, sectors[1]) -
				atomic_read(&disk->sync_io);
		/* sync IO will cause sync_io to increase before the disk_stats
		 * as sync_io is counted when a request starts, and
		 * disk_stats is counted when it completes.
		 * So resync activity will cause curr_events to be smaller than
		 * when there was no such activity.
		 * non-sync IO will cause disk_stat to increase without
		 * increasing sync_io so curr_events will (eventually)
		 * be larger than it was before.  Once it becomes
		 * substantially larger, the test below will cause
		 * the array to appear non-idle, and resync will slow
		 * down.
		 * If there is a lot of outstanding resync activity when
		 * we set last_event to curr_events, then all that activity
		 * completing might cause the array to appear non-idle
		 * and resync will be slowed down even though there might
		 * not have been non-resync activity.  This will only
		 * happen once though.  'last_events' will soon reflect
		 * the state where there is little or no outstanding
		 * resync requests, and further resync activity will
		 * always make curr_events less than last_events.
		 *
		 */
		if (curr_events - rdev->last_events > 4096) {
			rdev->last_events = curr_events;
			idle = 0;
		}
	}
	rcu_read_unlock();
	return idle;
}

void md_done_sync(mddev_t *mddev, int blocks, int ok)
{
	/* another "blocks" (512byte) blocks have been synced */
	atomic_sub(blocks, &mddev->recovery_active);
	wake_up(&mddev->recovery_wait);
	if (!ok) {
		set_bit(MD_RECOVERY_INTR, &mddev->recovery);
		md_wakeup_thread(mddev->thread);
		// stop recovery, signal do_sync ....
	}
}


void md_write_start(mddev_t *mddev, struct bio *bi)
{
	int did_change = 0;
	if (bio_data_dir(bi) != WRITE)
		return;

	BUG_ON(mddev->ro == 1);
	if (mddev->ro == 2) {
		/* need to switch to read/write */
		mddev->ro = 0;
		set_bit(MD_RECOVERY_NEEDED, &mddev->recovery);
		md_wakeup_thread(mddev->thread);
		md_wakeup_thread(mddev->sync_thread);
		did_change = 1;
	}
	atomic_inc(&mddev->writes_pending);
	if (mddev->safemode == 1)
		mddev->safemode = 0;
	if (mddev->in_sync) {
		spin_lock_irq(&mddev->write_lock);
		if (mddev->in_sync) {
			mddev->in_sync = 0;
			set_bit(MD_CHANGE_CLEAN, &mddev->flags);
			md_wakeup_thread(mddev->thread);
			did_change = 1;
		}
		spin_unlock_irq(&mddev->write_lock);
	}
	if (did_change)
		sysfs_notify_dirent(mddev->sysfs_state);
	wait_event(mddev->sb_wait,
		   !test_bit(MD_CHANGE_CLEAN, &mddev->flags) &&
		   !test_bit(MD_CHANGE_PENDING, &mddev->flags));
}

void md_write_end(mddev_t *mddev)
{
	if (atomic_dec_and_test(&mddev->writes_pending)) {
		if (mddev->safemode == 2)
			md_wakeup_thread(mddev->thread);
		else if (mddev->safemode_delay)
			mod_timer(&mddev->safemode_timer, jiffies + mddev->safemode_delay);
	}
}

int md_allow_write(mddev_t *mddev)
{
	if (!mddev->pers)
		return 0;
	if (mddev->ro)
		return 0;
	if (!mddev->pers->sync_request)
		return 0;

	spin_lock_irq(&mddev->write_lock);
	if (mddev->in_sync) {
		mddev->in_sync = 0;
		set_bit(MD_CHANGE_CLEAN, &mddev->flags);
		if (mddev->safemode_delay &&
		    mddev->safemode == 0)
			mddev->safemode = 1;
		spin_unlock_irq(&mddev->write_lock);
		md_update_sb(mddev, 0);
		sysfs_notify_dirent(mddev->sysfs_state);
	} else
		spin_unlock_irq(&mddev->write_lock);

	if (test_bit(MD_CHANGE_CLEAN, &mddev->flags))
		return -EAGAIN;
	else
		return 0;
}
EXPORT_SYMBOL_GPL(md_allow_write);

#define SYNC_MARKS	10
#define	SYNC_MARK_STEP	(3*HZ)
void md_do_sync(mddev_t *mddev)
{
	mddev_t *mddev2;
	unsigned int currspeed = 0,
		 window;
	sector_t max_sectors,j, io_sectors;
	unsigned long mark[SYNC_MARKS];
	sector_t mark_cnt[SYNC_MARKS];
	int last_mark,m;
	struct list_head *tmp;
	sector_t last_check;
	int skipped = 0;
	mdk_rdev_t *rdev;
	char *desc;

	/* just incase thread restarts... */
	if (test_bit(MD_RECOVERY_DONE, &mddev->recovery))
		return;
	if (mddev->ro) /* never try to sync a read-only array */
		return;

	if (test_bit(MD_RECOVERY_SYNC, &mddev->recovery)) {
		if (test_bit(MD_RECOVERY_CHECK, &mddev->recovery))
			desc = "data-check";
		else if (test_bit(MD_RECOVERY_REQUESTED, &mddev->recovery))
			desc = "requested-resync";
		else
			desc = "resync";
	} else if (test_bit(MD_RECOVERY_RESHAPE, &mddev->recovery))
		desc = "reshape";
	else
		desc = "recovery";

	/* we overload curr_resync somewhat here.
	 * 0 == not engaged in resync at all
	 * 2 == checking that there is no conflict with another sync
	 * 1 == like 2, but have yielded to allow conflicting resync to
	 *		commense
	 * other == active in resync - this many blocks
	 *
	 * Before starting a resync we must have set curr_resync to
	 * 2, and then checked that every "conflicting" array has curr_resync
	 * less than ours.  When we find one that is the same or higher
	 * we wait on resync_wait.  To avoid deadlock, we reduce curr_resync
	 * to 1 if we choose to yield (based arbitrarily on address of mddev structure).
	 * This will mean we have to start checking from the beginning again.
	 *
	 */

	do {
		mddev->curr_resync = 2;

	try_again:
		if (kthread_should_stop()) {
			set_bit(MD_RECOVERY_INTR, &mddev->recovery);
			goto skip;
		}
		for_each_mddev(mddev2, tmp) {
			if (mddev2 == mddev)
				continue;
			if (!mddev->parallel_resync
			&&  mddev2->curr_resync
			&&  match_mddev_units(mddev, mddev2)) {
				DEFINE_WAIT(wq);
				if (mddev < mddev2 && mddev->curr_resync == 2) {
					/* arbitrarily yield */
					mddev->curr_resync = 1;
					wake_up(&resync_wait);
				}
				if (mddev > mddev2 && mddev->curr_resync == 1)
					/* no need to wait here, we can wait the next
					 * time 'round when curr_resync == 2
					 */
					continue;
				/* We need to wait 'interruptible' so as not to
				 * contribute to the load average, and not to
				 * be caught by 'softlockup'
				 */
				prepare_to_wait(&resync_wait, &wq, TASK_INTERRUPTIBLE);
				if (!kthread_should_stop() &&
				    mddev2->curr_resync >= mddev->curr_resync) {
					printk(KERN_INFO "md: delaying %s of %s"
					       " until %s has finished (they"
					       " share one or more physical units)\n",
					       desc, mdname(mddev), mdname(mddev2));
					mddev_put(mddev2);
					if (signal_pending(current))
						flush_signals(current);
					schedule();
					finish_wait(&resync_wait, &wq);
					goto try_again;
				}
				finish_wait(&resync_wait, &wq);
			}
		}
	} while (mddev->curr_resync < 2);

	j = 0;
	if (test_bit(MD_RECOVERY_SYNC, &mddev->recovery)) {
		/* resync follows the size requested by the personality,
		 * which defaults to physical size, but can be virtual size
		 */
		max_sectors = mddev->resync_max_sectors;
		mddev->resync_mismatches = 0;
		/* we don't use the checkpoint if there's a bitmap */
		if (test_bit(MD_RECOVERY_REQUESTED, &mddev->recovery))
			j = mddev->resync_min;
		else if (!mddev->bitmap)
			j = mddev->recovery_cp;

	} else if (test_bit(MD_RECOVERY_RESHAPE, &mddev->recovery))
		max_sectors = mddev->size << 1;
	else {
		/* recovery follows the physical size of devices */
		max_sectors = mddev->size << 1;
		j = MaxSector;
		list_for_each_entry(rdev, &mddev->disks, same_set)
			if (rdev->raid_disk >= 0 &&
			    !test_bit(Faulty, &rdev->flags) &&
			    !test_bit(In_sync, &rdev->flags) &&
			    rdev->recovery_offset < j)
				j = rdev->recovery_offset;
	}

	printk(KERN_INFO "md: %s of RAID array %s\n", desc, mdname(mddev));
	printk(KERN_INFO "md: minimum _guaranteed_  speed:"
		" %d KB/sec/disk.\n", speed_min(mddev));
	printk(KERN_INFO "md: using maximum available idle IO bandwidth "
	       "(but not more than %d KB/sec) for %s.\n",
	       speed_max(mddev), desc);

	is_mddev_idle(mddev); /* this also initializes IO event counters */

	io_sectors = 0;
	for (m = 0; m < SYNC_MARKS; m++) {
		mark[m] = jiffies;
		mark_cnt[m] = io_sectors;
	}
	last_mark = 0;
	mddev->resync_mark = mark[last_mark];
	mddev->resync_mark_cnt = mark_cnt[last_mark];

	/*
	 * Tune reconstruction:
	 */
	window = 32*(PAGE_SIZE/512);
	printk(KERN_INFO "md: using %dk window, over a total of %llu blocks.\n",
		window/2,(unsigned long long) max_sectors/2);

	atomic_set(&mddev->recovery_active, 0);
	last_check = 0;

	if (j>2) {
		printk(KERN_INFO 
		       "md: resuming %s of %s from checkpoint.\n",
		       desc, mdname(mddev));
		mddev->curr_resync = j;
	}

	while (j < max_sectors) {
		sector_t sectors;

		skipped = 0;
		if (j >= mddev->resync_max) {
			sysfs_notify(&mddev->kobj, NULL, "sync_completed");
			wait_event(mddev->recovery_wait,
				   mddev->resync_max > j
				   || kthread_should_stop());
		}
		if (kthread_should_stop())
			goto interrupted;
		sectors = mddev->pers->sync_request(mddev, j, &skipped,
						  currspeed < speed_min(mddev));
		if (sectors == 0) {
			set_bit(MD_RECOVERY_INTR, &mddev->recovery);
			goto out;
		}

		if (!skipped) { /* actual IO requested */
			io_sectors += sectors;
			atomic_add(sectors, &mddev->recovery_active);
		}

		j += sectors;
		if (j>1) mddev->curr_resync = j;
		mddev->curr_mark_cnt = io_sectors;
		if (last_check == 0)
			/* this is the earliers that rebuilt will be
			 * visible in /proc/mdstat
			 */
			md_new_event(mddev);

		if (last_check + window > io_sectors || j == max_sectors)
			continue;

		last_check = io_sectors;

		if (test_bit(MD_RECOVERY_INTR, &mddev->recovery))
			break;

	repeat:
		if (time_after_eq(jiffies, mark[last_mark] + SYNC_MARK_STEP )) {
			/* step marks */
			int next = (last_mark+1) % SYNC_MARKS;

			mddev->resync_mark = mark[next];
			mddev->resync_mark_cnt = mark_cnt[next];
			mark[next] = jiffies;
			mark_cnt[next] = io_sectors - atomic_read(&mddev->recovery_active);
			last_mark = next;
		}


		if (kthread_should_stop())
			goto interrupted;


		/*
		 * this loop exits only if either when we are slower than
		 * the 'hard' speed limit, or the system was IO-idle for
		 * a jiffy.
		 * the system might be non-idle CPU-wise, but we only care
		 * about not overloading the IO subsystem. (things like an
		 * e2fsck being done on the RAID array should execute fast)
		 */
		blk_unplug(mddev->queue);
		cond_resched();

		currspeed = ((unsigned long)(io_sectors-mddev->resync_mark_cnt))/2
			/((jiffies-mddev->resync_mark)/HZ +1) +1;

		if (currspeed > speed_min(mddev)) {
			if ((currspeed > speed_max(mddev)) ||
					!is_mddev_idle(mddev)) {
				msleep(500);
				goto repeat;
			}
		}
	}
	printk(KERN_INFO "md: %s: %s done.\n",mdname(mddev), desc);
	/*
	 * this also signals 'finished resyncing' to md_stop
	 */
 out:
	blk_unplug(mddev->queue);

	wait_event(mddev->recovery_wait, !atomic_read(&mddev->recovery_active));

	/* tell personality that we are finished */
	mddev->pers->sync_request(mddev, max_sectors, &skipped, 1);

	if (!test_bit(MD_RECOVERY_CHECK, &mddev->recovery) &&
	    mddev->curr_resync > 2) {
		if (test_bit(MD_RECOVERY_SYNC, &mddev->recovery)) {
			if (test_bit(MD_RECOVERY_INTR, &mddev->recovery)) {
				if (mddev->curr_resync >= mddev->recovery_cp) {
					printk(KERN_INFO
					       "md: checkpointing %s of %s.\n",
					       desc, mdname(mddev));
					mddev->recovery_cp = mddev->curr_resync;
				}
			} else
				mddev->recovery_cp = MaxSector;
		} else {
			if (!test_bit(MD_RECOVERY_INTR, &mddev->recovery))
				mddev->curr_resync = MaxSector;
			list_for_each_entry(rdev, &mddev->disks, same_set)
				if (rdev->raid_disk >= 0 &&
				    !test_bit(Faulty, &rdev->flags) &&
				    !test_bit(In_sync, &rdev->flags) &&
				    rdev->recovery_offset < mddev->curr_resync)
					rdev->recovery_offset = mddev->curr_resync;
		}
	}
	set_bit(MD_CHANGE_DEVS, &mddev->flags);

 skip:
	mddev->curr_resync = 0;
	mddev->resync_min = 0;
	mddev->resync_max = MaxSector;
	sysfs_notify(&mddev->kobj, NULL, "sync_completed");
	wake_up(&resync_wait);
	set_bit(MD_RECOVERY_DONE, &mddev->recovery);
	md_wakeup_thread(mddev->thread);
	return;

 interrupted:
	/*
	 * got a signal, exit.
	 */
	printk(KERN_INFO
	       "md: md_do_sync() got signal ... exiting\n");
	set_bit(MD_RECOVERY_INTR, &mddev->recovery);
	goto out;

}
EXPORT_SYMBOL_GPL(md_do_sync);


static int remove_and_add_spares(mddev_t *mddev)
{
	mdk_rdev_t *rdev;
	int spares = 0;

	list_for_each_entry(rdev, &mddev->disks, same_set)
		if (rdev->raid_disk >= 0 &&
		    !test_bit(Blocked, &rdev->flags) &&
		    (test_bit(Faulty, &rdev->flags) ||
		     ! test_bit(In_sync, &rdev->flags)) &&
		    atomic_read(&rdev->nr_pending)==0) {
			if (mddev->pers->hot_remove_disk(
				    mddev, rdev->raid_disk)==0) {
				char nm[20];
				sprintf(nm,"rd%d", rdev->raid_disk);
				sysfs_remove_link(&mddev->kobj, nm);
				rdev->raid_disk = -1;
			}
		}

	if (mddev->degraded && ! mddev->ro && !mddev->recovery_disabled) {
		list_for_each_entry(rdev, &mddev->disks, same_set) {
			if (rdev->raid_disk >= 0 &&
			    !test_bit(In_sync, &rdev->flags) &&
			    !test_bit(Blocked, &rdev->flags))
				spares++;
			if (rdev->raid_disk < 0
			    && !test_bit(Faulty, &rdev->flags)) {
				rdev->recovery_offset = 0;
				if (mddev->pers->
				    hot_add_disk(mddev, rdev) == 0) {
					char nm[20];
					sprintf(nm, "rd%d", rdev->raid_disk);
					if (sysfs_create_link(&mddev->kobj,
							      &rdev->kobj, nm))
						printk(KERN_WARNING
						       "md: cannot register "
						       "%s for %s\n",
						       nm, mdname(mddev));
					spares++;
					md_new_event(mddev);
				} else
					break;
			}
		}
	}
	return spares;
}
void md_check_recovery(mddev_t *mddev)
{
	mdk_rdev_t *rdev;


	if (mddev->bitmap)
		bitmap_daemon_work(mddev->bitmap);

	if (mddev->ro)
		return;

	if (signal_pending(current)) {
		if (mddev->pers->sync_request && !mddev->external) {
			printk(KERN_INFO "md: %s in immediate safe mode\n",
			       mdname(mddev));
			mddev->safemode = 2;
		}
		flush_signals(current);
	}

	if (mddev->ro && !test_bit(MD_RECOVERY_NEEDED, &mddev->recovery))
		return;
	if ( ! (
		(mddev->flags && !mddev->external) ||
		test_bit(MD_RECOVERY_NEEDED, &mddev->recovery) ||
		test_bit(MD_RECOVERY_DONE, &mddev->recovery) ||
		(mddev->external == 0 && mddev->safemode == 1) ||
		(mddev->safemode == 2 && ! atomic_read(&mddev->writes_pending)
		 && !mddev->in_sync && mddev->recovery_cp == MaxSector)
		))
		return;

	if (mddev_trylock(mddev)) {
		int spares = 0;

		if (mddev->ro) {
			/* Only thing we do on a ro array is remove
			 * failed devices.
			 */
			remove_and_add_spares(mddev);
			clear_bit(MD_RECOVERY_NEEDED, &mddev->recovery);
			goto unlock;
		}

		if (!mddev->external) {
			int did_change = 0;
			spin_lock_irq(&mddev->write_lock);
			if (mddev->safemode &&
			    !atomic_read(&mddev->writes_pending) &&
			    !mddev->in_sync &&
			    mddev->recovery_cp == MaxSector) {
				mddev->in_sync = 1;
				did_change = 1;
				if (mddev->persistent)
					set_bit(MD_CHANGE_CLEAN, &mddev->flags);
			}
			if (mddev->safemode == 1)
				mddev->safemode = 0;
			spin_unlock_irq(&mddev->write_lock);
			if (did_change)
				sysfs_notify_dirent(mddev->sysfs_state);
		}

		if (mddev->flags)
			md_update_sb(mddev, 0);

		list_for_each_entry(rdev, &mddev->disks, same_set)
			if (test_and_clear_bit(StateChanged, &rdev->flags))
				sysfs_notify_dirent(rdev->sysfs_state);


		if (test_bit(MD_RECOVERY_RUNNING, &mddev->recovery) &&
		    !test_bit(MD_RECOVERY_DONE, &mddev->recovery)) {
			/* resync/recovery still happening */
			clear_bit(MD_RECOVERY_NEEDED, &mddev->recovery);
			goto unlock;
		}
		if (mddev->sync_thread) {
			/* resync has finished, collect result */
			md_unregister_thread(mddev->sync_thread);
			mddev->sync_thread = NULL;
			if (!test_bit(MD_RECOVERY_INTR, &mddev->recovery) &&
			    !test_bit(MD_RECOVERY_REQUESTED, &mddev->recovery)) {
				/* success...*/
				/* activate any spares */
				if (mddev->pers->spare_active(mddev))
					sysfs_notify(&mddev->kobj, NULL,
						     "degraded");
			}
			md_update_sb(mddev, 1);

			/* if array is no-longer degraded, then any saved_raid_disk
			 * information must be scrapped
			 */
			if (!mddev->degraded)
				list_for_each_entry(rdev, &mddev->disks, same_set)
					rdev->saved_raid_disk = -1;

			mddev->recovery = 0;
			/* flag recovery needed just to double check */
			set_bit(MD_RECOVERY_NEEDED, &mddev->recovery);
			sysfs_notify_dirent(mddev->sysfs_action);
			md_new_event(mddev);
			goto unlock;
		}
		/* Set RUNNING before clearing NEEDED to avoid
		 * any transients in the value of "sync_action".
		 */
		set_bit(MD_RECOVERY_RUNNING, &mddev->recovery);
		clear_bit(MD_RECOVERY_NEEDED, &mddev->recovery);
		/* Clear some bits that don't mean anything, but
		 * might be left set
		 */
		clear_bit(MD_RECOVERY_INTR, &mddev->recovery);
		clear_bit(MD_RECOVERY_DONE, &mddev->recovery);

		if (test_bit(MD_RECOVERY_FROZEN, &mddev->recovery))
			goto unlock;
		/* no recovery is running.
		 * remove any failed drives, then
		 * add spares if possible.
		 * Spare are also removed and re-added, to allow
		 * the personality to fail the re-add.
		 */

		if (mddev->reshape_position != MaxSector) {
			if (mddev->pers->check_reshape(mddev) != 0)
				/* Cannot proceed */
				goto unlock;
			set_bit(MD_RECOVERY_RESHAPE, &mddev->recovery);
			clear_bit(MD_RECOVERY_RECOVER, &mddev->recovery);
		} else if ((spares = remove_and_add_spares(mddev))) {
			clear_bit(MD_RECOVERY_SYNC, &mddev->recovery);
			clear_bit(MD_RECOVERY_CHECK, &mddev->recovery);
			clear_bit(MD_RECOVERY_REQUESTED, &mddev->recovery);
			set_bit(MD_RECOVERY_RECOVER, &mddev->recovery);
		} else if (mddev->recovery_cp < MaxSector) {
			set_bit(MD_RECOVERY_SYNC, &mddev->recovery);
			clear_bit(MD_RECOVERY_RECOVER, &mddev->recovery);
		} else if (!test_bit(MD_RECOVERY_SYNC, &mddev->recovery))
			/* nothing to be done ... */
			goto unlock;

		if (mddev->pers->sync_request) {
			if (spares && mddev->bitmap && ! mddev->bitmap->file) {
				/* We are adding a device or devices to an array
				 * which has the bitmap stored on all devices.
				 * So make sure all bitmap pages get written
				 */
				bitmap_write_all(mddev->bitmap);
			}
			mddev->sync_thread = md_register_thread(md_do_sync,
								mddev,
								"%s_resync");
			if (!mddev->sync_thread) {
				printk(KERN_ERR "%s: could not start resync"
					" thread...\n", 
					mdname(mddev));
				/* leave the spares where they are, it shouldn't hurt */
				mddev->recovery = 0;
			} else
				md_wakeup_thread(mddev->sync_thread);
			sysfs_notify_dirent(mddev->sysfs_action);
			md_new_event(mddev);
		}
	unlock:
		if (!mddev->sync_thread) {
			clear_bit(MD_RECOVERY_RUNNING, &mddev->recovery);
			if (test_and_clear_bit(MD_RECOVERY_RECOVER,
					       &mddev->recovery))
				if (mddev->sysfs_action)
					sysfs_notify_dirent(mddev->sysfs_action);
		}
		mddev_unlock(mddev);
	}
}

void md_wait_for_blocked_rdev(mdk_rdev_t *rdev, mddev_t *mddev)
{
	sysfs_notify_dirent(rdev->sysfs_state);
	wait_event_timeout(rdev->blocked_wait,
			   !test_bit(Blocked, &rdev->flags),
			   msecs_to_jiffies(5000));
	rdev_dec_pending(rdev, mddev);
}
EXPORT_SYMBOL(md_wait_for_blocked_rdev);

static int md_notify_reboot(struct notifier_block *this,
			    unsigned long code, void *x)
{
	struct list_head *tmp;
	mddev_t *mddev;

	if ((code == SYS_DOWN) || (code == SYS_HALT) || (code == SYS_POWER_OFF)) {

		printk(KERN_INFO "md: stopping all md devices.\n");

		for_each_mddev(mddev, tmp)
			if (mddev_trylock(mddev)) {
				/* Force a switch to readonly even array
				 * appears to still be in use.  Hence
				 * the '100'.
				 */
				do_md_stop(mddev, 1, 100);
				mddev_unlock(mddev);
			}
		/*
		 * certain more exotic SCSI devices are known to be
		 * volatile wrt too early system reboots. While the
		 * right place to handle this issue is the given
		 * driver, we do want to have a safe RAID driver ...
		 */
		mdelay(1000*1);
	}
	return NOTIFY_DONE;
}

static struct notifier_block md_notifier = {
	.notifier_call	= md_notify_reboot,
	.next		= NULL,
	.priority	= INT_MAX, /* before any real devices */
};

static void md_geninit(void)
{
	dprintk("md: sizeof(mdp_super_t) = %d\n", (int)sizeof(mdp_super_t));

	proc_create("mdstat", S_IRUGO, NULL, &md_seq_fops);
}

static int __init md_init(void)
{
	if (register_blkdev(MAJOR_NR, "md"))
		return -1;
	if ((mdp_major=register_blkdev(0, "mdp"))<=0) {
		unregister_blkdev(MAJOR_NR, "md");
		return -1;
	}
	blk_register_region(MKDEV(MAJOR_NR, 0), 1UL<<MINORBITS, THIS_MODULE,
			    md_probe, NULL, NULL);
	blk_register_region(MKDEV(mdp_major, 0), 1UL<<MINORBITS, THIS_MODULE,
			    md_probe, NULL, NULL);

	register_reboot_notifier(&md_notifier);
	raid_table_header = register_sysctl_table(raid_root_table);

	md_geninit();
	return 0;
}


#ifndef MODULE


static LIST_HEAD(all_detected_devices);
struct detected_devices_node {
	struct list_head list;
	dev_t dev;
};

void md_autodetect_dev(dev_t dev)
{
	struct detected_devices_node *node_detected_dev;

	node_detected_dev = kzalloc(sizeof(*node_detected_dev), GFP_KERNEL);
	if (node_detected_dev) {
		node_detected_dev->dev = dev;
		list_add_tail(&node_detected_dev->list, &all_detected_devices);
	} else {
		printk(KERN_CRIT "md: md_autodetect_dev: kzalloc failed"
			", skipping dev(%d,%d)\n", MAJOR(dev), MINOR(dev));
	}
}


static void autostart_arrays(int part)
{
	mdk_rdev_t *rdev;
	struct detected_devices_node *node_detected_dev;
	dev_t dev;
	int i_scanned, i_passed;

	i_scanned = 0;
	i_passed = 0;

	printk(KERN_INFO "md: Autodetecting RAID arrays.\n");

	while (!list_empty(&all_detected_devices) && i_scanned < INT_MAX) {
		i_scanned++;
		node_detected_dev = list_entry(all_detected_devices.next,
					struct detected_devices_node, list);
		list_del(&node_detected_dev->list);
		dev = node_detected_dev->dev;
		kfree(node_detected_dev);
		rdev = md_import_device(dev,0, 90);
		if (IS_ERR(rdev))
			continue;

		if (test_bit(Faulty, &rdev->flags)) {
			MD_BUG();
			continue;
		}
		set_bit(AutoDetected, &rdev->flags);
		list_add(&rdev->same_set, &pending_raid_disks);
		i_passed++;
	}

	printk(KERN_INFO "md: Scanned %d and added %d devices.\n",
						i_scanned, i_passed);

	autorun_devices(part);
}

#endif /* !MODULE */

static __exit void md_exit(void)
{
	mddev_t *mddev;
	struct list_head *tmp;

	blk_unregister_region(MKDEV(MAJOR_NR,0), 1U << MINORBITS);
	blk_unregister_region(MKDEV(mdp_major,0), 1U << MINORBITS);

	unregister_blkdev(MAJOR_NR,"md");
	unregister_blkdev(mdp_major, "mdp");
	unregister_reboot_notifier(&md_notifier);
	unregister_sysctl_table(raid_table_header);
	remove_proc_entry("mdstat", NULL);
	for_each_mddev(mddev, tmp) {
		export_array(mddev);
		mddev->hold_active = 0;
	}
}

subsys_initcall(md_init);
module_exit(md_exit)

static int get_ro(char *buffer, struct kernel_param *kp)
{
	return sprintf(buffer, "%d", start_readonly);
}
static int set_ro(const char *val, struct kernel_param *kp)
{
	char *e;
	int num = simple_strtoul(val, &e, 10);
	if (*val && (*e == '\0' || *e == '\n')) {
		start_readonly = num;
		return 0;
	}
	return -EINVAL;
}

module_param_call(start_ro, set_ro, get_ro, NULL, S_IRUSR|S_IWUSR);
module_param(start_dirty_degraded, int, S_IRUGO|S_IWUSR);

module_param_call(new_array, add_named_array, NULL, NULL, S_IWUSR);

EXPORT_SYMBOL(register_md_personality);
EXPORT_SYMBOL(unregister_md_personality);
EXPORT_SYMBOL(md_error);
EXPORT_SYMBOL(md_done_sync);
EXPORT_SYMBOL(md_write_start);
EXPORT_SYMBOL(md_write_end);
EXPORT_SYMBOL(md_register_thread);
EXPORT_SYMBOL(md_unregister_thread);
EXPORT_SYMBOL(md_wakeup_thread);
EXPORT_SYMBOL(md_check_recovery);
MODULE_LICENSE("GPL");
MODULE_ALIAS("md");
MODULE_ALIAS_BLOCKDEV_MAJOR(MD_MAJOR);