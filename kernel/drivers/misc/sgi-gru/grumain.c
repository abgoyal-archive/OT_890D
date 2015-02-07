

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/device.h>
#include <linux/list.h>
#include <asm/uv/uv_hub.h>
#include "gru.h"
#include "grutables.h"
#include "gruhandles.h"

unsigned long gru_options __read_mostly;

static struct device_driver gru_driver = {
	.name = "gru"
};

static struct device gru_device = {
	.init_name = "",
	.driver = &gru_driver,
};

struct device *grudev = &gru_device;

int gru_cpu_fault_map_id(void)
{
	return uv_blade_processor_id() % GRU_NUM_TFM;
}


/* Hit the asid limit. Start over */
static int gru_wrap_asid(struct gru_state *gru)
{
	gru_dbg(grudev, "gru %p\n", gru);
	STAT(asid_wrap);
	gru->gs_asid_gen++;
	gru_flush_all_tlb(gru);
	return MIN_ASID;
}

/* Find the next chunk of unused asids */
static int gru_reset_asid_limit(struct gru_state *gru, int asid)
{
	int i, gid, inuse_asid, limit;

	gru_dbg(grudev, "gru %p, asid 0x%x\n", gru, asid);
	STAT(asid_next);
	limit = MAX_ASID;
	if (asid >= limit)
		asid = gru_wrap_asid(gru);
	gid = gru->gs_gid;
again:
	for (i = 0; i < GRU_NUM_CCH; i++) {
		if (!gru->gs_gts[i])
			continue;
		inuse_asid = gru->gs_gts[i]->ts_gms->ms_asids[gid].mt_asid;
		gru_dbg(grudev, "gru %p, inuse_asid 0x%x, cxtnum %d, gts %p\n",
			gru, inuse_asid, i, gru->gs_gts[i]);
		if (inuse_asid == asid) {
			asid += ASID_INC;
			if (asid >= limit) {
				/*
				 * empty range: reset the range limit and
				 * start over
				 */
				limit = MAX_ASID;
				if (asid >= MAX_ASID)
					asid = gru_wrap_asid(gru);
				goto again;
			}
		}

		if ((inuse_asid > asid) && (inuse_asid < limit))
			limit = inuse_asid;
	}
	gru->gs_asid_limit = limit;
	gru->gs_asid = asid;
	gru_dbg(grudev, "gru %p, new asid 0x%x, new_limit 0x%x\n", gru, asid,
		limit);
	return asid;
}

/* Assign a new ASID to a thread context.  */
static int gru_assign_asid(struct gru_state *gru)
{
	int asid;

	spin_lock(&gru->gs_asid_lock);
	gru->gs_asid += ASID_INC;
	asid = gru->gs_asid;
	if (asid >= gru->gs_asid_limit)
		asid = gru_reset_asid_limit(gru, asid);
	spin_unlock(&gru->gs_asid_lock);

	gru_dbg(grudev, "gru %p, asid 0x%x\n", gru, asid);
	return asid;
}

static unsigned long reserve_resources(unsigned long *p, int n, int mmax,
				       char *idx)
{
	unsigned long bits = 0;
	int i;

	do {
		i = find_first_bit(p, mmax);
		if (i == mmax)
			BUG();
		__clear_bit(i, p);
		__set_bit(i, &bits);
		if (idx)
			*idx++ = i;
	} while (--n);
	return bits;
}

unsigned long gru_reserve_cb_resources(struct gru_state *gru, int cbr_au_count,
				       char *cbmap)
{
	return reserve_resources(&gru->gs_cbr_map, cbr_au_count, GRU_CBR_AU,
				 cbmap);
}

unsigned long gru_reserve_ds_resources(struct gru_state *gru, int dsr_au_count,
				       char *dsmap)
{
	return reserve_resources(&gru->gs_dsr_map, dsr_au_count, GRU_DSR_AU,
				 dsmap);
}

static void reserve_gru_resources(struct gru_state *gru,
				  struct gru_thread_state *gts)
{
	gru->gs_active_contexts++;
	gts->ts_cbr_map =
	    gru_reserve_cb_resources(gru, gts->ts_cbr_au_count,
				     gts->ts_cbr_idx);
	gts->ts_dsr_map =
	    gru_reserve_ds_resources(gru, gts->ts_dsr_au_count, NULL);
}

static void free_gru_resources(struct gru_state *gru,
			       struct gru_thread_state *gts)
{
	gru->gs_active_contexts--;
	gru->gs_cbr_map |= gts->ts_cbr_map;
	gru->gs_dsr_map |= gts->ts_dsr_map;
}

static int check_gru_resources(struct gru_state *gru, int cbr_au_count,
			       int dsr_au_count, int max_active_contexts)
{
	return hweight64(gru->gs_cbr_map) >= cbr_au_count
		&& hweight64(gru->gs_dsr_map) >= dsr_au_count
		&& gru->gs_active_contexts < max_active_contexts;
}

static int gru_load_mm_tracker(struct gru_state *gru, struct gru_mm_struct *gms,
			       int ctxnum)
{
	struct gru_mm_tracker *asids = &gms->ms_asids[gru->gs_gid];
	unsigned short ctxbitmap = (1 << ctxnum);
	int asid;

	spin_lock(&gms->ms_asid_lock);
	asid = asids->mt_asid;

	if (asid == 0 || asids->mt_asid_gen != gru->gs_asid_gen) {
		asid = gru_assign_asid(gru);
		asids->mt_asid = asid;
		asids->mt_asid_gen = gru->gs_asid_gen;
		STAT(asid_new);
	} else {
		STAT(asid_reuse);
	}

	BUG_ON(asids->mt_ctxbitmap & ctxbitmap);
	asids->mt_ctxbitmap |= ctxbitmap;
	if (!test_bit(gru->gs_gid, gms->ms_asidmap))
		__set_bit(gru->gs_gid, gms->ms_asidmap);
	spin_unlock(&gms->ms_asid_lock);

	gru_dbg(grudev,
		"gru %x, gms %p, ctxnum 0x%d, asid 0x%x, asidmap 0x%lx\n",
		gru->gs_gid, gms, ctxnum, asid, gms->ms_asidmap[0]);
	return asid;
}

static void gru_unload_mm_tracker(struct gru_state *gru,
				  struct gru_mm_struct *gms, int ctxnum)
{
	struct gru_mm_tracker *asids;
	unsigned short ctxbitmap;

	asids = &gms->ms_asids[gru->gs_gid];
	ctxbitmap = (1 << ctxnum);
	spin_lock(&gms->ms_asid_lock);
	BUG_ON((asids->mt_ctxbitmap & ctxbitmap) != ctxbitmap);
	asids->mt_ctxbitmap ^= ctxbitmap;
	gru_dbg(grudev, "gru %x, gms %p, ctxnum 0x%d, asidmap 0x%lx\n",
		gru->gs_gid, gms, ctxnum, gms->ms_asidmap[0]);
	spin_unlock(&gms->ms_asid_lock);
}

void gts_drop(struct gru_thread_state *gts)
{
	if (gts && atomic_dec_return(&gts->ts_refcnt) == 0) {
		gru_drop_mmu_notifier(gts->ts_gms);
		kfree(gts);
		STAT(gts_free);
	}
}

static struct gru_thread_state *gru_find_current_gts_nolock(struct gru_vma_data
			    *vdata, int tsid)
{
	struct gru_thread_state *gts;

	list_for_each_entry(gts, &vdata->vd_head, ts_next)
	    if (gts->ts_tsid == tsid)
		return gts;
	return NULL;
}

static struct gru_thread_state *gru_alloc_gts(struct vm_area_struct *vma,
					      struct gru_vma_data *vdata,
					      int tsid)
{
	struct gru_thread_state *gts;
	int bytes;

	bytes = DSR_BYTES(vdata->vd_dsr_au_count) +
				CBR_BYTES(vdata->vd_cbr_au_count);
	bytes += sizeof(struct gru_thread_state);
	gts = kzalloc(bytes, GFP_KERNEL);
	if (!gts)
		return NULL;

	STAT(gts_alloc);
	atomic_set(&gts->ts_refcnt, 1);
	mutex_init(&gts->ts_ctxlock);
	gts->ts_cbr_au_count = vdata->vd_cbr_au_count;
	gts->ts_dsr_au_count = vdata->vd_dsr_au_count;
	gts->ts_user_options = vdata->vd_user_options;
	gts->ts_tsid = tsid;
	gts->ts_user_options = vdata->vd_user_options;
	gts->ts_ctxnum = NULLCTX;
	gts->ts_mm = current->mm;
	gts->ts_vma = vma;
	gts->ts_tlb_int_select = -1;
	gts->ts_gms = gru_register_mmu_notifier();
	if (!gts->ts_gms)
		goto err;

	gru_dbg(grudev, "alloc vdata %p, new gts %p\n", vdata, gts);
	return gts;

err:
	gts_drop(gts);
	return NULL;
}

struct gru_vma_data *gru_alloc_vma_data(struct vm_area_struct *vma, int tsid)
{
	struct gru_vma_data *vdata = NULL;

	vdata = kmalloc(sizeof(*vdata), GFP_KERNEL);
	if (!vdata)
		return NULL;

	INIT_LIST_HEAD(&vdata->vd_head);
	spin_lock_init(&vdata->vd_lock);
	gru_dbg(grudev, "alloc vdata %p\n", vdata);
	return vdata;
}

struct gru_thread_state *gru_find_thread_state(struct vm_area_struct *vma,
					int tsid)
{
	struct gru_vma_data *vdata = vma->vm_private_data;
	struct gru_thread_state *gts;

	spin_lock(&vdata->vd_lock);
	gts = gru_find_current_gts_nolock(vdata, tsid);
	spin_unlock(&vdata->vd_lock);
	gru_dbg(grudev, "vma %p, gts %p\n", vma, gts);
	return gts;
}

struct gru_thread_state *gru_alloc_thread_state(struct vm_area_struct *vma,
					int tsid)
{
	struct gru_vma_data *vdata = vma->vm_private_data;
	struct gru_thread_state *gts, *ngts;

	gts = gru_alloc_gts(vma, vdata, tsid);
	if (!gts)
		return NULL;

	spin_lock(&vdata->vd_lock);
	ngts = gru_find_current_gts_nolock(vdata, tsid);
	if (ngts) {
		gts_drop(gts);
		gts = ngts;
		STAT(gts_double_allocate);
	} else {
		list_add(&gts->ts_next, &vdata->vd_head);
	}
	spin_unlock(&vdata->vd_lock);
	gru_dbg(grudev, "vma %p, gts %p\n", vma, gts);
	return gts;
}

static void gru_free_gru_context(struct gru_thread_state *gts)
{
	struct gru_state *gru;

	gru = gts->ts_gru;
	gru_dbg(grudev, "gts %p, gru %p\n", gts, gru);

	spin_lock(&gru->gs_lock);
	gru->gs_gts[gts->ts_ctxnum] = NULL;
	free_gru_resources(gru, gts);
	BUG_ON(test_bit(gts->ts_ctxnum, &gru->gs_context_map) == 0);
	__clear_bit(gts->ts_ctxnum, &gru->gs_context_map);
	gts->ts_ctxnum = NULLCTX;
	gts->ts_gru = NULL;
	spin_unlock(&gru->gs_lock);

	gts_drop(gts);
	STAT(free_context);
}

static void prefetch_data(void *p, int num, int stride)
{
	while (num-- > 0) {
		prefetchw(p);
		p += stride;
	}
}

static inline long gru_copy_handle(void *d, void *s)
{
	memcpy(d, s, GRU_HANDLE_BYTES);
	return GRU_HANDLE_BYTES;
}

static void gru_prefetch_context(void *gseg, void *cb, void *cbe, unsigned long cbrmap,
				unsigned long length)
{
	int i, scr;

	prefetch_data(gseg + GRU_DS_BASE, length / GRU_CACHE_LINE_BYTES,
		      GRU_CACHE_LINE_BYTES);

	for_each_cbr_in_allocation_map(i, &cbrmap, scr) {
		prefetch_data(cb, 1, GRU_CACHE_LINE_BYTES);
		prefetch_data(cbe + i * GRU_HANDLE_STRIDE, 1,
			      GRU_CACHE_LINE_BYTES);
		cb += GRU_HANDLE_STRIDE;
	}
}

static void gru_load_context_data(void *save, void *grubase, int ctxnum,
				  unsigned long cbrmap, unsigned long dsrmap)
{
	void *gseg, *cb, *cbe;
	unsigned long length;
	int i, scr;

	gseg = grubase + ctxnum * GRU_GSEG_STRIDE;
	cb = gseg + GRU_CB_BASE;
	cbe = grubase + GRU_CBE_BASE;
	length = hweight64(dsrmap) * GRU_DSR_AU_BYTES;
	gru_prefetch_context(gseg, cb, cbe, cbrmap, length);

	for_each_cbr_in_allocation_map(i, &cbrmap, scr) {
		save += gru_copy_handle(cb, save);
		save += gru_copy_handle(cbe + i * GRU_HANDLE_STRIDE, save);
		cb += GRU_HANDLE_STRIDE;
	}

	memcpy(gseg + GRU_DS_BASE, save, length);
}

static void gru_unload_context_data(void *save, void *grubase, int ctxnum,
				    unsigned long cbrmap, unsigned long dsrmap)
{
	void *gseg, *cb, *cbe;
	unsigned long length;
	int i, scr;

	gseg = grubase + ctxnum * GRU_GSEG_STRIDE;
	cb = gseg + GRU_CB_BASE;
	cbe = grubase + GRU_CBE_BASE;
	length = hweight64(dsrmap) * GRU_DSR_AU_BYTES;
	gru_prefetch_context(gseg, cb, cbe, cbrmap, length);

	for_each_cbr_in_allocation_map(i, &cbrmap, scr) {
		save += gru_copy_handle(save, cb);
		save += gru_copy_handle(save, cbe + i * GRU_HANDLE_STRIDE);
		cb += GRU_HANDLE_STRIDE;
	}
	memcpy(save, gseg + GRU_DS_BASE, length);
}

void gru_unload_context(struct gru_thread_state *gts, int savestate)
{
	struct gru_state *gru = gts->ts_gru;
	struct gru_context_configuration_handle *cch;
	int ctxnum = gts->ts_ctxnum;

	zap_vma_ptes(gts->ts_vma, UGRUADDR(gts), GRU_GSEG_PAGESIZE);
	cch = get_cch(gru->gs_gru_base_vaddr, ctxnum);

	lock_cch_handle(cch);
	if (cch_interrupt_sync(cch))
		BUG();
	gru_dbg(grudev, "gts %p\n", gts);

	gru_unload_mm_tracker(gru, gts->ts_gms, gts->ts_ctxnum);
	if (savestate)
		gru_unload_context_data(gts->ts_gdata, gru->gs_gru_base_vaddr,
					ctxnum, gts->ts_cbr_map,
					gts->ts_dsr_map);

	if (cch_deallocate(cch))
		BUG();
	gts->ts_force_unload = 0;	/* ts_force_unload locked by CCH lock */
	unlock_cch_handle(cch);

	gru_free_gru_context(gts);
	STAT(unload_context);
}

static void gru_load_context(struct gru_thread_state *gts)
{
	struct gru_state *gru = gts->ts_gru;
	struct gru_context_configuration_handle *cch;
	int err, asid, ctxnum = gts->ts_ctxnum;

	gru_dbg(grudev, "gts %p\n", gts);
	cch = get_cch(gru->gs_gru_base_vaddr, ctxnum);

	lock_cch_handle(cch);
	asid = gru_load_mm_tracker(gru, gts->ts_gms, gts->ts_ctxnum);
	cch->tfm_fault_bit_enable =
	    (gts->ts_user_options == GRU_OPT_MISS_FMM_POLL
	     || gts->ts_user_options == GRU_OPT_MISS_FMM_INTR);
	cch->tlb_int_enable = (gts->ts_user_options == GRU_OPT_MISS_FMM_INTR);
	if (cch->tlb_int_enable) {
		gts->ts_tlb_int_select = gru_cpu_fault_map_id();
		cch->tlb_int_select = gts->ts_tlb_int_select;
	}
	cch->tfm_done_bit_enable = 0;
	err = cch_allocate(cch, asid, gts->ts_cbr_map, gts->ts_dsr_map);
	if (err) {
		gru_dbg(grudev,
			"err %d: cch %p, gts %p, cbr 0x%lx, dsr 0x%lx\n",
			err, cch, gts, gts->ts_cbr_map, gts->ts_dsr_map);
		BUG();
	}

	gru_load_context_data(gts->ts_gdata, gru->gs_gru_base_vaddr, ctxnum,
			      gts->ts_cbr_map, gts->ts_dsr_map);

	if (cch_start(cch))
		BUG();
	unlock_cch_handle(cch);

	STAT(load_context);
}

static int gru_update_cch(struct gru_thread_state *gts, int int_select)
{
	struct gru_context_configuration_handle *cch;
	struct gru_state *gru = gts->ts_gru;
	int i, ctxnum = gts->ts_ctxnum, ret = 0;

	cch = get_cch(gru->gs_gru_base_vaddr, ctxnum);

	lock_cch_handle(cch);
	if (cch->state == CCHSTATE_ACTIVE) {
		if (gru->gs_gts[gts->ts_ctxnum] != gts)
			goto exit;
		if (cch_interrupt(cch))
			BUG();
		if (int_select >= 0) {
			gts->ts_tlb_int_select = int_select;
			cch->tlb_int_select = int_select;
		} else {
			for (i = 0; i < 8; i++)
				cch->asid[i] = 0;
			cch->tfm_fault_bit_enable = 0;
			cch->tlb_int_enable = 0;
			gts->ts_force_unload = 1;
		}
		if (cch_start(cch))
			BUG();
		ret = 1;
	}
exit:
	unlock_cch_handle(cch);
	return ret;
}

static int gru_retarget_intr(struct gru_thread_state *gts)
{
	if (gts->ts_tlb_int_select < 0
	    || gts->ts_tlb_int_select == gru_cpu_fault_map_id())
		return 0;

	gru_dbg(grudev, "retarget from %d to %d\n", gts->ts_tlb_int_select,
		gru_cpu_fault_map_id());
	return gru_update_cch(gts, gru_cpu_fault_map_id());
}


#define next_ctxnum(n)	((n) <  GRU_NUM_CCH - 2 ? (n) + 1 : 0)
#define next_gru(b, g)	(((g) < &(b)->bs_grus[GRU_CHIPLETS_PER_BLADE - 1]) ?  \
				 ((g)+1) : &(b)->bs_grus[0])

static void gru_steal_context(struct gru_thread_state *gts)
{
	struct gru_blade_state *blade;
	struct gru_state *gru, *gru0;
	struct gru_thread_state *ngts = NULL;
	int ctxnum, ctxnum0, flag = 0, cbr, dsr;

	cbr = gts->ts_cbr_au_count;
	dsr = gts->ts_dsr_au_count;

	preempt_disable();
	blade = gru_base[uv_numa_blade_id()];
	spin_lock(&blade->bs_lock);

	ctxnum = next_ctxnum(blade->bs_lru_ctxnum);
	gru = blade->bs_lru_gru;
	if (ctxnum == 0)
		gru = next_gru(blade, gru);
	ctxnum0 = ctxnum;
	gru0 = gru;
	while (1) {
		if (check_gru_resources(gru, cbr, dsr, GRU_NUM_CCH))
			break;
		spin_lock(&gru->gs_lock);
		for (; ctxnum < GRU_NUM_CCH; ctxnum++) {
			if (flag && gru == gru0 && ctxnum == ctxnum0)
				break;
			ngts = gru->gs_gts[ctxnum];
			/*
			 * We are grabbing locks out of order, so trylock is
			 * needed. GTSs are usually not locked, so the odds of
			 * success are high. If trylock fails, try to steal a
			 * different GSEG.
			 */
			if (ngts && mutex_trylock(&ngts->ts_ctxlock))
				break;
			ngts = NULL;
			flag = 1;
		}
		spin_unlock(&gru->gs_lock);
		if (ngts || (flag && gru == gru0 && ctxnum == ctxnum0))
			break;
		ctxnum = 0;
		gru = next_gru(blade, gru);
	}
	blade->bs_lru_gru = gru;
	blade->bs_lru_ctxnum = ctxnum;
	spin_unlock(&blade->bs_lock);
	preempt_enable();

	if (ngts) {
		STAT(steal_context);
		ngts->ts_steal_jiffies = jiffies;
		gru_unload_context(ngts, 1);
		mutex_unlock(&ngts->ts_ctxlock);
	} else {
		STAT(steal_context_failed);
	}
	gru_dbg(grudev,
		"stole gru %x, ctxnum %d from gts %p. Need cb %d, ds %d;"
		" avail cb %ld, ds %ld\n",
		gru->gs_gid, ctxnum, ngts, cbr, dsr, hweight64(gru->gs_cbr_map),
		hweight64(gru->gs_dsr_map));
}

static struct gru_state *gru_assign_gru_context(struct gru_thread_state *gts)
{
	struct gru_state *gru, *grux;
	int i, max_active_contexts;

	preempt_disable();

again:
	gru = NULL;
	max_active_contexts = GRU_NUM_CCH;
	for_each_gru_on_blade(grux, uv_numa_blade_id(), i) {
		if (check_gru_resources(grux, gts->ts_cbr_au_count,
					gts->ts_dsr_au_count,
					max_active_contexts)) {
			gru = grux;
			max_active_contexts = grux->gs_active_contexts;
			if (max_active_contexts == 0)
				break;
		}
	}

	if (gru) {
		spin_lock(&gru->gs_lock);
		if (!check_gru_resources(gru, gts->ts_cbr_au_count,
					 gts->ts_dsr_au_count, GRU_NUM_CCH)) {
			spin_unlock(&gru->gs_lock);
			goto again;
		}
		reserve_gru_resources(gru, gts);
		gts->ts_gru = gru;
		gts->ts_ctxnum =
		    find_first_zero_bit(&gru->gs_context_map, GRU_NUM_CCH);
		BUG_ON(gts->ts_ctxnum == GRU_NUM_CCH);
		atomic_inc(&gts->ts_refcnt);
		gru->gs_gts[gts->ts_ctxnum] = gts;
		__set_bit(gts->ts_ctxnum, &gru->gs_context_map);
		spin_unlock(&gru->gs_lock);

		STAT(assign_context);
		gru_dbg(grudev,
			"gseg %p, gts %p, gru %x, ctx %d, cbr %d, dsr %d\n",
			gseg_virtual_address(gts->ts_gru, gts->ts_ctxnum), gts,
			gts->ts_gru->gs_gid, gts->ts_ctxnum,
			gts->ts_cbr_au_count, gts->ts_dsr_au_count);
	} else {
		gru_dbg(grudev, "failed to allocate a GTS %s\n", "");
		STAT(assign_context_failed);
	}

	preempt_enable();
	return gru;
}

int gru_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	struct gru_thread_state *gts;
	unsigned long paddr, vaddr;

	vaddr = (unsigned long)vmf->virtual_address;
	gru_dbg(grudev, "vma %p, vaddr 0x%lx (0x%lx)\n",
		vma, vaddr, GSEG_BASE(vaddr));
	STAT(nopfn);

	/* The following check ensures vaddr is a valid address in the VMA */
	gts = gru_find_thread_state(vma, TSID(vaddr, vma));
	if (!gts)
		return VM_FAULT_SIGBUS;

again:
	preempt_disable();
	mutex_lock(&gts->ts_ctxlock);
	if (gts->ts_gru) {
		if (gts->ts_gru->gs_blade_id != uv_numa_blade_id()) {
			STAT(migrated_nopfn_unload);
			gru_unload_context(gts, 1);
		} else {
			if (gru_retarget_intr(gts))
				STAT(migrated_nopfn_retarget);
		}
	}

	if (!gts->ts_gru) {
		if (!gru_assign_gru_context(gts)) {
			mutex_unlock(&gts->ts_ctxlock);
			preempt_enable();
			schedule_timeout(GRU_ASSIGN_DELAY);  /* true hack ZZZ */
			if (gts->ts_steal_jiffies + GRU_STEAL_DELAY < jiffies)
				gru_steal_context(gts);
			goto again;
		}
		gru_load_context(gts);
		paddr = gseg_physical_address(gts->ts_gru, gts->ts_ctxnum);
		remap_pfn_range(vma, vaddr & ~(GRU_GSEG_PAGESIZE - 1),
				paddr >> PAGE_SHIFT, GRU_GSEG_PAGESIZE,
				vma->vm_page_prot);
	}

	mutex_unlock(&gts->ts_ctxlock);
	preempt_enable();

	return VM_FAULT_NOPAGE;
}

