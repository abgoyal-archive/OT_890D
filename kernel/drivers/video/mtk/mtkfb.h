

#ifndef __MTKFB_H
#define __MTKFB_H


#define MTK_FB_OVERLAY_SUPPORT

/* IOCTL commands. */

#define MTK_IOW(num, dtype)     _IOW('O', num, dtype)
#define MTK_IOR(num, dtype)     _IOR('O', num, dtype)
#define MTK_IOWR(num, dtype)    _IOWR('O', num, dtype)
#define MTK_IO(num)             _IO('O', num)

#define MTKFB_FILLRECT          MTK_IOW(0, struct fb_fillrect)
#define MTKFB_COPYAREA          MTK_IOW(1, struct fb_copyarea)
#define MTKFB_IMAGEBLIT         MTK_IOW(2, struct fb_image)

#define MTKFB_TRANSPARENT_BLIT  MTK_IOW(30, struct fb_image)
#define MTKFB_MIRROR            MTK_IOW(31, int)
#define MTKFB_SCALE             MTK_IOW(32, struct fb_scale)
#define MTKFB_SELECT_VIS_FRAME  MTK_IOW(33, int)
#define MTKFB_SELECT_SRC_FRAME  MTK_IOW(34, int)
#define MTKFB_SELECT_DST_FRAME  MTK_IOW(35, int)
#define MTKFB_GET_FRAME_OFFSET  MTK_IOWR(36, struct fb_frame_offset)
#define MTKFB_SYNC_GFX          MTK_IO(37)
#define MTKFB_VSYNC             MTK_IO(38)
#define MTKFB_LATE_ACTIVATE     MTK_IO(39)
#define MTKFB_SET_UPDATE_MODE   MTK_IOW(40, enum fb_update_mode)
#define MTKFB_UPDATE_WINDOW     MTK_IOW(41, struct fb_update_window)
#define MTKFB_GET_CAPS          MTK_IOR(42, unsigned long)
#define MTKFB_GET_UPDATE_MODE   MTK_IOW(43, enum fb_update_mode)

#define MTKFB_GETVFRAMEPHYSICAL MTK_IOW(44, unsigned long)

#ifdef MTK_FB_OVERLAY_SUPPORT
#define MTKFB_SET_OVERLAY_LAYER   MTK_IOW(45, struct fb_overlay_layer)
#define MTKFB_TRIG_OVERLAY_OUT    MTK_IO(46)
#define MTKFB_ENABLE_OVERLAY      MTK_IOW(47, unsigned long)
#define MTKFB_DISABLE_OVERLAY     MTK_IOW(48, unsigned long)
#define MTKFB_WAIT_OVERLAY_READY  MTK_IO(49)
#define MTKFB_GET_OVERLAY_LAYER_COUNT   MTK_IOR(50, unsigned long)
#define MTKFB_SET_VIDEO_LAYERS    MTK_IOW(51, struct fb_overlay_layer)
#endif // MTK_FB_OVERLAY_SUPPORT

#define MTKFB_LOCK_FRONT_BUFFER   MTK_IO(70)
#define MTKFB_UNLOCK_FRONT_BUFFER MTK_IO(71)

#define MTKFB_META_RESTORE_SCREEN MTK_IOW(101, unsigned long)


#define FBCAPS_GENERIC_MASK     0x00000fff
#define FBCAPS_LCDC_MASK        0x00fff000
#define FBCAPS_PANEL_MASK       0xff000000

#define FBCAPS_MANUAL_UPDATE    0x00001000
#define FBCAPS_SET_BACKLIGHT    0x01000000


#define MAKE_MTK_FB_FORMAT_ID(id, bpp)  (((id) << 8) | (bpp))

typedef enum
{
    MTK_FB_FORMAT_UNKNOWN = 0,
        
    MTK_FB_FORMAT_RGB565   = MAKE_MTK_FB_FORMAT_ID(1, 2),
	MTK_FB_FORMAT_RGB888   = MAKE_MTK_FB_FORMAT_ID(2, 3),
    MTK_FB_FORMAT_BGR888   = MAKE_MTK_FB_FORMAT_ID(3, 3),
    MTK_FB_FORMAT_ARGB8888 = MAKE_MTK_FB_FORMAT_ID(4, 4),
    MTK_FB_FORMAT_ABGR8888 = MAKE_MTK_FB_FORMAT_ID(5, 4),

    MTK_FB_FORMAT_BPP_MASK = 0xFF,
} MTK_FB_FORMAT;

#define GET_MTK_FB_FORMAT_BPP(f)    ((f) & MTK_FB_FORMAT_BPP_MASK)


struct fb_scale {
    unsigned int xscale, yscale;
};

struct fb_frame_offset {
    unsigned int idx;
    unsigned long offset;
};

struct fb_update_window {
    unsigned int x, y;
    unsigned int width, height;
};

struct fb_overlay_layer {
    unsigned int layer_id;
    unsigned int layer_enable;

    void* src_base_addr;
    void* src_phy_addr;
    unsigned int  src_direct_link;
    MTK_FB_FORMAT src_fmt;
    unsigned int  src_use_color_key;
    unsigned int  src_color_key;
    unsigned int  src_pitch;
    unsigned int  src_offset_x, src_offset_y;
    unsigned int  src_width, src_height;

    unsigned int  tgt_offset_x, tgt_offset_y;
    unsigned int  tgt_width, tgt_height;
};

#define HW_OVERLAY_COUNT     (6)
// Top layer is assigned to Debug Layer
// Second layer is assigned to UI
// Third layer is assigned to FD
#define RESERVED_LAYER_COUNT (3)
#define VIDEO_LAYER_COUNT    (HW_OVERLAY_COUNT - RESERVED_LAYER_COUNT)
#define FACE_DETECTION_LAYER_ID  (3)

#ifdef __KERNEL__

#include <linux/completion.h>
#include <linux/interrupt.h>

#define MTKFB_DRIVER "mt6516-fb"

#define PRNERR(fmt, args...)  printk(KERN_ERR MTKFB_DRIVER ": " fmt, ## args)

enum mtkfb_state {
    MTKFB_DISABLED  = 0,
    MTKFB_SUSPENDED = 99,
    MTKFB_ACTIVE    = 100
};

typedef enum {
    MTKFB_LAYER_ENABLE_DIRTY = (1 << 0),
    MTKFB_LAYER_FORMAT_DIRTY = (1 << 1),
} MTKFB_LAYER_CONFIG_DIRTY;

struct mtkfb_device {
    int             state;
    void           *fb_va_base;             /* MPU virtual address */
    dma_addr_t      fb_pa_base;             /* Bus physical address */
    unsigned long   fb_size_in_byte;

    unsigned long   layer_enable;
    MTK_FB_FORMAT   layer_format[HW_OVERLAY_COUNT];
    unsigned int    layer_config_dirty;

    int             xscale, yscale, mirror; /* transformations.
                                               rotate is stored in fb_info->var */
    u32             pseudo_palette[17];

    struct fb_info *fb_info;                /* Linux fbdev framework data */
    struct device  *dev;
};

#endif /* __KERNEL__ */

#endif /* __MTKFB_H */
