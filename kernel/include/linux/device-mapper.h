

#ifndef _LINUX_DEVICE_MAPPER_H
#define _LINUX_DEVICE_MAPPER_H

#include <linux/bio.h>
#include <linux/blkdev.h>

struct dm_target;
struct dm_table;
struct mapped_device;
struct bio_vec;

typedef enum { STATUSTYPE_INFO, STATUSTYPE_TABLE } status_type_t;

union map_info {
	void *ptr;
	unsigned long long ll;
};

typedef int (*dm_ctr_fn) (struct dm_target *target,
			  unsigned int argc, char **argv);

typedef void (*dm_dtr_fn) (struct dm_target *ti);

typedef int (*dm_map_fn) (struct dm_target *ti, struct bio *bio,
			  union map_info *map_context);
typedef int (*dm_map_request_fn) (struct dm_target *ti, struct request *clone,
				  union map_info *map_context);

typedef int (*dm_endio_fn) (struct dm_target *ti,
			    struct bio *bio, int error,
			    union map_info *map_context);
typedef int (*dm_request_endio_fn) (struct dm_target *ti,
				    struct request *clone, int error,
				    union map_info *map_context);

typedef void (*dm_flush_fn) (struct dm_target *ti);
typedef void (*dm_presuspend_fn) (struct dm_target *ti);
typedef void (*dm_postsuspend_fn) (struct dm_target *ti);
typedef int (*dm_preresume_fn) (struct dm_target *ti);
typedef void (*dm_resume_fn) (struct dm_target *ti);

typedef int (*dm_status_fn) (struct dm_target *ti, status_type_t status_type,
			     char *result, unsigned int maxlen);

typedef int (*dm_message_fn) (struct dm_target *ti, unsigned argc, char **argv);

typedef int (*dm_ioctl_fn) (struct dm_target *ti, unsigned int cmd,
			    unsigned long arg);

typedef int (*dm_merge_fn) (struct dm_target *ti, struct bvec_merge_data *bvm,
			    struct bio_vec *biovec, int max_size);

typedef int (*dm_busy_fn) (struct dm_target *ti);

void dm_error(const char *message);

void dm_set_device_limits(struct dm_target *ti, struct block_device *bdev);

struct dm_dev {
	struct block_device *bdev;
	fmode_t mode;
	char name[16];
};

int dm_get_device(struct dm_target *ti, const char *path, sector_t start,
		  sector_t len, fmode_t mode, struct dm_dev **result);
void dm_put_device(struct dm_target *ti, struct dm_dev *d);


#define DM_TARGET_SUPPORTS_BARRIERS 0x00000001

struct target_type {
	uint64_t features;
	const char *name;
	struct module *module;
	unsigned version[3];
	dm_ctr_fn ctr;
	dm_dtr_fn dtr;
	dm_map_fn map;
	dm_map_request_fn map_rq;
	dm_endio_fn end_io;
	dm_request_endio_fn rq_end_io;
	dm_flush_fn flush;
	dm_presuspend_fn presuspend;
	dm_postsuspend_fn postsuspend;
	dm_preresume_fn preresume;
	dm_resume_fn resume;
	dm_status_fn status;
	dm_message_fn message;
	dm_ioctl_fn ioctl;
	dm_merge_fn merge;
	dm_busy_fn busy;
};

struct io_restrictions {
	unsigned long bounce_pfn;
	unsigned long seg_boundary_mask;
	unsigned max_hw_sectors;
	unsigned max_sectors;
	unsigned max_segment_size;
	unsigned short hardsect_size;
	unsigned short max_hw_segments;
	unsigned short max_phys_segments;
	unsigned char no_cluster; /* inverted so that 0 is default */
};

struct dm_target {
	struct dm_table *table;
	struct target_type *type;

	/* target limits */
	sector_t begin;
	sector_t len;

	/* FIXME: turn this into a mask, and merge with io_restrictions */
	/* Always a power of 2 */
	sector_t split_io;

	/*
	 * These are automatically filled in by
	 * dm_table_get_device.
	 */
	struct io_restrictions limits;

	/* target specific data */
	void *private;

	/* Used to provide an error string from the ctr */
	char *error;
};

int dm_register_target(struct target_type *t);
void dm_unregister_target(struct target_type *t);


#define DM_ANY_MINOR (-1)
int dm_create(int minor, struct mapped_device **md);

struct mapped_device *dm_get_md(dev_t dev);
void dm_get(struct mapped_device *md);
void dm_put(struct mapped_device *md);

void dm_set_mdptr(struct mapped_device *md, void *ptr);
void *dm_get_mdptr(struct mapped_device *md);

int dm_suspend(struct mapped_device *md, unsigned suspend_flags);
int dm_resume(struct mapped_device *md);

uint32_t dm_get_event_nr(struct mapped_device *md);
int dm_wait_event(struct mapped_device *md, int event_nr);
uint32_t dm_next_uevent_seq(struct mapped_device *md);
void dm_uevent_add(struct mapped_device *md, struct list_head *elist);

const char *dm_device_name(struct mapped_device *md);
int dm_copy_name_and_uuid(struct mapped_device *md, char *name, char *uuid);
struct gendisk *dm_disk(struct mapped_device *md);
int dm_suspended(struct mapped_device *md);
int dm_noflush_suspending(struct dm_target *ti);
union map_info *dm_get_mapinfo(struct bio *bio);

int dm_get_geometry(struct mapped_device *md, struct hd_geometry *geo);
int dm_set_geometry(struct mapped_device *md, struct hd_geometry *geo);



int dm_table_create(struct dm_table **result, fmode_t mode,
		    unsigned num_targets, struct mapped_device *md);

int dm_table_add_target(struct dm_table *t, const char *type,
			sector_t start, sector_t len, char *params);

int dm_table_complete(struct dm_table *t);

void dm_table_unplug_all(struct dm_table *t);

struct dm_table *dm_get_table(struct mapped_device *md);
void dm_table_get(struct dm_table *t);
void dm_table_put(struct dm_table *t);

sector_t dm_table_get_size(struct dm_table *t);
unsigned int dm_table_get_num_targets(struct dm_table *t);
fmode_t dm_table_get_mode(struct dm_table *t);
struct mapped_device *dm_table_get_md(struct dm_table *t);

void dm_table_event(struct dm_table *t);

int dm_swap_table(struct mapped_device *md, struct dm_table *t);

void *dm_vcalloc(unsigned long nmemb, unsigned long elem_size);

#define DM_NAME "device-mapper"

#define DMCRIT(f, arg...) \
	printk(KERN_CRIT DM_NAME ": " DM_MSG_PREFIX ": " f "\n", ## arg)

#define DMERR(f, arg...) \
	printk(KERN_ERR DM_NAME ": " DM_MSG_PREFIX ": " f "\n", ## arg)
#define DMERR_LIMIT(f, arg...) \
	do { \
		if (printk_ratelimit())	\
			printk(KERN_ERR DM_NAME ": " DM_MSG_PREFIX ": " \
			       f "\n", ## arg); \
	} while (0)

#define DMWARN(f, arg...) \
	printk(KERN_WARNING DM_NAME ": " DM_MSG_PREFIX ": " f "\n", ## arg)
#define DMWARN_LIMIT(f, arg...) \
	do { \
		if (printk_ratelimit())	\
			printk(KERN_WARNING DM_NAME ": " DM_MSG_PREFIX ": " \
			       f "\n", ## arg); \
	} while (0)

#define DMINFO(f, arg...) \
	printk(KERN_INFO DM_NAME ": " DM_MSG_PREFIX ": " f "\n", ## arg)
#define DMINFO_LIMIT(f, arg...) \
	do { \
		if (printk_ratelimit())	\
			printk(KERN_INFO DM_NAME ": " DM_MSG_PREFIX ": " f \
			       "\n", ## arg); \
	} while (0)

#ifdef CONFIG_DM_DEBUG
#  define DMDEBUG(f, arg...) \
	printk(KERN_DEBUG DM_NAME ": " DM_MSG_PREFIX " DEBUG: " f "\n", ## arg)
#  define DMDEBUG_LIMIT(f, arg...) \
	do { \
		if (printk_ratelimit())	\
			printk(KERN_DEBUG DM_NAME ": " DM_MSG_PREFIX ": " f \
			       "\n", ## arg); \
	} while (0)
#else
#  define DMDEBUG(f, arg...) do {} while (0)
#  define DMDEBUG_LIMIT(f, arg...) do {} while (0)
#endif

#define DMEMIT(x...) sz += ((sz >= maxlen) ? \
			  0 : scnprintf(result + sz, maxlen - sz, x))

#define SECTOR_SHIFT 9

#define DM_ENDIO_INCOMPLETE	1
#define DM_ENDIO_REQUEUE	2

#define DM_MAPIO_SUBMITTED	0
#define DM_MAPIO_REMAPPED	1
#define DM_MAPIO_REQUEUE	DM_ENDIO_REQUEUE

#define dm_div_up(n, sz) (((n) + (sz) - 1) / (sz))

#define dm_sector_div_up(n, sz) ( \
{ \
	sector_t _r = ((n) + (sz) - 1); \
	sector_div(_r, (sz)); \
	_r; \
} \
)

#define dm_round_up(n, sz) (dm_div_up((n), (sz)) * (sz))

#define dm_array_too_big(fixed, obj, num) \
	((num) > (UINT_MAX - (fixed)) / (obj))

static inline sector_t to_sector(unsigned long n)
{
	return (n >> SECTOR_SHIFT);
}

static inline unsigned long to_bytes(sector_t n)
{
	return (n << SECTOR_SHIFT);
}

#endif	/* _LINUX_DEVICE_MAPPER_H */
