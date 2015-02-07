
#ifndef __MT6516_MPEG4_DECODER_H__
#define __MT6516_MPEG4_DECODER_H__

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
#include <linux/interrupt.h>
#include <mach/irqs.h>
#include <linux/wait.h>
#include <mach/mt6516_pll.h>
#include <mach/mt6516_typedefs.h> 
#include <mach/mt6516_reg_base.h> 
#include <mach/mt6516_mm_mem.h>
#include <mach/mt6516_IDP.h>
#include <mach/mt6516_timer.h>
#include <asm/cacheflush.h>
#include <asm/tcm.h>

#define MPEG4_DECODER_DEVNAME     "MT6516_MP4_DEC"

/* GMC PORT */
#define GMC2_MUX_PORT_SEL          (volatile unsigned long *)(GMC2_BASE+0x58)     /*RW*/

#define MP4_CODEC_COMD             (volatile unsigned long *)(MP4_BASE+0x0000)    /*WO*/
#define MP4_DEC_CODEC_CONF         (volatile unsigned long *)(MP4_BASE+0x0200)    /*RW*/ 
#define MP4_DEC_STS                (volatile unsigned long *)(MP4_BASE+0x0204)    /*RO*/
#define MP4_DEC_IRQ_MASK           (volatile unsigned long *)(MP4_BASE+0x0208)    /*RW*/
#define MP4_DEC_IRQ_STS            (volatile unsigned long *)(MP4_BASE+0x020c)    /*RO*/
#define MP4_DEC_IRQ_ACK            (volatile unsigned long *)(MP4_BASE+0x0210)    /*WC*/
#define MP4_DEC_REF_ADDR           (volatile unsigned long *)(MP4_BASE+0x0224)    /*RW*/
#define MP4_DEC_REC_ADDR           (volatile unsigned long *)(MP4_BASE+0x0228)    /*RW*/
#define MP4_DEC_DATA_STROE_ADDR    (volatile unsigned long *)(MP4_BASE+0x0230)    /*RW*/
#define MP4_DEC_DACP_ADDR          (volatile unsigned long *)(MP4_BASE+0x0234)    /*RW*/
#define MP4_DEC_MVP_ADDR           (volatile unsigned long *)(MP4_BASE+0x0238)    /*RW*/
#define MP4_DEC_VOP_STRUCT0        (volatile unsigned long *)(MP4_BASE+0x0240)    /*RW*/
#define MP4_DEC_VOP_STRUCT1        (volatile unsigned long *)(MP4_BASE+0x0244)    /*RW*/
#define MP4_DEC_VOP_STRUCT2        (volatile unsigned long *)(MP4_BASE+0x0248)    /*RW*/
#define MP4_DEC_MB_STRUCT0         (volatile unsigned long *)(MP4_BASE+0x024c)    /*RW*/
#define MP4_DEC_VLC_ADDR           (volatile unsigned long *)(MP4_BASE+0x0260)    /*RW*/
#define MP4_DEC_VLC_BIT            (volatile unsigned long *)(MP4_BASE+0x0264)    /*RW*/
#define MP4_DEC_VLC_LIMIT          (volatile unsigned long *)(MP4_BASE+0x0268)    /*RW*/
#define MP4_DEC_VLC_WORD           (volatile unsigned long *)(MP4_BASE+0x026c)    /*RO*/
#define MP4_DEC_VLC_BITCNT         (volatile unsigned long *)(MP4_BASE+0x0270)    /*RO*/
#define MP4_DEC_QS_ADDR            (volatile unsigned long *)(MP4_BASE+0x027c)    /*RW*/
#define MP4_CORE_VLC_ADDR          (volatile unsigned long *)(MP4_BASE+0x0378)    /*WO*/
#define MP4_VLC_DMA_COMD           (volatile unsigned long *)(MP4_BASE+0x0004)    /*WO*/

        
#define MP4_VLC_DMA_COMD_RELOAD    0x0002

#define MP4_DEC_START              0x0010

#define MP4_DEC_IRQ_STS_VLD        0x0001
#define MP4_DEC_IRQ_STS_RLD        0x0002
#define MP4_DEC_IRQ_STS_MARK       0x0004
#define MP4_DEC_IRQ_STS_DEC        0x0008
#define MP4_DEC_IRQ_STS_BLOCK      0x0010
#define MP4_DEC_IRQ_STS_DMA        0x0020

#define HW_WRITE(ptr,data) (*(ptr) = (data))
#define HW_READ(ptr)       (*(ptr))

#define MP4_DECODE_FAILED                        0           ///< The frame is decoded failed due to H/W report error
#define MP4_DECODE_OK                            1           ///< The frame is decode succesfully 
#define MP4_DECODE_OK_BYPASS                     2           ///< The frame is not decoded directly, use previous frame
#define MP4_DECODE_FAILED_TIMEOUT                3           ///< The frame is decoded failed due to timeout


//#define MT6516_MP4_DEC_DEBUG
#ifdef MT6516_MP4_DEC_DEBUG
#define MP4_DEC_DEBUG printk
#else
#define MP4_DEC_DEBUG(x,...)
#endif

#define MT6516_MP4_DEC
#ifdef MT6516_MP4_DEC
#define MP4_DEC printk
#else
#define MP4_DEC(x,...)
#endif

typedef enum 
{
    SET_MP4_CODEC_COMD = 0,
    SET_MP4_CODEC_COMD_START = 1,
    GET_MP4_CURRENT_YUV_BUFFER_PA = 2,
    SET_MP4_PMEM_BITSTREAMBUFFER = 3,
    SET_MP4_DEC_HW_CONFIG = 4,
    SET_MP4_PMEM_MTKYUVBUFFER = 5,
    SET_MP4_CLEAN_DCACHE = 6,
    SET_MP4_FLUSH_DCACHE = 7,
    GET_MP4_IRQ_STS = 8,                       
    SET_MP4_IRQ_ACK = 9,
    GET_MP4_FORCE_DWORD = 0xFFFFFFFF
} MPEG4_DECODER_CMD;

typedef struct
{
    UINT8 * va;
    UINT32 width;
    UINT32 height;
    UINT32 pa;
    UINT32 size;  // width * height * 1.5
} yuv_buffer;

typedef struct
{
    UINT8 * va;
    UINT32 pa;
    UINT32 size;  // num of pixels per frame * 1.5  
} dec_ref;

typedef struct
{
    UINT8 * bitstream_v_addr;
    UINT32 bytecnt;
    UINT32 bitcnt;
    UINT32 size;
} set_dec_vlc;

typedef struct
{
    UINT8 * va;
    UINT32 pa;
    UINT32 size;
    UINT32 bytecnt;
    UINT32 bitcnt;
} bitstream;

typedef struct
{
    UINT32 mp4_dec_codec_conf;
    UINT32 mp4_dec_irq_mask;
    UINT32 mp4_dec_vop_struct0;
    UINT32 mp4_dec_vop_struct1;
    UINT32 mp4_dec_vop_struct2;
    UINT32 mp4_dec_mb_struct0;
    UINT32 mp4_dec_data_store_addr_size;
    UINT32 mp4_dec_qs_addr_size;
    UINT32 mp4_dec_mvp_addr_size;
    set_dec_vlc dec_vlc;
    UINT32 mp4_dec_vlc_limit;   
} mp4_dec_hw_config;

typedef struct
{
    UINT8 * va;
    UINT32 pa;
} pmem_bitstream;

typedef struct
{
    UINT8 * current_va;
    UINT32 current_pa;
    UINT8 * reference_va;
    UINT32 reference_pa;
    UINT32 width;
    UINT32 height;
} pmem_MTKYUVBuffer;

typedef struct
{
    kal_uint32 va;
    kal_uint32 size;
}CleanDCache;

typedef struct
{
    kal_uint32 va;
    kal_uint32 size;
}FlushDCache;

#endif //__MT6516_MPEG4_DECODER_H__
