

#ifndef _MD_H
#define _MD_H

#include <linux/blkdev.h>
#include <linux/seq_file.h>


#include <linux/raid/md_p.h>
#include <linux/raid/md_u.h>
#include <linux/raid/md_k.h>

#ifdef CONFIG_MD

#define MD_MAJOR_VERSION                0
#define MD_MINOR_VERSION                90
#define MD_PATCHLEVEL_VERSION           3

extern int mdp_major;

extern int register_md_personality(struct mdk_personality *p);
extern int unregister_md_personality(struct mdk_personality *p);
extern mdk_thread_t * md_register_thread(void (*run) (mddev_t *mddev),
				mddev_t *mddev, const char *name);
extern void md_unregister_thread(mdk_thread_t *thread);
extern void md_wakeup_thread(mdk_thread_t *thread);
extern void md_check_recovery(mddev_t *mddev);
extern void md_write_start(mddev_t *mddev, struct bio *bi);
extern void md_write_end(mddev_t *mddev);
extern void md_done_sync(mddev_t *mddev, int blocks, int ok);
extern void md_error(mddev_t *mddev, mdk_rdev_t *rdev);

extern void md_super_write(mddev_t *mddev, mdk_rdev_t *rdev,
			   sector_t sector, int size, struct page *page);
extern void md_super_wait(mddev_t *mddev);
extern int sync_page_io(struct block_device *bdev, sector_t sector, int size,
			struct page *page, int rw);
extern void md_do_sync(mddev_t *mddev);
extern void md_new_event(mddev_t *mddev);
extern int md_allow_write(mddev_t *mddev);
extern void md_wait_for_blocked_rdev(mdk_rdev_t *rdev, mddev_t *mddev);

#endif /* CONFIG_MD */
#endif 

