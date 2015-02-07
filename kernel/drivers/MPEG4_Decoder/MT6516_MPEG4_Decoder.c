
#include "MT6516_MPEG4_Decoder.h"

static dev_t mpeg4_decoder_devno;
static struct cdev *mpeg4_decoder_cdev;
static struct class *mpeg4_decoder_class = NULL;

yuv_buffer g_yuv_buffer;
dec_ref g_dec_ref;
bitstream g_bitstream;
UINT32 g_yuv_buffer_current_frame_pa;
UINT32 g_irq_status;
UINT32 g_dma_limit_count;
UINT32 g_curr_vlc_addr;
      
static wait_queue_head_t decWaitQueue;

BOOL   g_DACP_Internal_SRAM_allocated = FALSE;
BOOL   g_DATA_STORE_Internal_SRAM_allocated = FALSE;
BOOL   g_QS_Internal_SRAM_allocated = FALSE;
BOOL   g_MVP_Internal_SRAM_allocated = FALSE;

UINT32 g_DACP_Internal_SRAM_addr;
UINT32 g_DATA_STORE_Internal_SRAM_addr;
UINT32 g_QS_Internal_SRAM_addr;
UINT32 g_MVP_Internal_SRAM_addr;
     
volatile BOOL   g_dec_interrupt_handler = FALSE;

static BOOL bMP4HWClockUsed = FALSE; 

void DEBUG_REG(void)
{
}

void mpeg4_decoder_relase_resource(void)
{
    BOOL flag;
    
    if (g_DACP_Internal_SRAM_allocated == TRUE)
    {
        g_DACP_Internal_SRAM_allocated = FALSE;
        free_internal_sram(INTERNAL_SRAM_MPEG4_DECODER_DACP, g_DACP_Internal_SRAM_addr);
    }

    if (g_DATA_STORE_Internal_SRAM_allocated == TRUE)
    {
        g_DATA_STORE_Internal_SRAM_allocated = FALSE;
        free_internal_sram(INTERNAL_SRAM_MPEG4_DECODER_DATA_STORE, g_DATA_STORE_Internal_SRAM_addr);
    }

    if (g_MVP_Internal_SRAM_allocated == TRUE)
    {
        g_MVP_Internal_SRAM_allocated = FALSE;
        free_internal_sram(INTERNAL_SRAM_MPEG4_DECODER_MVP, g_MVP_Internal_SRAM_addr);
    }

    if (g_QS_Internal_SRAM_allocated == TRUE)
    {
        g_QS_Internal_SRAM_allocated = FALSE;
        free_internal_sram(INTERNAL_SRAM_MPEG4_DECODER_QS, g_QS_Internal_SRAM_addr);
    }

    flag = hwDisableClock(MT6516_PDN_MM_MP4,"MPEG4_DEC");
    flag = hwDisableClock(MT6516_PDN_MM_DCT,"MPEG4_DEC");
    flag = hwDisableClock(MT6516_PDN_MM_GMC2,"MPEG4_DEC");
    
    NOT_REFERENCED(flag);
}

static __tcmfunc irqreturn_t mp4_dec_intr_dlr(int irq, void *dev_id)
{   
    g_dec_interrupt_handler = TRUE;
    wake_up_interruptible(&decWaitQueue);

    return IRQ_HANDLED;
}

static int mpeg4_decoder_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{   
    int ret = 0;
    UINT32 temp;
    UINT32 * p_temp_ptr_from_user;
    mp4_dec_hw_config * p_mp4_dec_hw_config_from_user;
    mp4_dec_hw_config Temp_mp4_dec_hw_config;    
    pmem_bitstream * p_pmem_bitstream_from_user;
    pmem_bitstream Temp_pmem_bitstream;
    pmem_MTKYUVBuffer * p_pmem_MTKYUVBuffer_from_user;
    pmem_MTKYUVBuffer Temp_pmem_MTKYUVBuffer;
    CleanDCache * p_CleanDCache_ptr_from_user;
    CleanDCache Tmep_CleanDCache;
    FlushDCache * p_FlushDCache_ptr_from_user;
    FlushDCache Tmep_FlushDCache;
    UINT32 temp_ptr;
    UINT32 temp_int;
    long timeout_jiff;
    UINT32 temp_pa_ptr;
    UINT8 * temp_va_ptr;
        
    switch(cmd)
    {  
        case SET_MP4_CODEC_COMD:
            MP4_DEC_DEBUG("[MP4_DEC] SET_MP4_CODEC_COMD case\n");
            p_temp_ptr_from_user = (UINT32 *)arg;
            ret = copy_from_user(&temp, p_temp_ptr_from_user, sizeof(UINT32));
            HW_WRITE(MP4_CODEC_COMD, temp);            
            break;

        case SET_MP4_CODEC_COMD_START:
            MP4_DEC_DEBUG("[MP4_DEC] SET_MP4_CODEC_COMD_START case\n"); 
            g_dma_limit_count = 0;
            HW_WRITE(MP4_CODEC_COMD, MP4_DEC_START);  // trigger start
            timeout_jiff = 80 * HZ / 1000; // wait 80 ms
            g_dec_interrupt_handler = FALSE;
            wait_event_interruptible_timeout(decWaitQueue, g_dec_interrupt_handler, timeout_jiff);
            g_dec_interrupt_handler = FALSE;
            break;

        case GET_MP4_IRQ_STS:
            MP4_DEC_DEBUG("[MP4_DEC] GET_MP4_IRQ_STS_AND_SET_ACK case\n");
            p_temp_ptr_from_user = (UINT32 *)arg;
            g_irq_status = HW_READ(MP4_DEC_IRQ_STS);	            	
            ret = copy_to_user(p_temp_ptr_from_user, &g_irq_status, sizeof(UINT32));            
            break;

        case SET_MP4_IRQ_ACK:
            MP4_DEC_DEBUG("[MP4_DEC] SET_MP4_IRQ_ACK case\n");
            
            if (g_irq_status & MP4_DEC_IRQ_STS_DEC)
            {
                // Decode Complete            
                g_yuv_buffer_current_frame_pa = g_yuv_buffer.pa;
                // swap ref_addr and rec_addr
                temp_pa_ptr = g_dec_ref.pa;
                temp_va_ptr = g_dec_ref.va;
                g_dec_ref.pa = g_yuv_buffer.pa;
                g_dec_ref.va = g_yuv_buffer.va;
                g_yuv_buffer.pa = temp_pa_ptr;
                g_yuv_buffer.va = temp_va_ptr;
                HW_WRITE(MP4_DEC_IRQ_ACK, g_irq_status);
            }
            else if (g_irq_status & MP4_DEC_IRQ_STS_DMA)
            {
                MP4_DEC("[MP4_DEC][WARNING] MP4_DEC_IRQ_STS_DMA WARNING\n");
                HW_WRITE(MP4_DEC_VLC_LIMIT, 0xFFFF);
                g_dma_limit_count++;
                HW_WRITE(MP4_CORE_VLC_ADDR, (g_curr_vlc_addr+0xFFFF*4*g_dma_limit_count));
                HW_WRITE(MP4_VLC_DMA_COMD, MP4_VLC_DMA_COMD_RELOAD);
                HW_WRITE(MP4_DEC_IRQ_ACK, g_irq_status);
                timeout_jiff = 80 * HZ / 1000; // wait 80 ms
                g_dec_interrupt_handler = FALSE;
                wait_event_interruptible_timeout(decWaitQueue, g_dec_interrupt_handler, timeout_jiff);
                g_dec_interrupt_handler = FALSE;   
            }
            else
            {
                //decode fail
                HW_WRITE(MP4_DEC_IRQ_ACK, g_irq_status);
            }         
            break;

        case GET_MP4_CURRENT_YUV_BUFFER_PA:
            MP4_DEC_DEBUG("[MP4_DEC] GET_MP4_CURRENT_YUV_BUFFER_PA case\n");
            p_temp_ptr_from_user = (UINT32 *)arg;
            temp = g_yuv_buffer_current_frame_pa;
            ret = copy_to_user(p_temp_ptr_from_user, &temp, sizeof(UINT32));
            break;

        case SET_MP4_PMEM_BITSTREAMBUFFER:
            MP4_DEC_DEBUG("[MP4_DEC] SET_MP4_PMEM_BITSTREAMBUFFER case\n");
            p_pmem_bitstream_from_user = (pmem_bitstream *)arg;
            ret = copy_from_user(&Temp_pmem_bitstream, p_pmem_bitstream_from_user, sizeof(pmem_bitstream));
            g_bitstream.pa = Temp_pmem_bitstream.pa;
            g_bitstream.va = Temp_pmem_bitstream.va; 
            break;

        case SET_MP4_DEC_HW_CONFIG:
            MP4_DEC_DEBUG("[MP4_DEC] SET_MP4_DEC_HW_CONFIG case\n");
            p_mp4_dec_hw_config_from_user = (mp4_dec_hw_config *)arg;
            ret = copy_from_user(&Temp_mp4_dec_hw_config, p_mp4_dec_hw_config_from_user, sizeof(mp4_dec_hw_config));

            HW_WRITE(MP4_DEC_CODEC_CONF, Temp_mp4_dec_hw_config.mp4_dec_codec_conf);
            HW_WRITE(MP4_DEC_IRQ_MASK, Temp_mp4_dec_hw_config.mp4_dec_irq_mask);
            HW_WRITE(MP4_DEC_VOP_STRUCT0, Temp_mp4_dec_hw_config.mp4_dec_vop_struct0);
            HW_WRITE(MP4_DEC_VOP_STRUCT1, Temp_mp4_dec_hw_config.mp4_dec_vop_struct1);
            HW_WRITE(MP4_DEC_VOP_STRUCT2, Temp_mp4_dec_hw_config.mp4_dec_vop_struct2);
            HW_WRITE(MP4_DEC_MB_STRUCT0, Temp_mp4_dec_hw_config.mp4_dec_mb_struct0);

            if (g_DACP_Internal_SRAM_allocated == FALSE)
            {
                g_DACP_Internal_SRAM_addr = alloc_internal_sram(INTERNAL_SRAM_MPEG4_DECODER_DACP, 4096, 4);
                g_DACP_Internal_SRAM_allocated = TRUE;
            }
            HW_WRITE(MP4_DEC_DACP_ADDR, g_DACP_Internal_SRAM_addr);

            if (g_DATA_STORE_Internal_SRAM_allocated == FALSE)
            {
                g_DATA_STORE_Internal_SRAM_addr = alloc_internal_sram(INTERNAL_SRAM_MPEG4_DECODER_DATA_STORE, 43200, 4);                            
                g_DATA_STORE_Internal_SRAM_allocated = TRUE;
            }
            HW_WRITE(MP4_DEC_DATA_STROE_ADDR, g_DATA_STORE_Internal_SRAM_addr);

            if (g_QS_Internal_SRAM_allocated == FALSE)
            {
                g_QS_Internal_SRAM_addr = alloc_internal_sram(INTERNAL_SRAM_MPEG4_DECODER_QS, 1440, 4);
                g_QS_Internal_SRAM_allocated = TRUE;
            }
            HW_WRITE(MP4_DEC_QS_ADDR, g_QS_Internal_SRAM_addr);

            if (g_MVP_Internal_SRAM_allocated == FALSE)
            {
                g_MVP_Internal_SRAM_addr = alloc_internal_sram(INTERNAL_SRAM_MPEG4_DECODER_MVP, 360, 4);
                g_MVP_Internal_SRAM_allocated = TRUE;
            }
            HW_WRITE(MP4_DEC_MVP_ADDR, g_MVP_Internal_SRAM_addr);
            
            HW_WRITE(MP4_DEC_REC_ADDR, g_yuv_buffer.pa);
            HW_WRITE(MP4_DEC_REF_ADDR, g_dec_ref.pa);

            g_bitstream.size = Temp_mp4_dec_hw_config.dec_vlc.size;
            g_bitstream.bytecnt = Temp_mp4_dec_hw_config.dec_vlc.bytecnt;
            g_bitstream.bitcnt = Temp_mp4_dec_hw_config.dec_vlc.bitcnt;
            temp_ptr = g_bitstream.pa + g_bitstream.bytecnt + (g_bitstream.bitcnt >> 3);
            temp_int = g_bitstream.bitcnt & 0x7;
            temp_int += (temp_ptr & 0x3) * 8;
            temp_ptr -= (temp_ptr & 0x3);         

            if (temp_ptr % 4 != 0)  // for 4 byte align
            {
                MP4_DEC("[MP4_DEC][ERROR] temp_ptr is not 4byte aligned!!\n");
            }
            
            HW_WRITE(MP4_DEC_VLC_ADDR, temp_ptr);
            g_curr_vlc_addr = temp_ptr;
            HW_WRITE(MP4_DEC_VLC_BIT, temp_int);
            HW_WRITE(MP4_DEC_VLC_LIMIT, Temp_mp4_dec_hw_config.mp4_dec_vlc_limit);
            //HW_WRITE(MP4_DEC_VLC_LIMIT, 0xFFFF);

            if (Temp_mp4_dec_hw_config.mp4_dec_vlc_limit != 0xFFFF)
            {
                MP4_DEC("[MPEG4_DEC][ERROR] Temp_mp4_dec_hw_config.mp4_dec_vlc_limit = %d\n", Temp_mp4_dec_hw_config.mp4_dec_vlc_limit);
            }            
            break;

        case SET_MP4_PMEM_MTKYUVBUFFER:
            MP4_DEC_DEBUG("[MP4_DEC] SET_MP4_PMEM_MTKYUVBUFFER case\n");
            p_pmem_MTKYUVBuffer_from_user = (pmem_MTKYUVBuffer *)arg;
            ret = copy_from_user(&Temp_pmem_MTKYUVBuffer, p_pmem_MTKYUVBuffer_from_user, sizeof(pmem_MTKYUVBuffer));
            g_dec_ref.va = Temp_pmem_MTKYUVBuffer.reference_va;
            g_dec_ref.pa = Temp_pmem_MTKYUVBuffer.reference_pa;
            g_dec_ref.size = (Temp_pmem_MTKYUVBuffer.width * Temp_pmem_MTKYUVBuffer.height * 3) >> 1;
            g_yuv_buffer.va = Temp_pmem_MTKYUVBuffer.current_va;
            g_yuv_buffer.pa = Temp_pmem_MTKYUVBuffer.current_pa;
            g_yuv_buffer.width = Temp_pmem_MTKYUVBuffer.width;
            g_yuv_buffer.height = Temp_pmem_MTKYUVBuffer.height;
            g_yuv_buffer.size = g_dec_ref.size;

            HW_WRITE(MP4_DEC_REC_ADDR, g_yuv_buffer.pa);
            HW_WRITE(MP4_DEC_REF_ADDR, g_dec_ref.pa);
            break;

        case SET_MP4_CLEAN_DCACHE:
            MP4_DEC_DEBUG("[MP4_DEC] SET_MP4_CLEAN_DCACHE case\n");
            p_CleanDCache_ptr_from_user = (CleanDCache *)arg;
            ret = copy_from_user(&Tmep_CleanDCache, p_CleanDCache_ptr_from_user, sizeof(CleanDCache));
            MP4_DEC_DEBUG("[CLEAN_DCACHE] va = 0x%08x, size = %d\n", Tmep_CleanDCache.va, Tmep_CleanDCache.size);
            dmac_clean_range((kal_uint8*)Tmep_CleanDCache.va, (kal_uint8*)(Tmep_CleanDCache.va+Tmep_CleanDCache.size));          
            break;

        case SET_MP4_FLUSH_DCACHE:
            MP4_DEC_DEBUG("[MP4_DEC] SET_MP4_FLUSH_DCACHE case\n");
            p_FlushDCache_ptr_from_user = (FlushDCache *)arg;
            ret = copy_from_user(&Tmep_FlushDCache, p_FlushDCache_ptr_from_user, sizeof(FlushDCache));
            MP4_DEC_DEBUG("[FLUSH_DCACHE] va = 0x%08x, size = %d\n", Tmep_FlushDCache.va, Tmep_FlushDCache.size);
            dmac_flush_range((kal_uint8*)Tmep_FlushDCache.va, (kal_uint8*)(Tmep_FlushDCache.va+Tmep_FlushDCache.size));            
            break;

        default:
            MP4_DEC("[MP4_DEC][ERROR] mpeg4_decoder_ioctl default case\n");
            break;
    }
    NOT_REFERENCED(ret);

    return 0xFF;
}

static int mpeg4_decoder_open(struct inode *inode, struct file *file)
{
    BOOL flag;
    MP4_DEC("[MP4_DEC] mpeg4_decoder_open\n");

    MT6516_DCT_Reset();

    // Initalize H/W 
    // Switch on power of MP4 decoder 
    // And select GMC port for MP4
    //
    flag = hwEnableClock(MT6516_PDN_MM_GMC2,"MPEG4_DEC");
    flag = hwEnableClock(MT6516_PDN_MM_MP4,"MPEG4_DEC");
    flag = hwEnableClock(MT6516_PDN_MM_DCT,"MPEG4_DEC");
    HW_WRITE(GMC2_MUX_PORT_SEL,0x0);
   
    NOT_REFERENCED(flag);

    bMP4HWClockUsed = TRUE;

    return 0;
}

static int mpeg4_decoder_flush(struct file *file, fl_owner_t id)
{
    MP4_DEC("[MP4_DEC] mpeg4_decoder_flush\n");

    if (bMP4HWClockUsed == TRUE)
    {
        MP4_DEC("[MP4_DEC] release resource\n");
        mpeg4_decoder_relase_resource();
        bMP4HWClockUsed = FALSE;
    }
    
    return 0;
}

static int mpeg4_decoder_release(struct inode *inode, struct file *file)
{
    MP4_DEC("[MP4_DEC] mpeg4_decoder_release\n");
  
    if (bMP4HWClockUsed == TRUE)
    {
        MP4_DEC("[MP4_DEC] release resource\n");
        mpeg4_decoder_relase_resource();
        bMP4HWClockUsed = FALSE;
    }
    
    return 0;
}

static struct file_operations mpeg4_decoder_fops = {
    .owner      = THIS_MODULE,
    .ioctl      = mpeg4_decoder_ioctl,
    .open       = mpeg4_decoder_open,
    .flush      = mpeg4_decoder_flush,
    .release    = mpeg4_decoder_release,
};

static int mpeg4_decoder_probe(struct platform_device *dev)
{
    struct class_device;
    
    int ret;
    struct class_device *class_dev = NULL;

    MP4_DEC("[MP4_DEC] mpeg4_decoder_probe\n");

    bMP4HWClockUsed = FALSE;

    ret = alloc_chrdev_region(&mpeg4_decoder_devno, 0, 1, MPEG4_DECODER_DEVNAME);
    if(ret)
    {
        MP4_DEC("[MP4_DEC][ERROR] Can't Get Major number for MPEG4 Decoder Device\n");
    }

    mpeg4_decoder_cdev = cdev_alloc();
    mpeg4_decoder_cdev->owner = THIS_MODULE;
    mpeg4_decoder_cdev->ops = &mpeg4_decoder_fops;

    ret = cdev_add(mpeg4_decoder_cdev, mpeg4_decoder_devno, 1);

    //Register Interrupt 
    if (request_irq(MT6516_MPEG4_DEC_IRQ_LINE, mp4_dec_intr_dlr, 0, MPEG4_DECODER_DEVNAME, NULL) < 0)
    {
       MP4_DEC("[MP4_DEC][ERROR] error to request MPEG4 irq\n"); 
    }
    else
    {
       MP4_DEC("[MP4_DEC] success to request MPEG4 irq\n");
    }

    mpeg4_decoder_class = class_create(THIS_MODULE, MPEG4_DECODER_DEVNAME);
    class_dev = (struct class_device *)device_create(mpeg4_decoder_class,
                                                     NULL,
                                                     mpeg4_decoder_devno,
                                                     NULL,
                                                     MPEG4_DECODER_DEVNAME
                                                     );
    
    init_waitqueue_head(&decWaitQueue);

    NOT_REFERENCED(class_dev);
    NOT_REFERENCED(ret);

    MP4_DEC("[MP4_DEC] mpeg4_decoder_probe Done\n");
    
    return 0;
}

static int mpeg4_decoder_remove(struct platform_device *dev)
{    
    return 0;
}

static void mpeg4_decoder_shutdown(struct platform_device *dev)
{
}

static int mpeg4_decoder_suspend(struct platform_device *dev, pm_message_t state)
{    
    BOOL flag;    
    MP4_DEC("[MP4_DEC] mpeg4_decoder_suspend\n");
 
    if (bMP4HWClockUsed == TRUE)
    {
        flag = hwDisableClock(MT6516_PDN_MM_MP4,"MPEG4_DEC");
        flag = hwDisableClock(MT6516_PDN_MM_DCT,"MPEG4_DEC");
    }
        
    NOT_REFERENCED(flag); 
    return 0;
}

static int mpeg4_decoder_resume(struct platform_device *dev)
{   
    BOOL flag;
    MP4_DEC("[MP4_DEC] mpeg4_decoder_resume\n");

    if (bMP4HWClockUsed == TRUE)
    {
        flag = hwEnableClock(MT6516_PDN_MM_MP4,"MPEG4_DEC");
        flag = hwEnableClock(MT6516_PDN_MM_DCT,"MPEG4_DEC");
        HW_WRITE(GMC2_MUX_PORT_SEL,0x0);
    }

    NOT_REFERENCED(flag);
    return 0;
}

static struct platform_driver mpeg4_decoder_driver = {
    .probe       = mpeg4_decoder_probe,
    .remove      = mpeg4_decoder_remove,
    .shutdown    = mpeg4_decoder_shutdown,
    .suspend     = mpeg4_decoder_suspend,
    .resume      = mpeg4_decoder_resume,
    .driver      = {
    .name        = MPEG4_DECODER_DEVNAME,
    },
};

static struct platform_device mpeg4_decoder_device = {
    .name     = MPEG4_DECODER_DEVNAME,
    .id       = 0,
};

static int __init mpeg4_decoder_driver_init(void)
{
    int ret;

    MP4_DEC("[MP4_DEC] mpeg4_decoder_driver_init\n");

    if (platform_device_register(&mpeg4_decoder_device)){
        MP4_DEC("[MP4_DEC][ERROR] failed to register mpeg4_decoder Device\n");
        ret = -ENODEV;
        return ret;
    }
    
    if (platform_driver_register(&mpeg4_decoder_driver)){
        MP4_DEC("[MP4_DEC][ERROR] failed to register MPEG4 Decoder Driver\n");
        platform_device_unregister(&mpeg4_decoder_device);
        ret = -ENODEV;
        return ret;
    }

    MP4_DEC("[MP4_DEC] mpeg4_decoder_driver_init Done\n");
    
    return 0;
}

static void __exit mpeg4_decoder_driver_exit(void)
{
    MP4_DEC("[MP4_DEC] mpeg4_decoder_driver_exit\n");

    device_destroy(mpeg4_decoder_class, mpeg4_decoder_devno);
    class_destroy(mpeg4_decoder_class);
   
    cdev_del(mpeg4_decoder_cdev);
    unregister_chrdev_region(mpeg4_decoder_devno, 1);
    platform_driver_unregister(&mpeg4_decoder_driver);
    platform_device_unregister(&mpeg4_decoder_device);
}

module_init(mpeg4_decoder_driver_init);
module_exit(mpeg4_decoder_driver_exit);
MODULE_AUTHOR("Jackal, Chen <jackal.chen@mediatek.com>");
MODULE_DESCRIPTION("MT6516 MPEG4 Decoder Driver");
MODULE_LICENSE("GPL");
  
