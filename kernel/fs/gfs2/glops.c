

#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/completion.h>
#include <linux/buffer_head.h>
#include <linux/gfs2_ondisk.h>
#include <linux/lm_interface.h>
#include <linux/bio.h>

#include "gfs2.h"
#include "incore.h"
#include "bmap.h"
#include "glock.h"
#include "glops.h"
#include "inode.h"
#include "log.h"
#include "meta_io.h"
#include "recovery.h"
#include "rgrp.h"
#include "util.h"
#include "trans.h"


static void gfs2_ail_empty_gl(struct gfs2_glock *gl)
{
	struct gfs2_sbd *sdp = gl->gl_sbd;
	unsigned int blocks;
	struct list_head *head = &gl->gl_ail_list;
	struct gfs2_bufdata *bd;
	struct buffer_head *bh;
	int error;

	blocks = atomic_read(&gl->gl_ail_count);
	if (!blocks)
		return;

	error = gfs2_trans_begin(sdp, 0, blocks);
	if (gfs2_assert_withdraw(sdp, !error))
		return;

	gfs2_log_lock(sdp);
	while (!list_empty(head)) {
		bd = list_entry(head->next, struct gfs2_bufdata,
				bd_ail_gl_list);
		bh = bd->bd_bh;
		gfs2_remove_from_ail(bd);
		bd->bd_bh = NULL;
		bh->b_private = NULL;
		bd->bd_blkno = bh->b_blocknr;
		gfs2_assert_withdraw(sdp, !buffer_busy(bh));
		gfs2_trans_add_revoke(sdp, bd);
	}
	gfs2_assert_withdraw(sdp, !atomic_read(&gl->gl_ail_count));
	gfs2_log_unlock(sdp);

	gfs2_trans_end(sdp);
	gfs2_log_flush(sdp, NULL);
}


static void gfs2_pte_inval(struct gfs2_glock *gl)
{
	struct gfs2_inode *ip;
	struct inode *inode;

	ip = gl->gl_object;
	inode = &ip->i_inode;
	if (!ip || !S_ISREG(inode->i_mode))
		return;

	unmap_shared_mapping_range(inode->i_mapping, 0, 0);
	if (test_bit(GIF_SW_PAGED, &ip->i_flags))
		set_bit(GLF_DIRTY, &gl->gl_flags);

}


static void meta_go_sync(struct gfs2_glock *gl)
{
	if (gl->gl_state != LM_ST_EXCLUSIVE)
		return;

	if (test_and_clear_bit(GLF_DIRTY, &gl->gl_flags)) {
		gfs2_log_flush(gl->gl_sbd, gl);
		gfs2_meta_sync(gl);
		gfs2_ail_empty_gl(gl);
	}
}


static void meta_go_inval(struct gfs2_glock *gl, int flags)
{
	if (!(flags & DIO_METADATA))
		return;

	gfs2_meta_inval(gl);
	if (gl->gl_object == GFS2_I(gl->gl_sbd->sd_rindex))
		gl->gl_sbd->sd_rindex_uptodate = 0;
	else if (gl->gl_ops == &gfs2_rgrp_glops && gl->gl_object) {
		struct gfs2_rgrpd *rgd = (struct gfs2_rgrpd *)gl->gl_object;

		rgd->rd_flags &= ~GFS2_RDF_UPTODATE;
	}
}


static void inode_go_sync(struct gfs2_glock *gl)
{
	struct gfs2_inode *ip = gl->gl_object;
	struct address_space *metamapping = gl->gl_aspace->i_mapping;
	int error;

	if (gl->gl_state != LM_ST_UNLOCKED)
		gfs2_pte_inval(gl);
	if (gl->gl_state != LM_ST_EXCLUSIVE)
		return;

	if (ip && !S_ISREG(ip->i_inode.i_mode))
		ip = NULL;

	if (test_bit(GLF_DIRTY, &gl->gl_flags)) {
		gfs2_log_flush(gl->gl_sbd, gl);
		filemap_fdatawrite(metamapping);
		if (ip) {
			struct address_space *mapping = ip->i_inode.i_mapping;
			filemap_fdatawrite(mapping);
			error = filemap_fdatawait(mapping);
			mapping_set_error(mapping, error);
		}
		error = filemap_fdatawait(metamapping);
		mapping_set_error(metamapping, error);
		clear_bit(GLF_DIRTY, &gl->gl_flags);
		gfs2_ail_empty_gl(gl);
	}
}


static void inode_go_inval(struct gfs2_glock *gl, int flags)
{
	struct gfs2_inode *ip = gl->gl_object;
	int meta = (flags & DIO_METADATA);

	if (meta) {
		gfs2_meta_inval(gl);
		if (ip)
			set_bit(GIF_INVALID, &ip->i_flags);
	}

	if (ip && S_ISREG(ip->i_inode.i_mode))
		truncate_inode_pages(ip->i_inode.i_mapping, 0);
}


static int inode_go_demote_ok(const struct gfs2_glock *gl)
{
	struct gfs2_sbd *sdp = gl->gl_sbd;
	if (sdp->sd_jindex == gl->gl_object || sdp->sd_rindex == gl->gl_object)
		return 0;
	return 1;
}


static int inode_go_lock(struct gfs2_holder *gh)
{
	struct gfs2_glock *gl = gh->gh_gl;
	struct gfs2_sbd *sdp = gl->gl_sbd;
	struct gfs2_inode *ip = gl->gl_object;
	int error = 0;

	if (!ip || (gh->gh_flags & GL_SKIP))
		return 0;

	if (test_bit(GIF_INVALID, &ip->i_flags)) {
		error = gfs2_inode_refresh(ip);
		if (error)
			return error;
	}

	if ((ip->i_diskflags & GFS2_DIF_TRUNC_IN_PROG) &&
	    (gl->gl_state == LM_ST_EXCLUSIVE) &&
	    (gh->gh_state == LM_ST_EXCLUSIVE)) {
		spin_lock(&sdp->sd_trunc_lock);
		if (list_empty(&ip->i_trunc_list))
			list_add(&sdp->sd_trunc_list, &ip->i_trunc_list);
		spin_unlock(&sdp->sd_trunc_lock);
		wake_up(&sdp->sd_quota_wait);
		return 1;
	}

	return error;
}


static int inode_go_dump(struct seq_file *seq, const struct gfs2_glock *gl)
{
	const struct gfs2_inode *ip = gl->gl_object;
	if (ip == NULL)
		return 0;
	gfs2_print_dbg(seq, " I: n:%llu/%llu t:%u f:0x%02lx d:0x%08x s:%llu/%llu\n",
		  (unsigned long long)ip->i_no_formal_ino,
		  (unsigned long long)ip->i_no_addr,
		  IF2DT(ip->i_inode.i_mode), ip->i_flags,
		  (unsigned int)ip->i_diskflags,
		  (unsigned long long)ip->i_inode.i_size,
		  (unsigned long long)ip->i_disksize);
	return 0;
}


static int rgrp_go_demote_ok(const struct gfs2_glock *gl)
{
	return !gl->gl_aspace->i_mapping->nrpages;
}


static int rgrp_go_lock(struct gfs2_holder *gh)
{
	return gfs2_rgrp_bh_get(gh->gh_gl->gl_object);
}


static void rgrp_go_unlock(struct gfs2_holder *gh)
{
	gfs2_rgrp_bh_put(gh->gh_gl->gl_object);
}


static int rgrp_go_dump(struct seq_file *seq, const struct gfs2_glock *gl)
{
	const struct gfs2_rgrpd *rgd = gl->gl_object;
	if (rgd == NULL)
		return 0;
	gfs2_print_dbg(seq, " R: n:%llu f:%02x b:%u/%u i:%u\n",
		       (unsigned long long)rgd->rd_addr, rgd->rd_flags,
		       rgd->rd_free, rgd->rd_free_clone, rgd->rd_dinodes);
	return 0;
}


static void trans_go_sync(struct gfs2_glock *gl)
{
	struct gfs2_sbd *sdp = gl->gl_sbd;

	if (gl->gl_state != LM_ST_UNLOCKED &&
	    test_bit(SDF_JOURNAL_LIVE, &sdp->sd_flags)) {
		gfs2_meta_syncfs(sdp);
		gfs2_log_shutdown(sdp);
	}
}


static int trans_go_xmote_bh(struct gfs2_glock *gl, struct gfs2_holder *gh)
{
	struct gfs2_sbd *sdp = gl->gl_sbd;
	struct gfs2_inode *ip = GFS2_I(sdp->sd_jdesc->jd_inode);
	struct gfs2_glock *j_gl = ip->i_gl;
	struct gfs2_log_header_host head;
	int error;

	if (test_bit(SDF_JOURNAL_LIVE, &sdp->sd_flags)) {
		j_gl->gl_ops->go_inval(j_gl, DIO_METADATA);

		error = gfs2_find_jhead(sdp->sd_jdesc, &head);
		if (error)
			gfs2_consist(sdp);
		if (!(head.lh_flags & GFS2_LOG_HEAD_UNMOUNT))
			gfs2_consist(sdp);

		/*  Initialize some head of the log stuff  */
		if (!test_bit(SDF_SHUTDOWN, &sdp->sd_flags)) {
			sdp->sd_log_sequence = head.lh_sequence + 1;
			gfs2_log_pointers_init(sdp, head.lh_blkno);
		}
	}
	return 0;
}


static int trans_go_demote_ok(const struct gfs2_glock *gl)
{
	return 0;
}


static int quota_go_demote_ok(const struct gfs2_glock *gl)
{
	return !atomic_read(&gl->gl_lvb_count);
}

const struct gfs2_glock_operations gfs2_meta_glops = {
	.go_xmote_th = meta_go_sync,
	.go_type = LM_TYPE_META,
};

const struct gfs2_glock_operations gfs2_inode_glops = {
	.go_xmote_th = inode_go_sync,
	.go_inval = inode_go_inval,
	.go_demote_ok = inode_go_demote_ok,
	.go_lock = inode_go_lock,
	.go_dump = inode_go_dump,
	.go_type = LM_TYPE_INODE,
	.go_min_hold_time = HZ / 5,
};

const struct gfs2_glock_operations gfs2_rgrp_glops = {
	.go_xmote_th = meta_go_sync,
	.go_inval = meta_go_inval,
	.go_demote_ok = rgrp_go_demote_ok,
	.go_lock = rgrp_go_lock,
	.go_unlock = rgrp_go_unlock,
	.go_dump = rgrp_go_dump,
	.go_type = LM_TYPE_RGRP,
	.go_min_hold_time = HZ / 5,
};

const struct gfs2_glock_operations gfs2_trans_glops = {
	.go_xmote_th = trans_go_sync,
	.go_xmote_bh = trans_go_xmote_bh,
	.go_demote_ok = trans_go_demote_ok,
	.go_type = LM_TYPE_NONDISK,
};

const struct gfs2_glock_operations gfs2_iopen_glops = {
	.go_type = LM_TYPE_IOPEN,
};

const struct gfs2_glock_operations gfs2_flock_glops = {
	.go_type = LM_TYPE_FLOCK,
};

const struct gfs2_glock_operations gfs2_nondisk_glops = {
	.go_type = LM_TYPE_NONDISK,
};

const struct gfs2_glock_operations gfs2_quota_glops = {
	.go_demote_ok = quota_go_demote_ok,
	.go_type = LM_TYPE_QUOTA,
};

const struct gfs2_glock_operations gfs2_journal_glops = {
	.go_type = LM_TYPE_JOURNAL,
};

