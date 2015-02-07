
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/module.h>
#include <linux/autoconf.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/param.h>

#include <asm/uaccess.h>


#include <mach/mt6516_pll.h>
#include <mach/irqs.h>

#include <mach/mt6516.h>
#include <mach/mt6516_reg_base.h>
#include <mach/mt6516_mm_mem.h>
#include <mach/mt6516_IDP.h>
#include "jpeg_drv.h"
#include "jpeg_drv_6516_common.h"

#include <asm/tcm.h>

//#include "../../video/mtk/disp_drv.h"
//--------------------------------------------------------------------------
// 
//--------------------------------------------------------------------------

#define JPEG_MSG(fmt,...)    
//#define JPEG_MSG printk
#define JPEG_DEVNAME "mt6516_jpeg"

#define TABLE_SIZE 4096
//--------------------------------------------------------------------------
// 
//--------------------------------------------------------------------------

// global function
extern void MT6516_IRQMask(unsigned int line);
extern void MT6516_IRQUnmask(unsigned int line);
extern void MT6516_IRQSensitivity(unsigned char code, unsigned char edge);
extern kal_uint32 _jpeg_enc_int_status;
extern kal_uint32 _jpeg_dec_int_status;

// device and driver
static dev_t jpeg_devno;
static struct cdev *jpeg_cdev;
static struct class *jpeg_class = NULL;

// decoder
static wait_queue_head_t decWaitQueue;
static spinlock_t jpegDecLock;
static int decID = 0;
static int decCount = 1;
//static unsigned char *table_buffer_va;
static unsigned int table_buffer_pa;

static unsigned char *dec_src_va;
static unsigned int dec_src_pa;
static unsigned int dec_src_size;
static unsigned int now_size;

static unsigned char *dec_dst_va;
static unsigned char *dec_user_va;
static unsigned int dec_dst_pa;
static unsigned int dec_dst_size;

static unsigned char *temp_buf_va;
static unsigned int temp_buf_pa;
static unsigned int temp_buf_size;
// encoder
static wait_queue_head_t encWaitQueue;
static spinlock_t jpegEncLock;
static int encID = 0;
static int encCount = 1;
static unsigned char *dstBufferVA;
static unsigned char *dstUserVA;
static unsigned int dstBufferPA;
static unsigned int dstBufferSize;

//--------------------------------------------------------------------------
// JPEG Common Function
//--------------------------------------------------------------------------

//static irqreturn_t jpeg_drv_isr(int irq, void *dev_id)
static __tcmfunc irqreturn_t jpeg_drv_isr(int irq, void *dev_id)
{
    JPEG_MSG("JPEG Codec Interrupt\n");
    
    if(irq == MT6516_JPEG_DEC_IRQ_LINE)
    {
        MT6516_IRQMask(MT6516_JPEG_DEC_IRQ_LINE);
        
        jpeg_isr_dec_lisr();      

        wake_up_interruptible(&decWaitQueue);
         
        MT6516_IRQUnmask(MT6516_JPEG_DEC_IRQ_LINE);        
    }
    else if(irq == MT6516_JPEG_ENC_IRQ_LINE)
    {
        JPEG_MSG("Call JPEG Encode ISR\n");
        MT6516_IRQMask(MT6516_JPEG_ENC_IRQ_LINE);
        
        jpeg_isr_enc_lisr();        
        wake_up_interruptible(&encWaitQueue);
        
        MT6516_IRQUnmask(MT6516_JPEG_ENC_IRQ_LINE);        
    }
    
    return IRQ_HANDLED;
}

void jpeg_drv_power_on(void)
{  
    BOOL ret;
    //ret = hwEnableClock(MT6516_PDN_MM_GMC1,"JPEG");
    //ret = hwEnableClock(MT6516_PDN_MM_GMC2,"JPEG");
    //ret = hwEnableClock(MT6516_PDN_MM_RESZLB,"JPEG");
   
    ret = hwEnableClock(MT6516_PDN_MM_GMC2,"JPEG");
    ret = hwEnableClock(MT6516_PDN_MM_DCT,"JPEG");
    ret = hwEnableClock(MT6516_PDN_MM_JPEG,"JPEG");

    NOT_REFERENCED(ret);
}

void jpeg_drv_power_off(void)
{  
    BOOL ret;
    //ret = hwDisableClock(MT6516_PDN_MM_GMC1,"JPEG");
    //ret = hwDisableClock(MT6516_PDN_MM_GMC2,"JPEG");
    //ret = hwDisableClock(MT6516_PDN_MM_RESZLB,"JPEG");

    ret = hwDisableClock(MT6516_PDN_MM_JPEG,"JPEG");
    ret = hwDisableClock(MT6516_PDN_MM_DCT,"JPEG");
    ret = hwDisableClock(MT6516_PDN_MM_GMC2,"JPEG");

    NOT_REFERENCED(ret);
}

//--------------------------------------------------------------------------
// JPEG Encoder IOCTRL Function
//--------------------------------------------------------------------------

static int jpeg_check_dec_id(int userID)
{
    int retValue = 0;
    
    spin_lock(&jpegDecLock);
    if(decID != userID)
    {
        printk("The JPEG Decoder is lock by another process\n");
        retValue = -EBUSY;
    }    
    spin_unlock(&jpegDecLock);

    return retValue;
}

static int jpeg_drv_dec_init(void)
{
    int retValue;
    
    spin_lock(&jpegDecLock);
    if(decID != 0)
    {
        printk("JPEG Decoder is busy\n");
        retValue = -EBUSY;
    }    
    else
    {
        decID = decCount++;
        retValue = 0;    
    }   
    spin_unlock(&jpegDecLock);

    if(retValue == 0)
    {
        MT6516_DCT_Reset();
        jpeg_drv_power_on();
        jpeg_drv_dec_reset();
    }

    return retValue;
}

static void jpeg_drv_dec_deinit(void)
{
    spin_lock(&jpegDecLock);
    decID = 0;
    spin_unlock(&jpegDecLock);
    jpeg_drv_dec_reset();
    jpeg_drv_power_off();
}

static int jpeg_check_enc_id(int userID)
{
    int retValue = 0;
    
    spin_lock(&jpegEncLock);
    if(encID != userID)
    {
        printk("The JPEG Encoder is lock by another process\n");
        retValue = -EBUSY;
    }    
    spin_unlock(&jpegEncLock);

    return retValue;
}

static int jpeg_drv_enc_init(void)
{
    int retValue;
    
    spin_lock(&jpegEncLock);
    if(encID != 0)
    {
        printk("JPEG Encoder is busy\n");
        retValue = -EBUSY;
    }    
    else
    {
        encID = encCount++;
        retValue = 0;    
    }   
    spin_unlock(&jpegEncLock);

    if(retValue == 0)
    {
        MT6516_DCT_Reset();
        jpeg_drv_power_on();
        jpeg_drv_enc_reset();
    }

    return retValue;
}

static void jpeg_drv_enc_deinit(void)
{
    spin_lock(&jpegEncLock);
    encID = 0;
    spin_unlock(&jpegEncLock);

    jpeg_drv_enc_reset();
    jpeg_drv_power_off();
}

static int jpeg_dec_ioctl(unsigned int cmd, unsigned long arg)
{
    int retValue;
    int userID;
    unsigned int decResult, i;
    long timeout_jiff;
    JPEG_DEC_DRV_IN inParams;
    JPEG_DEC_RESUME_IN reParams;
    JPEG_DEC_RANGE_IN  rangeParams;
    JPEG_DEC_DRV_OUT outParams;
    
    switch(cmd)
    {
        case JPEG_DEC_IOCTL_INIT:
            JPEG_MSG("JPEG Decoder Initial and Lock\n");

            retValue = jpeg_drv_dec_init();
            if(retValue == 0)
            {
                if(copy_to_user((int *)arg, &decID, sizeof(decID)))
                {
                    printk("JPEG Decoder : Copy to user error\n");
                    retValue = -EFAULT;
                }
            }
            if(retValue != 0)
            {
                jpeg_drv_dec_deinit();
                return -EBUSY;   
            }  
            dec_dst_va = 0;
            dec_src_va = 0;
            //table_buffer_va = 0;
            temp_buf_va = 0;
            table_buffer_pa = 0;
            break;
            
        case JPEG_DEC_IOCTL_CONFIG:
            JPEG_MSG("JPEG Decoder Configure Hardware\n");
            // copy input parameters
            if(copy_from_user(&inParams, (void *)arg, sizeof(JPEG_DEC_DRV_IN)))
            {
                printk("JPEG Decoder : Copy from user error\n");
                return -EFAULT;
            }

            retValue = jpeg_check_dec_id(inParams.decID);
            if(retValue != 0)
            {
                return retValue;
            }

            JPEG_MSG("JPEG Decoder src Size : %d\n", inParams.srcStreamSize);
            JPEG_MSG("JPEG Decoder dst Size : %d\n", inParams.dstBufferSize);
            JPEG_MSG("JPEG Decoder src format : %d\n", inParams.samplingFormat);
            JPEG_MSG("JPEG Decoder mcu row : %d\n", inParams.mcuRow);
            JPEG_MSG("JPEG Decoder mcu column : %d\n", inParams.mcuColumn);
            
            dec_src_size = inParams.srcStreamSize;
            now_size = inParams.srcStreamSize;
            
            if(!inParams.isPhyAddr)
            {
                dec_dst_size = inParams.dstBufferSize;
                /*
                if(dec_dst_size > 1024*512) 
                {
                    printk("JPEG Decoder : not enough physical memory\n");
                    return -EFAULT;
                }*/
                dec_user_va = inParams.dstBufferVA;
                dec_src_va = dma_alloc_coherent(0, dec_src_size + 16, &dec_src_pa, GFP_KERNEL);
                memset(dec_src_va, 0, (dec_src_size + 16));
                JPEG_MSG("dec source pa : 0x%x, va : 0x%x\n", dec_src_pa, dec_src_va);
                if(dec_src_va == 0)
                {
                    printk("JPEG Decoder : allocate memory error\n");
                    //jpeg_drv_dec_deinit();
                    return -EFAULT;
                }

                dec_dst_va = dma_alloc_coherent(0, dec_dst_size, &dec_dst_pa, GFP_KERNEL);                
                if(dec_dst_va == 0)
                {
                    printk("JPEG Decoder : allocate memory error\n");
                    //jpeg_drv_dec_deinit();
                    return -EFAULT;
                }
                    
                if(copy_from_user(dec_src_va, (void *)inParams.srcStreamAddr, dec_src_size))
                {
                    printk("JPEG Decoder : Copy from user error\n");
                    //jpeg_drv_dec_deinit();
                    return -EFAULT;
                }
                
                if(copy_to_user(inParams.dstBufferPA, &dec_dst_pa, sizeof(unsigned int)))
                {
                    printk("JPEG Decoder : Copy to user error\n");
                    //jpeg_drv_dec_deinit();
                    return -EFAULT;
                }
                
            }
            else
            {
                dec_src_va = 0;
                dec_dst_va = 0;
                dec_src_pa = inParams.srcStreamAddr;
            }

            if(inParams.needTempBuffer == 1)
            {
                temp_buf_size = inParams.tempBufferSize;
                temp_buf_va = dma_alloc_coherent(0, temp_buf_size, &temp_buf_pa, GFP_KERNEL);                
                if(temp_buf_va == 0)
                {
                    printk("JPEG Decoder : allocate memory error\n");
                    //jpeg_drv_dec_deinit();
                    return -EFAULT;
                }  

                if(copy_to_user(inParams.tempBufferPA, &temp_buf_pa, sizeof(unsigned int)))
                {
                    printk("JPEG Decoder : Copy to user error\n");
                    //jpeg_drv_dec_deinit();
                    return -EFAULT;
                }
            }
            else
            {
                temp_buf_va = 0;
            }
                       
            // 0. reset    
            jpeg_drv_dec_reset();

            // 1. set source address
            jpeg_drv_dec_set_file_buffer(dec_src_pa , dec_src_size);

            // 2. set table address
            //table_buffer_va = dma_alloc_coherent(0, TABLE_SIZE, &table_buffer_pa, GFP_KERNEL);
            table_buffer_pa = alloc_internal_sram(INTERNAL_SRAM_JPEG_DECODER, 4096, 2048);
            if(table_buffer_pa == 0)
            {
                printk("JPEG Decoder : table pa == 0!!!\n");
                return -EFAULT;
            }
            jpeg_drv_dec_set_table_address(table_buffer_pa);

            // 3. set sampling factor
            if(1 != jpeg_drv_dec_set_sampling_factor_related(inParams.samplingFormat))
            {
                printk("JPEG Decoder : Sampling Factor Unsupported!!!\n");
                return -EFAULT;
            }

            // 4. set component id
            if(inParams.componentNum == 1)
            {
                jpeg_drv_dec_set_component_id(inParams.componentID[0], 0, 0);
            }
            else
            {
                jpeg_drv_dec_set_component_id(inParams.componentID[0], inParams.componentID[1], inParams.componentID[2]);
            }
            
            // 5. set tatal mcu number
            jpeg_drv_dec_set_total_mcu(inParams.mcuRow * inParams.mcuColumn);

            // set mcu number per row
            //jpeg_drv_dec_set_mcu_per_row();

            // 6. set each DU
            for(i = 0 ; i < inParams.componentNum ; i++)
            {
                jpeg_drv_dec_set_du(i, inParams.totalDU[i], inParams.dummyDU[i], inParams.duPerMCURow[i]);
            }
            
            // 7. set file size
            jpeg_drv_dec_set_file_size(dec_src_size + 16);

            // 8. set Q-table id
            if(inParams.componentNum == 1)
            {
                jpeg_drv_dec_set_q_table_id(inParams.qTableSelector[0], 0, 0);
            }
            else
            {
                jpeg_drv_dec_set_q_table_id(inParams.qTableSelector[0], inParams.qTableSelector[1], inParams.qTableSelector[2]);
            }

            break;

        case JPEG_DEC_IOCTL_RANGE:
            JPEG_MSG("JPEG Decoder : JPEG_DEC_IOCTL_RANGE\n");

            if(copy_from_user(&rangeParams, (void *)arg, sizeof(JPEG_DEC_RANGE_IN)))
            {
                printk("JPEG Decoder : Copy from user error\n");
                return -EFAULT;
            }

            retValue = jpeg_check_dec_id(rangeParams.decID);
            if(retValue != 0)
            {
                return retValue;
            }

            JPEG_MSG("start :%d end:%d s1:%d s2:%d idct:%d\n", rangeParams.startIndex, rangeParams.endIndex, rangeParams.skipIndex1,rangeParams.skipIndex2, rangeParams.idctNum);
            jpeg_drv_dec_set_idct_index(rangeParams.startIndex, rangeParams.endIndex);
            jpeg_drv_dec_set_idct_skip_index(rangeParams.skipIndex1, rangeParams.skipIndex2);
            jpeg_drv_dec_set_idec_num(rangeParams.idctNum);
            jpeg_drv_dec_range_enable();
            
            break;
        
        case JPEG_DEC_IOCTL_START:
            // copy input parameters
            JPEG_MSG("JPEG Decoder : JPEG_DEC_IOCTL_START\n");
            if(copy_from_user(&userID, (void *)arg, sizeof(int)))
            {
                printk("JPEG Decoder : Copy from user error\n");
                return -EFAULT;
            }
            
            retValue = jpeg_check_dec_id(userID);
            if(retValue == 0)
            {
                table_buffer_pa = alloc_internal_sram(INTERNAL_SRAM_JPEG_DECODER, 4096, 2048);
                if(table_buffer_pa == 0)
                {
                    printk("JPEG Decoder : table pa == 0!!!\n");
                    return -EFAULT;
                }
                jpeg_drv_dec_set_table_address(table_buffer_pa);
                jpeg_drv_dec_start();
            }

            return retValue;
            
        case JPEG_DEC_IOCTL_RESUME:
            JPEG_MSG("JPEG Decoder : JPEG_DEC_IOCTL_RESUME (Enter)\n");
            if(copy_from_user(&reParams, (void *)arg, sizeof(JPEG_DEC_RESUME_IN)))
            {
                printk("JPEG Decoder : Copy from user error\n");
                return -EFAULT;
            }

            retValue = jpeg_check_dec_id(reParams.decID);
            if(retValue != 0)
            {
                return retValue;
            }

            if(dec_src_va != 0)
            {
                dma_free_coherent(0, dec_src_size + 16, dec_src_va, dec_src_pa);
                dec_src_va = 0;
            }

            dec_src_size = reParams.srcStreamSize;
            
            if(reParams.isPhyAddr == 0)
            {           
                JPEG_MSG("Resume alloc memory size : %d\n", dec_src_size);
                dec_src_va = dma_alloc_coherent(0, dec_src_size + 16, &dec_src_pa, GFP_KERNEL);
                memset(dec_src_va, 0, (dec_src_size + 16));
                if(dec_src_va == 0)
                {
                    printk("JPEG Decoder : allocate memory error\n");
                    //jpeg_drv_dec_deinit();
                    return -EFAULT;
                }
                
                if(copy_from_user(dec_src_va, (void *)reParams.srcStreamAddr, dec_src_size))
                {
                    printk("JPEG Decoder : Copy from user error\n");
                    //jpeg_drv_dec_deinit();
                    return -EFAULT;
                }
            }
            else
            {
                dec_src_va = 0;
                dec_src_pa = reParams.srcStreamAddr;
            }

            now_size += dec_src_size;

            if(table_buffer_pa == 0)
            {
                printk("JPEG Decoder : table pa == 0!!!\n");
                return -EFAULT;
            }

            jpeg_drv_dec_set_file_size(now_size + 16);
            jpeg_drv_dec_set_file_buffer(dec_src_pa , dec_src_size);
            
            break;
            
        case JPEG_DEC_IOCTL_WAIT:
            if(copy_from_user(&outParams, (void *)arg, sizeof(JPEG_DEC_DRV_OUT)))
            {
                printk("JPEG Decoder : Copy from user error\n");
                return -EFAULT;
            }

            retValue = jpeg_check_dec_id(outParams.decID);
            if(retValue != 0)
            {
                return retValue;
            }
            //set timeout
            timeout_jiff = outParams.timeout* HZ / 1000;
            JPEG_MSG("JPEG Decoder Time Jiffies : %ld\n", timeout_jiff);   
            wait_event_interruptible_timeout(decWaitQueue, _jpeg_dec_int_status, timeout_jiff);
            decResult = jpeg_drv_dec_get_result();
            _jpeg_dec_int_status = 0;
            if(decResult == 0)
                jpeg_drv_dec_reset();
            JPEG_MSG("Decode Result : %d [0x%x] [0x%x]\n", decResult, *(volatile kal_uint32 *)(JPEG_BASE + 0x94), *(volatile kal_uint32 *)(JPEG_BASE + 0x98));
            JPEG_MSG("MCU number per row : %d\n", *(volatile kal_uint32 *)(JPEG_BASE + 0x88));
            JPEG_MSG("index1 : %d\n", *(volatile kal_uint32 *)(JPEG_BASE + 0x8C));
            JPEG_MSG("index2 : %d\n", *(volatile kal_uint32 *)(JPEG_BASE + 0x90));
            JPEG_MSG("end index : %d\n", *(volatile kal_uint32 *)(JPEG_BASE + 0x84));
            if(copy_to_user(outParams.result, &decResult, sizeof(unsigned int)))
            {
                printk("JPEG Decoder : Copy to user error (result)\n");
                //jpeg_drv_dec_deinit();
                return -EFAULT;            
            }
            break;
            
        case JPEG_DEC_IOCTL_COPY:
                        // copy input parameters
            JPEG_MSG("JPEG_DEC_IOCTL_COPY Copy decode data\n");
            
            if(copy_from_user(&userID, (void *)arg, sizeof(int)))
            {
                printk("JPEG Decoder : Copy from user error\n");
                return -EFAULT;
            }
            
            retValue = jpeg_check_dec_id(userID);
            if(retValue != 0)
            {
                return -EFAULT;
            }

            if(dec_dst_va != 0)
            {
                retValue = copy_to_user(dec_user_va, dec_dst_va, dec_dst_size);
                if(retValue != 0)
                {
                    printk("JPEG Decoder : Copy to user error %d\n", retValue);
                    //jpeg_drv_dec_deinit();
                    return -EFAULT;
                }
            }

            break;
            
        case JPEG_DEC_IOCTL_DEINIT:
            // copy input parameters
            if(copy_from_user(&userID, (void *)arg, sizeof(int)))
            {
                printk("JPEG Decoder : Copy from user error\n");
                return -EFAULT;
            }
            
            retValue = jpeg_check_dec_id(userID);
            if(retValue != 0)
            {
                return -EFAULT;
            }

            jpeg_drv_dec_deinit();
            
            if(dec_src_va != 0)
            {
                dma_free_coherent(0, dec_src_size + 16, dec_src_va, dec_src_pa);
                dec_src_va = 0;
            }

            if(dec_dst_va != 0)
            {
                dma_free_coherent(0, dec_dst_size, dec_dst_va, dec_dst_pa);
                dec_dst_va = 0;
            }

            if(temp_buf_va != 0)
            {
                dma_free_coherent(0, temp_buf_size, temp_buf_va, temp_buf_pa);
                temp_buf_va = 0;
            }

            if(table_buffer_pa != 0)
            {
                free_internal_sram(INTERNAL_SRAM_JPEG_DECODER, table_buffer_pa);
                table_buffer_pa = 0;
            }
            break;
    }
    return 0;
}

extern int is_pmem_range(unsigned long *base, unsigned long size);

static int jpeg_enc_ioctl(unsigned int cmd, unsigned long arg)
{
    int retValue;
    int userID;
    long timeout_jiff;
    unsigned int fileSize, encResult;
    JPEG_ENC_DRV_IN inParams;
    JPEG_ENC_DRV_OUT outParams;
    
    switch(cmd)
    {       
        // initial and reset JPEG encoder
        case JPEG_ENC_IOCTL_INIT: 
            JPEG_MSG("JPEG Encoder Initial and Lock\n");
            
            retValue = jpeg_drv_enc_init();
            if(retValue == 0)
            {
                if(copy_to_user((int *)arg, &encID, sizeof(encID)))
                {
                    printk("JPEG Encoder : Copy to user error\n");
                    retValue = -EFAULT;
                }
            }
            
            if(retValue != 0)
            {
                jpeg_drv_enc_deinit();
                return -EBUSY;   
            }       
            break;

        // Configure the register
        case JPEG_ENC_IOCTL_CONFIG:
            JPEG_MSG("JPEG Encoder Configure Hardware\n");
            // copy input parameters
            if(copy_from_user(&inParams, (void *)arg, sizeof(JPEG_ENC_DRV_IN)))
            {
                printk("JPEG Encoder : Copy from user error\n");
                return -EFAULT;
            }

            retValue = jpeg_check_enc_id(inParams.encID);
            if(retValue != 0)
            {
                return retValue;
            }
            if(inParams.allocBuffer)
            {
                dstBufferSize = inParams.dstBufferSize;
                dstUserVA = inParams.dstBufferAddr;
                dstBufferVA = dma_alloc_coherent(0, dstBufferSize, &dstBufferPA, GFP_KERNEL);
            }
            else
            {
                dstBufferSize = 0;
                dstBufferPA = (unsigned int)inParams.dstBufferAddr;
            }

           
            //JPEG_MSG("JPEG Encoder Buffer Address : %d\n", inParams.dstBufferAddr);
            JPEG_MSG("JPEG Encoder Buffer Size : %d\n", inParams.dstBufferSize);
            JPEG_MSG("JPEG Encoder Buffer Width : %d\n", inParams.dstWidth);
            JPEG_MSG("JPEG Encoder Buffer Height : %d\n", inParams.dstHeight);
            JPEG_MSG("JPEG Encoder Buffer Format : %d\n", inParams.dstFormat);
            JPEG_MSG("JPEG Encoder Buffer Quality : %d\n", inParams.dstQuality);

            // 0. reset 
            jpeg_drv_enc_reset();

            // 1. set dst address
            if(is_pmem_range(dstBufferPA, inParams.dstBufferSize) == 0) 
            {
                printk("Jpeg Encoder PMEM Out of Range (0x%x %d)", dstBufferPA, inParams.dstBufferSize);
            }
            jpeg_drv_enc_set_dst_buffer_info(dstBufferPA, inParams.dstBufferSize, 0);

            // 2. set file format
            jpeg_drv_enc_set_file_format(inParams.enableEXIF);

            // 3. set quality
            jpeg_drv_enc_set_quality(inParams.dstQuality);

            // 4. single run
            jpeg_drv_enc_set_mode(0, 0);

            // 5. set sampling factor
            if(jpeg_drv_enc_set_sample_format_related(inParams.dstWidth, inParams.dstHeight, inParams.dstFormat))
            {
                printk("JPEG Encoder : Unvalid YUV Format\n");
                jpeg_drv_enc_deinit();
                return -EINVAL;
            }
            break;
            
        case JPEG_ENC_IOCTL_WAIT:
            if(copy_from_user(&outParams, (void *)arg, sizeof(JPEG_ENC_DRV_OUT)))
            {
                printk("JPEG Encoder : Copy from user error\n");
                return -EFAULT;
            }

            retValue = jpeg_check_enc_id(outParams.encID);
            if(retValue != 0)
            {
                return retValue;
            }
            //set timeout
            timeout_jiff = outParams.timeout* HZ / 1000;
            JPEG_MSG("JPEG Encoder Time Jiffies : %ld\n", timeout_jiff);   
            wait_event_interruptible_timeout(encWaitQueue, _jpeg_enc_int_status, timeout_jiff);
            encResult = jpeg_drv_enc_get_result(&fileSize);

            JPEG_MSG("Result : %d, Size : %d\n", encResult, fileSize);
            if(copy_to_user(outParams.fileSize, &fileSize, sizeof(unsigned int)))
            {
                printk("JPEG Encoder : Copy to user error (file size)\n");
                jpeg_drv_enc_deinit();
                return -EFAULT;
            }
            
            if(copy_to_user(outParams.result, &encResult, sizeof(unsigned int)))
            {
                printk("JPEG Encoder : Copy to user error (result)\n");
                jpeg_drv_enc_deinit();
                return -EFAULT;            
            }
            
            if(dstBufferSize != 0)
            {
                JPEG_MSG("Copy Data to User\n");
                if(copy_to_user(dstUserVA, dstBufferVA, fileSize))
                {
                    printk("JPEG Encoder : Copy to user error (dstbuffer)\n");
                    jpeg_drv_enc_deinit();
                    return -EFAULT; 
                }
                dma_free_coherent(0, dstBufferSize, dstBufferVA, dstBufferPA);
            }
            
            break;
            
        case JPEG_ENC_IOCTL_DEINIT:
            // copy input parameters
            if(copy_from_user(&userID, (void *)arg, sizeof(int)))
            {
                printk("JPEG Encoder : Copy from user error\n");
                return -EFAULT;
            }
            
            retValue = jpeg_check_enc_id(userID);
            if(retValue == 0)
            {
                jpeg_drv_enc_deinit();
            }

            return retValue;
    }
    
    return 0;
}
//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
static int jpeg_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    switch(cmd)
    {
        case JPEG_DEC_IOCTL_INIT:
        case JPEG_DEC_IOCTL_CONFIG:
        case JPEG_DEC_IOCTL_START:
        case JPEG_DEC_IOCTL_RANGE:
        case JPEG_DEC_IOCTL_RESUME:
        case JPEG_DEC_IOCTL_WAIT:
        case JPEG_DEC_IOCTL_COPY:
        case JPEG_DEC_IOCTL_DEINIT:
            return jpeg_dec_ioctl(cmd, arg);

        case JPEG_ENC_IOCTL_INIT: 
        case JPEG_ENC_IOCTL_CONFIG:
        case JPEG_ENC_IOCTL_WAIT:
        case JPEG_ENC_IOCTL_DEINIT:
            return jpeg_enc_ioctl(cmd, arg);
            
        default :
            break; 
    }
    
    return -EINVAL;
}

static int jpeg_open(struct inode *inode, struct file *file)
{
    //printk("MT6516 jpeg codec open\n");
		return 0;
}

static ssize_t jpeg_read(struct file *file, char __user *data, size_t len, loff_t *ppos)
{
    printk("MT6516 jpeg read\n");
    return 0;
}

static int jpeg_release(struct inode *inode, struct file *file)
{
    //printk("MT6516 jpeg release\n");
	return 0;
}

static int jpeg_flush(struct file * a_pstFile , fl_owner_t a_id)
{
    if(encID != 0) 
    {
        printk("Error! Enable error handling for jpeg encoder");
        jpeg_drv_enc_deinit();
    }

    if(decID != 0) 
    {
        printk("Error! Enable error handling for jpeg decoder");
        jpeg_drv_dec_deinit();
    }
    
    return 0;
}

/* Kernel interface */
static struct file_operations jpeg_fops = {
	.owner		= THIS_MODULE,
	.ioctl		= jpeg_ioctl,
	.open		= jpeg_open,
	.release	= jpeg_release,
	.flush		= jpeg_flush,
	.read       = jpeg_read,
};

static int jpeg_probe(struct platform_device *pdev)
{
    struct class_device;
    
	int ret;
    struct class_device *class_dev = NULL;
    
    printk("MT6516 jpeg probe\n");
	ret = alloc_chrdev_region(&jpeg_devno, 0, 1, JPEG_DEVNAME);

	if(ret)
	{
	    printk("Error: Can't Get Major number for JPEG Device\n");
	}
	else
	    printk("Get JPEG Device Major number (%d)\n", jpeg_devno);

	jpeg_cdev = cdev_alloc();
    jpeg_cdev->owner = THIS_MODULE;
	jpeg_cdev->ops = &jpeg_fops;

	ret = cdev_add(jpeg_cdev, jpeg_devno, 1);

    jpeg_class = class_create(THIS_MODULE, JPEG_DEVNAME);
    class_dev = (struct class_device *)device_create(jpeg_class, NULL, jpeg_devno, NULL, JPEG_DEVNAME);

    spin_lock_init(&jpegDecLock);
    spin_lock_init(&jpegEncLock);

    // initial decoder, register decoder ISR
    decID = 0;
    decCount = 1;
    _jpeg_dec_int_status = 0;

    MT6516_IRQSensitivity(MT6516_JPEG_DEC_IRQ_LINE, MT6516_LEVEL_SENSITIVE);
    MT6516_IRQUnmask(MT6516_JPEG_DEC_IRQ_LINE);

    if(request_irq(MT6516_JPEG_DEC_IRQ_LINE, jpeg_drv_isr, 0, "jpeg_dec" , NULL))
    {
        printk("JPEG Decoder request irq failed\n");
    }

    init_waitqueue_head(&decWaitQueue);  


    // initial encoder, register encoder ISR
    encID = 0;
    encCount = 1;
    _jpeg_enc_int_status = 0;
    
    MT6516_IRQSensitivity(MT6516_JPEG_ENC_IRQ_LINE, MT6516_LEVEL_SENSITIVE);
    MT6516_IRQUnmask(MT6516_JPEG_ENC_IRQ_LINE);

    if(request_irq(MT6516_JPEG_ENC_IRQ_LINE, jpeg_drv_isr, 0, "jpeg_enc" , NULL))
    {
        printk("JPEG Encode request irq failed\n");
    }

    init_waitqueue_head(&encWaitQueue);   
    
	printk("JPEG Probe Done\n");

	NOT_REFERENCED(class_dev);
	return 0;
}

static int jpeg_remove(struct platform_device *pdev)
{
	printk("MT6516 JPEG Codec remove\n");
	//unregister_chrdev(JPEGDEC_MAJOR, JPEGDEC_DEVNAME);
	free_irq(MT6516_JPEG_ENC_IRQ_LINE, NULL);
	
	printk("Done\n");
	return 0;
}

static void jpeg_shutdown(struct platform_device *pdev)
{
	printk("JPEG Codec shutdown\n");
	/* Nothing yet */
}

/* PM suspend */
static int mt6516_jpeg_suspend(struct platform_device *pdev, pm_message_t mesg)
{
    //jpeg_drv_power_off();
    return 0;
}

/* PM resume */
static int mt6516_jpeg_resume(struct platform_device *pdev)
{
    //jpeg_drv_power_on();
    return 0;
}


static struct platform_driver jpeg_driver = {
	.probe		= jpeg_probe,
	.remove		= jpeg_remove,
	.shutdown	= jpeg_shutdown,
	.suspend	= mt6516_jpeg_suspend,
	.resume		= mt6516_jpeg_resume,
	.driver     = {
	              .name = JPEG_DEVNAME,
	},
};

static void mt6516_jpeg_release(struct device *dev)
{
	// Nothing to release? 
}

static u64 jpegdec_dmamask = ~(u32)0;

static struct platform_device jpeg_device = {
	.name	 = JPEG_DEVNAME,
	.id      = 0,
	.dev     = {
		.release = mt6516_jpeg_release,
		.dma_mask = &jpegdec_dmamask,
		.coherent_dma_mask = 0xffffffff,
	},
	.num_resources = 0,
};

static int __init jpeg_init(void)
{
    int ret;

    printk("JPEG Codec initialize\n");
	
    printk("Register the JPEG Codec device\n");
	if(platform_device_register(&jpeg_device))
	{
        printk("failed to register jpeg codec device\n");
        ret = -ENODEV;
        return ret;
	}

    printk("Register the JPEG Codec driver\n");    
    if(platform_driver_register(&jpeg_driver))
    {
        printk("failed to register jpeg codec driver\n");
        platform_device_unregister(&jpeg_device);
        ret = -ENODEV;
        return ret;
    }

    return 0;
}

static void __exit jpeg_exit(void)
{
    cdev_del(jpeg_cdev);
    unregister_chrdev_region(jpeg_devno, 1);
	//printk("Unregistering driver\n");
    platform_driver_unregister(&jpeg_driver);
	platform_device_unregister(&jpeg_device);
	
	device_destroy(jpeg_class, jpeg_devno);
	class_destroy(jpeg_class);
	
	printk("Done\n");
}

module_init(jpeg_init);
module_exit(jpeg_exit);
MODULE_AUTHOR("Tzu-Meng, Chung <Tzu-Meng.Chung@mediatek.com>");
MODULE_DESCRIPTION("MT6516 JPEG Codec Driver");
MODULE_LICENSE("GPL");
