


#include "drmP.h"

int drm_dma_setup(struct drm_device *dev)
{
	int i;

	dev->dma = drm_alloc(sizeof(*dev->dma), DRM_MEM_DRIVER);
	if (!dev->dma)
		return -ENOMEM;

	memset(dev->dma, 0, sizeof(*dev->dma));

	for (i = 0; i <= DRM_MAX_ORDER; i++)
		memset(&dev->dma->bufs[i], 0, sizeof(dev->dma->bufs[0]));

	return 0;
}

void drm_dma_takedown(struct drm_device *dev)
{
	struct drm_device_dma *dma = dev->dma;
	int i, j;

	if (!dma)
		return;

	/* Clear dma buffers */
	for (i = 0; i <= DRM_MAX_ORDER; i++) {
		if (dma->bufs[i].seg_count) {
			DRM_DEBUG("order %d: buf_count = %d,"
				  " seg_count = %d\n",
				  i,
				  dma->bufs[i].buf_count,
				  dma->bufs[i].seg_count);
			for (j = 0; j < dma->bufs[i].seg_count; j++) {
				if (dma->bufs[i].seglist[j]) {
					drm_pci_free(dev, dma->bufs[i].seglist[j]);
				}
			}
			drm_free(dma->bufs[i].seglist,
				 dma->bufs[i].seg_count
				 * sizeof(*dma->bufs[0].seglist), DRM_MEM_SEGS);
		}
		if (dma->bufs[i].buf_count) {
			for (j = 0; j < dma->bufs[i].buf_count; j++) {
				if (dma->bufs[i].buflist[j].dev_private) {
					drm_free(dma->bufs[i].buflist[j].
						 dev_private,
						 dma->bufs[i].buflist[j].
						 dev_priv_size, DRM_MEM_BUFS);
				}
			}
			drm_free(dma->bufs[i].buflist,
				 dma->bufs[i].buf_count *
				 sizeof(*dma->bufs[0].buflist), DRM_MEM_BUFS);
		}
	}

	if (dma->buflist) {
		drm_free(dma->buflist,
			 dma->buf_count * sizeof(*dma->buflist), DRM_MEM_BUFS);
	}

	if (dma->pagelist) {
		drm_free(dma->pagelist,
			 dma->page_count * sizeof(*dma->pagelist),
			 DRM_MEM_PAGES);
	}
	drm_free(dev->dma, sizeof(*dev->dma), DRM_MEM_DRIVER);
	dev->dma = NULL;
}

void drm_free_buffer(struct drm_device *dev, struct drm_buf * buf)
{
	if (!buf)
		return;

	buf->waiting = 0;
	buf->pending = 0;
	buf->file_priv = NULL;
	buf->used = 0;

	if (drm_core_check_feature(dev, DRIVER_DMA_QUEUE)
	    && waitqueue_active(&buf->dma_wait)) {
		wake_up_interruptible(&buf->dma_wait);
	}
}

void drm_core_reclaim_buffers(struct drm_device *dev,
			      struct drm_file *file_priv)
{
	struct drm_device_dma *dma = dev->dma;
	int i;

	if (!dma)
		return;
	for (i = 0; i < dma->buf_count; i++) {
		if (dma->buflist[i]->file_priv == file_priv) {
			switch (dma->buflist[i]->list) {
			case DRM_LIST_NONE:
				drm_free_buffer(dev, dma->buflist[i]);
				break;
			case DRM_LIST_WAIT:
				dma->buflist[i]->list = DRM_LIST_RECLAIM;
				break;
			default:
				/* Buffer already on hardware. */
				break;
			}
		}
	}
}

EXPORT_SYMBOL(drm_core_reclaim_buffers);
