

#include <linux/list.h>
#include "drmP.h"
#include "drm.h"
#include "drm_crtc.h"

void drm_mode_debug_printmodeline(struct drm_display_mode *mode)
{
	DRM_DEBUG("Modeline %d:\"%s\" %d %d %d %d %d %d %d %d %d %d 0x%x 0x%x\n",
		  mode->base.id, mode->name, mode->vrefresh, mode->clock,
		  mode->hdisplay, mode->hsync_start,
		  mode->hsync_end, mode->htotal,
		  mode->vdisplay, mode->vsync_start,
		  mode->vsync_end, mode->vtotal, mode->type, mode->flags);
}
EXPORT_SYMBOL(drm_mode_debug_printmodeline);

void drm_mode_set_name(struct drm_display_mode *mode)
{
	snprintf(mode->name, DRM_DISPLAY_MODE_LEN, "%dx%d", mode->hdisplay,
		 mode->vdisplay);
}
EXPORT_SYMBOL(drm_mode_set_name);

void drm_mode_list_concat(struct list_head *head, struct list_head *new)
{

	struct list_head *entry, *tmp;

	list_for_each_safe(entry, tmp, head) {
		list_move_tail(entry, new);
	}
}
EXPORT_SYMBOL(drm_mode_list_concat);

int drm_mode_width(struct drm_display_mode *mode)
{
	return mode->hdisplay;

}
EXPORT_SYMBOL(drm_mode_width);

int drm_mode_height(struct drm_display_mode *mode)
{
	return mode->vdisplay;
}
EXPORT_SYMBOL(drm_mode_height);

int drm_mode_vrefresh(struct drm_display_mode *mode)
{
	int refresh = 0;
	unsigned int calc_val;

	if (mode->vrefresh > 0)
		refresh = mode->vrefresh;
	else if (mode->htotal > 0 && mode->vtotal > 0) {
		/* work out vrefresh the value will be x1000 */
		calc_val = (mode->clock * 1000);

		calc_val /= mode->htotal;
		calc_val *= 1000;
		calc_val /= mode->vtotal;

		refresh = calc_val;
		if (mode->flags & DRM_MODE_FLAG_INTERLACE)
			refresh *= 2;
		if (mode->flags & DRM_MODE_FLAG_DBLSCAN)
			refresh /= 2;
		if (mode->vscan > 1)
			refresh /= mode->vscan;
	}
	return refresh;
}
EXPORT_SYMBOL(drm_mode_vrefresh);

void drm_mode_set_crtcinfo(struct drm_display_mode *p, int adjust_flags)
{
	if ((p == NULL) || ((p->type & DRM_MODE_TYPE_CRTC_C) == DRM_MODE_TYPE_BUILTIN))
		return;

	p->crtc_hdisplay = p->hdisplay;
	p->crtc_hsync_start = p->hsync_start;
	p->crtc_hsync_end = p->hsync_end;
	p->crtc_htotal = p->htotal;
	p->crtc_hskew = p->hskew;
	p->crtc_vdisplay = p->vdisplay;
	p->crtc_vsync_start = p->vsync_start;
	p->crtc_vsync_end = p->vsync_end;
	p->crtc_vtotal = p->vtotal;

	if (p->flags & DRM_MODE_FLAG_INTERLACE) {
		if (adjust_flags & CRTC_INTERLACE_HALVE_V) {
			p->crtc_vdisplay /= 2;
			p->crtc_vsync_start /= 2;
			p->crtc_vsync_end /= 2;
			p->crtc_vtotal /= 2;
		}

		p->crtc_vtotal |= 1;
	}

	if (p->flags & DRM_MODE_FLAG_DBLSCAN) {
		p->crtc_vdisplay *= 2;
		p->crtc_vsync_start *= 2;
		p->crtc_vsync_end *= 2;
		p->crtc_vtotal *= 2;
	}

	if (p->vscan > 1) {
		p->crtc_vdisplay *= p->vscan;
		p->crtc_vsync_start *= p->vscan;
		p->crtc_vsync_end *= p->vscan;
		p->crtc_vtotal *= p->vscan;
	}

	p->crtc_vblank_start = min(p->crtc_vsync_start, p->crtc_vdisplay);
	p->crtc_vblank_end = max(p->crtc_vsync_end, p->crtc_vtotal);
	p->crtc_hblank_start = min(p->crtc_hsync_start, p->crtc_hdisplay);
	p->crtc_hblank_end = max(p->crtc_hsync_end, p->crtc_htotal);

	p->crtc_hadjusted = false;
	p->crtc_vadjusted = false;
}
EXPORT_SYMBOL(drm_mode_set_crtcinfo);


struct drm_display_mode *drm_mode_duplicate(struct drm_device *dev,
					    struct drm_display_mode *mode)
{
	struct drm_display_mode *nmode;
	int new_id;

	nmode = drm_mode_create(dev);
	if (!nmode)
		return NULL;

	new_id = nmode->base.id;
	*nmode = *mode;
	nmode->base.id = new_id;
	INIT_LIST_HEAD(&nmode->head);
	return nmode;
}
EXPORT_SYMBOL(drm_mode_duplicate);

bool drm_mode_equal(struct drm_display_mode *mode1, struct drm_display_mode *mode2)
{
	/* do clock check convert to PICOS so fb modes get matched
	 * the same */
	if (mode1->clock && mode2->clock) {
		if (KHZ2PICOS(mode1->clock) != KHZ2PICOS(mode2->clock))
			return false;
	} else if (mode1->clock != mode2->clock)
		return false;

	if (mode1->hdisplay == mode2->hdisplay &&
	    mode1->hsync_start == mode2->hsync_start &&
	    mode1->hsync_end == mode2->hsync_end &&
	    mode1->htotal == mode2->htotal &&
	    mode1->hskew == mode2->hskew &&
	    mode1->vdisplay == mode2->vdisplay &&
	    mode1->vsync_start == mode2->vsync_start &&
	    mode1->vsync_end == mode2->vsync_end &&
	    mode1->vtotal == mode2->vtotal &&
	    mode1->vscan == mode2->vscan &&
	    mode1->flags == mode2->flags)
		return true;

	return false;
}
EXPORT_SYMBOL(drm_mode_equal);

void drm_mode_validate_size(struct drm_device *dev,
			    struct list_head *mode_list,
			    int maxX, int maxY, int maxPitch)
{
	struct drm_display_mode *mode;

	list_for_each_entry(mode, mode_list, head) {
		if (maxPitch > 0 && mode->hdisplay > maxPitch)
			mode->status = MODE_BAD_WIDTH;

		if (maxX > 0 && mode->hdisplay > maxX)
			mode->status = MODE_VIRTUAL_X;

		if (maxY > 0 && mode->vdisplay > maxY)
			mode->status = MODE_VIRTUAL_Y;
	}
}
EXPORT_SYMBOL(drm_mode_validate_size);

void drm_mode_validate_clocks(struct drm_device *dev,
			      struct list_head *mode_list,
			      int *min, int *max, int n_ranges)
{
	struct drm_display_mode *mode;
	int i;

	list_for_each_entry(mode, mode_list, head) {
		bool good = false;
		for (i = 0; i < n_ranges; i++) {
			if (mode->clock >= min[i] && mode->clock <= max[i]) {
				good = true;
				break;
			}
		}
		if (!good)
			mode->status = MODE_CLOCK_RANGE;
	}
}
EXPORT_SYMBOL(drm_mode_validate_clocks);

void drm_mode_prune_invalid(struct drm_device *dev,
			    struct list_head *mode_list, bool verbose)
{
	struct drm_display_mode *mode, *t;

	list_for_each_entry_safe(mode, t, mode_list, head) {
		if (mode->status != MODE_OK) {
			list_del(&mode->head);
			if (verbose) {
				drm_mode_debug_printmodeline(mode);
				DRM_DEBUG("Not using %s mode %d\n", mode->name, mode->status);
			}
			drm_mode_destroy(dev, mode);
		}
	}
}
EXPORT_SYMBOL(drm_mode_prune_invalid);

static int drm_mode_compare(struct list_head *lh_a, struct list_head *lh_b)
{
	struct drm_display_mode *a = list_entry(lh_a, struct drm_display_mode, head);
	struct drm_display_mode *b = list_entry(lh_b, struct drm_display_mode, head);
	int diff;

	diff = ((b->type & DRM_MODE_TYPE_PREFERRED) != 0) -
		((a->type & DRM_MODE_TYPE_PREFERRED) != 0);
	if (diff)
		return diff;
	diff = b->hdisplay * b->vdisplay - a->hdisplay * a->vdisplay;
	if (diff)
		return diff;
	diff = b->clock - a->clock;
	return diff;
}

/* FIXME: what we don't have a list sort function? */
/* list sort from Mark J Roberts (mjr@znex.org) */
void list_sort(struct list_head *head,
	       int (*cmp)(struct list_head *a, struct list_head *b))
{
	struct list_head *p, *q, *e, *list, *tail, *oldhead;
	int insize, nmerges, psize, qsize, i;

	list = head->next;
	list_del(head);
	insize = 1;
	for (;;) {
		p = oldhead = list;
		list = tail = NULL;
		nmerges = 0;

		while (p) {
			nmerges++;
			q = p;
			psize = 0;
			for (i = 0; i < insize; i++) {
				psize++;
				q = q->next == oldhead ? NULL : q->next;
				if (!q)
					break;
			}

			qsize = insize;
			while (psize > 0 || (qsize > 0 && q)) {
				if (!psize) {
					e = q;
					q = q->next;
					qsize--;
					if (q == oldhead)
						q = NULL;
				} else if (!qsize || !q) {
					e = p;
					p = p->next;
					psize--;
					if (p == oldhead)
						p = NULL;
				} else if (cmp(p, q) <= 0) {
					e = p;
					p = p->next;
					psize--;
					if (p == oldhead)
						p = NULL;
				} else {
					e = q;
					q = q->next;
					qsize--;
					if (q == oldhead)
						q = NULL;
				}
				if (tail)
					tail->next = e;
				else
					list = e;
				e->prev = tail;
				tail = e;
			}
			p = q;
		}

		tail->next = list;
		list->prev = tail;

		if (nmerges <= 1)
			break;

		insize *= 2;
	}

	head->next = list;
	head->prev = list->prev;
	list->prev->next = head;
	list->prev = head;
}

void drm_mode_sort(struct list_head *mode_list)
{
	list_sort(mode_list, drm_mode_compare);
}
EXPORT_SYMBOL(drm_mode_sort);

void drm_mode_connector_list_update(struct drm_connector *connector)
{
	struct drm_display_mode *mode;
	struct drm_display_mode *pmode, *pt;
	int found_it;

	list_for_each_entry_safe(pmode, pt, &connector->probed_modes,
				 head) {
		found_it = 0;
		/* go through current modes checking for the new probed mode */
		list_for_each_entry(mode, &connector->modes, head) {
			if (drm_mode_equal(pmode, mode)) {
				found_it = 1;
				/* if equal delete the probed mode */
				mode->status = pmode->status;
				list_del(&pmode->head);
				drm_mode_destroy(connector->dev, pmode);
				break;
			}
		}

		if (!found_it) {
			list_move_tail(&pmode->head, &connector->modes);
		}
	}
}
EXPORT_SYMBOL(drm_mode_connector_list_update);
