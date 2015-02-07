

#ifndef DM_BIO_RECORD_H
#define DM_BIO_RECORD_H

#include <linux/bio.h>

struct dm_bio_details {
	sector_t bi_sector;
	struct block_device *bi_bdev;
	unsigned int bi_size;
	unsigned short bi_idx;
	unsigned long bi_flags;
};

static inline void dm_bio_record(struct dm_bio_details *bd, struct bio *bio)
{
	bd->bi_sector = bio->bi_sector;
	bd->bi_bdev = bio->bi_bdev;
	bd->bi_size = bio->bi_size;
	bd->bi_idx = bio->bi_idx;
	bd->bi_flags = bio->bi_flags;
}

static inline void dm_bio_restore(struct dm_bio_details *bd, struct bio *bio)
{
	bio->bi_sector = bd->bi_sector;
	bio->bi_bdev = bd->bi_bdev;
	bio->bi_size = bd->bi_size;
	bio->bi_idx = bd->bi_idx;
	bio->bi_flags = bd->bi_flags;
}

#endif
