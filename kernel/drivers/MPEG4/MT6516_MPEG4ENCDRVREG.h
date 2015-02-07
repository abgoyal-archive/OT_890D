

#ifndef __MT6516_MPEG4ENCDRVREG_H__
#define __MT6516_MPEG4ENCDRVREG_H__


//#define MP4_base 0x800C0000 //extern DWORD MP4_base;
#define MP4_base MP4_BASE //extern DWORD MP4_base;

#define MP4_CODEC_COMD                    (MP4_base+0x0) /*RW*/
#define MP4_VLC_DMA_COMD                  (MP4_base+0x4) /*WO*/
/*ENC register*/
#define MP4_ENC_CODEC_CONF                (MP4_base+0x100) /*RW*/
#define MP4_ENC_STS                       (MP4_base+0x104) /*RO*/
#define MP4_ENC_IRQ_MASK                  (MP4_base+0x108) /*RW*/
#define MP4_ENC_IRQ_STS                   (MP4_base+0x10c) /*RO*/
#define MP4_ENC_IRQ_ACK			            (MP4_base+0x110) /*WC*/
#define MP4_ENC_CONF			               (MP4_base+0x114) /*R/W*/
#define MP4_ENC_CODEC_BASE		            (MP4_base+0x120) /*R/W*/
#define MP4_ENC_VOP_ADDR	               (MP4_base+0x124) /*RW*/
#define MP4_ENC_REF_ADDR			         (MP4_base+0x128) /*RW*/
#define MP4_ENC_REC_ADDR			         (MP4_base+0x12C) /*RW*/
#define MP4_ENC_STORE_ADDR			         (MP4_base+0x130) /*RW*/
#define MP4_ENC_DACP_ADDR			         (MP4_base+0x134) /*RW*/
#define MP4_ENC_MVP_ADDR			         (MP4_base+0x138) /*RW, new*/
#define MP4_ENC_VOP_STRUC0                (MP4_base+0x140) /*R/W*/
#define MP4_ENC_VOP_STRUC1                (MP4_base+0x144) /*R/W*/
#define MP4_ENC_VOP_STRUC2                (MP4_base+0x148) /*R/W*/
#define MP4_ENC_VOP_STRUC3                (MP4_base+0x14c) /*R/W*/
#define MP4_ENC_MB_STRUC0                 (MP4_base+0x150) /*R/W*/
#define MP4_ENC_VLC_ADDR                  (MP4_base+0x160) /*WO*/
#define MP4_ENC_VLC_BIT                   (MP4_base+0x164) /*WO*/
#define MP4_ENC_VLC_LIMIT                 (MP4_base+0x168) /*R/W*/
#define MP4_ENC_VLC_WORD                  (MP4_base+0x16c) /*RO*/
#define MP4_ENC_VLC_BITCNT                (MP4_base+0x170) /*RO*/
#define MP4_ENC_RESYNC_CONF0              (MP4_base+0x180) /*RO*/
#define MP4_ENC_RESYNC_CONF1              (MP4_base+0x184) /*RO*/
#define MP4_ENC_TIME_BASE                 (MP4_base+0x188) /*RO*/
#define MP4_ENC_REF_INT_ADDR              (MP4_base+0x18c)    /*RW*/
#define MP4_ENC_CUR_INT_ADDR              (MP4_base+0x190)    /*RW*/
#define MP4_ENC_CYCLE_COUNT               (MP4_base+0x194)    /*RO*/   

/*DEC register*/
#define MP4_DEC_CODEC_CONF                (MP4_base+0x200) /*RW*/
#define MP4_DEC_STS                       (MP4_base+0x204) /*RO*/
#define MP4_DEC_IRQ_MASK                  (MP4_base+0x208) /*RW*/
#define MP4_DEC_IRQ_STS                   (MP4_base+0x20c) /*RO*/
#define MP4_DEC_IRQ_ACK			            (MP4_base+0x210) /*WC*/
#define MP4_DEC_CODEC_BASE		            (MP4_base+0x220) /*R/W*/
#define MP4_DEC_REF_ADDR			         (MP4_base+0x224) /*RW*/
#define MP4_DEC_REC_ADDR			         (MP4_base+0x228) /*RW*/
#define MP4_DEC_DEBLOCK_ADDR		         (MP4_base+0x22c) /*RW*/
#define MP4_DEC_STORE_ADDR			         (MP4_base+0x230) /*RW*/
#define MP4_DEC_DACP_ADDR			         (MP4_base+0x234) /*RW*/
#define MP4_DEC_MVP_ADDR			         (MP4_base+0x238) /*RW, new*/
#define MP4_DEC_VOP_STRUC0                (MP4_base+0x240) /*R/W*/
#define MP4_DEC_VOP_STRUC1                (MP4_base+0x244) /*R/W*/
#define MP4_DEC_VOP_STRUC2                (MP4_base+0x248) /*R/W*/
#define MP4_DEC_MB_STRUC0                 (MP4_base+0x24c) /*R/W*/
#define MP4_DEC_VLC_ADDR                  (MP4_base+0x260) /*WO*/
#define MP4_DEC_VLC_BIT                   (MP4_base+0x264) /*WO*/
#define MP4_DEC_VLC_LIMIT                 (MP4_base+0x268) /*R/W*/
#define MP4_DEC_VLC_WORD                  (MP4_base+0x26c) /*RO*/
#define MP4_DEC_VLC_BITCNT                (MP4_base+0x270) /*RO*/
#define MP4_DEC_QS_ADDR                   (MP4_base+0x27c)    /*RW*/
#define MP4_DEC_CYCLE_COUNT               (MP4_base+0x280)    /*RO*/
/*Core*/
#define MP4_CORE_CONF                     (MP4_base+0x300) /*RW*/
#define MP4_CORE_CODEC_CONF               (MP4_base+0x304) /*RW, ENC*/
#define MP4_CORE_DUPLEX_STS               (MP4_base+0x308) /*RO*/
#define MP4_CORE_CODEC_BASE		         (MP4_base+0x310) /*R/W*/
#define MP4_CORE_VOP_ADDR	               (MP4_base+0x314) /*RW*/
#define MP4_CORE_REF_ADDR			         (MP4_base+0x318) /*RW*/
#define MP4_CORE_REC_ADDR			         (MP4_base+0x31c) /*RW*/
#define MP4_CORE_DEBLOCK_ADDR		         (MP4_base+0x320) /*RW*/
#define MP4_CORE_STORE_ADDR		         (MP4_base+0x324) /*RW*/
#define MP4_CORE_DACP_ADDR			         (MP4_base+0x328) /*RW*/
#define MP4_CORE_MVP_ADDR			         (MP4_base+0x32c) /*RW, new*/
#define MP4_CORE_VOP_STRUC0               (MP4_base+0x330) /*R/W*/
#define MP4_CORE_VOP_STRUC1               (MP4_base+0x334) /*R/W*/
#define MP4_CORE_VOP_STRUC2               (MP4_base+0x338) /*R/W*/
#define MP4_CORE_VOP_STRUC3               (MP4_base+0x33c) /*R/W*/
#define MP4_CORE_MB_STRUC0                (MP4_base+0x340) /*R/W*/
#define MP4_CORE_MB_STRUC1                (MP4_base+0x344) /*R/W*/
#define MP4_CORE_MB_STRUC2                (MP4_base+0x348) /*R/W*/
#define MP4_CORE_MB_STRUC3                (MP4_base+0x34c) /*R/W*/
#define MP4_CORE_MB_STRUC4                (MP4_base+0x350) /*R/W*/
#define MP4_CORE_MB_STRUC5                (MP4_base+0x354) /*R/W*/
#define MP4_CORE_MB_STRUC6                (MP4_base+0x358) /*R/W*/
#define MP4_CORE_MB_STRUC7                (MP4_base+0x35c) /*R/W*/
#define MP4_CORE_VLC_STS                  (MP4_base+0x370) /*RO*/
#define MP4_CORE_VLE_STS                  (MP4_base+0x374) /*RO*/
#define MP4_CORE_VLC_ADDR                 (MP4_base+0x378) /*WO*/
#define MP4_CORE_VLC_BIT                  (MP4_base+0x37c) /*WO*/
#define MP4_CORE_VLC_LIMIT                (MP4_base+0x380) /*R/W*/
#define MP4_CORE_VLC_WORD                 (MP4_base+0x384) /*RO*/
#define MP4_CORE_VLC_BITCNT               (MP4_base+0x388) /*RO*/
#define MP4_CORE_SVLD_COMD                (MP4_base+0x400)
#define MP4_CORE_SVLD_BITCNT              (MP4_base+0x404)
#define MP4_CORE_SVLD_MARK                (MP4_base+0x408)
#define MP4_CORE_SVLD_CODE                (MP4_base+0x40C)
#define MP4_CORE_SAD_Y				         (MP4_base+0x500)
#define MP4_CORE_SAD_U		               (MP4_base+0x504)
#define MP4_CORE_SAD_V				         (MP4_base+0x508)
#define MP4_CORE_RESYNC_CONF0             (MP4_base+0x600) /*R/W*/
#define MP4_CORE_RESYNC_CONF1             (MP4_base+0x604) /*R/W*/
#define MP4_CORE_TIME_BASE                (MP4_base+0x60c) /*R/W*/

#endif /*__MT6516_MPEG4ENCDRVREG_H__*/


