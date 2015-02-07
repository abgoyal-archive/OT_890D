

#ifndef __MT6516_H264_KERNEL_DRIVER_H__
#define __MT6516_H264_KERNEL_DRIVER_H__

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
#include <asm/cacheflush.h>
#include <asm/tcm.h>
#include "mt6516_h264_hw.h"

#define H264_DEC_DEVNAME "MT6516_H264_DEC"

#define HW_WRITE(ptr,data) (*(ptr) = (data))
#define HW_READ(ptr)       (*(ptr))

//#define MT6516_H264_DEC_DEBUG
#ifdef MT6516_H264_DEC_DEBUG
#define H264_DEC_DEBUG printk
#else
#define H264_DEC_DEBUG(x,...)
#endif

#define MT6516_H264_DEC
#ifdef MT6516_H264_DEC
#define H264_DEC printk
#else
#define H264_DEC(x,...)
#endif

#define DMA_ALLOC(ptr,dev,size,hdl,flag) \
do { \
    ptr = dma_alloc_coherent(dev, size, hdl, flag); \
    BUG_ON(!ptr); \
    H264_DEC("[H264_DEC_MEM_ALLOC] Alloc_vaddr=0x%08x, Alloc_paddr=0x%08x, Alloc_size=%d, LINE:%d\n", ptr, *hdl, size, __LINE__); \
} while (0)

#define DMA_FREE(dev, size, ptr, hdl)\
do { \
    H264_DEC("[H264_DEC_MEM_FREE] Free_addr=0x%08x, Alloc_size=%d, LINE:%d\n", ptr, size, __LINE__); \
    dma_free_coherent(dev, size, ptr, hdl); \
} while (0)


typedef enum 
{
    H264_DEC_SET_INTERNAL_SRAM = 0,
    H264_DEC_WAIT_DECODE_DONE = 1,
    H264_DEC_RST = 2,
    H264_DEC_GET_IRQ_STS_SET_ACK = 3,
    H264_DEC_RELOAD_VLC_DMA = 4,
    H264_DEC_SET_SLICE_MAP_ADDR = 5,
    H264_DEC_SET_REF_FRAME_ADDR = 6,
    H264_DEC_SET_PIC_CONF = 7,
    H264_DEC_SET_SLICE_CONF = 8,
    H264_DEC_SET_DMA_LIMIT = 9,
    H264_DEC_SET_MC_LINE_BUF_OFFSET = 10,
    H264_DEC_SET_INTERNAL_SRAM_TO_HW = 11,
    H264_DEC_SET_MC_LINE_BUF_SIZE = 12,
    H264_DEC_SET_REC_ADDR = 13,
    H264_DEC_SET_REC_Y_SIZE = 14,
    H264_DEC_SET_IRQ_POS_AND_MASK = 15,
    H264_DEC_SET_HW_START = 16,
    H264_DEC_CLEAN_DCACHE = 17,
    H264_DEC_FLUSH_DCACHE = 18,
    MAX_H264_DEC_CMD = 0xFFFFFFFF
} H264_DEC_CMD;

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

#endif //__MT6516_H264_KERNEL_DRIVER_H__
