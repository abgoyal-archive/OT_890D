

#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/completion.h>
#include <linux/buffer_head.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/writeback.h>
#include <linux/swap.h>
#include <linux/delay.h>
#include <linux/bio.h>
#include <linux/gfs2_ondisk.h>
#include <linux/lm_interface.h>

#include "gfs2.h"
#include "incore.h"
#include "glock.h"
#include "glops.h"
#include "inode.h"
#include "log.h"
#include "lops.h"
#include "meta_io.h"
#include "rgrp.h"
#include "trans.h"
#include "util.h"
#include "ops_address.h"

static int aspace_get_block(struct inode *inode, sector_t lblock,
			    struct buffer_head *bh_result, int create)
{
	gfs2_assert_warn(inode->i_sb->s_fs_info, 0);
	return -EOPNOTSUPP;
}

static int gfs2_aspace_writepage(struct page *page,
				 struct writeback_control *wbc)
{
	return block_write_full_page(page, aspace_get_block, wbc);
}

static const struct address_space_operations aspace_aops = {
	.writepage = gfs2_aspace_writepage,
	.releasepage = gfs2_releasepage,
	.sync_page = block_sync_page,
};


struct inode *gfs2_aspace_get(struct gfs2_sbd *sdp)
{
	struct inode *aspace;
	struct gfs2_inode *ip;

	aspace = new_inode(sdp->sd_vfs);
	if (aspace) {
		mapping_set_gfp_mask(aspace->i_mapping, GFP_NOFS);
		aspace->i_mapping->a_ops = &aspace_aops;
		aspace->i_size = ~0ULL;
		ip = GFS2_I(aspace);
		clear_bit(GIF_USER, &ip->i_flags);
		insert_inode_hash(aspace);
	}
	return aspace;
}

void gfs2_aspace_put(struct inode *aspace)
{
	remove_inode_hash(aspace);
	iput(aspace);
}


void gfs2_meta_inval(struct gfs2_glock *gl)
{
	struct gfs2_sbd *sdp = gl->gl_sbd;
	struct inode *aspace = gl->gl_aspace;
	struct address_space *mapping = gl->gl_aspace->i_mapping;

	gfs2_assert_withdraw(sdp, !atomic_read(&gl->gl_ail_count));

	atomic_inc(&aspace->i_writecount);
	truncate_inode_pages(mapping, 0);
	atomic_dec(&aspace->i_writecount);

	gfs2_assert_withdraw(sdp, !mapping->nrpages);
}


void gfs2_meta_sync(struct gfs2_glock *gl)
{
	struct address_space *mapping = gl->gl_aspace->i_mapping;
	int error;

	filemap_fdatawrite(mapping);
	error = filemap_fdatawait(mapping);

	if (error)
		gfs2_io_error(gl->gl_sbd);
}


struct buffer_head *gfs2_getbuf(struct gfs2_glock *gl, u64 blkno, int create)
{
	struct address_space *mapping = gl->gl_aspace->i_mapping;
	struct gfs2_sbd *sdp = gl->gl_sbd;
	struct page *page;
	struct buffer_head *bh;
	unsigned int shift;
	unsigned long index;
	unsigned int bufnum;

	shift = PAGE_CACHE_SHIFT - sdp->sd_sb.sb_bsize_shift;
	index = blkno >> shift;             /* convert block to page */
	bufnum = blkno - (index << shift);  /* block buf index within page */

	if (create) {
		for (;;) {
			page = grab_cache_page(mapping, index);
			if (page)
				break;
			yield();
		}
	} else {
		page = find_lock_page(mapping, index);
		if (!page)
			return NULL;
	}

	if (!page_has_buffers(page))
		create_empty_buffers(page, sdp->sd_sb.sb_bsize, 0);

	/* Locate header for our buffer within our page */
	for (bh = page_buffers(page); bufnum--; bh = bh->b_this_page)
		/* Do nothing */;
	get_bh(bh);

	if (!buffer_mapped(bh))
		map_bh(bh, sdp->sd_vfs, blkno);

	unlock_page(page);
	mark_page_accessed(page);
	page_cache_release(page);

	return bh;
}

static void meta_prep_new(struct buffer_head *bh)
{
	struct gfs2_meta_header *mh = (struct gfs2_meta_header *)bh->b_data;

	lock_buffer(bh);
	clear_buffer_dirty(bh);
	set_buffer_uptodate(bh);
	unlock_buffer(bh);

	mh->mh_magic = cpu_to_be32(GFS2_MAGIC);
}


struct buffer_head *gfs2_meta_new(struct gfs2_glock *gl, u64 blkno)
{
	struct buffer_head *bh;
	bh = gfs2_getbuf(gl, blkno, CREATE);
	meta_prep_new(bh);
	return bh;
}


int gfs2_meta_read(struct gfs2_glock *gl, u64 blkno, int flags,
		   struct buffer_head **bhp)
{
	*bhp = gfs2_getbuf(gl, blkno, CREATE);
	if (!buffer_uptodate(*bhp)) {
		ll_rw_block(READ_META, 1, bhp);
		if (flags & DIO_WAIT) {
			int error = gfs2_meta_wait(gl->gl_sbd, *bhp);
			if (error) {
				brelse(*bhp);
				return error;
			}
		}
	}

	return 0;
}


int gfs2_meta_wait(struct gfs2_sbd *sdp, struct buffer_head *bh)
{
	if (unlikely(test_bit(SDF_SHUTDOWN, &sdp->sd_flags)))
		return -EIO;

	wait_on_buffer(bh);

	if (!buffer_uptodate(bh)) {
		struct gfs2_trans *tr = current->journal_info;
		if (tr && tr->tr_touched)
			gfs2_io_error_bh(sdp, bh);
		return -EIO;
	}
	if (unlikely(test_bit(SDF_SHUTDOWN, &sdp->sd_flags)))
		return -EIO;

	return 0;
}


void gfs2_attach_bufdata(struct gfs2_glock *gl, struct buffer_head *bh,
			 int meta)
{
	struct gfs2_bufdata *bd;

	if (meta)
		lock_page(bh->b_page);

	if (bh->b_private) {
		if (meta)
			unlock_page(bh->b_page);
		return;
	}

	bd = kmem_cache_zalloc(gfs2_bufdata_cachep, GFP_NOFS | __GFP_NOFAIL);
	bd->bd_bh = bh;
	bd->bd_gl = gl;

	INIT_LIST_HEAD(&bd->bd_list_tr);
	if (meta)
		lops_init_le(&bd->bd_le, &gfs2_buf_lops);
	else
		lops_init_le(&bd->bd_le, &gfs2_databuf_lops);
	bh->b_private = bd;

	if (meta)
		unlock_page(bh->b_page);
}

void gfs2_remove_from_journal(struct buffer_head *bh, struct gfs2_trans *tr, int meta)
{
	struct gfs2_sbd *sdp = GFS2_SB(bh->b_page->mapping->host);
	struct gfs2_bufdata *bd = bh->b_private;
	if (test_clear_buffer_pinned(bh)) {
		list_del_init(&bd->bd_le.le_list);
		if (meta) {
			gfs2_assert_warn(sdp, sdp->sd_log_num_buf);
			sdp->sd_log_num_buf--;
			tr->tr_num_buf_rm++;
		} else {
			gfs2_assert_warn(sdp, sdp->sd_log_num_databuf);
			sdp->sd_log_num_databuf--;
			tr->tr_num_databuf_rm++;
		}
		tr->tr_touched = 1;
		brelse(bh);
	}
	if (bd) {
		if (bd->bd_ail) {
			gfs2_remove_from_ail(bd);
			bh->b_private = NULL;
			bd->bd_bh = NULL;
			bd->bd_blkno = bh->b_blocknr;
			gfs2_trans_add_revoke(sdp, bd);
		}
	}
	clear_buffer_dirty(bh);
	clear_buffer_uptodate(bh);
}


void gfs2_meta_wipe(struct gfs2_inode *ip, u64 bstart, u32 blen)
{
	struct gfs2_sbd *sdp = GFS2_SB(&ip->i_inode);
	struct buffer_head *bh;

	while (blen) {
		bh = gfs2_getbuf(ip->i_gl, bstart, NO_CREATE);
		if (bh) {
			lock_buffer(bh);
			gfs2_log_lock(sdp);
			gfs2_remove_from_journal(bh, current->journal_info, 1);
			gfs2_log_unlock(sdp);
			unlock_buffer(bh);
			brelse(bh);
		}

		bstart++;
		blen--;
	}
}


int gfs2_meta_indirect_buffer(struct gfs2_inode *ip, int height, u64 num,
			      int new, struct buffer_head **bhp)
{
	struct gfs2_sbd *sdp = GFS2_SB(&ip->i_inode);
	struct gfs2_glock *gl = ip->i_gl;
	struct buffer_head *bh;
	int ret = 0;

	if (new) {
		BUG_ON(height == 0);
		bh = gfs2_meta_new(gl, num);
		gfs2_trans_add_bh(ip->i_gl, bh, 1);
		gfs2_metatype_set(bh, GFS2_METATYPE_IN, GFS2_FORMAT_IN);
		gfs2_buffer_clear_tail(bh, sizeof(struct gfs2_meta_header));
	} else {
		u32 mtype = height ? GFS2_METATYPE_IN : GFS2_METATYPE_DI;
		ret = gfs2_meta_read(gl, num, DIO_WAIT, &bh);
		if (ret == 0 && gfs2_metatype_check(sdp, bh, mtype)) {
			brelse(bh);
			ret = -EIO;
		}
	}
	*bhp = bh;
	return ret;
}


struct buffer_head *gfs2_meta_ra(struct gfs2_glock *gl, u64 dblock, u32 extlen)
{
	struct gfs2_sbd *sdp = gl->gl_sbd;
	struct buffer_head *first_bh, *bh;
	u32 max_ra = gfs2_tune_get(sdp, gt_max_readahead) >>
			  sdp->sd_sb.sb_bsize_shift;

	BUG_ON(!extlen);

	if (max_ra < 1)
		max_ra = 1;
	if (extlen > max_ra)
		extlen = max_ra;

	first_bh = gfs2_getbuf(gl, dblock, CREATE);

	if (buffer_uptodate(first_bh))
		goto out;
	if (!buffer_locked(first_bh))
		ll_rw_block(READ_META, 1, &first_bh);

	dblock++;
	extlen--;

	while (extlen) {
		bh = gfs2_getbuf(gl, dblock, CREATE);

		if (!buffer_uptodate(bh) && !buffer_locked(bh))
			ll_rw_block(READA, 1, &bh);
		brelse(bh);
		dblock++;
		extlen--;
		if (!buffer_locked(first_bh) && buffer_uptodate(first_bh))
			goto out;
	}

	wait_on_buffer(first_bh);
out:
	return first_bh;
}

