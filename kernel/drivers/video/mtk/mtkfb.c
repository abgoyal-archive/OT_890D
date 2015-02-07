

#include <linux/autoconf.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/earlysuspend.h>
#include <linux/kthread.h>
#include <linux/rtpm_prio.h>

#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/mach-types.h>
#include <asm/cacheflush.h>
#include <asm/io.h>

#include <mach/dma.h>
#include <mach/irqs.h>

#include <mach/mt6516_typedefs.h>
#include <mach/mt6516_boot.h>
#include <mach/mt6516_gpt_sw.h>

#include "debug.h"
#include "disp_drv.h"
#include "dpi_drv.h"
#include "lcd_drv.h"

#include "mtkfb.h"
#include "mtkfb_console.h"

#define INIT_FB_AS_COLOR_BAR    (0)

static u32 MTK_FB_XRES  = 0;
static u32 MTK_FB_YRES  = 0;
static u32 MTK_FB_BPP   = 0;
static u32 MTK_FB_PAGES = 0;

#define MTK_FB_XRESV (MTK_FB_XRES)
#define MTK_FB_YRESV (MTK_FB_YRES * MTK_FB_PAGES) /* For page flipping */
#define MTK_FB_BYPP  ((MTK_FB_BPP + 7) >> 3)
#define MTK_FB_LINE  (MTK_FB_XRES * MTK_FB_BYPP)
#define MTK_FB_SIZE  (MTK_FB_LINE * MTK_FB_YRES)

#define MTK_FB_SIZEV (MTK_FB_LINE * MTK_FB_YRESV)

#define CHECK_RET(expr)    \
    do {                   \
        int ret = (expr);  \
        ASSERT(0 == ret);  \
    } while (0)

// ---------------------------------------------------------------------------
//  local variables
// ---------------------------------------------------------------------------

static XGPT_CONFIG xgpt_cfg =
{
    .num        = XGPT4,
    .mode       = XGPT_ONE_SHOT,
    .clkDiv     = XGPT_CLK_DIV_1,   // 32K Hz
    .bIrqEnable = TRUE,
    .u4Compare  = 0
};


static const struct timeval FRAME_INTERVAL = {0, 33333};  // 33ms

static atomic_t has_pending_update = ATOMIC_INIT(0);
static struct timeval last_update_time = {0};

DECLARE_MUTEX(sem_flipping);

DECLARE_MUTEX(sem_early_suspend);
static BOOL is_early_suspended = FALSE;

// ---------------------------------------------------------------------------
//  local function declarations
// ---------------------------------------------------------------------------

static int init_framebuffer(struct fb_info *info);
static int mtkfb_set_overlay_layer(struct fb_info *info,
                                   struct fb_overlay_layer* layerInfo);
static void mtkfb_update_screen_impl(void);


// ---------------------------------------------------------------------------
//  Timer Routines
// ---------------------------------------------------------------------------

static struct task_struct *screen_update_task = NULL;
static wait_queue_head_t screen_update_wq;

static int screen_update_kthread(void *data)
{
	struct sched_param param = { .sched_priority = RTPM_PRIO_SCRN_UPDATE };
	sched_setscheduler(current, SCHED_RR, &param);
    
    for( ;; ) {
        wait_event_interruptible(screen_update_wq,
                                 atomic_read(&has_pending_update));

        mtkfb_update_screen_impl();

        if (kthread_should_stop())
            break;
    }

    return 0;
}


static void XGPT_ISR(unsigned short input)
{
    wake_up_interruptible(&screen_update_wq);
}


// return out = a - b
void timeval_sub(struct timeval *out,
                 const struct timeval *a,
                 const struct timeval *b)
{
    out->tv_sec  = a->tv_sec - b->tv_sec;
    out->tv_usec = a->tv_usec - b->tv_usec;

    if (out->tv_usec < 0) {
        -- out->tv_sec;
        out->tv_usec += 1000000;
    }
}

// return if a > b
static __inline BOOL timeval_larger(const struct timeval *a, 
                                    const struct timeval *b)
{
    if (a->tv_sec > b->tv_sec) return TRUE;
    return (a->tv_usec > b->tv_usec) ? TRUE : FALSE;
}

// convert to 32KHz unit
static __inline time_t convert_to_32K_ticks(const struct timeval *x)
{
    return (x->tv_sec * 32000 + x->tv_usec * 32000 / 1000000);
}


static BOOL is_lcm_inited = FALSE;

void mtkfb_set_lcm_inited(BOOL inited)
{
    is_lcm_inited = inited;
}

/* Called each time the mtkfb device is opened */
static int mtkfb_open(struct fb_info *info, int user)
{
    NOT_REFERENCED(info);
    NOT_REFERENCED(user);
    
    MSG_FUNC_ENTER();
    MSG_FUNC_LEAVE();
    return 0;
}

static int mtkfb_release(struct fb_info *info, int user)
{
    NOT_REFERENCED(info);
    NOT_REFERENCED(user);

    MSG_FUNC_ENTER();
    MSG_FUNC_LEAVE();
    return 0;
}

static int mtkfb_setcolreg(u_int regno, u_int red, u_int green,
                           u_int blue, u_int transp,
                           struct fb_info *info)
{
    int r = 0;
    unsigned bpp, m;

    NOT_REFERENCED(transp);

    MSG_FUNC_ENTER();

    bpp = info->var.bits_per_pixel;
    m = 1 << bpp;
    if (regno >= m)
    {
        r = -EINVAL;
        goto exit;
    }

    switch (bpp)
    {
    case 16:
        /* RGB 565 */
        ((u32 *)(info->pseudo_palette))[regno] = 
            ((red & 0xF800) |
            ((green & 0xFC00) >> 5) |
            ((blue & 0xF800) >> 11));
        break;
    case 32:
        /* ARGB8888 */
        ((u32 *)(info->pseudo_palette))[regno] = 
             (0xff000000)           |
            ((red   & 0xFF00) << 8) |
            ((green & 0xFF00)     ) |
            ((blue  & 0xFF00) >> 8);
        break;

    // TODO: RGB888, BGR888, ABGR8888
    
    default:
        ASSERT(0);
    }

exit:
    MSG_FUNC_LEAVE();
    return r;
}


static int mtkfb_patch_android_abgr8888(struct mtkfb_device *fbdev)
{
    u32 i, hasBGR = 0;

    static const S2_8 RB_SWAP_MATRIX[9] = 
    {
        0, 0, (1 << 8),
        0, (1 << 8), 0,
        (1 << 8), 0, 0,
    };

    // (0) Check if any layer with "BGR" order color format
    
    for (i = 0; i < ARY_SIZE(fbdev->layer_format); ++ i)
    {
        if (!(fbdev->layer_enable & (1 << i))) continue;
        
        if (MTK_FB_FORMAT_BGR888   == fbdev->layer_format[i] ||
            MTK_FB_FORMAT_ABGR8888 == fbdev->layer_format[i])
        {
            hasBGR = 1;
        }
    }

    if (hasBGR) {

        // (1) Swap any RGB888 layer to BGR888
        
        for (i = 0; i < ARY_SIZE(fbdev->layer_format); ++ i)
        {
            if (!(fbdev->layer_enable & (1 << i))) continue;

            switch(fbdev->layer_format[i])
            {
            case MTK_FB_FORMAT_RGB888 :
                LCD_LayerEnableByteSwap(i, TRUE);
                break;
                
            case MTK_FB_FORMAT_RGB565 :
            case MTK_FB_FORMAT_ARGB8888 :
                printk(KERN_ERR "Android ABGR8888 patch failed, "
                       "layer %d format: %d\n", i, fbdev->layer_format[i]);
                ASSERT(0);
                break;
                
            default:
                LCD_LayerEnableByteSwap(i, FALSE);
            }
        }

        // (2) enable LCD color matrix to convert BGR back to RGB

        LCD_EnableColorMatrix(LCD_IF_ALL, TRUE);
        LCD_SetColorMatrix(RB_SWAP_MATRIX);

    } else {
        LCD_LayerEnableByteSwap(LCD_LAYER_ALL, FALSE);
        LCD_EnableColorMatrix(LCD_IF_ALL, FALSE);
    }

    return 0;
}


static void mtkfb_update_screen_impl(void)
{
    DISP_CHECK_RET(DISP_UpdateScreen(0, 0, MTK_FB_XRES, MTK_FB_YRES));

    do_gettimeofday(&last_update_time);
    atomic_set(&has_pending_update, 0);
}


static int mtkfb_update_screen(struct fb_info *info)
{
    struct mtkfb_device *fbdev = (struct mtkfb_device *)info->par;
    struct timeval current_time, diff;

    if (down_interruptible(&sem_early_suspend)) {
        printk("[FB Driver] can't get semaphore in mtkfb_update_screen()\n");
        return -ERESTARTSYS;
    }

    if (is_early_suspended) goto End;

    LCD_WaitForNotBusy();

    if (fbdev->layer_config_dirty)
    {
        mtkfb_patch_android_abgr8888(fbdev);
        fbdev->layer_config_dirty = 0;
    }

    // In order to merge each layer update request,
    // limit the screen update rate to 30 FPS and merge all requests during
    // every 33ms (only in direct-link mode)
    //
    if (atomic_read(&has_pending_update)) goto End;

    do_gettimeofday(&current_time);
    timeval_sub(&diff, &current_time, &last_update_time);

    if (!DISP_IsInOverlayMode() ||
        timeval_larger(&diff, &FRAME_INTERVAL)) {
        mtkfb_update_screen_impl();
    } else {
        // delay the screen update
        struct timeval delay;
        timeval_sub(&delay, &FRAME_INTERVAL, &diff);
        xgpt_cfg.u4Compare = convert_to_32K_ticks(&delay);
        if (!XGPT_Config(xgpt_cfg)) {
            printk("config XGPT failed!\n");
            return -EFAULT;
        }
        XGPT_ClearCount(xgpt_cfg.num);
        XGPT_Start(xgpt_cfg.num);
        atomic_set(&has_pending_update, 1);
    }

End:
    up(&sem_early_suspend);
    return 0;
}


static int mtkfb_pan_display_impl(struct fb_var_screeninfo *var, struct fb_info *info)
{
    UINT32 offset;
    UINT32 paStart;
    char *vaStart, *vaEnd;
    int ret = 0;

    MSG_FUNC_ENTER();

    MSG(ARGU, "xoffset=%u, yoffset=%u, xres=%u, yres=%u, xresv=%u, yresv=%u\n",
        var->xoffset, var->yoffset, 
        info->var.xres, info->var.yres,
        info->var.xres_virtual,
        info->var.yres_virtual);

    if (down_interruptible(&sem_flipping)) {
        printk("[FB Driver] can't get semaphore in mtkfb_pan_display_impl()\n");
        return -ERESTARTSYS;
    }

    info->var.yoffset = var->yoffset;

    offset = var->yoffset * info->fix.line_length;

    paStart = info->fix.smem_start + offset;
    vaStart = info->screen_base + offset;
    vaEnd   = vaStart + info->var.yres * info->fix.line_length;
    
	// force writeback the dcache lines of the back buffer
#if 1
	dmac_clean_range(vaStart, vaEnd);
#else
    flush_cache_all();
#endif

    LCD_WaitForNotBusy();

    DISP_CHECK_RET(DISP_SetFrameBufferAddr(paStart));

    ret = mtkfb_update_screen(info);

    up(&sem_flipping);

    return ret;
}


static int mtkfb_pan_display_proxy(struct fb_var_screeninfo *var, struct fb_info *info)
{
    return mtkfb_pan_display_impl(var, info);
}


static void set_fb_fix(struct mtkfb_device *fbdev)
{
    struct fb_info           *fbi   = fbdev->fb_info;
    struct fb_fix_screeninfo *fix   = &fbi->fix;
    struct fb_var_screeninfo *var   = &fbi->var;
    struct fb_ops            *fbops = fbi->fbops;

    strncpy(fix->id, MTKFB_DRIVER, sizeof(fix->id));
    fix->type = FB_TYPE_PACKED_PIXELS;

    switch (var->bits_per_pixel)
    {
    case 16:
    case 24:
    case 32:
        fix->visual = FB_VISUAL_TRUECOLOR;
        break;
    case 1:
    case 2:
    case 4:
    case 8:
        fix->visual = FB_VISUAL_PSEUDOCOLOR;
        break;
    default:
        ASSERT(0);
    }
    
    fix->accel       = FB_ACCEL_NONE;
    fix->line_length = var->xres_virtual * var->bits_per_pixel / 8;
    fix->smem_len    = fbdev->fb_size_in_byte;
    fix->smem_start  = fbdev->fb_pa_base;

    fix->xpanstep = 0;
    fix->ypanstep = 1;

    fbops->fb_fillrect  = cfb_fillrect;
    fbops->fb_copyarea  = cfb_copyarea;
    fbops->fb_imageblit = cfb_imageblit;
}


static int mtkfb_check_var(struct fb_var_screeninfo *var, struct fb_info *fbi)
{
    unsigned int bpp;
    unsigned long max_frame_size;
    unsigned long line_size;

    struct mtkfb_device *fbdev = (struct mtkfb_device *)fbi->par;

    MSG_FUNC_ENTER();

    MSG(ARGU, "xres=%u, yres=%u, xres_virtual=%u, yres_virtual=%u, "
              "xoffset=%u, yoffset=%u, bits_per_pixel=%u)\n",
        var->xres, var->yres, var->xres_virtual, var->yres_virtual,
        var->xoffset, var->yoffset, var->bits_per_pixel);

    bpp = var->bits_per_pixel;

    if (bpp != 16 && bpp != 24 && bpp != 32) {
        printk("[%s]unsupported bpp: %d", __func__, bpp);
        return -1;
    }

    switch (var->rotate) {
    case 0:
    case 180:
        var->xres = MTK_FB_XRES;
        var->yres = MTK_FB_YRES;
        break;
    case 90:
    case 270:
        var->xres = MTK_FB_YRES;
        var->yres = MTK_FB_XRES;
        break;
    default:
        return -1;
    }

    if (var->xres_virtual < var->xres)
        var->xres_virtual = var->xres;
    if (var->yres_virtual < var->yres)
        var->yres_virtual = var->yres;
    
    max_frame_size = fbdev->fb_size_in_byte;
    line_size = var->xres_virtual * bpp / 8;

    if (line_size * var->yres_virtual > max_frame_size) {
        /* Try to keep yres_virtual first */
        line_size = max_frame_size / var->yres_virtual;
        var->xres_virtual = line_size * 8 / bpp;
        if (var->xres_virtual < var->xres) {
            /* Still doesn't fit. Shrink yres_virtual too */
            var->xres_virtual = var->xres;
            line_size = var->xres * bpp / 8;
            var->yres_virtual = max_frame_size / line_size;
        }
    }
    if (var->xres + var->xoffset > var->xres_virtual)
        var->xoffset = var->xres_virtual - var->xres;
    if (var->yres + var->yoffset > var->yres_virtual)
        var->yoffset = var->yres_virtual - var->yres;

    if (16 == bpp) {
        var->red.offset    = 11;  var->red.length    = 5;
        var->green.offset  =  5;  var->green.length  = 6;
        var->blue.offset   =  0;  var->blue.length   = 5;
        var->transp.offset =  0;  var->transp.length = 0;
    }
    else if (24 == bpp)
    {
        var->red.length = var->green.length = var->blue.length = 8;
        var->transp.length = 0;

        // Check if format is RGB565 or BGR565
        
        ASSERT(8 == var->green.offset);
        ASSERT(16 == var->red.offset + var->blue.offset);
        ASSERT(16 == var->red.offset || 0 == var->red.offset);
    }
    else if (32 == bpp)
    {
        var->red.length = var->green.length = 
        var->blue.length = var->transp.length = 8;

        // Check if format is ARGB565 or ABGR565
        
        ASSERT(8 == var->green.offset && 24 == var->transp.offset);
        ASSERT(16 == var->red.offset + var->blue.offset);
        ASSERT(16 == var->red.offset || 0 == var->red.offset);
    }

    var->red.msb_right = var->green.msb_right = 
    var->blue.msb_right = var->transp.msb_right = 0;

    var->activate = FB_ACTIVATE_NOW;

    var->height    = UINT_MAX;
    var->width     = UINT_MAX;
    var->grayscale = 0;
    var->nonstd    = 0;

    var->pixclock     = UINT_MAX;
    var->left_margin  = UINT_MAX;
    var->right_margin = UINT_MAX;
    var->upper_margin = UINT_MAX;
    var->lower_margin = UINT_MAX;
    var->hsync_len    = UINT_MAX;
    var->vsync_len    = UINT_MAX;

    var->vmode = FB_VMODE_NONINTERLACED;
    var->sync  = 0;

    MSG_FUNC_LEAVE();
    return 0;
}


static int mtkfb_set_par(struct fb_info *fbi)
{
    struct fb_var_screeninfo *var = &fbi->var;
    struct mtkfb_device *fbdev = (struct mtkfb_device *)fbi->par;
    struct fb_overlay_layer fb_layer;
    u32 bpp = var->bits_per_pixel;

    MSG_FUNC_ENTER();

    switch(bpp)
    {
    case 16 :
        fb_layer.src_fmt = MTK_FB_FORMAT_RGB565;
        fb_layer.src_use_color_key = 1;
        break;

    case 24 :
        fb_layer.src_use_color_key = 1;
        fb_layer.src_fmt = (0 == var->blue.offset) ? 
                           MTK_FB_FORMAT_RGB888 :
                           MTK_FB_FORMAT_BGR888;
        break;
        
    case 32 :
        fb_layer.src_use_color_key = 0;
        fb_layer.src_fmt = (0 == var->blue.offset) ? 
                           MTK_FB_FORMAT_ARGB8888 :
                           MTK_FB_FORMAT_ABGR8888;
        break;

    default :
        fb_layer.src_fmt = MTK_FB_FORMAT_UNKNOWN;
        printk("[%s]unsupported bpp: %d", __func__, bpp);
        return -1;
    }

    // If the framebuffer format is NOT changed, nothing to do
    //
    if (fb_layer.src_fmt == fbdev->layer_format[FB_LAYER]) {
        goto Done;
    }

    // else, begin change display mode
    //    
    set_fb_fix(fbdev);

    fb_layer.layer_id = FB_LAYER;
    fb_layer.layer_enable = 1;
    fb_layer.src_base_addr = fbdev->fb_va_base;
    fb_layer.src_phy_addr = (void *)fbdev->fb_pa_base;
    fb_layer.src_direct_link = 0;
    fb_layer.src_offset_x = fb_layer.src_offset_y = 0;
    fb_layer.src_width = fb_layer.tgt_width = fb_layer.src_pitch = var->xres;
    fb_layer.src_height = fb_layer.tgt_height = var->yres;
    fb_layer.tgt_offset_x = fb_layer.tgt_offset_y = 0;

    fb_layer.src_color_key = 0;

    mtkfb_set_overlay_layer(fbi, &fb_layer);
    memset(fbi->screen_base, 0, fbi->screen_size);  // clear the whole VRAM as zero

Done:    
    MSG_FUNC_LEAVE();
    return 0;
}


static int mtkfb_soft_cursor(struct fb_info *info, struct fb_cursor *cursor)
{
    NOT_REFERENCED(info);
    NOT_REFERENCED(cursor);
    
    return 0;
}



static int mtkfb_set_overlay_layer(struct fb_info *info, struct fb_overlay_layer* layerInfo)
{
    struct mtkfb_device *fbdev = (struct mtkfb_device *)info->par;
    
    unsigned int u4OvlPhyAddr;
	LCD_LAYER_FORMAT eFormat;
    unsigned int id = layerInfo->layer_id;
    int enable = layerInfo->layer_enable ? 1 : 0;
    int ret = 0;
    
    MSG_FUNC_ENTER();

    if (down_interruptible(&sem_early_suspend)) {
        printk("[FB Driver] can't get semaphore in mtkfb_set_overlay_layer()\n");
        return -ERESTARTSYS;
    }

    /** LCD registers can't be R/W when its clock is gated in early suspend
        mode; power on/off LCD to modify register values before/after func.
    */
    if (is_early_suspended) {
        LCD_CHECK_RET(LCD_PowerOn());
    }

    MSG(ARGU, "[FB Driver] mtkfb_set_overlay_layer():layer id = %u, layer en = %u, src format = %u, direct link: %u, src vir addr = %u, src phy addr = %u, src pitch=%u, src xoff=%u, src yoff=%u, src w=%u, src h=%u\n",
        layerInfo->layer_id,
        layerInfo->layer_enable, 
        layerInfo->src_fmt,
        (unsigned int)(layerInfo->src_direct_link),
        (unsigned int)(layerInfo->src_base_addr),
        (unsigned int)(layerInfo->src_phy_addr),
        layerInfo->src_pitch,
        layerInfo->src_offset_x,
        layerInfo->src_offset_y,
        layerInfo->src_width,
        layerInfo->src_height);
    MSG(ARGU, "[FB Driver] mtkfb_set_overlay_layer():target xoff=%u, target yoff=%u, target w=%u, target h=%u\n",
        layerInfo->tgt_offset_x,
        layerInfo->tgt_offset_y, 
        layerInfo->tgt_width,
        layerInfo->tgt_height);

    u4OvlPhyAddr = (unsigned int)(layerInfo->src_phy_addr);

    LCD_WaitForNotBusy();

    // Update Layer Enable Bits and Layer Config Dirty Bits

    if ((((fbdev->layer_enable >> id) & 1) ^ enable)) {
        fbdev->layer_enable ^= (1 << id);
        fbdev->layer_config_dirty |= MTKFB_LAYER_ENABLE_DIRTY;
    }

    // Update Layer Format and Layer Config Dirty Bits

    if (fbdev->layer_format[id] != layerInfo->src_fmt) {
        fbdev->layer_format[id] = layerInfo->src_fmt;
        fbdev->layer_config_dirty |= MTKFB_LAYER_FORMAT_DIRTY;
    }

    // Enter Overlay Mode if any layer is enabled except the FB layer

    if (fbdev->layer_enable & ~(1 << FB_LAYER)) {
        if (DISP_STATUS_OK == DISP_EnterOverlayMode()) {
            printk("mtkfb_ioctl(MTKFB_ENABLE_OVERLAY)\n");
        }
    }
    
    if (!enable || !layerInfo->src_direct_link) {
        DISP_DisableDirectLinkMode(id);
    }

    // set layer enable
    LCD_CHECK_RET(LCD_LayerEnable(id, enable));

    if (!enable) {
        ret = 0;
        goto LeaveOverlayMode;
    }

    if (layerInfo->src_direct_link) {
        DISP_EnableDirectLinkMode(id);
    }

    switch (layerInfo->src_fmt)
    {
    case MTK_FB_FORMAT_RGB565:
        eFormat = LCD_LAYER_FORMAT_RGB565;
        break;

    case MTK_FB_FORMAT_RGB888:
    case MTK_FB_FORMAT_BGR888:
        eFormat = LCD_LAYER_FORMAT_RGB888;
        break;

    case MTK_FB_FORMAT_ARGB8888:
    case MTK_FB_FORMAT_ABGR8888:
        eFormat = LCD_LAYER_FORMAT_ARGB8888;
        break;

    default:
        PRNERR("Invalid color format: 0x%x\n", layerInfo->src_fmt);
        ret = -EFAULT;
        goto LeaveOverlayMode;
    }

    LCD_CHECK_RET(LCD_LayerSetAddress(id, u4OvlPhyAddr));
    LCD_CHECK_RET(LCD_LayerSetFormat(id, eFormat));

    //set Alpha blending
	if (MTK_FB_FORMAT_ARGB8888 == layerInfo->src_fmt ||
        MTK_FB_FORMAT_ABGR8888 == layerInfo->src_fmt)
    {
        LCD_CHECK_RET(LCD_LayerSetAlphaBlending(id, TRUE, 0xFF));
	} else {
		LCD_CHECK_RET(LCD_LayerSetAlphaBlending(id, FALSE, 0xFF));
	}

    //set line pitch.
    //mt6516 hw do not support line pitch    
    ASSERT((layerInfo->src_pitch) == (layerInfo->src_width));

    //set src x, y offset
    //mt65616 do not support source x ,y offset
    ASSERT((layerInfo->src_offset_x) == 0);
    ASSERT((layerInfo->src_offset_y) == 0);

    //set src width, src height
    LCD_CHECK_RET(LCD_LayerSetSize(id, layerInfo->src_width, layerInfo->src_height));

    //set target x, y offset
    LCD_CHECK_RET(LCD_LayerSetOffset(id, layerInfo->tgt_offset_x, layerInfo->tgt_offset_y));

    //set target width, height
    //mt6516: target w/h must = sourc width/height
    ASSERT((layerInfo->src_width) == (layerInfo->tgt_width));
    ASSERT((layerInfo->src_height) == (layerInfo->tgt_height));
    
    //set color key
    LCD_CHECK_RET(LCD_LayerSetSourceColorKey(id, layerInfo->src_use_color_key, layerInfo->src_color_key));

    //data transferring is triggerred in MTKFB_TRIG_OVERLAY_OUT

	// force writing cached data into dram
	dmac_clean_range(layerInfo->src_base_addr, 
                     layerInfo->src_base_addr + 
                     (layerInfo->src_pitch * layerInfo->src_height *
                     GET_MTK_FB_FORMAT_BPP(layerInfo->src_fmt)));

LeaveOverlayMode:
    // Leave Overlay Mode if only FB layer is enabled

    if ((fbdev->layer_enable & ~(1 << FB_LAYER)) == 0) {
        if (DISP_STATUS_OK == DISP_LeaveOverlayMode()) {
            printk("mtkfb_ioctl(MTKFB_DISABLE_OVERLAY)\n");
        }
    }

    if (is_early_suspended) {
        LCD_CHECK_RET(LCD_PowerOff());
    }

    up(&sem_early_suspend);

    MSG_FUNC_LEAVE();

    return ret;
}


static int mtkfb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
    void __user *argp = (void __user *)arg;
    struct mtkfb_device *fbdev = (struct mtkfb_device *)info->par;
    DISP_STATUS ret;
    int r = 0;

    switch (cmd) {
    case MTKFB_GETVFRAMEPHYSICAL:
        return copy_to_user(argp, &fbdev->fb_pa_base,
                            sizeof(fbdev->fb_pa_base)) ? -EFAULT : 0;

#ifdef MTK_FB_OVERLAY_SUPPORT
    case MTKFB_SET_OVERLAY_LAYER:
    {
        struct fb_overlay_layer layerInfo;
        MSG(ARGU, " mtkfb_ioctl():MTKFB_SET_OVERLAY_LAYER\n");

        if (copy_from_user(&layerInfo, (void __user *)arg, sizeof(layerInfo))) {
            printk("[FB]: copy_from_user failed! line:%d \n", __LINE__);
            r = -EFAULT;
        } else {
            mtkfb_set_overlay_layer(info, &layerInfo);
        }

        return (r);
    }

    case MTKFB_SET_VIDEO_LAYERS:
    {
        struct fb_overlay_layer layerInfo[VIDEO_LAYER_COUNT];
        MSG(ARGU, " mtkfb_ioctl():MTKFB_SET_VIDEO_LAYERS\n");

        if (copy_from_user(&layerInfo, (void __user *)arg, sizeof(layerInfo))) {
            printk("[FB]: copy_from_user failed! line:%d \n", __LINE__);
            r = -EFAULT;
        } else {
            int32_t i;
            for (i = 0; i < VIDEO_LAYER_COUNT; ++i) {
                mtkfb_set_overlay_layer(info, &layerInfo[i]);
            }
        }

        return (r);
    }

    case MTKFB_WAIT_OVERLAY_READY:
        ret = LCD_WaitForNotBusy();
        ASSERT(LCD_STATUS_OK == ret);
        r = 0;
        return (r);

    case MTKFB_GET_OVERLAY_LAYER_COUNT:
    {
        int hw_layer_count = HW_OVERLAY_COUNT; 
        if (copy_to_user((void __user *)arg, &hw_layer_count, sizeof(hw_layer_count)))
        {
            return -EFAULT;
        }
        return 0;
    }

    case MTKFB_TRIG_OVERLAY_OUT:
        return mtkfb_update_screen(info);

#endif // MTK_FB_OVERLAY_SUPPORT

    case MTKFB_META_RESTORE_SCREEN:
    {
        struct fb_var_screeninfo var;

        // This ioctl code is only used in META test mode
        ASSERT(is_meta_mode());

		if (copy_from_user(&var, argp, sizeof(var)))
			return -EFAULT;

        info->var.yoffset = var.yoffset;
        init_framebuffer(info);

        return mtkfb_pan_display_impl(&var, info);
    }

    case MTKFB_LOCK_FRONT_BUFFER:
        if (down_interruptible(&sem_flipping)) {
            printk("[FB Driver] can't get semaphore when lock front buffer\n");
            return -ERESTARTSYS;
        }
        return 0;

    case MTKFB_UNLOCK_FRONT_BUFFER:
        up(&sem_flipping);
        return 0;

    default:
        return -EINVAL;
    }
}


static struct fb_ops mtkfb_ops = {
    .owner          = THIS_MODULE,
    .fb_open        = mtkfb_open,
    .fb_release     = mtkfb_release,
    .fb_setcolreg   = mtkfb_setcolreg,
    .fb_pan_display = mtkfb_pan_display_proxy,
    .fb_fillrect    = cfb_fillrect,
    .fb_copyarea    = cfb_copyarea,
    .fb_imageblit   = cfb_imageblit,
    .fb_cursor      = mtkfb_soft_cursor,
    .fb_check_var   = mtkfb_check_var,
    .fb_set_par     = mtkfb_set_par,
    .fb_ioctl       = mtkfb_ioctl,
};


static int mtkfb_register_sysfs(struct mtkfb_device *fbdev)
{
    NOT_REFERENCED(fbdev);

    return 0;
}

static void mtkfb_unregister_sysfs(struct mtkfb_device *fbdev)
{
    NOT_REFERENCED(fbdev);
}

static int mtkfb_fbinfo_init(struct fb_info *info)
{
    struct mtkfb_device *fbdev = (struct mtkfb_device *)info->par;
    struct fb_var_screeninfo var;
    int r = 0;

    MSG_FUNC_ENTER();

    BUG_ON(!fbdev->fb_va_base);
    info->fbops = &mtkfb_ops;
    info->flags = FBINFO_FLAG_DEFAULT;
    info->screen_base = (char *) fbdev->fb_va_base;
    info->screen_size = fbdev->fb_size_in_byte;
    info->pseudo_palette = fbdev->pseudo_palette;

    r = fb_alloc_cmap(&info->cmap, 16, 0);
    if (r != 0)
        PRNERR("unable to allocate color map memory\n");

    // setup the initial video mode (RGB565)

    memset(&var, 0, sizeof(var));
    
    var.xres         = MTK_FB_XRES;
    var.yres         = MTK_FB_YRES;
    var.xres_virtual = MTK_FB_XRESV;
    var.yres_virtual = MTK_FB_YRESV;

    var.bits_per_pixel = 16;

    var.red.offset   = 11; var.red.length   = 5;
    var.green.offset =  5; var.green.length = 6;
    var.blue.offset  =  0; var.blue.length  = 5;

    var.activate = FB_ACTIVATE_NOW;

    r = mtkfb_check_var(&var, info);
    if (r != 0)
        PRNERR("failed to mtkfb_check_var\n");

    info->var = var;

    r = mtkfb_set_par(info);
    if (r != 0)
        PRNERR("failed to mtkfb_set_par\n");

    MSG_FUNC_LEAVE();
    return r;
}

/* Release the fb_info object */
static void mtkfb_fbinfo_cleanup(struct mtkfb_device *fbdev)
{
    MSG_FUNC_ENTER();

    fb_dealloc_cmap(&fbdev->fb_info->cmap);

    MSG_FUNC_LEAVE();
}


#if INIT_FB_AS_COLOR_BAR
static void fill_color(u8 *buffer,
                       u32 fillColor,
                       u8  bpp,
                       u32 linePitchInPixels,
                       u32 startX,
                       u32 startY,
                       u32 fillWidth,
                       u32 fillHeight)
{
    u32 linePitchInBytes = linePitchInPixels * bpp;
    u8 *linePtr = buffer + startY * linePitchInBytes + startX * bpp;
    s32 h;
    u32 i;
    u8 color[4];

    for(i = 0; i < bpp; ++ i)
    {
        color[i] = fillColor & 0xFF;
        fillColor >>= 8;
    }

    h = (s32)fillHeight;
    while (--h >= 0)
    {
        u8 *ptr = linePtr;
        s32 w = (s32)fillWidth;
        while (--w >= 0)
        {
            memcpy(ptr, color, bpp);
            ptr += bpp;
        }
        linePtr += linePitchInBytes;
    }
}
#endif

#define RGB565_TO_ARGB8888(x)   \
    ((((x) &   0x1F) << 3) |    \
     (((x) &  0x7E0) << 5) |    \
     (((x) & 0xF800) << 8) |    \
     (0xFF << 24)) // opaque

static void copy_rect(u8 *dstBuffer,
                      const u8 *srcBuffer,
                      u8  dstBpp,
                      u8  srcBpp,
                      u32 dstPitchInPixels,
                      u32 srcPitchInPixels,
                      u32 dstX,
                      u32 dstY,
                      u32 copyWidth,
                      u32 copyHeight)
{
    u32 dstPitchInBytes  = dstPitchInPixels * dstBpp;
    u32 srcPitchInBytes  = srcPitchInPixels * srcBpp;
    u8 *dstPtr = dstBuffer + dstY * dstPitchInBytes + dstX * dstBpp;
    const u8 *srcPtr = srcBuffer;
    s32 h = (s32)copyHeight;
    
    if (dstBpp == srcBpp)
    {
        u32 copyBytesPerLine = copyWidth * dstBpp;
        
        while (--h >= 0)
        {
            memcpy(dstPtr, srcPtr, copyBytesPerLine);
            dstPtr += dstPitchInBytes;
            srcPtr += srcPitchInBytes;
        }
    }
    else if (srcBpp == 2 && dstBpp == 4)    // RGB565 copy to ARGB8888
    {
        while (--h >= 0)
        {
            const u16 *s = (const u16 *)srcPtr;
            u32 *d = (u32 *)dstPtr;
            s32 w = (s32)copyWidth;

            while (--w >= 0)
            {
                u16 rgb565 = *s;
                *d = RGB565_TO_ARGB8888(rgb565);
                ++ d; ++ s;
            }
            
            dstPtr += dstPitchInBytes;
            srcPtr += srcPitchInBytes;
        }
    }
    else
    {
        printk("un-supported bpp in copy_rect(), srcBpp: %d --> dstBpp: %d\n",
               srcBpp, dstBpp);

        ASSERT(0);
    }
}


/* Init frame buffer content as 3 R/G/B color bars for debug */
static int init_framebuffer(struct fb_info *info)
{
    void *buffer = info->screen_base + 
                   info->var.yoffset * info->fix.line_length;

    u32 bpp = (info->var.bits_per_pixel + 7) >> 3;

#if INIT_FB_AS_COLOR_BAR
    int i;
    
    u32 colorRGB565[] =
    {
        0xffff, // White
        0xf800, // Red
        0x07e0, // Green
        0x001f, // Blue
    };

    u32 xSteps[ARY_SIZE(colorRGB565) + 1];

    xSteps[0] = 0;
    xSteps[1] = info->var.xres / 4 * 1;
    xSteps[2] = info->var.xres / 4 * 2;
    xSteps[3] = info->var.xres / 4 * 3;
    xSteps[4] = info->var.xres;

    for(i = 0; i < ARY_SIZE(colorRGB565); ++ i)
    {
        fill_color(buffer,
                   colorRGB565[i],
                   bpp,
                   info->var.xres,
                   xSteps[i],
                   0,
                   xSteps[i+1] - xSteps[i],
                   info->var.yres);
    }
#else
    // clean whole frame buffer as black
    memset(buffer, 0, info->var.xres * info->var.yres * bpp);

    if (is_meta_mode())
    {
        MFC_HANDLE handle = NULL;
        MFC_CHECK_RET(MFC_Open(&handle, buffer,
                               info->var.xres, info->var.yres,
                               bpp, 0xFFFF, 0x0));
        MFC_CHECK_RET(MFC_Print(handle, "<< META Test Mode >>\n"));
        MFC_CHECK_RET(MFC_Close(handle));
    }
#endif
    return 0;
}


static void mtkfb_free_resources(struct mtkfb_device *fbdev, int state)
{
    int r = 0;
    
    switch (state) {
    case MTKFB_ACTIVE:
        r = unregister_framebuffer(fbdev->fb_info);
        ASSERT(0 == r);
      //lint -fallthrough
    case 5:
        mtkfb_unregister_sysfs(fbdev);
      //lint -fallthrough
    case 4:
        mtkfb_fbinfo_cleanup(fbdev);
      //lint -fallthrough
    case 3:
        DISP_CHECK_RET(DISP_Deinit());
      //lint -fallthrough
    case 2:
        dma_free_coherent(0, fbdev->fb_size_in_byte,
                          fbdev->fb_va_base, fbdev->fb_pa_base);
      //lint -fallthrough
    case 1:
        dev_set_drvdata(fbdev->dev, NULL);
        framebuffer_release(fbdev->fb_info);
      //lint -fallthrough
    case 0:
      /* nothing to free */
        break;
    default:
        BUG();
    }
}


static int mtkfb_probe(struct device *dev)
{
    struct platform_device *pdev;
    struct mtkfb_device    *fbdev = NULL;
    struct fb_info         *fbi;
    int                    init_state;
    int                    r = 0;

    MSG_FUNC_ENTER();

    init_state = 0;

    pdev = to_platform_device(dev);
    if (pdev->num_resources != 1) {
        PRNERR("probed for an unknown device\n");
        r = -ENODEV;
        goto cleanup;
    }

    fbi = framebuffer_alloc(sizeof(struct mtkfb_device), dev);
    if (!fbi) {
        PRNERR("unable to allocate memory for device info\n");
        r = -ENOMEM;
        goto cleanup;
    }

    fbdev = (struct mtkfb_device *)fbi->par;
    fbdev->fb_info = fbi;
    fbdev->dev = dev;
    dev_set_drvdata(dev, fbdev);

    init_state++;   // 1

    /* Allocate and initialize video frame buffer */
    
    fbdev->fb_size_in_byte = MTK_FB_SIZEV;
    {
        struct resource *res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
        fbdev->fb_pa_base = res->start;
        fbdev->fb_va_base = ioremap_nocache(res->start, res->end - res->start + 1);
        ASSERT(DISP_GetVRamSize() <= (res->end - res->start + 1));
        //memset(fbdev->fb_va_base, 0, (res->end - res->start + 1));
    }

	printk("[FB Driver] fbdev->fb_pa_base = %x\n", fbdev->fb_pa_base);

    if (!fbdev->fb_va_base) {
        PRNERR("unable to allocate memory for frame buffer\n");
        r = -ENOMEM;
        goto cleanup;
    }

    init_state++;   // 2

    /* Initialize Display Driver PDD Layer */

    if (DISP_STATUS_OK != DISP_Init((DWORD)fbdev->fb_va_base,
                                    (DWORD)fbdev->fb_pa_base,
                                    is_lcm_inited))
    {
        r = -1;
        goto cleanup;
    }

    init_state++;   // 3

    /* Register to system */

    r = mtkfb_fbinfo_init(fbi);
    if (r)
        goto cleanup;
    init_state++;   // 4

    r = mtkfb_register_sysfs(fbdev);
    if (r)
        goto cleanup;
    init_state++;   // 5

    r = register_framebuffer(fbi);
    if (r != 0) {
        PRNERR("register_framebuffer failed\n");
        goto cleanup;
    }

    fbdev->state = MTKFB_ACTIVE;

    MSG(INFO, "MTK framebuffer initialized vram=%lu\n", fbdev->fb_size_in_byte);

    MSG_FUNC_LEAVE();
    return 0;

cleanup:
    mtkfb_free_resources(fbdev, init_state);

    MSG_FUNC_LEAVE();
    return r;
}

/* Called when the device is being detached from the driver */
static int mtkfb_remove(struct device *dev)
{
    struct mtkfb_device *fbdev = dev_get_drvdata(dev);
    enum mtkfb_state saved_state = fbdev->state;

    MSG_FUNC_ENTER();
    /* FIXME: wait till completion of pending events */

    fbdev->state = MTKFB_DISABLED;
    mtkfb_free_resources(fbdev, saved_state);

    MSG_FUNC_LEAVE();
    return 0;
}

/* PM suspend */
static int mtkfb_suspend(struct device *pdev, pm_message_t mesg)
{
    NOT_REFERENCED(pdev);
    MSG_FUNC_ENTER();
    printk("[FB Driver] mtkfb_suspend(): 0x%x\n", mesg.event);
    MSG_FUNC_LEAVE();
    return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void mtkfb_early_suspend(struct early_suspend *h)
{
    MSG_FUNC_ENTER();

    printk("[FB Driver] enter early_suspend\n");
    
    if (down_interruptible(&sem_early_suspend)) {
        printk("[FB Driver] can't get semaphore in mtkfb_early_suspend()\n");
        return;
    }

    is_early_suspended = TRUE;
    DISP_CHECK_RET(DISP_PanelEnable(FALSE));
    DISP_CHECK_RET(DISP_PowerEnable(FALSE));

    up(&sem_early_suspend);

    printk("[FB Driver] leave early_suspend\n");

    MSG_FUNC_LEAVE();
}
#endif

/* PM resume */
static int mtkfb_resume(struct device *pdev)
{
    NOT_REFERENCED(pdev);
    MSG_FUNC_ENTER();
    printk("[FB Driver] mtkfb_resume()\n");
    MSG_FUNC_LEAVE();
    return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void mtkfb_late_resume(struct early_suspend *h)
{
    MSG_FUNC_ENTER();

    printk("[FB Driver] enter late_resume\n");

    if (down_interruptible(&sem_early_suspend)) {
        printk("[FB Driver] can't get semaphore in mtkfb_late_resume()\n");
        return;
    }

    DISP_CHECK_RET(DISP_PowerEnable(TRUE));
    DISP_CHECK_RET(DISP_PanelEnable(TRUE));
    is_early_suspended = FALSE;

    up(&sem_early_suspend);

    printk("[FB Driver] leave late_resume\n");
    
    MSG_FUNC_LEAVE();
}
#endif


static struct platform_driver mtkfb_driver = 
{
    .driver = {
        .name    = MTKFB_DRIVER,
        .bus     = &platform_bus_type,
        .probe   = mtkfb_probe,
        .remove  = mtkfb_remove,    
        .suspend = mtkfb_suspend,
        .resume  = mtkfb_resume,
    },    
};

#ifdef CONFIG_HAS_EARLYSUSPEND
static struct early_suspend mtkfb_early_suspend_handler = 
{
	.level = EARLY_SUSPEND_LEVEL_DISABLE_FB,
	.suspend = mtkfb_early_suspend,
	.resume = mtkfb_late_resume,
};
#endif

/* Register both the driver and the device */
int __init mtkfb_init(void)
{
    int r = 0;

    MSG_FUNC_ENTER();

    MTK_FB_XRES  = DISP_GetScreenWidth();
    MTK_FB_YRES  = DISP_GetScreenHeight();
    MTK_FB_BPP   = DISP_GetScreenBpp();
    MTK_FB_PAGES = DISP_GetPages();

    /* Register the driver with LDM */

    if (platform_driver_register(&mtkfb_driver)) {
        PRNERR("failed to register mtkfb driver\n");
        r = -ENODEV;
        goto exit;
    }

    XGPT_Init(xgpt_cfg.num, XGPT_ISR);

    init_waitqueue_head(&screen_update_wq);

    screen_update_task = kthread_create(
        screen_update_kthread, NULL, "screen_update_kthread");

    if (IS_ERR(screen_update_task)) {
        return PTR_ERR(screen_update_task);
    }
    wake_up_process(screen_update_task);

#ifdef CONFIG_HAS_EARLYSUSPEND
   	register_early_suspend(&mtkfb_early_suspend_handler);
#endif

    DBG_Init();

exit:
    MSG_FUNC_LEAVE();
    return r;
}


static void __exit mtkfb_cleanup(void)
{
    MSG_FUNC_ENTER();

    platform_driver_unregister(&mtkfb_driver);

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&mtkfb_early_suspend_handler);
#endif

    kthread_stop(screen_update_task);

    DBG_Deinit();

    MSG_FUNC_LEAVE();
}


module_init(mtkfb_init);
module_exit(mtkfb_cleanup);

MODULE_DESCRIPTION("MEDIATEK MT6516 framebuffer driver");
MODULE_AUTHOR("Jett Liu <jett.liu@mediatek.com>");
MODULE_LICENSE("GPL");
