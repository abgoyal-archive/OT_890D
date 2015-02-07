

#include "drmP.h"
#include "drm.h"
#include "i915_drm.h"
#include "i915_drv.h"


void
i915_gem_detect_bit_6_swizzle(struct drm_device *dev)
{
	drm_i915_private_t *dev_priv = dev->dev_private;
	uint32_t swizzle_x = I915_BIT_6_SWIZZLE_UNKNOWN;
	uint32_t swizzle_y = I915_BIT_6_SWIZZLE_UNKNOWN;

	if (!IS_I9XX(dev)) {
		/* As far as we know, the 865 doesn't have these bit 6
		 * swizzling issues.
		 */
		swizzle_x = I915_BIT_6_SWIZZLE_NONE;
		swizzle_y = I915_BIT_6_SWIZZLE_NONE;
	} else if ((!IS_I965G(dev) && !IS_G33(dev)) || IS_I965GM(dev) ||
		   IS_GM45(dev)) {
		uint32_t dcc;

		/* On 915-945 and GM965, channel interleave by the CPU is
		 * determined by DCC.  The CPU will alternate based on bit 6
		 * in interleaved mode, and the GPU will then also alternate
		 * on bit 6, 9, and 10 for X, but the CPU may also optionally
		 * alternate based on bit 17 (XOR not disabled and XOR
		 * bit == 17).
		 */
		dcc = I915_READ(DCC);
		switch (dcc & DCC_ADDRESSING_MODE_MASK) {
		case DCC_ADDRESSING_MODE_SINGLE_CHANNEL:
		case DCC_ADDRESSING_MODE_DUAL_CHANNEL_ASYMMETRIC:
			swizzle_x = I915_BIT_6_SWIZZLE_NONE;
			swizzle_y = I915_BIT_6_SWIZZLE_NONE;
			break;
		case DCC_ADDRESSING_MODE_DUAL_CHANNEL_INTERLEAVED:
			if (IS_I915G(dev) || IS_I915GM(dev) ||
			    dcc & DCC_CHANNEL_XOR_DISABLE) {
				swizzle_x = I915_BIT_6_SWIZZLE_9_10;
				swizzle_y = I915_BIT_6_SWIZZLE_9;
			} else if ((IS_I965GM(dev) || IS_GM45(dev)) &&
				   (dcc & DCC_CHANNEL_XOR_BIT_17) == 0) {
				/* GM965/GM45 does either bit 11 or bit 17
				 * swizzling.
				 */
				swizzle_x = I915_BIT_6_SWIZZLE_9_10_11;
				swizzle_y = I915_BIT_6_SWIZZLE_9_11;
			} else {
				/* Bit 17 or perhaps other swizzling */
				swizzle_x = I915_BIT_6_SWIZZLE_UNKNOWN;
				swizzle_y = I915_BIT_6_SWIZZLE_UNKNOWN;
			}
			break;
		}
		if (dcc == 0xffffffff) {
			DRM_ERROR("Couldn't read from MCHBAR.  "
				  "Disabling tiling.\n");
			swizzle_x = I915_BIT_6_SWIZZLE_UNKNOWN;
			swizzle_y = I915_BIT_6_SWIZZLE_UNKNOWN;
		}
	} else {
		/* The 965, G33, and newer, have a very flexible memory
		 * configuration.  It will enable dual-channel mode
		 * (interleaving) on as much memory as it can, and the GPU
		 * will additionally sometimes enable different bit 6
		 * swizzling for tiled objects from the CPU.
		 *
		 * Here's what I found on the G965:
		 *    slot fill         memory size  swizzling
		 * 0A   0B   1A   1B    1-ch   2-ch
		 * 512  0    0    0     512    0     O
		 * 512  0    512  0     16     1008  X
		 * 512  0    0    512   16     1008  X
		 * 0    512  0    512   16     1008  X
		 * 1024 1024 1024 0     2048   1024  O
		 *
		 * We could probably detect this based on either the DRB
		 * matching, which was the case for the swizzling required in
		 * the table above, or from the 1-ch value being less than
		 * the minimum size of a rank.
		 */
		if (I915_READ16(C0DRB3) != I915_READ16(C1DRB3)) {
			swizzle_x = I915_BIT_6_SWIZZLE_NONE;
			swizzle_y = I915_BIT_6_SWIZZLE_NONE;
		} else {
			swizzle_x = I915_BIT_6_SWIZZLE_9_10;
			swizzle_y = I915_BIT_6_SWIZZLE_9;
		}
	}

	dev_priv->mm.bit_6_swizzle_x = swizzle_x;
	dev_priv->mm.bit_6_swizzle_y = swizzle_y;
}


static int
i915_get_fence_size(struct drm_device *dev, int size)
{
	int i;
	int start;

	if (IS_I965G(dev)) {
		/* The 965 can have fences at any page boundary. */
		return ALIGN(size, 4096);
	} else {
		/* Align the size to a power of two greater than the smallest
		 * fence size.
		 */
		if (IS_I9XX(dev))
			start = 1024 * 1024;
		else
			start = 512 * 1024;

		for (i = start; i < size; i <<= 1)
			;

		return i;
	}
}

/* Check pitch constriants for all chips & tiling formats */
static bool
i915_tiling_ok(struct drm_device *dev, int stride, int size, int tiling_mode)
{
	int tile_width;

	/* Linear is always fine */
	if (tiling_mode == I915_TILING_NONE)
		return true;

	if (tiling_mode == I915_TILING_Y && HAS_128_BYTE_Y_TILING(dev))
		tile_width = 128;
	else
		tile_width = 512;

	/* 965+ just needs multiples of tile width */
	if (IS_I965G(dev)) {
		if (stride & (tile_width - 1))
			return false;
		return true;
	}

	/* Pre-965 needs power of two tile widths */
	if (stride < tile_width)
		return false;

	if (stride & (stride - 1))
		return false;

	/* We don't handle the aperture area covered by the fence being bigger
	 * than the object size.
	 */
	if (i915_get_fence_size(dev, size) != size)
		return false;

	return true;
}

int
i915_gem_set_tiling(struct drm_device *dev, void *data,
		   struct drm_file *file_priv)
{
	struct drm_i915_gem_set_tiling *args = data;
	drm_i915_private_t *dev_priv = dev->dev_private;
	struct drm_gem_object *obj;
	struct drm_i915_gem_object *obj_priv;

	obj = drm_gem_object_lookup(dev, file_priv, args->handle);
	if (obj == NULL)
		return -EINVAL;
	obj_priv = obj->driver_private;

	if (!i915_tiling_ok(dev, args->stride, obj->size, args->tiling_mode)) {
		drm_gem_object_unreference(obj);
		return -EINVAL;
	}

	mutex_lock(&dev->struct_mutex);

	if (args->tiling_mode == I915_TILING_NONE) {
		obj_priv->tiling_mode = I915_TILING_NONE;
		args->swizzle_mode = I915_BIT_6_SWIZZLE_NONE;
	} else {
		if (args->tiling_mode == I915_TILING_X)
			args->swizzle_mode = dev_priv->mm.bit_6_swizzle_x;
		else
			args->swizzle_mode = dev_priv->mm.bit_6_swizzle_y;
		/* If we can't handle the swizzling, make it untiled. */
		if (args->swizzle_mode == I915_BIT_6_SWIZZLE_UNKNOWN) {
			args->tiling_mode = I915_TILING_NONE;
			args->swizzle_mode = I915_BIT_6_SWIZZLE_NONE;
		}
	}
	if (args->tiling_mode != obj_priv->tiling_mode) {
		int ret;

		/* Unbind the object, as switching tiling means we're
		 * switching the cache organization due to fencing, probably.
		 */
		ret = i915_gem_object_unbind(obj);
		if (ret != 0) {
			WARN(ret != -ERESTARTSYS,
			     "failed to unbind object for tiling switch");
			args->tiling_mode = obj_priv->tiling_mode;
			mutex_unlock(&dev->struct_mutex);
			drm_gem_object_unreference(obj);

			return ret;
		}
		obj_priv->tiling_mode = args->tiling_mode;
	}
	obj_priv->stride = args->stride;

	drm_gem_object_unreference(obj);
	mutex_unlock(&dev->struct_mutex);

	return 0;
}

int
i915_gem_get_tiling(struct drm_device *dev, void *data,
		   struct drm_file *file_priv)
{
	struct drm_i915_gem_get_tiling *args = data;
	drm_i915_private_t *dev_priv = dev->dev_private;
	struct drm_gem_object *obj;
	struct drm_i915_gem_object *obj_priv;

	obj = drm_gem_object_lookup(dev, file_priv, args->handle);
	if (obj == NULL)
		return -EINVAL;
	obj_priv = obj->driver_private;

	mutex_lock(&dev->struct_mutex);

	args->tiling_mode = obj_priv->tiling_mode;
	switch (obj_priv->tiling_mode) {
	case I915_TILING_X:
		args->swizzle_mode = dev_priv->mm.bit_6_swizzle_x;
		break;
	case I915_TILING_Y:
		args->swizzle_mode = dev_priv->mm.bit_6_swizzle_y;
		break;
	case I915_TILING_NONE:
		args->swizzle_mode = I915_BIT_6_SWIZZLE_NONE;
		break;
	default:
		DRM_ERROR("unknown tiling mode\n");
	}

	drm_gem_object_unreference(obj);
	mutex_unlock(&dev->struct_mutex);

	return 0;
}
