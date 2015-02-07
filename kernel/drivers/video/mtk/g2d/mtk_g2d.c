
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>

#include <linux/dma-mapping.h>
#include <linux/mm_types.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/sched.h>
#include <asm/uaccess.h>

#include <asm/current.h>
#include <asm/pgtable.h>
#include <asm/page.h>

#include <mach/mt6516_typedefs.h>

#include "g2d_drv.h"
#include "mtk_g2d.h"


// ---------------------------------------------------------------------------

static dev_t m2d_devno;
static struct cdev *m2d_cdev = NULL;
static struct class *m2d_class = NULL;

static DECLARE_MUTEX(m2dIoctl);

// ---------------------------------------------------------------------------

#if 0
static unsigned long v2p_ver2(unsigned long va)
{
    unsigned long pageStart = (va & PAGE_MASK);
    unsigned long pageOffset = (va & (PAGE_SIZE - 1));
    struct page *pPage = NULL;
    unsigned long pa = 0;
    int ret = 0;

    down_read(&current->mm->mmap_sem);
    ret = get_user_pages(current, current->mm,
                         pageStart,
                         1,     // size in page
                         0,     // write
                         0,     // force
                         &pPage,
                         NULL);
    up_read(&current->mm->mmap_sem);

    if (ret < 1) {
        printk("get_user_pages failed, ret: 0x%x\n", ret);
    } else {
        pa = page_to_phys(pPage) | pageOffset;
    }

    page_cache_release(pPage);

    return pa;
}
#endif

static unsigned long v2p(unsigned long va)
{
    unsigned long pageOffset = (va & (PAGE_SIZE - 1));
    pgd_t *pgd;
    pmd_t *pmd;
    pte_t *pte;
    unsigned long pa;

    pgd = pgd_offset(current->mm, va); /* what is tsk->mm */
    pmd = pmd_offset(pgd, va);
    pte = pte_offset_map(pmd, va);
    
    pa = (pte_val(*pte) & (PAGE_MASK)) | pageOffset;

#if 0
    printk("virtual to physical (ver1): 0x%lx --> 0x%lx\n", va, pa);
    printk("virtual to physical (ver2): 0x%lx --> 0x%lx\n", va, v2p_ver2(va));
#endif

    return pa;
}


static __inline unsigned int _m2d_color_format_to_bpp(M2D_COLOR_FORMAT fmt)
{
    switch(fmt)
    {
    case M2D_COLOR_ARGB8888 : return 4;
    case M2D_COLOR_RGB565   : return 2;
    case M2D_COLOR_RGB888   : return 3;
    case M2D_COLOR_ARGB4444 : return 2;
    case M2D_COLOR_8BPP     : return 1;
    default : 
        printk(KERN_ERR "unkown M2D_COLOR_FORMAT 0x%x", fmt);
        ASSERT(0);
    }
}


static __inline void flush_cache_m2d_surface(const m2d_surface *surf,
                                             int withInvalidate)
{
    unsigned int bpp = _m2d_color_format_to_bpp(surf->format);

    unsigned int start = surf->virtAddr + 
                         surf->rect.y * surf->pitchInPixels +
                         surf->rect.x * bpp;

    unsigned int length = surf->rect.height * surf->pitchInPixels +
                          surf->rect.width * bpp;

#if 0
    flush_cache_range(find_vma(current->mm, start),
                      (unsigned long)start,
                      (unsigned long)(start + length));
#else
    if (withInvalidate) {
        // writeback and invalidate surface rectangle cache lines
        dmac_flush_range((const void *)start, (const void *)(start + length));
    } else {
        // writeback surface rectangle cache lines
        dmac_clean_range((const void *)start, (const void *)(start + length));
    }
#endif
}


static void _G2D_Bitblt(const m2d_bitblt *bitblt)
{
    const m2d_surface *dst = &bitblt->dstSurf;
    const m2d_surface *src = &bitblt->srcSurf;

    G2D_CHECK_RET(G2D_PowerOn());

    G2D_CHECK_RET(G2D_EnableClipping(FALSE));
    G2D_CHECK_RET(G2D_SetStretchBitblt(FALSE, 0, 0));

    G2D_CHECK_RET(G2D_SetRotateMode((G2D_ROTATE)bitblt->rotate));

    G2D_CHECK_RET(G2D_SetDstBuffer((u8*)v2p(dst->virtAddr),
                  dst->pitchInPixels, 0,
                  (G2D_COLOR_FORMAT)dst->format));
    
    G2D_CHECK_RET(G2D_SetDstRect(dst->rect.x, dst->rect.y,
                                 dst->rect.width, dst->rect.height));

    G2D_CHECK_RET(G2D_SetSrcBuffer((u8*)v2p(src->virtAddr),
                                   src->pitchInPixels, 0,
                                   (G2D_COLOR_FORMAT)src->format));
    
    G2D_CHECK_RET(G2D_SetSrcRect(src->rect.x, src->rect.y,
                                 src->rect.width, src->rect.height));

#if 0
    flush_cache_m2d_surface(dst, 1);
    flush_cache_m2d_surface(src, 0);
#else
    /** 
        Due to MT6516(ARM9) is VIVT cache, it's better to flush all
        cache lines before trigger HW G2D to prevent from data coherence
        problem.
    */
    flush_cache_all();
#endif

    if (bitblt->enAlphaBlending) {
        G2D_CHECK_RET(G2D_StartAlphaBitblt(bitblt->constAlphaValue,
                                           bitblt->enPremultipliedAlpha));
    } else {
        G2D_CHECK_RET(G2D_StartBitblt());
    }

    G2D_CHECK_RET(G2D_WaitForNotBusy());

    G2D_CHECK_RET(G2D_PowerOff());
}

// ---------------------------------------------------------------------------

static int m2d_ioctl(struct inode *inode,
                     struct file *file,
                     unsigned int cmd,
                     unsigned long arg)
{
    int ret;

    if (down_interruptible(&m2dIoctl))
        return -ERESTARTSYS;

    switch(cmd)
    {
    case M2D_IOC_BITBLT :
    {
        m2d_bitblt bitblt;
        ret = copy_from_user(&bitblt, (uint *)arg, sizeof(m2d_bitblt));
        _G2D_Bitblt(&bitblt);        
        break;
    }

    default:
        printk(KERN_ERR "unkown M2D ioctl code (0x%x)", cmd);
        break;
    }

    up(&m2dIoctl);
    
    return 0;
}


static struct file_operations m2d_fops = {
    .owner = THIS_MODULE,
    .ioctl = m2d_ioctl,
};


static int m2d_probe(struct platform_device *dev)
{
    int ret;
    struct class_device *class_dev = NULL;

	printk(KERN_INFO "m2d_probe() is called\n");

    ret = alloc_chrdev_region(&m2d_devno, 0, 1, M2D_DEV_NAME);

    if (ret)
        printk(KERN_ERR "Failed to allocate dev_no for MTK G2D Device\n");
    else
        printk("MTK G2D Device NO: (%d:%d)\n", MAJOR(m2d_devno), MINOR(m2d_devno));

    m2d_cdev = cdev_alloc();
    m2d_cdev->owner = THIS_MODULE;
    m2d_cdev->ops = &m2d_fops;

    ret = cdev_add(m2d_cdev, m2d_devno, 1);

    m2d_class = class_create(THIS_MODULE, M2D_DEV_NAME);
    class_dev = (struct class_device *)device_create(m2d_class,
                                                     NULL,
                                                     m2d_devno,
                                                     NULL,
                                                     M2D_DEV_NAME);
    return 0;
}


static int m2d_remove(struct platform_device *dev)
{
	printk(KERN_INFO "m2d_remove() is called\n");

    device_destroy(m2d_class, m2d_devno);
    class_destroy(m2d_class);
    cdev_del(m2d_cdev);
    unregister_chrdev_region(m2d_devno, 1);

    return 0;
}


static void m2d_shutdown(struct platform_device *dev)
{
	printk(KERN_INFO "m2d_shutdown() is called\n");
}


static int m2d_suspend(struct platform_device *dev, pm_message_t state)
{    
	printk(KERN_INFO "m2d_suspend() is called\n");
    return 0;
}


static int m2d_resume(struct platform_device *dev)
{   
	printk(KERN_INFO "m2d_resume() is called\n");
    return 0;
}


static struct platform_device m2d_device = {
    .name = M2D_DEV_NAME,
    .id   = 0,
};


static struct platform_driver m2d_driver = {
    .probe      = m2d_probe,
    .remove     = m2d_remove,
    .shutdown   = m2d_shutdown,
    .suspend    = m2d_suspend,
    .resume     = m2d_resume,
    .driver     = {
        .name = M2D_DEV_NAME,
    },
};


static int __init m2d_init(void)
{
	printk(KERN_INFO "m2d_init() is called\n");

	printk("----------- Initializing MTK G2D Driver -----------\n");

    if (platform_device_register(&m2d_device)) 
    {
        printk(KERN_ERR "Failed to register MTK G2D Device\n");
        return -ENODEV;
    }

    if (platform_driver_register(&m2d_driver)) 
    {
        printk(KERN_ERR "Failed to register MTK G2D Driver\n");
        platform_device_unregister(&m2d_device);
        return -ENODEV;
    }

    G2D_CHECK_RET(G2D_Init());

    return 0;
}


static void __exit m2d_exit(void)
{
	printk(KERN_INFO "m2d_exit() is called\n");

    platform_driver_unregister(&m2d_driver);
    platform_device_unregister(&m2d_device);
}


module_init(m2d_init);
module_exit(m2d_exit);

MODULE_AUTHOR("Jett, Liu <jett.liu@mediatek.com>");
MODULE_DESCRIPTION("MTK G2D Driver");
MODULE_LICENSE("GPL");

