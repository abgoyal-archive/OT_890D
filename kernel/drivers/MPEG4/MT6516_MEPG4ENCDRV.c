
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
#include <linux/sampletrigger.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <asm/page.h>
#include <linux/vmalloc.h>
#include <mach/mt6516_pll.h>
#include <mach/mt6516_typedefs.h> 
#include <mach/mt6516_reg_base.h> 
#include <mach/mt6516_mm_mem.h>
#include <mach/mt6516_IDP.h>
#include <linux/interrupt.h>
#include <mach/irqs.h>
#include <linux/delay.h>  
#include <asm/cacheflush.h>
#include "MT6516_MPEG4ENCDRV.h"
#include "MT6516_MPEG4ENCDRVREG.h"
#include "MT6516_MPEG4_UTILITY.h"
#include <asm/tcm.h>

/* GMC PORT */
#define GMC2_MUX_PORT_SEL          (volatile unsigned long *)(GMC2_BASE+0x58)     /*RW*/

//#define  MP4_ENC_TIME_PROFILE 

#define MPEG4_ENCODER_DEVNAME     "MT6516_MP4_ENC"
static struct class *mpeg4_encoder_class = NULL; //for android.

//#define DRV_WriteReg32(addr,data)     ((*(volatile UINT32 *)(addr)) = (UINT32)(data))
//#define DRV_Reg32(addr)               (*(volatile UINT32 *)(addr))
#define CODEC_DRV_WriteReg32(addr,data)     ((*(volatile UINT32 *)(addr)) = (UINT32)(data))
#define CODEC_DRV_Reg32(addr)               (*(volatile UINT32 *)(addr))

//#define HW_WRITE(ptr,data) (*(ptr) = (data))
//#define HW_READ(ptr)       (*(ptr))
#define BS_BUF_SIZE		128000
#define USE_INTERNAL_SRAM		1

#define DMA_ALLOC(ptr,dev,size,hdl,flag) \
    do { \
    ptr = dma_alloc_coherent(dev, size, hdl, flag); \
    BUG_ON(!ptr); \
    printk("Alloc addr=0x%08x, LINE:%d\n", (UINT32)ptr, __LINE__); \
    } while (0)

#define DMA_FREE(dev, size, ptr, hdl)\
    do { \
    printk("Free addr=0x%08x, LINE:%d\n", (UINT32)ptr, __LINE__); \
    dma_free_coherent(dev, size, ptr, hdl); \
    } while (0)


static mp4_enc_info g_mp4_enc_hal_info;
static dev_t mpeg4_encoder_devno;
static struct cdev *mpeg4_encoder_cdev;
static struct fasync_struct *mpeg4_encoder_async = NULL;
static mp4_enc_proc_start_struct g_mp4_enc_hal_proc_start_param;
static BOOL g_enc_interrupt_handler = false;
//static UINT8 *p_mp4_enc_hal_info_ret;
static BOOL g_enc_open = false;
static BOOL g_enc_init = false;
static wait_queue_head_t encWaitQueue;
static UINT32 g_stuffing_bitcnt = 0;

extern void * mm_kmalloc_sram(size_t size, gfp_t flags);
extern unsigned long mm_k_virt_to_phys_sram(void * VA);
extern void mm_kfree_sram(const void* VA);
extern void * mm_vmalloc_sram(unsigned long size);
extern unsigned long mm_v_virt_to_phys_sram(void * VA);
extern void mm_vfree_sram(const void* VA);

extern BOOL hwEnableSubsys(MT6516_SUBSYS subsysId);
//extern BOOL hwEnableClock(MT6516_CLOCK clockId, char *mode_name);
extern BOOL hwDisableSubsys(MT6516_SUBSYS subsysId);

static void mpeg4_encoder_reset(void);
static int mpeg4_encoder_start(UINT8 *pParam);
static int mpeg4_encoder_init(UINT8 *pParam);
static int mpeg4_encoder_open(struct inode *inode, struct file *file);
static int mpeg4_encoder_flush(struct file *file, fl_owner_t id);
static int mpeg4_encoder_release(struct inode *inode, struct file *file);
static int mpeg4_encoder_free_alloc_buffer(void);

 unsigned long ts[16];


//-----------------------------------------------------------------------------
//static irqreturn_t mp4_enc_intr_dlr(int irq, void *dev_id)
static __tcmfunc irqreturn_t mp4_enc_intr_dlr(int irq, void *dev_id)
{
    UINT32 temp_ptr, temp_int;
    UINT32 bitcnt;
    UINT32 frame_length;
    UINT32 status;
    BOOL b_DMA = FALSE;
    BOOL b_ret = FALSE;	
    //RC_UPDATE_STRUCT rc_data;
    UINT8 *tmp_ptr = NULL;
    //UINT32 i;

    //printk("+mp4_enc_intr_dlr\r\n");
#ifdef  MP4_ENC_TIME_PROFILE 
ts[7] = sampletrigger(0, 0, 1);
#endif

    status = CODEC_DRV_Reg32(MP4_ENC_IRQ_STS);
    CODEC_DRV_WriteReg32(MP4_ENC_IRQ_ACK, status);

    if (status & MP4_ENC_IRQ_STS_ENC)
    {
        //printk("+mp4_enc_intr_dlr: mp4 encode stat ENC\r\n");
        temp_ptr = CODEC_DRV_Reg32(MP4_ENC_VLC_WORD);
        temp_int = CODEC_DRV_Reg32(MP4_ENC_VLC_BITCNT);
				g_stuffing_bitcnt = 0;
				
        // If hit reach limit condition,
        // we don't need to exchange, because the frame is going to be dropped
        if (!g_mp4_enc_hal_proc_start_param.mp4_enc_prev_bitstream_reach_limit_flag)
        {	

        //		if(g_mp4_enc_info.b_reach_limit == FALSE)
        {
            //bitcnt = (temp_ptr - (kal_int32)g_mp4_enc_info.p_enc_bitstreams_ptr) * 8 + temp_int;
            // Need to add reach DMA limit counted bit count
            bitcnt = (temp_ptr - (UINT32)g_mp4_enc_hal_info.p_enc_bitstreams_ptr) * 8 + temp_int + ((UINT32)g_mp4_enc_hal_proc_start_param.g_encoded_bitstream_length * 8);
            g_stuffing_bitcnt = bitcnt;  
            while ((bitcnt & 7) != 0)
            {
            	bitcnt++;
            }       	
            frame_length = (bitcnt / 8) + (((bitcnt & 7) == 0) ? 0 : 1);
            g_mp4_enc_hal_info.enc_toal_save_bytes+=frame_length; 	
            g_mp4_enc_hal_info.frame_size = frame_length;	
            g_mp4_enc_hal_info.enc_trigger_limit = 0;
            g_mp4_enc_hal_info.enc_total_frame++;
            b_ret = true;	

            g_mp4_enc_hal_proc_start_param.enc_toal_save_bytes = g_mp4_enc_hal_info.enc_toal_save_bytes;
            g_mp4_enc_hal_proc_start_param.frame_size = g_mp4_enc_hal_info.frame_size;
            g_mp4_enc_hal_proc_start_param.enc_trigger_limit = g_mp4_enc_hal_info.enc_trigger_limit;
            g_mp4_enc_hal_proc_start_param.enc_total_frame = g_mp4_enc_hal_info.enc_total_frame;
            /**/
            //printk("enc done, len:%d, first 20:\n ", frame_length);
            tmp_ptr = (UINT8 *)g_mp4_enc_hal_info.v_enc_bitstreams_ptr;
            //for(i=0; i<frame_length; i++)
            /*
            for(i=0; i<20; i++)
            {
                printk("0x%X ", *(tmp_ptr+i));
            }		
            */

        }
        //		else
        //		{
        //			RETAILMSG(DEBUG_CTL, (TEXT("mp4_enc_intr_dlr: encode failed\r\n")));
        //			b_ret = FALSE;
        //		}

		 
            /*address exchange */
            temp_int = CODEC_DRV_Reg32(MP4_ENC_REF_ADDR);
            CODEC_DRV_WriteReg32(MP4_ENC_REF_ADDR, CODEC_DRV_Reg32(MP4_ENC_REC_ADDR));
            CODEC_DRV_WriteReg32(MP4_ENC_REC_ADDR, temp_int);
            //printk("addr exchange,MP4_ENC_REF_ADDR:0x%08X, MP4_ENC_REC_ADDR:0x%08X\n", CODEC_DRV_Reg32(MP4_ENC_REF_ADDR), CODEC_DRV_Reg32(MP4_ENC_REC_ADDR));
        }
        else
        {            
            b_ret = true;
        }

        //printk("-mp4_enc_intr_dlr: mp4 encode stat ENC\r\n");
    }
    else if(status&MP4_ENC_IRQ_STS_DMA)
    {
        //printk("+mp4_enc_intr_dlr: mp4 encode stat DMA\r\n");
        b_ret = FALSE;
        b_DMA = TRUE;
        //g_dma_limit_count++;

        // The following are to issue a new DMA command
        {
            DWORD last_dma_limit_length;
            DWORD dma_limit_length;


            //printk("mp4_enc_intr_dlr: Reach limit: 0x%08x\r\n", status);

            last_dma_limit_length = CODEC_DRV_Reg32(MP4_ENC_VLC_LIMIT);

            g_mp4_enc_hal_proc_start_param.g_encoded_bitstream_length += last_dma_limit_length * 4;

            //printk("mp4_enc_intr_dlr: BitstreamBuffLength: 0x%d, Encoded Length: 0x%d\r\n", g_mp4_enc_hal_proc_start_param.g_real_bitstream_length, g_mp4_enc_hal_proc_start_param.g_encoded_bitstream_length);

            if (g_mp4_enc_hal_proc_start_param.g_real_bitstream_length <= g_mp4_enc_hal_proc_start_param.g_encoded_bitstream_length)
            {
                // Bitstream buffer is NOT enough for encode
                // When hit the problem, we just re-configure limit and encode start address as the original setting
                // Finally, this frame should be dropped and next frame should be I frame
                g_mp4_enc_hal_proc_start_param.mp4_enc_prev_bitstream_reach_limit_flag = TRUE;
                dma_limit_length = g_mp4_enc_hal_proc_start_param.g_real_bitstream_length;
                CODEC_DRV_WriteReg32(MP4_CORE_VLC_LIMIT, (dma_limit_length / 4));

                // Set encode start addr to the start addr of bitstream buffer
                //CODEC_DRV_WriteReg32(MP4_CORE_VLC_ADDR, g_curr_vlc_addr);
                CODEC_DRV_WriteReg32(MP4_CORE_VLC_ADDR, g_mp4_enc_hal_proc_start_param.g_encoded_bitstream_start_addr);

                CODEC_DRV_WriteReg32(MP4_VLC_DMA_COMD, MP4_VLC_DMA_COMD_RELOAD);
                //printk("mp4_enc_intr_dlr: Bitstream buffer length is too small\r\n");
            }
            else
            {
                // Process next
                dma_limit_length = g_mp4_enc_hal_proc_start_param.g_real_bitstream_length - g_mp4_enc_hal_proc_start_param.g_encoded_bitstream_length;
                g_mp4_enc_hal_proc_start_param.g_curr_vlc_addr += g_mp4_enc_hal_proc_start_param.g_encoded_bitstream_length;
                CODEC_DRV_WriteReg32(MP4_CORE_VLC_LIMIT, (dma_limit_length / 4));
                CODEC_DRV_WriteReg32(MP4_CORE_VLC_ADDR, g_mp4_enc_hal_proc_start_param.g_curr_vlc_addr);
                CODEC_DRV_WriteReg32(MP4_VLC_DMA_COMD, MP4_VLC_DMA_COMD_RELOAD);
            }

            //if (last_dma_limit_length == 0xFFFF){
            //	dma_limit_length = g_real_bitstream_length - (0xFFFF * 4 * g_dma_limit_count);
            //	g_curr_vlc_addr += (0xFFFF * 4);
            //	CODEC_DRV_WriteReg32(MP4_CORE_VLC_LIMIT, (dma_limit_length / 4));
            //	CODEC_DRV_WriteReg32(MP4_CORE_VLC_ADDR, g_curr_vlc_addr);
            //	CODEC_DRV_WriteReg32(MP4_VLC_DMA_COMD, MP4_VLC_DMA_COMD_RELOAD);
            //}else{
            //	// The bitstream buffer is really NOT enough
            //	RETAILMSG(1, (TEXT("mp4_enc_intr_dlr: Reach limit: 0x%08x\r\n"), status));
            //	ASSERT(0);
            //	g_mp4_enc_info.b_reach_limit = TRUE;	// This means the bitstream buffer length is NOT enough for encode data
            //	b_DMA = FALSE;
            //}

        }
        //printk("-mp4_enc_intr_dlr: mp4 encode stat DMA\r\n");
    }else{
        ASSERT(0);
        printk("mp4_enc_intr_dlr: ERROR!! Unknown INTR state: 0x%08x\r\n", status);
    }


    if(b_ret == true)
    {
        //		kal_set_eg_events(g_mp4_enc_event_id, EVENT_MP4_ENC_DONE, KAL_OR);
        g_mp4_enc_hal_proc_start_param.g_mp4_enc_event_id = EVENT_MP4_ENC_DONE;
        //printk("mp4_enc_intr_dlr: mp4 encode done\r\n");
        //SetEvent(gmp4_encode_intr_event); 
#ifdef  MP4_ENC_TIME_PROFILE 
ts[8] = sampletrigger(0, 0, 1);     		
#endif
        g_enc_interrupt_handler = true;
        wake_up_interruptible(&encWaitQueue);
        //wake_up(&encWaitQueue);
    }
    else if(b_DMA == false)
    {
        //		kal_set_eg_events(g_mp4_enc_event_id, EVENT_MP4_ENC_ERR, KAL_OR);
        g_mp4_enc_hal_proc_start_param.g_mp4_enc_event_id = EVENT_MP4_ENC_ERR;
        //printk("mp4_enc_intr_dlr: mp4 encode error\r\n");
        //SetEvent(gmp4_encode_intr_event);
#ifdef  MP4_ENC_TIME_PROFILE 
ts[8] = sampletrigger(0, 0, 1);     
#endif
        g_enc_interrupt_handler = true;
        wake_up_interruptible(&encWaitQueue);
        //wake_up(&encWaitQueue);
    }

    //printk("-mp4_enc_intr_dlr\r\n");



    return IRQ_HANDLED;
}

static int mpeg4_encoder_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{   
    int ret;
    /*
    UINT32 temp;
    UINT32 width,height;
    UINT32 * p_temp_ptr_from_user;
    */
   

    switch(cmd)
    { 

    case SET_ENC_HAL_START:
        //printk("----mpeg4_encoder_ioctl SET_ENC_HAL_START----\n");
#ifdef  MP4_ENC_TIME_PROFILE 
sampletrigger(0, 0, 0);
ts[0] = sampletrigger(0, 0, 1);
#endif
        mpeg4_encoder_start((UINT8 *)arg);
        //printk("----mpeg4_decoder_ioctl SET_MP4_CODEC_COMD case----\n");
        //pg_dec_done_param = (dec_done_param *)arg;
        //printk("----DECODE_START----\n");
        //HW_WRITE(MP4_CODEC_COMD, MP4_DEC_START);  // trigger start
        //mp4_dec_intr_dlr();
        
		ret = copy_to_user((UINT8 *)g_mp4_enc_hal_proc_start_param.p_mp4_enc_info, &g_mp4_enc_hal_info, sizeof(mp4_enc_info));
		ret = copy_to_user((UINT8 *)arg, &g_mp4_enc_hal_proc_start_param, sizeof(mp4_enc_proc_start_struct));
#ifdef  MP4_ENC_TIME_PROFILE 
ts[3] = sampletrigger(0, 0, 1);
#endif

#ifdef  MP4_ENC_TIME_PROFILE 
sampletrigger(0, 0, 2); 
///*
printk("enc done, enc:%u, cp1:%u, cp2:%u, cp:%u, cp1:%u, cp2:%u, cp:%u, cp1:%u, cp2:%u\n", 
	(ts[1]-ts[0])/1000,
	(ts[2]-ts[1])/1000,
	(ts[3]-ts[2])/1000,
	(ts[4]-ts[0])/1000,
	(ts[5]-ts[4])/1000,
	(ts[6]-ts[5])/1000,
	(ts[7]-ts[6])/1000,
	(ts[8]-ts[7])/1000,
	(ts[1]-ts[8])/1000);
//*/
#endif
	/*
printk("enc done2, cp:%u, cp1:%u, cp2:%u\n", 
	(ts[4]-ts[0])/1000,
	(ts[5]-ts[4])/1000,
	(ts[6]-ts[5])/1000);
printk("enc done3, cp:%u, cp1:%u, cp2:%u\n", 
	(ts[7]-ts[6])/1000,
	(ts[8]-ts[7])/1000,
	(ts[1]-ts[8])/1000);
	*/
	      //printk("----mpeg4_encoder_ioctl SET_ENC_HAL_START done----\n");
        break;
    case SET_ENC_HAL_CB:
        break;
    case SET_ENC_HAL_REF_BUF:
        break;
    case SET_ENC_HAL_FRM_BUF:
        break;
    case SET_ENC_HAL_BS_BUF:
        break;
    case SET_ENC_HAL_RC:
        break;
    case SET_ENC_HAL_ME:
        break;
    case SET_ENC_HAL_ENC_PARAM:
        break;
    case SET_ENC_HAL_MP4_ENC_PARAM:
        break;
    case SET_ENC_HAL_INIT:				
        mpeg4_encoder_init((UINT8 *) arg);
        break;
    case SET_ENC_HAL_RST:
        mpeg4_encoder_reset();
        break;
    default:
        printk("----mpeg4_encoder_ioctl default case----\n");

        break;
    }
    return 0;
}

static int mpeg4_encoder_start(UINT8 *pParam)
{
    INT32 ret;
    //INT32 i;
    //UINT8 *tmp_ptr = NULL;

    //printk("enter mpeg4_encoder_start()\n");
    ret = copy_from_user(&g_mp4_enc_hal_proc_start_param, pParam, sizeof(mp4_enc_proc_start_struct));
#ifdef  MP4_ENC_TIME_PROFILE 
ts[4] = sampletrigger(0, 0, 1);
#endif
		if(TRUE == g_mp4_enc_hal_info.b_use_pmem)
		{
				g_mp4_enc_hal_info.v_enc_input_yuv_data = (UINT8 *) g_mp4_enc_hal_proc_start_param.u4_MP4_ENC_VOP_ADDR_VA;
				g_mp4_enc_hal_info.p_enc_input_yuv_data = g_mp4_enc_hal_proc_start_param.u4_MP4_ENC_VOP_ADDR_PA;				
				dmac_flush_range(g_mp4_enc_hal_info.v_enc_input_yuv_data, g_mp4_enc_hal_info.v_enc_input_yuv_data+g_mp4_enc_hal_info.enc_input_yuv_data_size);
		}
		else
		{
    		ret = copy_from_user(g_mp4_enc_hal_info.v_enc_input_yuv_data, (void *)g_mp4_enc_hal_proc_start_param.u4_MP4_ENC_VOP_ADDR_VA, g_mp4_enc_hal_info.enc_input_yuv_data_size);
  	}
#ifdef  MP4_ENC_TIME_PROFILE 
ts[5] = sampletrigger(0, 0, 1);
#endif
    /*
    printk("YUV data from kernel:\n");
    tmp_ptr = (UINT8 *)g_mp4_enc_hal_info.v_enc_input_yuv_data;
    for(i = 0; i<20; i++)
    {
        printk("0x%X ", *(tmp_ptr+i));
    }
    */
    //p_mp4_enc_hal_info_ret = g_mp4_enc_hal_proc_start_param.p_mp4_enc_info;

    CODEC_DRV_WriteReg32(MP4_ENC_VOP_ADDR, g_mp4_enc_hal_info.p_enc_input_yuv_data);

    CODEC_DRV_WriteReg32(MP4_ENC_CONF, g_mp4_enc_hal_proc_start_param.u4_MP4_ENC_CONF);

    CODEC_DRV_WriteReg32(MP4_ENC_RESYNC_CONF0, g_mp4_enc_hal_proc_start_param.u4_MP4_ENC_RESYNC_CONF0); 

    CODEC_DRV_WriteReg32(MP4_ENC_VOP_STRUC0, g_mp4_enc_hal_proc_start_param.u4_MP4_ENC_VOP_STRUC0);

    CODEC_DRV_WriteReg32(MP4_ENC_VOP_STRUC1, g_mp4_enc_hal_proc_start_param.u4_MP4_ENC_VOP_STRUC1);
//printk("MP4_ENC_VOP_STRUC1:0x%08X\n ", g_mp4_enc_hal_proc_start_param.u4_MP4_ENC_VOP_STRUC1);
    CODEC_DRV_WriteReg32(MP4_ENC_VOP_STRUC2, g_mp4_enc_hal_proc_start_param.u4_MP4_ENC_VOP_STRUC2);

    CODEC_DRV_WriteReg32(MP4_ENC_VOP_STRUC3, g_mp4_enc_hal_proc_start_param.u4_MP4_ENC_VOP_STRUC3);

    //CODEC_DRV_WriteReg32(MP4_ENC_VLC_ADDR, g_mp4_enc_hal_proc_start_param.u4_MP4_ENC_VLC_ADDR);
    /*
    printk("VOP data from user in kernel:\n ");
    tmp_ptr = (UINT8 *)g_mp4_enc_hal_proc_start_param.u4_MP4_ENC_ADDR;
    for(i=0; i<(g_mp4_enc_hal_proc_start_param.u4_MP4_ENC_VLC_ADDR-g_mp4_enc_hal_proc_start_param.u4_MP4_ENC_ADDR)+4; i++)
    {
        printk("0x%X ", *(tmp_ptr+i));
    }		
    */
		if(TRUE == g_mp4_enc_hal_info.b_use_pmem)
		{
				g_mp4_enc_hal_info.v_enc_bitstreams_ptr = (UINT8 *) g_mp4_enc_hal_proc_start_param.u4_MP4_ENC_ADDR_VA;
				g_mp4_enc_hal_info.p_enc_bitstreams_ptr = g_mp4_enc_hal_proc_start_param.u4_MP4_ENC_ADDR_PA;				
				dmac_flush_range(g_mp4_enc_hal_info.v_enc_bitstreams_ptr, g_mp4_enc_hal_info.v_enc_bitstreams_ptr + g_mp4_enc_hal_proc_start_param.u4_buffer_length);
				//printk("PMEM BS ptr:%08X, %08X", g_mp4_enc_hal_info.v_enc_bitstreams_ptr, g_mp4_enc_hal_info.p_enc_bitstreams_ptr);
		}
		else
		{
				memset(g_mp4_enc_hal_info.v_enc_bitstreams_ptr, 0, BS_BUF_SIZE);    
    		ret = copy_from_user(g_mp4_enc_hal_info.v_enc_bitstreams_ptr, (void *)g_mp4_enc_hal_proc_start_param.u4_MP4_ENC_ADDR_VA, g_mp4_enc_hal_proc_start_param.u4_MP4_ENC_VLC_ADDR-g_mp4_enc_hal_proc_start_param.u4_MP4_ENC_ADDR_PA+4);
  	}
#ifdef  MP4_ENC_TIME_PROFILE 
ts[6] = sampletrigger(0, 0, 1);
#endif
//printk("VLC_ADDR: %08X, %08X", g_mp4_enc_hal_proc_start_param.u4_MP4_ENC_VLC_ADDR, g_mp4_enc_hal_info.p_enc_bitstreams_ptr+(g_mp4_enc_hal_proc_start_param.u4_MP4_ENC_VLC_ADDR-g_mp4_enc_hal_proc_start_param.u4_MP4_ENC_ADDR_PA));
    CODEC_DRV_WriteReg32(MP4_ENC_VLC_ADDR, g_mp4_enc_hal_info.p_enc_bitstreams_ptr+(g_mp4_enc_hal_proc_start_param.u4_MP4_ENC_VLC_ADDR-g_mp4_enc_hal_proc_start_param.u4_MP4_ENC_ADDR_PA));
    /*
    printk("VOP data in kernel:\n ");
    tmp_ptr = (UINT8 *)g_mp4_enc_hal_info.v_enc_bitstreams_ptr;
    for(i=0; i<(g_mp4_enc_hal_proc_start_param.u4_MP4_ENC_VLC_ADDR-g_mp4_enc_hal_proc_start_param.u4_MP4_ENC_ADDR_PA)+4; i++)
    {
        printk("0x%X ", *(tmp_ptr+i));
    }		
		*/
		
    CODEC_DRV_WriteReg32(MP4_ENC_VLC_BIT, g_mp4_enc_hal_proc_start_param.u4_MP4_ENC_VLC_BIT);

    CODEC_DRV_WriteReg32(MP4_ENC_VLC_LIMIT, g_mp4_enc_hal_proc_start_param.u4_MP4_ENC_VLC_LIMIT);
    
    /*
    printk("\n mpeg4_encoder_start settings:\n");
    printk("\n MP4_ENC_VOP_ADDR:0x%08X\n", CODEC_DRV_Reg32(MP4_ENC_VOP_ADDR));
    printk("\n MP4_ENC_CONF:0x%08X\n", CODEC_DRV_Reg32(MP4_ENC_CONF));
    printk("\n MP4_ENC_RESYNC_CONF0:0x%08X\n", CODEC_DRV_Reg32(MP4_ENC_RESYNC_CONF0));
    printk("\n MP4_ENC_VOP_STRUC0:0x%08X\n", CODEC_DRV_Reg32(MP4_ENC_VOP_STRUC0));
    printk("\n MP4_ENC_VOP_STRUC1:0x%08X\n", CODEC_DRV_Reg32(MP4_ENC_VOP_STRUC1));
    printk("\n MP4_ENC_VOP_STRUC2:0x%08X\n", CODEC_DRV_Reg32(MP4_ENC_VOP_STRUC2));
    printk("\n MP4_ENC_VOP_STRUC3:0x%08X\n", CODEC_DRV_Reg32(MP4_ENC_VOP_STRUC3));
    printk("\n MP4_ENC_VLC_ADDR:0x%08X\n", CODEC_DRV_Reg32(MP4_ENC_VLC_ADDR));
    printk("\n MP4_ENC_VLC_BIT:0x%08X\n", CODEC_DRV_Reg32(MP4_ENC_VLC_BIT));
    printk("\n MP4_ENC_VLC_LIMIT:0x%08X\n", CODEC_DRV_Reg32(MP4_ENC_VLC_LIMIT));
    //printk("\n :0x%X\n", (UINT32));
		*/
		g_enc_interrupt_handler = FALSE;   
    CODEC_DRV_WriteReg32(MP4_CODEC_COMD, MP4_CODEC_COMD_ENC_START); // Video Object Plane Level startforfun

		/*
		while(g_enc_interrupt_handler == FALSE)
		{
		    //wait g_dec_interrupt_handler to be TRUE;
		    mdelay(1);	
		}
		*/
		wait_event_interruptible(encWaitQueue, g_enc_interrupt_handler);
		//wait_event(encWaitQueue, g_enc_interrupt_handler);
		g_enc_interrupt_handler = FALSE;   

#ifdef  MP4_ENC_TIME_PROFILE 
ts[1] = sampletrigger(0, 0, 1);
#endif

    //move out from ISR.
    // stuffing bits to byte-aligned    
    while ((g_stuffing_bitcnt & 7) != 0)
    {
		    if (g_mp4_enc_hal_info.enc_im.b_short_header == 0) 
		    {  // stuffing bits: one zero and followed by 1 to 7 ones
		        mp4_putbits((UINT8 *)g_mp4_enc_hal_info.v_enc_bitstreams_ptr, (INT32 *)&g_stuffing_bitcnt,0,1);
		        while ((g_stuffing_bitcnt & 7) != 0)
		        {
		            mp4_putbits((UINT8 *)g_mp4_enc_hal_info.v_enc_bitstreams_ptr, (INT32 *)&g_stuffing_bitcnt,1,1);
		        }
		    } 
		    else 
		    { // short-header mode: stuffing bits all zeros
		        while ((g_stuffing_bitcnt & 7) != 0)
		        {
		            mp4_putbits((UINT8 *)g_mp4_enc_hal_info.v_enc_bitstreams_ptr, (INT32 *)&g_stuffing_bitcnt,0,1);
		        }
		    }	
  	}
  	
		//printk("----mpeg4_encoder_ioctl SET_ENC_HAL_START, leave g_enc_interrupt_handler == FALSE----\n");         
		if(TRUE == g_mp4_enc_hal_info.b_use_pmem)
		{
				dmac_flush_range(g_mp4_enc_hal_info.v_enc_bitstreams_ptr, g_mp4_enc_hal_info.v_enc_bitstreams_ptr + g_mp4_enc_hal_proc_start_param.u4_buffer_length);
		}
		else
		{
				ret = copy_to_user((UINT8 *)g_mp4_enc_hal_proc_start_param.u4_MP4_ENC_ADDR_VA, g_mp4_enc_hal_info.v_enc_bitstreams_ptr, g_mp4_enc_hal_info.frame_size);
		}
#ifdef  MP4_ENC_TIME_PROFILE 
ts[2] = sampletrigger(0, 0, 1);
#endif

    return 0;
}
static int mpeg4_encoder_free_alloc_buffer()
{	
#if	USE_INTERNAL_SRAM
		free_internal_sram(INTERNAL_SRAM_MPEG4_ENCODER, g_mp4_enc_hal_info.enc_working_memory.pa);
#else
    DMA_FREE(0, g_mp4_enc_hal_info.enc_working_memory.size, g_mp4_enc_hal_info.enc_working_memory.va, g_mp4_enc_hal_info.enc_working_memory.pa);
#endif
    if(FALSE == g_mp4_enc_hal_info.b_use_pmem)
    {
    		DMA_FREE(0, g_mp4_enc_hal_info.enc_frame_buffer_addr.size, g_mp4_enc_hal_info.enc_frame_buffer_addr.va, g_mp4_enc_hal_info.enc_frame_buffer_addr.pa);
		    DMA_FREE(0, g_mp4_enc_hal_info.enc_input_yuv_data_size, g_mp4_enc_hal_info.v_enc_input_yuv_data, g_mp4_enc_hal_info.p_enc_input_yuv_data);
		    DMA_FREE(0, BS_BUF_SIZE, g_mp4_enc_hal_info.v_enc_bitstreams_ptr, g_mp4_enc_hal_info.p_enc_bitstreams_ptr);
		}
    return 0;
}
static int mpeg4_encoder_init(UINT8 *pParam)
{
    INT32 ret;

    printk("enter mpeg4_encoder_init()\n");
    if(true == g_enc_init)
    {
		printk("error!! already init! \n");
        //mpeg4_encoder_free_alloc_buffer();
		return 1;
    }

    
    memset(&g_mp4_enc_hal_info, 0, sizeof(mp4_enc_info));
    ret = copy_from_user(&g_mp4_enc_hal_info, pParam, sizeof(mp4_enc_info));

    g_mp4_enc_hal_info.enc_frame_buffer_addr.size = 
        g_mp4_enc_hal_info.enc_im.width *g_mp4_enc_hal_info.enc_im.height*3/2*2;
    if(FALSE == g_mp4_enc_hal_info.b_use_pmem)
    {    	
    		DMA_ALLOC(g_mp4_enc_hal_info.enc_frame_buffer_addr.va, 0, g_mp4_enc_hal_info.enc_frame_buffer_addr.size, &g_mp4_enc_hal_info.enc_frame_buffer_addr.pa, GFP_KERNEL);
  	}
    CODEC_DRV_WriteReg32(MP4_ENC_REC_ADDR, (INT32 )g_mp4_enc_hal_info.enc_frame_buffer_addr.pa);
    CODEC_DRV_WriteReg32(MP4_ENC_REF_ADDR, (INT32)g_mp4_enc_hal_info.enc_frame_buffer_addr.pa + (g_mp4_enc_hal_info.enc_im.width *g_mp4_enc_hal_info.enc_im.height*3/2) );

    g_mp4_enc_hal_info.enc_working_memory.size = 3072 + 1024 + 360+(g_mp4_enc_hal_info.enc_im.width*(56+32))+(16*16*3/2*3);
    //g_mp4_enc_hal_info.enc_working_memory.size = 3072 + 2048 + 368 +(g_mp4_enc_hal_info.enc_im.width*(56+32))+(16*16*3/2*3);
#if	USE_INTERNAL_SRAM
		g_mp4_enc_hal_info.enc_working_memory.va = NULL;
		g_mp4_enc_hal_info.enc_working_memory.pa = alloc_internal_sram(INTERNAL_SRAM_MPEG4_ENCODER, g_mp4_enc_hal_info.enc_working_memory.size, 8);
#else    
    DMA_ALLOC(g_mp4_enc_hal_info.enc_working_memory.va, 0, g_mp4_enc_hal_info.enc_working_memory.size, &g_mp4_enc_hal_info.enc_working_memory.pa, GFP_KERNEL);
#endif

    CODEC_DRV_WriteReg32(MP4_ENC_DACP_ADDR, (INT32)g_mp4_enc_hal_info.enc_working_memory.pa);  //1024 bytes for DC predition
    CODEC_DRV_WriteReg32(MP4_ENC_STORE_ADDR, (INT32)g_mp4_enc_hal_info.enc_working_memory.pa + 1024);   //3072 bytes for load-store memory
    CODEC_DRV_WriteReg32(MP4_ENC_MVP_ADDR, (INT32)g_mp4_enc_hal_info.enc_working_memory.pa + 3072 + 1024); //320 bytes for MV

    CODEC_DRV_WriteReg32(MP4_ENC_REF_INT_ADDR, (INT32)g_mp4_enc_hal_info.enc_working_memory.pa + 3072 + 1024 + 360); // width*(56+32) for REF_INT
    CODEC_DRV_WriteReg32(MP4_ENC_CUR_INT_ADDR, (INT32)g_mp4_enc_hal_info.enc_working_memory.pa + 3072 + 1024 + 360+ (g_mp4_enc_hal_info.enc_im.width*(56+32))); // 3 macroblock for CUR_INT

    CODEC_DRV_WriteReg32(MP4_ENC_CODEC_CONF, g_mp4_enc_hal_info.u4_MP4_ENC_CODEC_CONF);
    CODEC_DRV_WriteReg32(MP4_ENC_IRQ_MASK, g_mp4_enc_hal_info.u4_MP4_ENC_IRQ_MASK);

    g_mp4_enc_hal_info.enc_input_yuv_data_size = g_mp4_enc_hal_info.enc_im.width*g_mp4_enc_hal_info.enc_im.height*3/2;
    printk("use pmem:%d\n", g_mp4_enc_hal_info.b_use_pmem);
    if(FALSE == g_mp4_enc_hal_info.b_use_pmem)
    {    	
		    DMA_ALLOC(g_mp4_enc_hal_info.v_enc_input_yuv_data, 0, g_mp4_enc_hal_info.enc_input_yuv_data_size, &g_mp4_enc_hal_info.p_enc_input_yuv_data, GFP_KERNEL);
		    memset(g_mp4_enc_hal_info.v_enc_input_yuv_data, 0, g_mp4_enc_hal_info.enc_input_yuv_data_size);
		    DMA_ALLOC(g_mp4_enc_hal_info.v_enc_bitstreams_ptr, 0, BS_BUF_SIZE, &g_mp4_enc_hal_info.p_enc_bitstreams_ptr, GFP_KERNEL);
				memset(g_mp4_enc_hal_info.v_enc_bitstreams_ptr, 0, BS_BUF_SIZE);
		}
    	
	g_enc_interrupt_handler = false;
	g_enc_init = true;

    printk("\n mpeg4_encoder_init settings:\n");
    printk("\n MP4_ENC_REC_ADDR:0x%08X\n", CODEC_DRV_Reg32(MP4_ENC_REC_ADDR));
    printk("\n MP4_ENC_REF_ADDR:0x%08X\n", CODEC_DRV_Reg32(MP4_ENC_REF_ADDR));
    printk("\n MP4_ENC_DACP_ADDR:0x%08X\n", CODEC_DRV_Reg32(MP4_ENC_DACP_ADDR));
    printk("\n MP4_ENC_STORE_ADDR:0x%08X\n", CODEC_DRV_Reg32(MP4_ENC_STORE_ADDR));
    printk("\n MP4_ENC_MVP_ADDR:0x%08X\n", CODEC_DRV_Reg32(MP4_ENC_MVP_ADDR));
    printk("\n MP4_ENC_REF_INT_ADDR:0x%08X\n", CODEC_DRV_Reg32(MP4_ENC_REF_INT_ADDR));
    printk("\n MP4_ENC_CUR_INT_ADDR:0x%08X\n", CODEC_DRV_Reg32(MP4_ENC_CUR_INT_ADDR));
    printk("\n MP4_ENC_CODEC_CONF:0x%08X\n", CODEC_DRV_Reg32(MP4_ENC_CODEC_CONF));
    printk("\n MP4_ENC_IRQ_MASK:0x%08X\n", CODEC_DRV_Reg32(MP4_ENC_IRQ_MASK));
    printk("\n g_mp4_enc_hal_info.enc_working_memory.size:0x%08X\n", (UINT32)g_mp4_enc_hal_info.enc_working_memory.size);

    return 0;
}
extern BOOL MT6516_hwEnableClock(MT6516_CLOCK clockId);
static int mpeg4_encoder_open(struct inode *inode, struct file *file)
{
    printk("enter mpeg4_encoder_open()\n");
	if(true == g_enc_open)
    {
		printk("error!! alreay open!\n");        
		return 1;
    }
	g_enc_open = true;
    /*
    int * v_VA;  
    unsigned long v_PA;
    int * k_VA;
    unsigned long k_PA;

    // allocate 4 byte memory using kmalloc 
    k_VA = (int *)mm_kmalloc_sram(sizeof(int),GFP_KERNEL);

    // get physical address for allocated memory using kmalloc  
    k_PA = mm_k_virt_to_phys_sram(k_VA);

    // allocate 4 byte memory using vmalloc 
    v_VA = (int *)mm_vmalloc_sram(sizeof(int));

    // get physical address for allocated memory using vmalloc  
    v_PA = mm_v_virt_to_phys_sram(v_VA);

    *k_VA = 1;  // assign value for test 

    *v_VA = 2;  // assign value for test 

    printk("k_VA = %x\n",k_VA);
    printk("*k_VA = %d\n",*k_VA);
    printk("k_PA = %x\n",k_PA);

    printk("v_VA = %x\n",v_VA);
    printk("*v_VA = %d\n",*v_VA);
    printk("v_PA = %x\n",v_PA);

    // free 4 byte memory using kfree 
    mm_kfree_sram(k_VA);

    // free 4 byte memory using vfree 
    mm_vfree_sram(v_VA);
    */

    //  int temp; 

    //printk("----mpeg4_decoder_open----\n");
    //printk("----TEST_R/W_REGISTER----\n");
    // ToDO: reset一些reg, MP4_BASE +0x000h之類的
    //MT6516_hwEnableSubsys(MT6516_SUBSYS_GRAPH2SYS);
	CODEC_DRV_WriteReg32(GMC2_MUX_PORT_SEL,0x0);
#if 1
    MT6516_DCT_Reset();
    hwEnableClock(MT6516_PDN_MM_GMC2,"MP4VENC");
    hwEnableClock(MT6516_PDN_MM_MP4,"MP4VENC");
    //hwEnableClock(MT6516_PDN_MM_GMC1,"MP4VENC");
    //hwEnableClock(MT6516_PDN_MM_GMC2,"MP4VENC");
    //hwEnableClock(MT6516_PDN_MM_IMGDMA0,"MP4VENC");
    //hwEnableClock(MT6516_PDN_MM_IMGDMA1,"MP4VENC");
    hwEnableClock(MT6516_PDN_MM_DCT,"MP4VENC");    
#else
    MT6516_hwEnableClock(MT6516_PDN_MM_MP4);
    //MT6516_hwEnableClock(MT6516_PDN_MM_GMC1);
    //MT6516_hwEnableClock(MT6516_PDN_MM_GMC2);
    //MT6516_hwEnableClock(MT6516_PDN_MM_IMGDMA0);
    //MT6516_hwEnableClock(MT6516_PDN_MM_IMGDMA1);
    MT6516_hwEnableClock(MT6516_PDN_MM_DCT);    
#endif
    //   MT6516_hwDisableSubsys(MT6516_SUBSYS_GRAPH2SYS);
    /*   
    HW_WRITE(MP4_DEC_CODEC_CONF,0x1f);
    temp = HW_READ(MP4_DEC_CODEC_CONF);

    printk("temp = %d\n",temp);
    */  
    g_enc_init = false;
    printk("mpeg4_encoder_open done\n");
    return 0;
}

static ssize_t mpeg4_encoder_read(struct file *file, char __user *data, size_t len, loff_t *ppos)
{
    //printk("----mpeg4_encoder_read----\n");
    return len;
}

static ssize_t mpeg4_encoder_write(struct file *file, const char __user *data, size_t len, loff_t *ppos)
{
    //printk("----mpeg4_encoder_write----\n");   
    return len;
}

static int mpeg4_encoder_flush(struct file *file, fl_owner_t id)
{
	BOOL flag;

    printk("----mpeg4_encoder_flush----\n");
    if(false == g_enc_open)
    {
		printk("not open yet\n");        
		return 0;
    }    
    if(true == g_enc_init)
    {
        mpeg4_encoder_free_alloc_buffer();
		g_enc_init = false;
    }
    
#if 1

	flag = hwDisableClock(MT6516_PDN_MM_MP4,"MP4VENC");
    //flag = hwDisableClock(MT6516_PDN_MM_GMC1,"MP4VENC");
    //flag = hwDisableClock(MT6516_PDN_MM_GMC2,"MP4VENC");
    //flag = hwDisableClock(MT6516_PDN_MM_IMGDMA0,"MP4VENC");
    //flag = hwDisableClock(MT6516_PDN_MM_IMGDMA1,"MP4VENC");
    flag = hwDisableClock(MT6516_PDN_MM_DCT,"MP4VENC");
    flag = hwDisableClock(MT6516_PDN_MM_GMC2,"MP4VENC");
	NOT_REFERENCED(flag);
#else
	MT6516_hwDisableClock(MT6516_PDN_MM_MP4);
	//MT6516_hwDisableClock(MT6516_PDN_MM_GMC1);
	//MT6516_hwDisableClock(MT6516_PDN_MM_GMC2);
	//MT6516_hwDisableClock(MT6516_PDN_MM_IMGDMA0);
	//MT6516_hwDisableClock(MT6516_PDN_MM_IMGDMA1);
	MT6516_hwDisableClock(MT6516_PDN_MM_DCT);    
#endif

	g_enc_open = false;

    return 0;
}

static int mpeg4_encoder_release(struct inode *inode, struct file *file)
{
	BOOL flag;

    printk("----mpeg4_encoder_release----\n");
    if(false == g_enc_open)
    {
		printk("flush or not open\n");        
		return 0;
    }
    /*
    BOOL   g_dec_dacp_memory_allocated = FALSE;
    BOOL   g_dec_data_store_memory_allocated = FALSE;
    BOOL   g_dec_qs_memory_allocated = FALSE;
    BOOL   g_dec_mvp_memory_allocated = FALSE;
    BOOL   g_dec_vlc_memory_allocated = FALSE;
    BOOL   g_dec_ref_memory_allocated = FALSE;
    BOOL   g_dec_yuv_buffer_memory_allocated = FALSE;
    */
    /*
    if (g_dec_dacp_memory_allocated == TRUE)
    {
    g_dec_dacp_memory_allocated = FALSE;
    //dma_free_coherent(0, g_dec_dacp.size, g_dec_dacp.va, g_dec_dacp.pa);
    DMA_FREE(0, g_dec_dacp.size, g_dec_dacp.va, g_dec_dacp.pa);
    }

    if (g_dec_data_store_memory_allocated == TRUE)
    {
    g_dec_data_store_memory_allocated = FALSE;
    //dma_free_coherent(0, g_dec_data_store.size, g_dec_data_store.va, g_dec_data_store.pa);
    DMA_FREE(0, g_dec_data_store.size, g_dec_data_store.va, g_dec_data_store.pa);
    }

    if (g_dec_qs_memory_allocated == TRUE)
    {
    g_dec_qs_memory_allocated = FALSE;
    //dma_free_coherent(0, g_dec_qs.size, g_dec_qs.va, g_dec_qs.pa);
    DMA_FREE(0, g_dec_qs.size, g_dec_qs.va, g_dec_qs.pa);
    }

    if (g_dec_mvp_memory_allocated == TRUE)
    {
    g_dec_mvp_memory_allocated = FALSE;
    //dma_free_coherent(0, g_dec_mvp.size, g_dec_mvp.va, g_dec_mvp.pa);
    DMA_FREE(0, g_dec_mvp.size, g_dec_mvp.va, g_dec_mvp.pa);
    }

    if (g_dec_vlc_memory_allocated == TRUE)
    {
    g_dec_vlc_memory_allocated = FALSE;
    //dma_free_coherent(0, g_bitstream.size, g_bitstream.va, g_bitstream.pa); 
    //dma_free_coherent(0, g_bitstream.size, g_bitstream_temp.va, g_bitstream_temp.pa);
    DMA_FREE(0, g_bitstream.size, g_bitstream.va, g_bitstream.pa); 
    DMA_FREE(0, g_bitstream.size, g_bitstream_temp.va, g_bitstream_temp.pa);
    }

    if (g_dec_ref_memory_allocated == TRUE)
    {
    g_dec_ref_memory_allocated = FALSE;
    //dma_free_coherent(0, g_dec_ref.size, g_dec_ref.va, g_dec_ref.pa);
    DMA_FREE(0, g_dec_ref.size, g_dec_ref.va, g_dec_ref.pa);
    }

    //dma_free_coherent(0, g_yuv_buffer.size, g_yuv_buffer.va, g_yuv_buffer.pa); 
    DMA_FREE(0, g_yuv_buffer.size, g_yuv_buffer.va, g_yuv_buffer.pa); 

    //printk("release memory!!\n");
    */
    if(true == g_enc_init)
    {
        mpeg4_encoder_free_alloc_buffer();
		g_enc_init = false;
    }
    
#if 1

	flag = hwDisableClock(MT6516_PDN_MM_MP4,"MP4VENC");
    //flag = hwDisableClock(MT6516_PDN_MM_GMC1,"MP4VENC");
    //flag = hwDisableClock(MT6516_PDN_MM_GMC2,"MP4VENC");
    //flag = hwDisableClock(MT6516_PDN_MM_IMGDMA0,"MP4VENC");
    //flag = hwDisableClock(MT6516_PDN_MM_IMGDMA1,"MP4VENC");
    flag = hwDisableClock(MT6516_PDN_MM_DCT,"MP4VENC");
    flag = hwDisableClock(MT6516_PDN_MM_GMC2,"MP4VENC");
	NOT_REFERENCED(flag);
#else
	MT6516_hwDisableClock(MT6516_PDN_MM_MP4);
	//MT6516_hwDisableClock(MT6516_PDN_MM_GMC1);
	//MT6516_hwDisableClock(MT6516_PDN_MM_GMC2);
	//MT6516_hwDisableClock(MT6516_PDN_MM_IMGDMA0);
	//MT6516_hwDisableClock(MT6516_PDN_MM_IMGDMA1);
	MT6516_hwDisableClock(MT6516_PDN_MM_DCT);    
#endif

	g_enc_open = false;

    return 0;
}

static int mpeg4_encoder_fasync(int fd, struct file *file, int mode)
{
    //printk("----mpeg4_encoder_fasync----\n");
    return fasync_helper(fd, file, mode, &mpeg4_encoder_async);
}

static struct file_operations mpeg4_encoder_fops = {
    .owner		= THIS_MODULE,
    .ioctl		= mpeg4_encoder_ioctl,
    .open			= mpeg4_encoder_open,
    .read     = mpeg4_encoder_read,
    .write    = mpeg4_encoder_write,
	.flush      = mpeg4_encoder_flush,
    .release	= mpeg4_encoder_release,
    .fasync   = mpeg4_encoder_fasync,

};

static int mpeg4_encoder_probe(struct platform_device *dev)
{
    int ret;
    struct class_device *class_dev = NULL;

#if 1 //dynamic
#else
    int major = 283;
    int minor = 0;
    mpeg4_encoder_devno = MKDEV(major, minor);
#endif

    printk("----mpeg4_encoder_probe----\n");

#if 1 //dynamic
    ret = alloc_chrdev_region(&mpeg4_encoder_devno, 0, 1, MPEG4_ENCODER_DEVNAME);
    if(ret)
    {
        printk("Error: Can't Get Major number for MPEG4 Encoder Device\n");
    }
    /*
    else
        printk("Get MPEG4 Encoder Device Major number (%d)\n", major);
    */
#else //static
    ret = register_chrdev_region(mpeg4_encoder_devno, 1, MPEG4_ENCODER_DEVNAME);
#endif

    mpeg4_encoder_cdev = cdev_alloc();
    mpeg4_encoder_cdev->owner = THIS_MODULE;
    mpeg4_encoder_cdev->ops = &mpeg4_encoder_fops;

    ret = cdev_add(mpeg4_encoder_cdev, mpeg4_encoder_devno, 1);

    //Register Interrupt 
    if (request_irq(MT6516_MPEG4_ENC_IRQ_LINE, mp4_enc_intr_dlr, 0, "MT6516_MPEG4_ENCODER", NULL) < 0)
    {
        printk("----error to request MPEG4 irq----!!\n"); 
    }
    else
    {
        printk("----success to request MPEG4 irq----!!\n");
    }

    //for android.
    mpeg4_encoder_class = class_create(THIS_MODULE, MPEG4_ENCODER_DEVNAME);
    class_dev = (struct class_device *) device_create(mpeg4_encoder_class,
                                NULL,
                                mpeg4_encoder_devno,
                                NULL,
                                MPEG4_ENCODER_DEVNAME
                                );
                                
    init_waitqueue_head(&encWaitQueue);
		
    printk("----mpeg4_encoder_probe Done----\n");

    return 0;
}

static int mpeg4_encoder_remove(struct platform_device *dev)
{	
    return 0;
}

static void mpeg4_encoder_shutdown(struct platform_device *dev)
{
}

static int mpeg4_encoder_suspend(struct platform_device *dev, pm_message_t state)
{    
    bool flag;    

	if(true == g_enc_open)
	{
		printk("[MP4_ENC] mpeg4_encoder_suspend\n");

		flag = hwDisableClock(MT6516_PDN_MM_MP4,"MP4VENC");
		//flag = hwDisableClock(MT6516_PDN_MM_GMC1,"MP4VENC");
		//flag = hwDisableClock(MT6516_PDN_MM_GMC2,"MP4VENC");
		//flag = hwDisableClock(MT6516_PDN_MM_IMGDMA0,"MP4VENC");
		//flag = hwDisableClock(MT6516_PDN_MM_IMGDMA1,"MP4VENC");
		flag = hwDisableClock(MT6516_PDN_MM_DCT,"MP4VENC");    
flag = hwDisableClock(MT6516_PDN_MM_GMC2,"MP4VENC");
		NOT_REFERENCED(flag); 
	}
    return 0;
}



static int mpeg4_encoder_resume(struct platform_device *dev)
{   
	bool flag;

	if(true == g_enc_open)
	{
		printk("[MP4_ENC] mpeg4_encoder_resume\n");
		CODEC_DRV_WriteReg32(GMC2_MUX_PORT_SEL,0x0);
		MT6516_DCT_Reset();
		flag = hwEnableClock(MT6516_PDN_MM_GMC2,"MP4VENC");
		flag = hwEnableClock(MT6516_PDN_MM_MP4,"MP4VENC");
		//flag = hwEnableClock(MT6516_PDN_MM_GMC1,"MP4VENC");
		//flag = hwEnableClock(MT6516_PDN_MM_GMC2,"MP4VENC");
		//flag = hwEnableClock(MT6516_PDN_MM_IMGDMA0,"MP4VENC");
		//flag = hwEnableClock(MT6516_PDN_MM_IMGDMA1,"MP4VENC");
		flag = hwEnableClock(MT6516_PDN_MM_DCT,"MP4VENC");    

		NOT_REFERENCED(flag);
	}
    return 0;
}

static struct platform_driver mpeg4_encoder_driver = {
    .probe		= mpeg4_encoder_probe,
    .remove		= mpeg4_encoder_remove,
    .shutdown	= mpeg4_encoder_shutdown,
    .suspend	= mpeg4_encoder_suspend,
    .resume		= mpeg4_encoder_resume,
    .driver     = {
        .name		= MPEG4_ENCODER_DEVNAME,
    },
};

static struct platform_device mpeg4_encoder_device = {
    .name	 = MPEG4_ENCODER_DEVNAME,
    .id      = 0,
};

static int __init mpeg4_encoder_driver_init(void)
{
    int ret;

    printk("----mpeg4_encoder_driver_init----\n");

    if (platform_device_register(&mpeg4_encoder_device)){
        printk("----failed to register mpeg4_encoder Device----\n");
        ret = -ENODEV;
        return ret;
    }

    if (platform_driver_register(&mpeg4_encoder_driver)){
        printk("----failed to register MPEG4 Encoder Driver----\n");
        platform_device_unregister(&mpeg4_encoder_device);
        ret = -ENODEV;
        return ret;
    }

    printk("----mpeg4_encoder_driver_init Done----\n");

    return 0;
}

static void __exit mpeg4_encoder_driver_exit(void)
{
    printk("----mpeg4_encoder_driver_exit----\n");

    //for android.
    device_destroy(mpeg4_encoder_class, mpeg4_encoder_devno);
    class_destroy(mpeg4_encoder_class);

    cdev_del(mpeg4_encoder_cdev);
    unregister_chrdev_region(mpeg4_encoder_devno, 1);
    platform_driver_register(&mpeg4_encoder_driver);
    platform_device_unregister(&mpeg4_encoder_device);
}

static void mpeg4_encoder_reset(void)
{
    //UINT8 index;
    printk("enter mpeg4_encoder_reset()\n");
    // encode reset
    //printk("WriteReg32(0x%X, 0x%X)\n", MP4_CODEC_COMD, MP4_CODEC_COMD_ENC_RST);
    CODEC_DRV_WriteReg32(MP4_CODEC_COMD, MP4_CODEC_COMD_ENC_RST);
    //	for (index = 0; index < 20; index++)
    //		;
    mdelay(1);	
    CODEC_DRV_WriteReg32(MP4_CODEC_COMD, 0);

    //core reset
    CODEC_DRV_WriteReg32(MP4_CODEC_COMD, MP4_CODEC_COMD_CORE_RST);
    //	for (index = 0; index < 20; index++)
    //		;
    mdelay(1);
    CODEC_DRV_WriteReg32(MP4_CODEC_COMD, 0);
    printk("enter mpeg4_encoder_reset() done\n");
}

module_init(mpeg4_encoder_driver_init);
module_exit(mpeg4_encoder_driver_exit);
MODULE_AUTHOR("TeChien Chen <techien.chen@mediatek.com>");
MODULE_DESCRIPTION("MT6516 MPEG4 Encoder Driver");
MODULE_LICENSE("GPL");

