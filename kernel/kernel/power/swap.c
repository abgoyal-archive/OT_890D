

#include <linux/module.h>
#include <linux/file.h>
#include <linux/utsname.h>
#include <linux/delay.h>
#include <linux/bitops.h>
#include <linux/genhd.h>
#include <linux/device.h>
#include <linux/buffer_head.h>
#include <linux/bio.h>
#include <linux/blkdev.h>
#include <linux/swap.h>
#include <linux/swapops.h>
#include <linux/pm.h>

#include "power.h"

#define SWSUSP_SIG	"S1SUSPEND"

struct swsusp_header {
	char reserved[PAGE_SIZE - 20 - sizeof(sector_t) - sizeof(int)];
	sector_t image;
	unsigned int flags;	/* Flags to pass to the "boot" kernel */
	char	orig_sig[10];
	char	sig[10];
} __attribute__((packed));

static struct swsusp_header *swsusp_header;


static unsigned short root_swap = 0xffff;
static struct block_device *resume_bdev;

static int submit(int rw, pgoff_t page_off, struct page *page,
			struct bio **bio_chain)
{
	const int bio_rw = rw | (1 << BIO_RW_SYNCIO) | (1 << BIO_RW_UNPLUG);
	struct bio *bio;

	bio = bio_alloc(__GFP_WAIT | __GFP_HIGH, 1);
	if (!bio)
		return -ENOMEM;
	bio->bi_sector = page_off * (PAGE_SIZE >> 9);
	bio->bi_bdev = resume_bdev;
	bio->bi_end_io = end_swap_bio_read;

	if (bio_add_page(bio, page, PAGE_SIZE, 0) < PAGE_SIZE) {
		printk(KERN_ERR "PM: Adding page to bio failed at %ld\n",
			page_off);
		bio_put(bio);
		return -EFAULT;
	}

	lock_page(page);
	bio_get(bio);

	if (bio_chain == NULL) {
		submit_bio(bio_rw, bio);
		wait_on_page_locked(page);
		if (rw == READ)
			bio_set_pages_dirty(bio);
		bio_put(bio);
	} else {
		if (rw == READ)
			get_page(page);	/* These pages are freed later */
		bio->bi_private = *bio_chain;
		*bio_chain = bio;
		submit_bio(bio_rw, bio);
	}
	return 0;
}

static int bio_read_page(pgoff_t page_off, void *addr, struct bio **bio_chain)
{
	return submit(READ, page_off, virt_to_page(addr), bio_chain);
}

static int bio_write_page(pgoff_t page_off, void *addr, struct bio **bio_chain)
{
	return submit(WRITE, page_off, virt_to_page(addr), bio_chain);
}

static int wait_on_bio_chain(struct bio **bio_chain)
{
	struct bio *bio;
	struct bio *next_bio;
	int ret = 0;

	if (bio_chain == NULL)
		return 0;

	bio = *bio_chain;
	if (bio == NULL)
		return 0;
	while (bio) {
		struct page *page;

		next_bio = bio->bi_private;
		page = bio->bi_io_vec[0].bv_page;
		wait_on_page_locked(page);
		if (!PageUptodate(page) || PageError(page))
			ret = -EIO;
		put_page(page);
		bio_put(bio);
		bio = next_bio;
	}
	*bio_chain = NULL;
	return ret;
}


static int mark_swapfiles(sector_t start, unsigned int flags)
{
	int error;

	bio_read_page(swsusp_resume_block, swsusp_header, NULL);
	if (!memcmp("SWAP-SPACE",swsusp_header->sig, 10) ||
	    !memcmp("SWAPSPACE2",swsusp_header->sig, 10)) {
		memcpy(swsusp_header->orig_sig,swsusp_header->sig, 10);
		memcpy(swsusp_header->sig,SWSUSP_SIG, 10);
		swsusp_header->image = start;
		swsusp_header->flags = flags;
		error = bio_write_page(swsusp_resume_block,
					swsusp_header, NULL);
	} else {
		printk(KERN_ERR "PM: Swap header not found!\n");
		error = -ENODEV;
	}
	return error;
}


static int swsusp_swap_check(void) /* This is called before saving image */
{
	int res;

	res = swap_type_of(swsusp_resume_device, swsusp_resume_block,
			&resume_bdev);
	if (res < 0)
		return res;

	root_swap = res;
	res = blkdev_get(resume_bdev, FMODE_WRITE);
	if (res)
		return res;

	res = set_blocksize(resume_bdev, PAGE_SIZE);
	if (res < 0)
		blkdev_put(resume_bdev, FMODE_WRITE);

	return res;
}


static int write_page(void *buf, sector_t offset, struct bio **bio_chain)
{
	void *src;

	if (!offset)
		return -ENOSPC;

	if (bio_chain) {
		src = (void *)__get_free_page(__GFP_WAIT | __GFP_HIGH);
		if (src) {
			memcpy(src, buf, PAGE_SIZE);
		} else {
			WARN_ON_ONCE(1);
			bio_chain = NULL;	/* Go synchronous */
			src = buf;
		}
	} else {
		src = buf;
	}
	return bio_write_page(offset, src, bio_chain);
}


#define MAP_PAGE_ENTRIES	(PAGE_SIZE / sizeof(sector_t) - 1)

struct swap_map_page {
	sector_t entries[MAP_PAGE_ENTRIES];
	sector_t next_swap;
};


struct swap_map_handle {
	struct swap_map_page *cur;
	sector_t cur_swap;
	unsigned int k;
};

static void release_swap_writer(struct swap_map_handle *handle)
{
	if (handle->cur)
		free_page((unsigned long)handle->cur);
	handle->cur = NULL;
}

static int get_swap_writer(struct swap_map_handle *handle)
{
	handle->cur = (struct swap_map_page *)get_zeroed_page(GFP_KERNEL);
	if (!handle->cur)
		return -ENOMEM;
	handle->cur_swap = alloc_swapdev_block(root_swap);
	if (!handle->cur_swap) {
		release_swap_writer(handle);
		return -ENOSPC;
	}
	handle->k = 0;
	return 0;
}

static int swap_write_page(struct swap_map_handle *handle, void *buf,
				struct bio **bio_chain)
{
	int error = 0;
	sector_t offset;

	if (!handle->cur)
		return -EINVAL;
	offset = alloc_swapdev_block(root_swap);
	error = write_page(buf, offset, bio_chain);
	if (error)
		return error;
	handle->cur->entries[handle->k++] = offset;
	if (handle->k >= MAP_PAGE_ENTRIES) {
		error = wait_on_bio_chain(bio_chain);
		if (error)
			goto out;
		offset = alloc_swapdev_block(root_swap);
		if (!offset)
			return -ENOSPC;
		handle->cur->next_swap = offset;
		error = write_page(handle->cur, handle->cur_swap, NULL);
		if (error)
			goto out;
		memset(handle->cur, 0, PAGE_SIZE);
		handle->cur_swap = offset;
		handle->k = 0;
	}
 out:
	return error;
}

static int flush_swap_writer(struct swap_map_handle *handle)
{
	if (handle->cur && handle->cur_swap)
		return write_page(handle->cur, handle->cur_swap, NULL);
	else
		return -EINVAL;
}


static int save_image(struct swap_map_handle *handle,
                      struct snapshot_handle *snapshot,
                      unsigned int nr_to_write)
{
	unsigned int m;
	int ret;
	int error = 0;
	int nr_pages;
	int err2;
	struct bio *bio;
	struct timeval start;
	struct timeval stop;

	printk(KERN_INFO "PM: Saving image data pages (%u pages) ...     ",
		nr_to_write);
	m = nr_to_write / 100;
	if (!m)
		m = 1;
	nr_pages = 0;
	bio = NULL;
	do_gettimeofday(&start);
	do {
		ret = snapshot_read_next(snapshot, PAGE_SIZE);
		if (ret > 0) {
			error = swap_write_page(handle, data_of(*snapshot),
						&bio);
			if (error)
				break;
			if (!(nr_pages % m))
				printk("\b\b\b\b%3d%%", nr_pages / m);
			nr_pages++;
		}
	} while (ret > 0);
	err2 = wait_on_bio_chain(&bio);
	do_gettimeofday(&stop);
	if (!error)
		error = err2;
	if (!error)
		printk("\b\b\b\bdone\n");
	swsusp_show_speed(&start, &stop, nr_to_write, "Wrote");
	return error;
}


static int enough_swap(unsigned int nr_pages)
{
	unsigned int free_swap = count_swap_pages(root_swap, 1);

	pr_debug("PM: Free swap pages: %u\n", free_swap);
	return free_swap > nr_pages + PAGES_FOR_IO;
}


int swsusp_write(unsigned int flags)
{
	struct swap_map_handle handle;
	struct snapshot_handle snapshot;
	struct swsusp_info *header;
	int error;

	error = swsusp_swap_check();
	if (error) {
		printk(KERN_ERR "PM: Cannot find swap device, try "
				"swapon -a.\n");
		return error;
	}
	memset(&snapshot, 0, sizeof(struct snapshot_handle));
	error = snapshot_read_next(&snapshot, PAGE_SIZE);
	if (error < PAGE_SIZE) {
		if (error >= 0)
			error = -EFAULT;

		goto out;
	}
	header = (struct swsusp_info *)data_of(snapshot);
	if (!enough_swap(header->pages)) {
		printk(KERN_ERR "PM: Not enough free swap\n");
		error = -ENOSPC;
		goto out;
	}
	error = get_swap_writer(&handle);
	if (!error) {
		sector_t start = handle.cur_swap;

		error = swap_write_page(&handle, header, NULL);
		if (!error)
			error = save_image(&handle, &snapshot,
					header->pages - 1);

		if (!error) {
			flush_swap_writer(&handle);
			printk(KERN_INFO "PM: S");
			error = mark_swapfiles(start, flags);
			printk("|\n");
		}
	}
	if (error)
		free_all_swap_pages(root_swap);

	release_swap_writer(&handle);
 out:
	swsusp_close(FMODE_WRITE);
	return error;
}


static void release_swap_reader(struct swap_map_handle *handle)
{
	if (handle->cur)
		free_page((unsigned long)handle->cur);
	handle->cur = NULL;
}

static int get_swap_reader(struct swap_map_handle *handle, sector_t start)
{
	int error;

	if (!start)
		return -EINVAL;

	handle->cur = (struct swap_map_page *)get_zeroed_page(__GFP_WAIT | __GFP_HIGH);
	if (!handle->cur)
		return -ENOMEM;

	error = bio_read_page(start, handle->cur, NULL);
	if (error) {
		release_swap_reader(handle);
		return error;
	}
	handle->k = 0;
	return 0;
}

static int swap_read_page(struct swap_map_handle *handle, void *buf,
				struct bio **bio_chain)
{
	sector_t offset;
	int error;

	if (!handle->cur)
		return -EINVAL;
	offset = handle->cur->entries[handle->k];
	if (!offset)
		return -EFAULT;
	error = bio_read_page(offset, buf, bio_chain);
	if (error)
		return error;
	if (++handle->k >= MAP_PAGE_ENTRIES) {
		error = wait_on_bio_chain(bio_chain);
		handle->k = 0;
		offset = handle->cur->next_swap;
		if (!offset)
			release_swap_reader(handle);
		else if (!error)
			error = bio_read_page(offset, handle->cur, NULL);
	}
	return error;
}


static int load_image(struct swap_map_handle *handle,
                      struct snapshot_handle *snapshot,
                      unsigned int nr_to_read)
{
	unsigned int m;
	int error = 0;
	struct timeval start;
	struct timeval stop;
	struct bio *bio;
	int err2;
	unsigned nr_pages;

	printk(KERN_INFO "PM: Loading image data pages (%u pages) ...     ",
		nr_to_read);
	m = nr_to_read / 100;
	if (!m)
		m = 1;
	nr_pages = 0;
	bio = NULL;
	do_gettimeofday(&start);
	for ( ; ; ) {
		error = snapshot_write_next(snapshot, PAGE_SIZE);
		if (error <= 0)
			break;
		error = swap_read_page(handle, data_of(*snapshot), &bio);
		if (error)
			break;
		if (snapshot->sync_read)
			error = wait_on_bio_chain(&bio);
		if (error)
			break;
		if (!(nr_pages % m))
			printk("\b\b\b\b%3d%%", nr_pages / m);
		nr_pages++;
	}
	err2 = wait_on_bio_chain(&bio);
	do_gettimeofday(&stop);
	if (!error)
		error = err2;
	if (!error) {
		printk("\b\b\b\bdone\n");
		snapshot_write_finalize(snapshot);
		if (!snapshot_image_loaded(snapshot))
			error = -ENODATA;
	}
	swsusp_show_speed(&start, &stop, nr_to_read, "Read");
	return error;
}


int swsusp_read(unsigned int *flags_p)
{
	int error;
	struct swap_map_handle handle;
	struct snapshot_handle snapshot;
	struct swsusp_info *header;

	*flags_p = swsusp_header->flags;
	if (IS_ERR(resume_bdev)) {
		pr_debug("PM: Image device not initialised\n");
		return PTR_ERR(resume_bdev);
	}

	memset(&snapshot, 0, sizeof(struct snapshot_handle));
	error = snapshot_write_next(&snapshot, PAGE_SIZE);
	if (error < PAGE_SIZE)
		return error < 0 ? error : -EFAULT;
	header = (struct swsusp_info *)data_of(snapshot);
	error = get_swap_reader(&handle, swsusp_header->image);
	if (!error)
		error = swap_read_page(&handle, header, NULL);
	if (!error)
		error = load_image(&handle, &snapshot, header->pages - 1);
	release_swap_reader(&handle);

	blkdev_put(resume_bdev, FMODE_READ);

	if (!error)
		pr_debug("PM: Image successfully loaded\n");
	else
		pr_debug("PM: Error %d resuming\n", error);
	return error;
}


int swsusp_check(void)
{
	int error;

	resume_bdev = open_by_devnum(swsusp_resume_device, FMODE_READ);
	if (!IS_ERR(resume_bdev)) {
		set_blocksize(resume_bdev, PAGE_SIZE);
		memset(swsusp_header, 0, PAGE_SIZE);
		error = bio_read_page(swsusp_resume_block,
					swsusp_header, NULL);
		if (error)
			return error;

		if (!memcmp(SWSUSP_SIG, swsusp_header->sig, 10)) {
			memcpy(swsusp_header->sig, swsusp_header->orig_sig, 10);
			/* Reset swap signature now */
			error = bio_write_page(swsusp_resume_block,
						swsusp_header, NULL);
		} else {
			return -EINVAL;
		}
		if (error)
			blkdev_put(resume_bdev, FMODE_READ);
		else
			pr_debug("PM: Signature found, resuming\n");
	} else {
		error = PTR_ERR(resume_bdev);
	}

	if (error)
		pr_debug("PM: Error %d checking image file\n", error);

	return error;
}


void swsusp_close(fmode_t mode)
{
	if (IS_ERR(resume_bdev)) {
		pr_debug("PM: Image device not initialised\n");
		return;
	}

	blkdev_put(resume_bdev, mode);
}

static int swsusp_header_init(void)
{
	swsusp_header = (struct swsusp_header*) __get_free_page(GFP_KERNEL);
	if (!swsusp_header)
		panic("Could not allocate memory for swsusp_header\n");
	return 0;
}

core_initcall(swsusp_header_init);
