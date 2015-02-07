

#ifndef __MT6516_H264_HW_H__
#define __MT6516_H264_HW_H__

#define H264_base H264_BASE

/* GMC PORT */
#define GMC2_MUX_PORT_SEL               (volatile unsigned long *)(GMC2_BASE+0x58) /*RW*/

/* Main Configuration & Commands */
#define H264_DEC_COMD                   (volatile unsigned long *)(H264_base+0x0) /*WO*/
#define H264_DEC_DMA_COMD               (volatile unsigned long *)(H264_base+0x4) /*WO*/
#define H264_DEC_SLICE_MAP_ADDR         (volatile unsigned long *)(H264_base+0x8) /*RW*/
#define H264_DEC_PIC_CONF               (volatile unsigned long *)(H264_base+0xc) /*RW*/
#define H264_DEC_SLICE_CONF             (volatile unsigned long *)(H264_base+0x10) /*RW*/
   
/* DMA */
#define H264_DEC_DMA_LIMIT              (volatile unsigned long *)(H264_base+0x20) /*RW*/
#define H264_DEC_DMA_ADDR               (volatile unsigned long *)(H264_base+0x24) /*RW*/
#define H264_DEC_DMA_BITCNT             (volatile unsigned long *)(H264_base+0x28) /*RW*/
   
/* MC */
#define H264_REF_FRAME_ADDR             (volatile unsigned long *)(H264_base+0x30) /*RW*/
#define H264_MC_LINE_BUF_ADDR           (volatile unsigned long *)(H264_base+0x34) /*RW*/
#define H264_MC_LINE_BUF_OFFSET         (volatile unsigned long *)(H264_base+0x38) /*RW*/
#define H264_MC_LINE_BUF_SIZE           (volatile unsigned long *)(H264_base+0x3c) /*RW*/
#define H264_MC_MV_BUFFER_ADDR          (volatile unsigned long *)(H264_base+0x40) /*RW*/
   
/* Rec & Deblock */
#define H264_DEC_REC_ADDR               (volatile unsigned long *)(H264_base+0x44) /*RW*/
#define H264_DEC_DEB_BUF_ADDR           (volatile unsigned long *)(H264_base+0x48) /*RW*/
#define H264_DEC_REC_Y_SIZE             (volatile unsigned long *)(H264_base+0x4c) /*RW*/
#define H264_DEC_DEB_DAT_BUF0_ADDR      (volatile unsigned long *)(H264_base+0x50) /*RW*/
#define H264_DEC_DEB_DAT_BUF1_ADDR      (volatile unsigned long *)(H264_base+0x54) /*RW*/
   
/* CAVLC */
#define H264_CAVLC_BASE_ADDR            (volatile unsigned long *)(H264_base+0x60) /*RW*/
   
/* Interrupt */
#define H264_DEC_IRQ_STS                (volatile unsigned long *)(H264_base+0x70) /*RW*/
#define H264_DEC_IRQ_MASK               (volatile unsigned long *)(H264_base+0x74) /*RW*/
#define H264_DEC_IRQ_ACK                (volatile unsigned long *)(H264_base+0x78) /*RW*/
#define H264_DEC_IRQ_POS                (volatile unsigned long *)(H264_base+0x7C) /*RW*/
   
/* Debug Infomation */
#define H264_DEC_DMA_STS                (volatile unsigned long *)(H264_base+0x100) /*RO*/
#define H264_DEC_DEBUG_INFO0            (volatile unsigned long *)(H264_base+0x104) /*RO*/
#define H264_DEC_DEBUG_INFO1            (volatile unsigned long *)(H264_base+0x108) /*RO*/
#define H264_DEC_DEBUG_INFO2            (volatile unsigned long *)(H264_base+0x10c) /*RO*/
#define H264_DEC_DEBUG_INFO3            (volatile unsigned long *)(H264_base+0x110) /*RO*/
#define H264_DEC_DEBUG_INFO4            (volatile unsigned long *)(H264_base+0x114) /*RO*/
#define H264_DEC_DEBUG_INFO5            (volatile unsigned long *)(H264_base+0x118) /*RO*/
#define H264_DEC_DEBUG_INFO6            (volatile unsigned long *)(H264_base+0x11c) /*RO*/
#define H264_DEC_DEBUG_INFO7            (volatile unsigned long *)(H264_base+0x120) /*RO*/
#define H264_DEC_DEBUG_INFO8            (volatile unsigned long *)(H264_base+0x124) /*RO*/
#define H264_DEC_DEBUG_INFO9            (volatile unsigned long *)(H264_base+0x128) /*RO*/
#define H264_DEC_DEBUG_INFO10           (volatile unsigned long *)(H264_base+0x12c) /*RO*/
#define H264_DEC_DEBUG_INFO11           (volatile unsigned long *)(H264_base+0x130) /*RO*/
   
/* DEC COMD */
#define H264_DEC_COMD_RST               0x0001
#define H264_DEC_COMD_START             0x0010

/* DMA COMD */
#define H264_DEC_DMA_COMD_STOP          0x0001
#define H264_DEC_DMA_COMD_RESUME        0x0002

/* PIC CONF */
#define H264_PIC_CONF_LAST_REF_INDEX_OFFSET 21
#define H264_PIC_CONF_REF_CURR_FRAME        0x00100000
#define H264_PIC_CONF_FRAME_TYPE            0x00080000
#define H264_PIC_CONF_ERR_STALL_EN          0x00040000
#define H264_PIC_CONF_INTRA_FLAG            0x00020000
#define H264_PIC_CONF_IRQ_EN                0x00010000
#define H264_PIC_CONF_MC_MF_CACHE_EN        0x04000000
#define H264_PIC_CONF_PIC_HEIGHT_OFFSET     8
#define H264_PIC_CONF_PIC_WIDTH_OFFSET      0

//fix value
#define H264_MC_LINE_YBUF_OFFSET            4
#define H264_MC_LINE_UVBUF_OFFSET           2

/* The Dma Limit is 256KB */
#define H264_DMA_LIMIT_SIZE                 (256*1024)

/* MC LINE BUF OFFSET*/
#define H264_MC_PFH_EN                      0x10000
#define H264_MC_UVBUF_OFFSET                4
#define H264_MC_YBUF_OFFSET                 0

/* MC_LINE_BUF_SIZE */
#define H264_MC_LINE_UVBUF_SIZE_OFFSET      16
#define H264_MC_LINE_YBUF_SIZE_OFFSET       0

/* H264_DEC_IRQ_STS */
#define H264_DEC_IRQ_STS_DEC_DONE           0x0001
#define H264_DEC_IRQ_STS_DMA_PAUSE          0x0002
#define H264_DEC_IRQ_STS_VLD_ERROR          0x0004
#define H264_DEC_IRQ_STS_OVERFLOW           0x0008
#define H264_DEC_IRQ_MB_TYPE_ERROR          0x0010
   
/* H264_DEC_IRQ_MASK */
#define H264_DEC_IRQ_MASK_DEC_DONE          0x0001
#define H264_DEC_IRQ_MASK_DMA_PAUSE         0x0002
#define H264_DEC_IRQ_MASK_VLD_ERROR         0x0004	
#define H264_DEC_IRQ_MASK_OVERFLOE          0x0008
#define H264_DEC_IRQ_MASK_MB_TYPE_ERROR     0x0010
   
/* H264_DEC_IRQ_ACK */
#define H264_DEC_IRQ_ACK_DEC_DONE           0x0001
#define H264_DEC_IRQ_ACK_DMA_PAUSE          0x0002
#define H264_DEC_IRQ_ACK_VLD_ERROR          0x0004	
#define H264_DEC_IRQ_ACK_OVERFLOW           0x0008
#define H264_DEC_IRQ_ACK_MB_TYPE_ERROR      0x0010

/* H264_DEC_DEBUG_INFO0 */
#define H264_DEC_DEBUG_INFO0_VLD_ERR        0x08000000
   
/* H264_DEC_DEBUG_INFO2*/
#define H264_DEC_DEBUG_INFO2_IDCT_OVERFLOW  0x02000000
   
/* H264_DEC_DEBUG_INFO11*/
#define H264_DEC_DEBUG_INFO11_SEQ_IDLE      0x1

#endif //__MT6516_H264_HW_H__
