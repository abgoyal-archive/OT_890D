

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/mm_types.h>
#include <linux/mm.h>
#include <linux/jiffies.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <asm/page.h>
#include <linux/vmalloc.h>
#include <mach/mt6516_pll.h>
#include <mach/mt6516_typedefs.h> 
#include <mach/mt6516_reg_base.h> 
#include <linux/interrupt.h>
#include <mach/irqs.h>
#include <linux/wait.h>
#include <mach/mt6516_mm_mem.h> 

//#define MT6516SRAM_DEBUG
#ifdef MT6516SRAM_DEBUG
#define SRAM_DEBUG printk
#else
#define SRAM_DEBUG(x,...)
#endif

static struct class *Internal_SRAM_class = NULL;
static dev_t Internal_SRAM_devno;
static struct cdev *Internal_SRAM_cdev;

unsigned int alloc_internal_sram(USING_INTERNAL_SRAM_DRIVER driver_module, UINT32 size, UINT32 byte_alignment)
{
    UINT32 PA = 0x00000000;

    switch (driver_module)
    {
        case INTERNAL_SRAM_PNG_DECODER:
            if ((byte_alignment != 2) || (size > 34680))
            {
                printk("[INTERNAL_SRAM][ERROR] PNG_DECODER size or byte_alignment is incorrect!!\n");
            }
            else
            {
                PA = 0x40000000;
                SRAM_DEBUG("[INTERNAL_SRAM] PNG_DECODER alloc addr = 0x%08x, size = %d\n", PA, size);
            }
            break;

        case INTERNAL_SRAM_JPEG_DECODER:
            if ((byte_alignment != 2048) || (size > 4096))
            {
                printk("[INTERNAL_SRAM][ERROR] JPEG_DECODER size or byte_alignment is incorrect!!\n");
            }
            else
            {
                PA = 0x40020000; 
                SRAM_DEBUG("[INTERNAL_SRAM] JPEG_DECODER alloc addr = 0x%08x, size = %d\n", PA, size);
            }
            break;

        case INTERNAL_SRAM_H264_DECODER_MC_LINE_BUF:
            if ((byte_alignment != 16) || (size > 51840))
            {
                printk("[INTERNAL_SRAM][ERROR] H264_DECODER_MC_LINE_BUF size or byte_alignment is incorrect!!\n");
            }
            else
            {
                PA = 0x40020000; 
                SRAM_DEBUG("[INTERNAL_SRAM] H264_DECODER_MC_LINE_BUF alloc addr = 0x%08x, size = %d\n", PA, size);
            }
            break;

        case INTERNAL_SRAM_H264_DECODER_MC_MV_BUFFER:
            if ((byte_alignment != 16) || (size > 3328))
            {
                printk("[INTERNAL_SRAM][ERROR] H264_DECODER_MC_MV_BUFFER size or byte_alignment is incorrect!!\n");
            }
            else
            {
                PA = 0x4002CA80; 
                SRAM_DEBUG("[INTERNAL_SRAM] H264_DECODER_MC_MV_BUFFER alloc addr = 0x%08x, size = %d\n", PA, size);
            }
            break;

        case INTERNAL_SRAM_H264_DECODER_DEC_DEB_BUF:
            if ((byte_alignment != 8192) || (size > 5888))
            {
                printk("[INTERNAL_SRAM][ERROR] H264_DECODER_DEC_DEB_BUF size or byte_alignment is incorrect!!\n");
            }
            else
            {
                PA = 0x40038000; 
                SRAM_DEBUG("[INTERNAL_SRAM] H264_DECODER_DEC_DEB_BUF alloc addr = 0x%08x, size = %d\n", PA, size);
            }
            break;

        case INTERNAL_SRAM_H264_DECODER_DEC_DEB_DAT_BUF0:
            if ((byte_alignment != 512) || (size > 384))
            {
                printk("[INTERNAL_SRAM][ERROR] H264_DECODER_DEC_DEB_DAT_BUF0 size or byte_alignment is incorrect!!\n");
            }
            else
            {
                PA = 0x4002F000; 
                SRAM_DEBUG("[INTERNAL_SRAM] H264_DECODER_DEC_DEB_DAT_BUF0 alloc addr = 0x%08x, size = %d\n", PA, size);
            }
            break;

        case INTERNAL_SRAM_H264_DECODER_DEC_DEB_DAT_BUF1:
            if ((byte_alignment != 512) || (size > 384))
            {
                printk("[INTERNAL_SRAM][ERROR] H264_DECODER_DEC_DEB_DAT_BUF1 size or byte_alignment is incorrect!!\n");
            }
            else
            {
                PA = 0x4002F200; 
                SRAM_DEBUG("[INTERNAL_SRAM] H264_DECODER_DEC_DEB_DAT_BUF1 alloc addr = 0x%08x, size = %d\n", PA, size);
            }
            break;

        case INTERNAL_SRAM_H264_DECODER_CALVC:
            if ((byte_alignment != 32) || (size > 520))
            {
                printk("[INTERNAL_SRAM][ERROR] H264_DECODER_CALVC size or byte_alignment is incorrect!!\n");
            }
            else
            {
                PA = 0x4002F400; 
                SRAM_DEBUG("[INTERNAL_SRAM] H264_DECODER_CALVC alloc addr = 0x%08x, size = %d\n", PA, size);
            }
            break;

        case INTERNAL_SRAM_MPEG4_DECODER_DATA_STORE:
            if ((byte_alignment != 4) || (size > 43200))
            {
                printk("[INTERNAL_SRAM][ERROR] MPEG4_DECODER_DATA_STORE size or byte_alignment is incorrect!!\n");
            }
            else
            {
                PA = 0x40020000; 
                SRAM_DEBUG("[INTERNAL_SRAM] MPEG4_DECODER_DATA_STORE alloc addr = 0x%08x, size = %d\n", PA, size);
            }
            break;
        
        case INTERNAL_SRAM_MPEG4_DECODER_DACP:
            if ((byte_alignment != 4) || (size > 4096))
            {
                printk("[INTERNAL_SRAM][ERROR] MPEG4_DECODER_DACP size or byte_alignment is incorrect!!\n");
            }
            else
            {
                PA = 0x4002A8C0; 
                SRAM_DEBUG("[INTERNAL_SRAM] MPEG4_DECODER_DACP alloc addr = 0x%08x, size = %d\n", PA, size);
            }
            break;

        case INTERNAL_SRAM_MPEG4_DECODER_MVP:
            if ((byte_alignment != 4) || (size > 360))
            {
                printk("[INTERNAL_SRAM][ERROR] MPEG4_DECODER_MVP size or byte_alignment is incorrect!!\n");
            }
            else
            {
                PA = 0x4002B8C0; 
                SRAM_DEBUG("[INTERNAL_SRAM] MPEG4_DECODER_MVP alloc addr = 0x%08x, size = %d\n", PA, size);
            }
            break;

        case INTERNAL_SRAM_MPEG4_DECODER_QS:
            if (size > 1440)
            {
                printk("[INTERNAL_SRAM][ERROR] MPEG4_DECODER_QS size is incorrect!!\n");
            }
            else
            {
                PA = 0x4002BA30; 
                SRAM_DEBUG("[INTERNAL_SRAM] MPEG4_DECODER_QS alloc addr = 0x%08x, size = %d\n", PA, size);
            }
            break;

        case INTERNAL_SRAM_MPEG4_ENCODER:
            if ((byte_alignment != 8) || (size > 69992))
            {
                printk("[INTERNAL_SRAM][ERROR] MPEG4_ENCODER size or byte_alignment is incorrect!!\n");
            }
            else
            {
                PA = 0x40020000; 
                SRAM_DEBUG("[INTERNAL_SRAM] MPEG4_ENCODER alloc addr = 0x%08x, size = %d\n", PA, size);
            }
            break;

        case INTERNAL_SRAM_IDP_IRT0:
            if ((byte_alignment != 8) || (size > 17280))
            {
                printk("[INTERNAL_SRAM][ERROR] IDP_IRT0 size or byte_alignment is incorrect!!\n");
            }
            else
            {
                PA = 0x40031170; 
                SRAM_DEBUG("[INTERNAL_SRAM] IDP_IRT0 alloc addr = 0x%08x, size = %d\n", PA, size);
            }
            break;

        case INTERNAL_SRAM_IDP_IRT1:
            if ((byte_alignment != 8) || (size > 55296))
            {
                printk("[INTERNAL_SRAM][ERROR] IDP_IRT1 size or byte_alignment is incorrect!!\n");
            }
            else
            {
                PA = 0x40000000; 
                SRAM_DEBUG("[INTERNAL_SRAM] IDP_IRT1 alloc addr = 0x%08x, size = %d\n", PA, size);
            }
            break;

        case INTERNAL_SRAM_IDP_VDO_ENC_WDMA:
            if ((byte_alignment != 8) || (size > 11520))
            {
                printk("[INTERNAL_SRAM][ERROR] IDP_VDO_ENC_WDMA size or byte_alignment is incorrect!!\n");
            }
            else
            {
                PA = 0x40039700; 
                SRAM_DEBUG("[INTERNAL_SRAM] IDP_VDO_ENC_WDMA alloc addr = 0x%08x, size = %d\n", PA, size);
            }
            break;

        case INTERNAL_SRAM_IDP_JPEG_DMA:
            if ((byte_alignment != 8) || (size > 124416))
            {
                printk("[INTERNAL_SRAM][ERROR] IDP_JPEG_DMA size or byte_alignment is incorrect!!\n");
            }
            else
            {
                PA = 0x40020000; 
                SRAM_DEBUG("[INTERNAL_SRAM] IDP_JPEG_DMA alloc addr = 0x%08x, size = %d\n", PA, size);
            }
            break;

        case INTERNAL_SRAM_IDP_PRZ:
            if ((byte_alignment != 4) || (size > 13824))
            {
                printk("[INTERNAL_SRAM][ERROR] IDP_PRZ size or byte_alignment is incorrect!!\n");
            }
            else
            {
                PA = 0x4003E600; 
                SRAM_DEBUG("[INTERNAL_SRAM] IDP_PRZ alloc addr = 0x%08x, size = %d\n", PA, size);
            }
            break;

        case INTERNAL_SRAM_IDP_MP4DEBLK:
            if ((byte_alignment != 4) || (size > 8640))
            {
                printk("[INTERNAL_SRAM][ERROR] IDP_MP4DEBLK size or byte_alignment is incorrect!!\n");
            }
            else
            {
                PA = 0x400354F0; 
                SRAM_DEBUG("[INTERNAL_SRAM] IDP_MP4DEBLK alloc addr = 0x%08x, size = %d\n", PA, size);
            }
            break;
            
        case INTERNAL_SRAM_IDP_PRZ_MCU:  
            if ((byte_alignment != 4) || (size > 131072))
            {
                printk("[INTERNAL_SRAM][ERROR] IDP_PRZ_MCU size or byte_alignment is incorrect!!\n");
            }
            else
            {
                PA = 0x40021000; 
                SRAM_DEBUG("[INTERNAL_SRAM] IDP_PRZ_MCU alloc addr = 0x%08x, size = %d\n", PA, size);
            }
            break; 
            
        default:
            printk("[INTERNAL_SRAM][ERROR] do not use internal sram for this driver module!!\n");
            break;
    }
    
    return PA;
}

void free_internal_sram(USING_INTERNAL_SRAM_DRIVER driver_module, UINT32 phy_addr)
{
    switch (driver_module)
    {
        case INTERNAL_SRAM_PNG_DECODER:
            if (phy_addr != 0x40000000)
            {
                printk("[INTERNAL_SRAM][ERROR] PNG_DECODER phy_addr is incorrect!!\n");
            }
            else
            {
                SRAM_DEBUG("[INTERNAL_SRAM] PNG_DECODER free addr = 0x%08x\n", phy_addr);
            }
            break;

        case INTERNAL_SRAM_JPEG_DECODER:
            if (phy_addr != 0x40020000)
            {
                printk("[INTERNAL_SRAM][ERROR] JPEG_DECODER phy_addr is incorrect!!\n");
            }
            else
            {
                SRAM_DEBUG("[INTERNAL_SRAM] JPEG_DECODER free addr = 0x%08x\n", phy_addr);
            }
            break;

        case INTERNAL_SRAM_H264_DECODER_MC_LINE_BUF:
            if (phy_addr != 0x40020000)
            {
                printk("[INTERNAL_SRAM][ERROR] H264_DECODER_MC_LINE_BUF phy_addr is incorrect!!\n");
            }
            else
            {
                SRAM_DEBUG("[INTERNAL_SRAM] H264_DECODER_MC_LINE_BUF free addr = 0x%08x\n", phy_addr);
            }
            break;

        case INTERNAL_SRAM_H264_DECODER_MC_MV_BUFFER:
            if (phy_addr != 0x4002CA80)
            {
                printk("[INTERNAL_SRAM][ERROR] H264_DECODER_MC_MV_BUFFER phy_addr is incorrect!!\n");
            }
            else
            {
                SRAM_DEBUG("[INTERNAL_SRAM] H264_DECODER_MC_MV_BUFFER free addr = 0x%08x\n", phy_addr);
            }
            break;

        case INTERNAL_SRAM_H264_DECODER_DEC_DEB_BUF:
            if (phy_addr != 0x40038000)
            {
                printk("[INTERNAL_SRAM][ERROR] H264_DECODER_DEC_DEB_BUF phy_addr is incorrect!!\n");
            }
            else
            {
                SRAM_DEBUG("[INTERNAL_SRAM] H264_DECODER_DEC_DEB_BUF free addr = 0x%08x\n", phy_addr);
            }
            break;

        case INTERNAL_SRAM_H264_DECODER_DEC_DEB_DAT_BUF0:
            if (phy_addr != 0x4002F000)
            {
                printk("[INTERNAL_SRAM][ERROR] H264_DECODER_DEC_DEB_DAT_BUF0 phy_addr is incorrect!!\n");
            }
            else
            {
                SRAM_DEBUG("[INTERNAL_SRAM] H264_DECODER_DEC_DEB_DAT_BUF0 free addr = 0x%08x\n", phy_addr);
            }
            break;

        case INTERNAL_SRAM_H264_DECODER_DEC_DEB_DAT_BUF1:
            if (phy_addr != 0x4002F200)
            {
                printk("[INTERNAL_SRAM][ERROR] H264_DECODER_DEC_DEB_DAT_BUF0 phy_addr is incorrect!!\n");
            }
            else
            {
                SRAM_DEBUG("[INTERNAL_SRAM] H264_DECODER_DEC_DEB_DAT_BUF0 free addr = 0x%08x\n", phy_addr);
            }
            break;

        case INTERNAL_SRAM_H264_DECODER_CALVC:
            if (phy_addr != 0x4002F400)
            {
                printk("[INTERNAL_SRAM][ERROR] H264_DECODER_CALVC phy_addr is incorrect!!\n");
            }
            else
            {
                SRAM_DEBUG("[INTERNAL_SRAM] H264_DECODER_CALVC free addr = 0x%08x\n", phy_addr);
            }
            break;

        case INTERNAL_SRAM_MPEG4_DECODER_DATA_STORE:
            if (phy_addr != 0x40020000)
            {
                printk("[INTERNAL_SRAM][ERROR] MPEG4_DECODER_DATA_STORE phy_addr is incorrect!!\n");
            }
            else
            {
                SRAM_DEBUG("[INTERNAL_SRAM] MPEG4_DECODER_DATA_STORE free addr = 0x%08x\n", phy_addr);
            }
            break;
        
        case INTERNAL_SRAM_MPEG4_DECODER_DACP:
            if (phy_addr != 0x4002A8C0)
            {
                printk("[INTERNAL_SRAM][ERROR] MPEG4_DECODER_DACP phy_addr is incorrect!!\n");
            }
            else
            {
                SRAM_DEBUG("[INTERNAL_SRAM] MPEG4_DECODER_DACP free addr = 0x%08x\n", phy_addr);
            }
            break;

        case INTERNAL_SRAM_MPEG4_DECODER_MVP:
            if (phy_addr != 0x4002B8C0)
            {
                printk("[INTERNAL_SRAM][ERROR] MPEG4_DECODER_MVP phy_addr is incorrect!!\n");
            }
            else
            {
                SRAM_DEBUG("[INTERNAL_SRAM] MPEG4_DECODER_MVP free addr = 0x%08x\n", phy_addr);
            }
            break;

        case INTERNAL_SRAM_MPEG4_DECODER_QS:
            if (phy_addr != 0x4002BA30)
            {
                printk("[INTERNAL_SRAM][ERROR] MPEG4_DECODER_QS phy_addr is incorrect!!\n");
            }
            else
            {
                SRAM_DEBUG("[INTERNAL_SRAM] MPEG4_DECODER_QS free addr = 0x%08x\n", phy_addr);
            }
            break;

        case INTERNAL_SRAM_MPEG4_ENCODER:
            if (phy_addr != 0x40020000)
            {
                printk("[INTERNAL_SRAM][ERROR] MPEG4_ENCODER phy_addr is incorrect!!\n");
            }
            else
            {
                SRAM_DEBUG("[INTERNAL_SRAM] MPEG4_ENCODER free addr = 0x%08x\n", phy_addr);
            }            
            break;

        case INTERNAL_SRAM_IDP_IRT0:
            if (phy_addr != 0x40031170)
            {
                printk("[INTERNAL_SRAM][ERROR] IDP_IRT0 phy_addr is incorrect!!\n");
            }
            else
            {
                SRAM_DEBUG("[INTERNAL_SRAM] IDP_IRT0 free addr = 0x%08x\n", phy_addr);
            } 
            break;

        case INTERNAL_SRAM_IDP_IRT1:
            if (phy_addr != 0x40000000)
            {
                printk("[INTERNAL_SRAM][ERROR] IDP_IRT1 phy_addr is incorrect!!\n");
            }
            else
            {
                SRAM_DEBUG("[INTERNAL_SRAM] IDP_IRT1 free addr = 0x%08x\n", phy_addr);
            } 
            break;

        case INTERNAL_SRAM_IDP_VDO_ENC_WDMA:
            if (phy_addr != 0x40039700)
            {
                printk("[INTERNAL_SRAM][ERROR] IDP_VDO_ENC_WDMA phy_addr is incorrect!!\n");
            }
            else
            {
                SRAM_DEBUG("[INTERNAL_SRAM] IDP_VDO_ENC_WDMA free addr = 0x%08x\n", phy_addr);
            } 
            break;

        case INTERNAL_SRAM_IDP_JPEG_DMA:
            if (phy_addr != 0x40020000)
            {
                printk("[INTERNAL_SRAM][ERROR] IDP_JPEG_DMA phy_addr is incorrect!!\n");
            }
            else
            {
                SRAM_DEBUG("[INTERNAL_SRAM] IDP_JPEG_DMA free addr = 0x%08x\n", phy_addr);
            } 
            break;

        case INTERNAL_SRAM_IDP_PRZ:
            if (phy_addr != 0x4003E600)
            {
                printk("[INTERNAL_SRAM][ERROR] IDP_PRZ phy_addr is incorrect!!\n");
            }
            else
            {
                SRAM_DEBUG("[INTERNAL_SRAM] IDP_PRZ free addr = 0x%08x\n", phy_addr);
            }            
            break;

        case INTERNAL_SRAM_IDP_MP4DEBLK:
            if (phy_addr != 0x400354F0)
            {
                printk("[INTERNAL_SRAM][ERROR] IDP_MP4DEBLK phy_addr is incorrect!!\n");
            }
            else
            {
                SRAM_DEBUG("[INTERNAL_SRAM] IDP_MP4DEBLK free addr = 0x%08x\n", phy_addr);
            }
            break;

        case INTERNAL_SRAM_IDP_PRZ_MCU:
            if (phy_addr != 0x40021000)
            {
                printk("[INTERNAL_SRAM][ERROR] IDP_PRZ_MCU phy_addr is incorrect!!\n");
            }
            else
            {
                SRAM_DEBUG("[INTERNAL_SRAM] IDP_PRZ_MCU free addr = 0x%08x\n", phy_addr);
            }            
            break;
            
        default:
            printk("[INTERNAL_SRAM][ERROR] do not use internal sram for this driver module!!\n");
            break;
    }
}

static int Internal_SRAM_open(struct inode *inode, struct file *file)
{
    SRAM_DEBUG("[INTERNAL_SRAM] open\n");
    return 0;
}

static int Internal_SRAM_release(struct inode *inode, struct file *file)
{
    SRAM_DEBUG("[INTERNAL_SRAM] release\n");
    return 0;
}

static struct file_operations Internal_SRAM_fops = {
    .owner      = THIS_MODULE,
    .open       = Internal_SRAM_open,
    .release    = Internal_SRAM_release,
};

static int Internal_SRAM_probe(struct platform_device *dev)
{
    struct class_device;

    int ret;
    struct class_device *class_dev = NULL;

    SRAM_DEBUG("[INTERNAL_SRAM] Internal_SRAM_probe\n");

    ret = alloc_chrdev_region(&Internal_SRAM_devno, 0, 1, INTERNAL_SRAM_DEVNAME);
    if(ret)
    {
        printk("[INTERNAL_SRAM][Error] can't get major number for device\n");
    }

    Internal_SRAM_cdev = cdev_alloc();
    Internal_SRAM_cdev->owner = THIS_MODULE;
    Internal_SRAM_cdev->ops = &Internal_SRAM_fops;

    ret = cdev_add(Internal_SRAM_cdev, Internal_SRAM_devno, 1);

    if (ret < 0)
    {
        printk("[INTERNAL_SRAM] cdev add error!!\n");
    }

    Internal_SRAM_class = class_create(THIS_MODULE, INTERNAL_SRAM_DEVNAME);
    class_dev = (struct class_device *)device_create(Internal_SRAM_class,
                                              NULL,
                                              Internal_SRAM_devno,
                                              NULL,
                                              INTERNAL_SRAM_DEVNAME
                                              );

    NOT_REFERENCED(class_dev);
    
    SRAM_DEBUG("[INTERNAL_SRAM] Internal_SRAM_probe done\n");
    
    return 0;
}

static int Internal_SRAM_remove(struct platform_device *dev)
{    
    return 0;
}

static void Internal_SRAM_shutdown(struct platform_device *dev)
{

}

static int Internal_SRAM_suspend(struct platform_device *dev, pm_message_t state)
{    
    return 0;
}

static int Internal_SRAM_resume(struct platform_device *dev)
{   
    return 0;
}

static struct platform_driver Internal_SRAM_driver = {
    .probe       = Internal_SRAM_probe,
    .remove      = Internal_SRAM_remove,
    .shutdown    = Internal_SRAM_shutdown,
    .suspend     = Internal_SRAM_suspend,
    .resume      = Internal_SRAM_resume,
    .driver      = {
    .name        = INTERNAL_SRAM_DEVNAME,
    },
};

static struct platform_device Internal_SRAM_device = {
    .name     = INTERNAL_SRAM_DEVNAME,
    .id       = 0,
};

static int __init Internal_SRAM_driver_init(void)
{
    int ret;

    SRAM_DEBUG("[INTERNAL_SRAM] driver_init\n");

    if (platform_device_register(&Internal_SRAM_device)){
        printk("[INTERNAL_SRAM][ERROR] fail to register device\n");
        ret = -ENODEV;
        return ret;
    }
    
    if (platform_driver_register(&Internal_SRAM_driver)){
        printk("[INTERNAL_SRAM][ERROR] fail to register driver\n");
        platform_device_unregister(&Internal_SRAM_device);
        ret = -ENODEV;
        return ret;
    }

    SRAM_DEBUG("[INTERNAL_SRAM] driver_init done\n");
    
    return 0;
}

static void __exit Internal_SRAM_driver_exit(void)
{
    SRAM_DEBUG("[INTERNAL_SRAM] driver_exit\n");

    device_destroy(Internal_SRAM_class, Internal_SRAM_devno);
    class_destroy(Internal_SRAM_class);
   
    cdev_del(Internal_SRAM_cdev);
    unregister_chrdev_region(Internal_SRAM_devno, 1);
    platform_driver_unregister(&Internal_SRAM_driver);
    platform_device_unregister(&Internal_SRAM_device);
}

module_init(Internal_SRAM_driver_init);
module_exit(Internal_SRAM_driver_exit);
MODULE_AUTHOR("Jackal, Chen <jackal.chen@mediatek.com>");
MODULE_DESCRIPTION("MT6516 Internal SRAM Driver");
MODULE_LICENSE("GPL");

