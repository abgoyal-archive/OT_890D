



#define	WriteTransient	0
#define	ReadTransient	1
#define	WritePersistent	2
#define	ReadPersistent	3
#define	WriteAll	4 /* doesn't go to device */
#define	ReadFixable	5
#define	Modes	6

#define	ClearErrors	31
#define	ClearFaults	30

#define AllPersist	100 /* internal use only */
#define	NoPersist	101

#define	ModeMask	0x1f
#define	ModeShift	5

#define MaxFault	50
#include <linux/raid/md.h>


static void faulty_fail(struct bio *bio, int error)
{
	struct bio *b = bio->bi_private;

	b->bi_size = bio->bi_size;
	b->bi_sector = bio->bi_sector;

	bio_put(bio);

	bio_io_error(b);
}

typedef struct faulty_conf {
	int period[Modes];
	atomic_t counters[Modes];
	sector_t faults[MaxFault];
	int	modes[MaxFault];
	int nfaults;
	mdk_rdev_t *rdev;
} conf_t;

static int check_mode(conf_t *conf, int mode)
{
	if (conf->period[mode] == 0 &&
	    atomic_read(&conf->counters[mode]) <= 0)
		return 0; /* no failure, no decrement */


	if (atomic_dec_and_test(&conf->counters[mode])) {
		if (conf->period[mode])
			atomic_set(&conf->counters[mode], conf->period[mode]);
		return 1;
	}
	return 0;
}

static int check_sector(conf_t *conf, sector_t start, sector_t end, int dir)
{
	/* If we find a ReadFixable sector, we fix it ... */
	int i;
	for (i=0; i<conf->nfaults; i++)
		if (conf->faults[i] >= start &&
		    conf->faults[i] < end) {
			/* found it ... */
			switch (conf->modes[i] * 2 + dir) {
			case WritePersistent*2+WRITE: return 1;
			case ReadPersistent*2+READ: return 1;
			case ReadFixable*2+READ: return 1;
			case ReadFixable*2+WRITE:
				conf->modes[i] = NoPersist;
				return 0;
			case AllPersist*2+READ:
			case AllPersist*2+WRITE: return 1;
			default:
				return 0;
			}
		}
	return 0;
}

static void add_sector(conf_t *conf, sector_t start, int mode)
{
	int i;
	int n = conf->nfaults;
	for (i=0; i<conf->nfaults; i++)
		if (conf->faults[i] == start) {
			switch(mode) {
			case NoPersist: conf->modes[i] = mode; return;
			case WritePersistent:
				if (conf->modes[i] == ReadPersistent ||
				    conf->modes[i] == ReadFixable)
					conf->modes[i] = AllPersist;
				else
					conf->modes[i] = WritePersistent;
				return;
			case ReadPersistent:
				if (conf->modes[i] == WritePersistent)
					conf->modes[i] = AllPersist;
				else
					conf->modes[i] = ReadPersistent;
				return;
			case ReadFixable:
				if (conf->modes[i] == WritePersistent ||
				    conf->modes[i] == ReadPersistent)
					conf->modes[i] = AllPersist;
				else
					conf->modes[i] = ReadFixable;
				return;
			}
		} else if (conf->modes[i] == NoPersist)
			n = i;

	if (n >= MaxFault)
		return;
	conf->faults[n] = start;
	conf->modes[n] = mode;
	if (conf->nfaults == n)
		conf->nfaults = n+1;
}

static int make_request(struct request_queue *q, struct bio *bio)
{
	mddev_t *mddev = q->queuedata;
	conf_t *conf = (conf_t*)mddev->private;
	int failit = 0;

	if (bio_data_dir(bio) == WRITE) {
		/* write request */
		if (atomic_read(&conf->counters[WriteAll])) {
			/* special case - don't decrement, don't generic_make_request,
			 * just fail immediately
			 */
			bio_endio(bio, -EIO);
			return 0;
		}

		if (check_sector(conf, bio->bi_sector, bio->bi_sector+(bio->bi_size>>9),
				 WRITE))
			failit = 1;
		if (check_mode(conf, WritePersistent)) {
			add_sector(conf, bio->bi_sector, WritePersistent);
			failit = 1;
		}
		if (check_mode(conf, WriteTransient))
			failit = 1;
	} else {
		/* read request */
		if (check_sector(conf, bio->bi_sector, bio->bi_sector + (bio->bi_size>>9),
				 READ))
			failit = 1;
		if (check_mode(conf, ReadTransient))
			failit = 1;
		if (check_mode(conf, ReadPersistent)) {
			add_sector(conf, bio->bi_sector, ReadPersistent);
			failit = 1;
		}
		if (check_mode(conf, ReadFixable)) {
			add_sector(conf, bio->bi_sector, ReadFixable);
			failit = 1;
		}
	}
	if (failit) {
		struct bio *b = bio_clone(bio, GFP_NOIO);
		b->bi_bdev = conf->rdev->bdev;
		b->bi_private = bio;
		b->bi_end_io = faulty_fail;
		generic_make_request(b);
		return 0;
	} else {
		bio->bi_bdev = conf->rdev->bdev;
		return 1;
	}
}

static void status(struct seq_file *seq, mddev_t *mddev)
{
	conf_t *conf = (conf_t*)mddev->private;
	int n;

	if ((n=atomic_read(&conf->counters[WriteTransient])) != 0)
		seq_printf(seq, " WriteTransient=%d(%d)",
			   n, conf->period[WriteTransient]);

	if ((n=atomic_read(&conf->counters[ReadTransient])) != 0)
		seq_printf(seq, " ReadTransient=%d(%d)",
			   n, conf->period[ReadTransient]);

	if ((n=atomic_read(&conf->counters[WritePersistent])) != 0)
		seq_printf(seq, " WritePersistent=%d(%d)",
			   n, conf->period[WritePersistent]);

	if ((n=atomic_read(&conf->counters[ReadPersistent])) != 0)
		seq_printf(seq, " ReadPersistent=%d(%d)",
			   n, conf->period[ReadPersistent]);


	if ((n=atomic_read(&conf->counters[ReadFixable])) != 0)
		seq_printf(seq, " ReadFixable=%d(%d)",
			   n, conf->period[ReadFixable]);

	if ((n=atomic_read(&conf->counters[WriteAll])) != 0)
		seq_printf(seq, " WriteAll");

	seq_printf(seq, " nfaults=%d", conf->nfaults);
}


static int reconfig(mddev_t *mddev, int layout, int chunk_size)
{
	int mode = layout & ModeMask;
	int count = layout >> ModeShift;
	conf_t *conf = mddev->private;

	if (chunk_size != -1)
		return -EINVAL;

	/* new layout */
	if (mode == ClearFaults)
		conf->nfaults = 0;
	else if (mode == ClearErrors) {
		int i;
		for (i=0 ; i < Modes ; i++) {
			conf->period[i] = 0;
			atomic_set(&conf->counters[i], 0);
		}
	} else if (mode < Modes) {
		conf->period[mode] = count;
		if (!count) count++;
		atomic_set(&conf->counters[mode], count);
	} else
		return -EINVAL;
	mddev->layout = -1; /* makes sure further changes come through */
	return 0;
}

static int run(mddev_t *mddev)
{
	mdk_rdev_t *rdev;
	int i;

	conf_t *conf = kmalloc(sizeof(*conf), GFP_KERNEL);
	if (!conf)
		return -ENOMEM;

	for (i=0; i<Modes; i++) {
		atomic_set(&conf->counters[i], 0);
		conf->period[i] = 0;
	}
	conf->nfaults = 0;

	list_for_each_entry(rdev, &mddev->disks, same_set)
		conf->rdev = rdev;

	mddev->array_sectors = mddev->size * 2;
	mddev->private = conf;

	reconfig(mddev, mddev->layout, -1);

	return 0;
}

static int stop(mddev_t *mddev)
{
	conf_t *conf = (conf_t *)mddev->private;

	kfree(conf);
	mddev->private = NULL;
	return 0;
}

static struct mdk_personality faulty_personality =
{
	.name		= "faulty",
	.level		= LEVEL_FAULTY,
	.owner		= THIS_MODULE,
	.make_request	= make_request,
	.run		= run,
	.stop		= stop,
	.status		= status,
	.reconfig	= reconfig,
};

static int __init raid_init(void)
{
	return register_md_personality(&faulty_personality);
}

static void raid_exit(void)
{
	unregister_md_personality(&faulty_personality);
}

module_init(raid_init);
module_exit(raid_exit);
MODULE_LICENSE("GPL");
MODULE_ALIAS("md-personality-10"); /* faulty */
MODULE_ALIAS("md-faulty");
MODULE_ALIAS("md-level--5");