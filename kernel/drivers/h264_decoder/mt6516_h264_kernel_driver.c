

#include "mt6516_h264_kernel_driver.h"

//#define AVC_KERNEL_TRACING

static struct class *pg_h264_dec_class = NULL;
static dev_t g_h264_dec_devno;
static struct cdev *pg_h264_dec_cdev;

static wait_queue_head_t h264decWaitQueue;

// Internal sram working buffer
kal_uint32 h264_dec_deb_buff_pa;
kal_uint32 h264_dec_deb_dat_buf0_pa;
kal_uint32 h264_dec_deb_dat_buf1_pa;
kal_uint32 h264_mc_line_buf_pa;
kal_uint32 h264_mc_mv_buf_pa;
kal_uint32 h264_cavlc_base_pa;

volatile kal_bool   g_h264dec_interrupt_handler = FALSE;

static kal_bool bH264HWClockUsed = FALSE;

static __tcmfunc irqreturn_t h264_dec_kernel_intr_dlr(int irq, void *dev_id)
{   
    g_h264dec_interrupt_handler = TRUE;
    wake_up_interruptible(&h264decWaitQueue);

    return IRQ_HANDLED;
}

static int h264_dec_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    kal_uint32 status;
    kal_uint32 * p_uint32_ptr_from_user;
    kal_uint32 ret;
    kal_uint32 hw_setting;
    kal_uint32 temp_pa;
    CleanDCache * p_CleanDCache_ptr_from_user;
    CleanDCache Tmep_CleanDCache;
    FlushDCache * p_FlushDCache_ptr_from_user;
    FlushDCache Tmep_FlushDCache;
    long timeout_jiff;
    
#ifdef AVC_KERNEL_TRACING
    struct timeval t1;
#endif
        
    switch(cmd)
    {
        case H264_DEC_SET_INTERNAL_SRAM: 
            H264_DEC_DEBUG("[H264_DEC_IOCTL] H264_DEC_SET_INTERNAL_SRAM case\n");

            // Internal sram DBLK buffer allocation, need 8K byte aligned
            //
            h264_dec_deb_buff_pa = alloc_internal_sram(INTERNAL_SRAM_H264_DECODER_DEC_DEB_BUF, 5888, 8192);
            HW_WRITE(H264_DEC_DEB_BUF_ADDR, h264_dec_deb_buff_pa);
            
            // Internal sram DBLK data buffer0, need 512 byte aligned 
            //
            h264_dec_deb_dat_buf0_pa = alloc_internal_sram(INTERNAL_SRAM_H264_DECODER_DEC_DEB_DAT_BUF0, 384, 512);
            HW_WRITE(H264_DEC_DEB_DAT_BUF0_ADDR, h264_dec_deb_dat_buf0_pa);    
        
            // Internal sram DBLK data buffer1, need 512 byte aligned 
            //
            h264_dec_deb_dat_buf1_pa = alloc_internal_sram(INTERNAL_SRAM_H264_DECODER_DEC_DEB_DAT_BUF1, 384, 512);
            HW_WRITE(H264_DEC_DEB_DAT_BUF1_ADDR, h264_dec_deb_dat_buf1_pa);
            
            // Internal sram MC line buffer, need 16 byte aligned
            //
            h264_mc_line_buf_pa = alloc_internal_sram(INTERNAL_SRAM_H264_DECODER_MC_LINE_BUF, 51840, 16);
            HW_WRITE(H264_MC_LINE_BUF_ADDR, h264_mc_line_buf_pa);
        
            // Internal sram MC MV buffer, need 16 byte aligned 
            //
            h264_mc_mv_buf_pa = alloc_internal_sram(INTERNAL_SRAM_H264_DECODER_MC_MV_BUFFER, 3328, 16);
            HW_WRITE(H264_MC_MV_BUFFER_ADDR, h264_mc_mv_buf_pa);
        
            // Internal sram CAVLC base addr, need 32 byte aligned
            //
            h264_cavlc_base_pa = alloc_internal_sram(INTERNAL_SRAM_H264_DECODER_CALVC, 520, 32);;
            HW_WRITE(H264_CAVLC_BASE_ADDR, h264_cavlc_base_pa);

            break;

        case H264_DEC_WAIT_DECODE_DONE:
            H264_DEC_DEBUG("[H264_DEC_IOCTL] H264_DEC_WAIT_DECODE_DONE case\n");
            
            //while(HW_READ(H264_DEC_IRQ_STS) == 0);

            break;

        case H264_DEC_RST:
            H264_DEC_DEBUG("[H264_DEC_IOCTL] H264_DEC_RST case\n");

            HW_WRITE(H264_DEC_COMD, H264_DEC_COMD_RST);

            break;

        case H264_DEC_GET_IRQ_STS_SET_ACK:
            H264_DEC_DEBUG("[H264_DEC_IOCTL] H264_DEC_GET_IRQ_STS_SET_ACK case\n");

            p_uint32_ptr_from_user = (kal_uint32 *)arg;
            status = HW_READ(H264_DEC_IRQ_STS);	
            HW_WRITE(H264_DEC_IRQ_ACK, status);	
            ret = copy_to_user(p_uint32_ptr_from_user, &status, sizeof(kal_uint32));
            g_h264dec_interrupt_handler = FALSE;

            break;

        case H264_DEC_RELOAD_VLC_DMA:
            H264_DEC_DEBUG("[H264_DEC_IOCTL] H264_DEC_RELOAD_VLC_DMA case\n");

            HW_WRITE(H264_DEC_DMA_LIMIT,0xffff);
    		HW_WRITE(H264_DEC_DMA_COMD, H264_DEC_DMA_COMD_RESUME);

            timeout_jiff = 80 * HZ / 1000; // wait 80 ms
            g_h264dec_interrupt_handler = FALSE;
            wait_event_interruptible_timeout(h264decWaitQueue, g_h264dec_interrupt_handler, timeout_jiff);
            g_h264dec_interrupt_handler = FALSE;

            break;

        case H264_DEC_SET_SLICE_MAP_ADDR:
            H264_DEC_DEBUG("[H264_DEC_IOCTL] H264_DEC_SET_SLICE_MAP_ADDR case\n");

            p_uint32_ptr_from_user = (kal_uint32 *)arg;
            ret = copy_from_user(&temp_pa, p_uint32_ptr_from_user,sizeof(kal_uint32));
            HW_WRITE(H264_DEC_SLICE_MAP_ADDR, temp_pa);
            
            break; 

        case H264_DEC_SET_REF_FRAME_ADDR:
            H264_DEC_DEBUG("[H264_DEC_IOCTL] H264_DEC_SET_REF_FRAME_ADDR case\n");

            p_uint32_ptr_from_user = (kal_uint32 *)arg;
            ret = copy_from_user(&temp_pa, p_uint32_ptr_from_user,sizeof(kal_uint32));
            HW_WRITE(H264_REF_FRAME_ADDR, temp_pa);
    
            break;
 
        case H264_DEC_SET_PIC_CONF:
            H264_DEC_DEBUG("[H264_DEC_IOCTL] H264_DEC_SET_PIC_CONF case\n");

            p_uint32_ptr_from_user = (kal_uint32 *)arg;
            ret = copy_from_user(&hw_setting, p_uint32_ptr_from_user, sizeof(kal_uint32));
            HW_WRITE(H264_DEC_PIC_CONF, (kal_uint32)hw_setting);

            break;
  
        case H264_DEC_SET_SLICE_CONF:
            H264_DEC_DEBUG("[H264_DEC_IOCTL] H264_DEC_SET_SLICE_CONF case\n");

            p_uint32_ptr_from_user = (kal_uint32 *)arg;
            ret = copy_from_user(&temp_pa, p_uint32_ptr_from_user,sizeof(kal_uint32));
            HW_WRITE(H264_DEC_SLICE_CONF, temp_pa);

            break;

        case H264_DEC_SET_DMA_LIMIT:
            H264_DEC_DEBUG("[H264_DEC_IOCTL] H264_DEC_SET_DMA_LIMIT case\n");

            p_uint32_ptr_from_user = (kal_uint32 *)arg;
            ret = copy_from_user(&hw_setting, p_uint32_ptr_from_user, sizeof(kal_uint32));
            HW_WRITE(H264_DEC_DMA_LIMIT, (kal_uint32)hw_setting);

            break;

        case H264_DEC_SET_MC_LINE_BUF_OFFSET:    
            H264_DEC_DEBUG("[H264_DEC_IOCTL] H264_DEC_SET_MC_LINE_BUF_OFFSET case\n");

            p_uint32_ptr_from_user = (kal_uint32 *)arg;
            ret = copy_from_user(&hw_setting, p_uint32_ptr_from_user, sizeof(kal_uint32));
            HW_WRITE(H264_MC_LINE_BUF_OFFSET, (kal_uint32)hw_setting);

            break;

        case H264_DEC_SET_INTERNAL_SRAM_TO_HW:
            H264_DEC_DEBUG("[H264_DEC_IOCTL] H264_DEC_SET_INTERNAL_SRAM_TO_HW case\n");

            HW_WRITE(H264_DEC_DEB_BUF_ADDR, h264_dec_deb_buff_pa);
            HW_WRITE(H264_DEC_DEB_DAT_BUF0_ADDR, h264_dec_deb_dat_buf0_pa);
            HW_WRITE(H264_DEC_DEB_DAT_BUF1_ADDR, h264_dec_deb_dat_buf1_pa);
            HW_WRITE(H264_MC_LINE_BUF_ADDR, h264_mc_line_buf_pa);
            HW_WRITE(H264_MC_MV_BUFFER_ADDR, h264_mc_mv_buf_pa);
            HW_WRITE(H264_CAVLC_BASE_ADDR, h264_cavlc_base_pa);
            
            break;

        case H264_DEC_SET_MC_LINE_BUF_SIZE:
            H264_DEC_DEBUG("[H264_DEC_IOCTL] H264_DEC_SET_MC_LINE_BUF_SIZE case\n");

            p_uint32_ptr_from_user = (kal_uint32 *)arg;
            ret = copy_from_user(&hw_setting, p_uint32_ptr_from_user, sizeof(kal_uint32));
            HW_WRITE(H264_MC_LINE_BUF_SIZE, (kal_uint32)hw_setting);

            break;

        case H264_DEC_SET_REC_ADDR:
            H264_DEC_DEBUG("[H264_DEC_IOCTL] H264_DEC_SET_REC_ADDR case\n");

            p_uint32_ptr_from_user = (kal_uint32 *)arg;
            ret = copy_from_user(&hw_setting, p_uint32_ptr_from_user, sizeof(kal_uint32));
            HW_WRITE(H264_DEC_REC_ADDR, (kal_uint32)hw_setting);

            break;

        case H264_DEC_SET_REC_Y_SIZE:
            H264_DEC_DEBUG("[H264_DEC_IOCTL] H264_DEC_SET_REC_Y_SIZE case\n");

            p_uint32_ptr_from_user = (kal_uint32 *)arg;
            ret = copy_from_user(&hw_setting, p_uint32_ptr_from_user, sizeof(kal_uint32));
            HW_WRITE(H264_DEC_REC_Y_SIZE, (kal_uint32)hw_setting);

            break;

        case H264_DEC_SET_IRQ_POS_AND_MASK:
            H264_DEC_DEBUG("[H264_DEC_IOCTL] H264_DEC_SET_IRQ_POS_AND_MASK case\n");

            HW_WRITE(H264_DEC_IRQ_POS,0xffffffff);
            HW_WRITE(H264_DEC_IRQ_MASK, 0x20);
            
            break;

        case H264_DEC_SET_HW_START:
            H264_DEC_DEBUG("[H264_DEC_IOCTL] H264_DEC_SET_HW_START case\n");

#ifdef AVC_KERNEL_TRACING
            do_gettimeofday(&t1);
            H264_DEC("H264_DEC_SET_HW_START %u %u\n", (t1.tv_sec & 0xff), t1.tv_usec);
#endif

            HW_WRITE(H264_DEC_COMD, H264_DEC_COMD_START);

            timeout_jiff = 80 * HZ / 1000; // wait 80 ms
            g_h264dec_interrupt_handler = FALSE;
            wait_event_interruptible_timeout(h264decWaitQueue, g_h264dec_interrupt_handler, timeout_jiff);
            g_h264dec_interrupt_handler = FALSE;
            
            //while(HW_READ(H264_DEC_IRQ_STS) == 0);

#ifdef AVC_KERNEL_TRACING            
            do_gettimeofday(&t1);
            H264_DEC("H264_DEC_SET_HW_END %u %u\n", (t1.tv_sec & 0xff), t1.tv_usec);
#endif
   
            break;

        case H264_DEC_CLEAN_DCACHE:
            H264_DEC_DEBUG("[H264_DEC_IOCTL] H264_DEC_CLEAN_DCACHE case\n");
            p_CleanDCache_ptr_from_user = (CleanDCache *)arg;
            ret = copy_from_user(&Tmep_CleanDCache, p_CleanDCache_ptr_from_user, sizeof(CleanDCache));
            H264_DEC_DEBUG("[CLEAN_DCACHE] va = 0x%08x, size = %d\n", Tmep_CleanDCache.va, Tmep_CleanDCache.size);
            dmac_clean_range((kal_uint8*)Tmep_CleanDCache.va, (kal_uint8*)(Tmep_CleanDCache.va+Tmep_CleanDCache.size));
            
            break;

        case H264_DEC_FLUSH_DCACHE:
            H264_DEC_DEBUG("[H264_DEC_IOCTL] H264_DEC_FLUSH_DCACHE case\n");
            p_FlushDCache_ptr_from_user = (FlushDCache *)arg;
            ret = copy_from_user(&Tmep_FlushDCache, p_FlushDCache_ptr_from_user, sizeof(FlushDCache));
            H264_DEC_DEBUG("[FLUSH_DCACHE] va = 0x%08x, size = %d\n", Tmep_FlushDCache.va, Tmep_FlushDCache.size);
            dmac_flush_range((kal_uint8*)Tmep_FlushDCache.va, (kal_uint8*)(Tmep_FlushDCache.va+Tmep_FlushDCache.size));
            
            break;
            
        default:
            H264_DEC("[H264_DEC_IOCTL][ERROR] default case\n");
 
            break;
    }
    
    NOT_REFERENCED(ret);
    return 0;
}

static int h264_dec_open(struct inode *inode, struct file *file)
{
    kal_bool flag;
    
    H264_DEC("[H264_DEC] h264_dec_open\n");

    // Initalize H/W 
    // Switch on power of H.264 decoder 
    // And select GMC port for H.264
    //
    flag = hwEnableClock(MT6516_PDN_MM_GMC2,"H264");
    flag = hwEnableClock(MT6516_PDN_MM_H264,"H264");
    HW_WRITE(GMC2_MUX_PORT_SEL,0x1);
    NOT_REFERENCED(flag);
    bH264HWClockUsed = TRUE;
    g_h264dec_interrupt_handler = FALSE;
    return 0;
}

void h264_decoder_relase_resource(void)
{
    kal_bool flag;
    
    flag = hwDisableClock(MT6516_PDN_MM_H264,"H264");
    flag = hwDisableClock(MT6516_PDN_MM_GMC2,"H264");
    free_internal_sram(INTERNAL_SRAM_H264_DECODER_DEC_DEB_BUF, h264_dec_deb_buff_pa);
    free_internal_sram(INTERNAL_SRAM_H264_DECODER_DEC_DEB_DAT_BUF0, h264_dec_deb_dat_buf0_pa);
    free_internal_sram(INTERNAL_SRAM_H264_DECODER_DEC_DEB_DAT_BUF1, h264_dec_deb_dat_buf1_pa);
    free_internal_sram(INTERNAL_SRAM_H264_DECODER_MC_LINE_BUF, h264_mc_line_buf_pa);
    free_internal_sram(INTERNAL_SRAM_H264_DECODER_MC_MV_BUFFER, h264_mc_mv_buf_pa);
    free_internal_sram(INTERNAL_SRAM_H264_DECODER_CALVC, h264_cavlc_base_pa);

    NOT_REFERENCED(flag);
}

static int h264_dec_flush(struct file *file, fl_owner_t id)
{
    H264_DEC("[H264_DEC] h264_dec_flush\n");

    if (bH264HWClockUsed == TRUE)
    {
        H264_DEC("[H264_DEC] release resource\n");
        h264_decoder_relase_resource();
        bH264HWClockUsed = FALSE;
    }
    
    return 0;
}

static int h264_dec_release(struct inode *inode, struct file *file)
{
    H264_DEC("[H264_DEC] h264_dec_release\n");

    if (bH264HWClockUsed == TRUE)
    {
        H264_DEC("[H264_DEC] release resource\n");
        h264_decoder_relase_resource();
        bH264HWClockUsed = FALSE;
    }
    
    return 0;
}

static struct file_operations h264_dec_fops = {
    .owner      = THIS_MODULE,
    .ioctl      = h264_dec_ioctl,
    .open       = h264_dec_open,
    .flush      = h264_dec_flush,
    .release    = h264_dec_release,
};

static int h264_dec_probe(struct platform_device *dev)
{
    struct class_device;
    
    int ret;
    struct class_device *class_dev = NULL;

    H264_DEC("[H264_DEC] h264_dec_probe\n");

    bH264HWClockUsed = FALSE;

    ret = alloc_chrdev_region(&g_h264_dec_devno, 0, 1, H264_DEC_DEVNAME);
    if(ret)
    {
        H264_DEC("[H264_DEC][Error] can't get major number for device\n");
    }

    pg_h264_dec_cdev = cdev_alloc();
    pg_h264_dec_cdev->owner = THIS_MODULE;
    pg_h264_dec_cdev->ops = &h264_dec_fops;

    ret = cdev_add(pg_h264_dec_cdev, g_h264_dec_devno, 1);

    if (ret < 0)
    {
        H264_DEC("[H264_DEC] cdev add error!!\n");
    }

    //Register Interrupt 
    if (request_irq(MT6516_H264DEC_IRQ_LINE, h264_dec_kernel_intr_dlr, 0, H264_DEC_DEVNAME, NULL) < 0)
    {
       H264_DEC("[H264_DEC][ERROR] error to request H264 irq\n"); 
    }
    else
    {
       H264_DEC("[H264_DEC] success to request H264 irq\n");
    }

    pg_h264_dec_class = class_create(THIS_MODULE, H264_DEC_DEVNAME);
    class_dev = (struct class_device *)device_create(pg_h264_dec_class,
                                              NULL,
                                              g_h264_dec_devno,
                                              NULL,
                                              H264_DEC_DEVNAME
                                              );

    init_waitqueue_head(&h264decWaitQueue);

    NOT_REFERENCED(class_dev);
    NOT_REFERENCED(ret);
    
    H264_DEC("[H264_DEC] h264_dec_probe done\n");
 
    return 0;
}

static int h264_dec_remove(struct platform_device *dev)
{
    H264_DEC("[H264_DEC] h264_dec_remove\n");
    return 0;
}

static void h264_dec_shutdown(struct platform_device *dev)
{
    H264_DEC("[H264_DEC] h264_dec_shutdown\n");
}

static int h264_dec_suspend(struct platform_device *dev, pm_message_t state)
{    
    kal_bool flag;

    H264_DEC("[H264_DEC] h264_dec_suspend\n");

    if (bH264HWClockUsed == TRUE)
    {
        flag = hwDisableClock(MT6516_PDN_MM_H264,"H264");
    }
    
    return 0;
}

static int h264_dec_resume(struct platform_device *dev)
{   
    kal_bool flag;
    
    H264_DEC("[H264_DEC] h264_dec_resume\n");

    if (bH264HWClockUsed == TRUE)
    {
        flag = hwEnableClock(MT6516_PDN_MM_H264,"H264");
        HW_WRITE(GMC2_MUX_PORT_SEL,0x1);
    }
    
    return 0;
}

static struct platform_driver h264_dec_driver = {
    .probe       = h264_dec_probe,
    .remove      = h264_dec_remove,
    .shutdown    = h264_dec_shutdown,
    .suspend     = h264_dec_suspend,
    .resume      = h264_dec_resume,
    .driver      = {
    .name        = H264_DEC_DEVNAME,
    },
};

static struct platform_device h264_dec_device = {
    .name     = H264_DEC_DEVNAME,
    .id       = 0,
};

static int __init h264_dec_driver_init(void)
{
    int ret;

    H264_DEC("[H264_DEC] h264_dec_driver_init\n");

    if (platform_device_register(&h264_dec_device)){
        H264_DEC("[H264_DEC][ERROR] fail to register device\n");
        ret = -ENODEV;
        return ret;
    }
    
    if (platform_driver_register(&h264_dec_driver)){
        H264_DEC("[H264_DEC][ERROR] fail to register driver\n");
        platform_device_unregister(&h264_dec_device);
        ret = -ENODEV;
        return ret;
    }

    H264_DEC("[H264_DEC] h264_dec_driver_init done\n");
    
    return 0;
}

static void __exit h264_dec_driver_exit(void)
{
    H264_DEC("[H264_DEC] h264_dec_driver_exit\n");

    device_destroy(pg_h264_dec_class, g_h264_dec_devno);
    class_destroy(pg_h264_dec_class);
    cdev_del(pg_h264_dec_cdev);
    unregister_chrdev_region(g_h264_dec_devno, 1);
    platform_driver_unregister(&h264_dec_driver);
    platform_device_unregister(&h264_dec_device);
}

module_init(h264_dec_driver_init);
module_exit(h264_dec_driver_exit);
MODULE_AUTHOR("Jackal, Chen <jackal.chen@mediatek.com>");
MODULE_DESCRIPTION("MT6516 H264 Decoder Driver");
MODULE_LICENSE("GPL");

