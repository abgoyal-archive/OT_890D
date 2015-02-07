
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/bio.h>
#include <linux/blkdev.h>
#include <scsi/sg.h>		/* for struct sg_iovec */

#include "blk.h"

int blk_rq_append_bio(struct request_queue *q, struct request *rq,
		      struct bio *bio)
{
	if (!rq->bio)
		blk_rq_bio_prep(q, rq, bio);
	else if (!ll_back_merge_fn(q, rq, bio))
		return -EINVAL;
	else {
		rq->biotail->bi_next = bio;
		rq->biotail = bio;

		rq->data_len += bio->bi_size;
	}
	return 0;
}
EXPORT_SYMBOL(blk_rq_append_bio);

static int __blk_rq_unmap_user(struct bio *bio)
{
	int ret = 0;

	if (bio) {
		if (bio_flagged(bio, BIO_USER_MAPPED))
			bio_unmap_user(bio);
		else
			ret = bio_uncopy_user(bio);
	}

	return ret;
}

static int __blk_rq_map_user(struct request_queue *q, struct request *rq,
			     struct rq_map_data *map_data, void __user *ubuf,
			     unsigned int len, gfp_t gfp_mask)
{
	unsigned long uaddr;
	struct bio *bio, *orig_bio;
	int reading, ret;

	reading = rq_data_dir(rq) == READ;

	/*
	 * if alignment requirement is satisfied, map in user pages for
	 * direct dma. else, set up kernel bounce buffers
	 */
	uaddr = (unsigned long) ubuf;
	if (blk_rq_aligned(q, ubuf, len) && !map_data)
		bio = bio_map_user(q, NULL, uaddr, len, reading, gfp_mask);
	else
		bio = bio_copy_user(q, map_data, uaddr, len, reading, gfp_mask);

	if (IS_ERR(bio))
		return PTR_ERR(bio);

	if (map_data && map_data->null_mapped)
		bio->bi_flags |= (1 << BIO_NULL_MAPPED);

	orig_bio = bio;
	blk_queue_bounce(q, &bio);

	/*
	 * We link the bounce buffer in and could have to traverse it
	 * later so we have to get a ref to prevent it from being freed
	 */
	bio_get(bio);

	ret = blk_rq_append_bio(q, rq, bio);
	if (!ret)
		return bio->bi_size;

	/* if it was boucned we must call the end io function */
	bio_endio(bio, 0);
	__blk_rq_unmap_user(orig_bio);
	bio_put(bio);
	return ret;
}

int blk_rq_map_user(struct request_queue *q, struct request *rq,
		    struct rq_map_data *map_data, void __user *ubuf,
		    unsigned long len, gfp_t gfp_mask)
{
	unsigned long bytes_read = 0;
	struct bio *bio = NULL;
	int ret;

	if (len > (q->max_hw_sectors << 9))
		return -EINVAL;
	if (!len)
		return -EINVAL;

	if (!ubuf && (!map_data || !map_data->null_mapped))
		return -EINVAL;

	while (bytes_read != len) {
		unsigned long map_len, end, start;

		map_len = min_t(unsigned long, len - bytes_read, BIO_MAX_SIZE);
		end = ((unsigned long)ubuf + map_len + PAGE_SIZE - 1)
								>> PAGE_SHIFT;
		start = (unsigned long)ubuf >> PAGE_SHIFT;

		/*
		 * A bad offset could cause us to require BIO_MAX_PAGES + 1
		 * pages. If this happens we just lower the requested
		 * mapping len by a page so that we can fit
		 */
		if (end - start > BIO_MAX_PAGES)
			map_len -= PAGE_SIZE;

		ret = __blk_rq_map_user(q, rq, map_data, ubuf, map_len,
					gfp_mask);
		if (ret < 0)
			goto unmap_rq;
		if (!bio)
			bio = rq->bio;
		bytes_read += ret;
		ubuf += ret;

		if (map_data)
			map_data->offset += ret;
	}

	if (!bio_flagged(bio, BIO_USER_MAPPED))
		rq->cmd_flags |= REQ_COPY_USER;

	rq->buffer = rq->data = NULL;
	return 0;
unmap_rq:
	blk_rq_unmap_user(bio);
	rq->bio = NULL;
	return ret;
}
EXPORT_SYMBOL(blk_rq_map_user);

int blk_rq_map_user_iov(struct request_queue *q, struct request *rq,
			struct rq_map_data *map_data, struct sg_iovec *iov,
			int iov_count, unsigned int len, gfp_t gfp_mask)
{
	struct bio *bio;
	int i, read = rq_data_dir(rq) == READ;
	int unaligned = 0;

	if (!iov || iov_count <= 0)
		return -EINVAL;

	for (i = 0; i < iov_count; i++) {
		unsigned long uaddr = (unsigned long)iov[i].iov_base;

		if (uaddr & queue_dma_alignment(q)) {
			unaligned = 1;
			break;
		}
	}

	if (unaligned || (q->dma_pad_mask & len) || map_data)
		bio = bio_copy_user_iov(q, map_data, iov, iov_count, read,
					gfp_mask);
	else
		bio = bio_map_user_iov(q, NULL, iov, iov_count, read, gfp_mask);

	if (IS_ERR(bio))
		return PTR_ERR(bio);

	if (bio->bi_size != len) {
		/*
		 * Grab an extra reference to this bio, as bio_unmap_user()
		 * expects to be able to drop it twice as it happens on the
		 * normal IO completion path
		 */
		bio_get(bio);
		bio_endio(bio, 0);
		__blk_rq_unmap_user(bio);
		return -EINVAL;
	}

	if (!bio_flagged(bio, BIO_USER_MAPPED))
		rq->cmd_flags |= REQ_COPY_USER;

	blk_queue_bounce(q, &bio);
	bio_get(bio);
	blk_rq_bio_prep(q, rq, bio);
	rq->buffer = rq->data = NULL;
	return 0;
}
EXPORT_SYMBOL(blk_rq_map_user_iov);

int blk_rq_unmap_user(struct bio *bio)
{
	struct bio *mapped_bio;
	int ret = 0, ret2;

	while (bio) {
		mapped_bio = bio;
		if (unlikely(bio_flagged(bio, BIO_BOUNCED)))
			mapped_bio = bio->bi_private;

		ret2 = __blk_rq_unmap_user(mapped_bio);
		if (ret2 && !ret)
			ret = ret2;

		mapped_bio = bio;
		bio = bio->bi_next;
		bio_put(mapped_bio);
	}

	return ret;
}
EXPORT_SYMBOL(blk_rq_unmap_user);

int blk_rq_map_kern(struct request_queue *q, struct request *rq, void *kbuf,
		    unsigned int len, gfp_t gfp_mask)
{
	int reading = rq_data_dir(rq) == READ;
	int do_copy = 0;
	struct bio *bio;

	if (len > (q->max_hw_sectors << 9))
		return -EINVAL;
	if (!len || !kbuf)
		return -EINVAL;

	do_copy = !blk_rq_aligned(q, kbuf, len) || object_is_on_stack(kbuf);
	if (do_copy)
		bio = bio_copy_kern(q, kbuf, len, gfp_mask, reading);
	else
		bio = bio_map_kern(q, kbuf, len, gfp_mask);

	if (IS_ERR(bio))
		return PTR_ERR(bio);

	if (rq_data_dir(rq) == WRITE)
		bio->bi_rw |= (1 << BIO_RW);

	if (do_copy)
		rq->cmd_flags |= REQ_COPY_USER;

	blk_rq_bio_prep(q, rq, bio);
	blk_queue_bounce(q, &rq->bio);
	rq->buffer = rq->data = NULL;
	return 0;
}
EXPORT_SYMBOL(blk_rq_map_kern);