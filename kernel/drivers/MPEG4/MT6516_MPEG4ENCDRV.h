

#ifndef __MT6516_MPEG4ENCDRV_H__
#define __MT6516_MPEG4ENCDRV_H__

#define EVENT_MP4_ENC_DONE				((UINT32)0x00000001)
#define EVENT_MP4_ENC_ERR				((UINT32)0x00000002)

/* GMC PORT */
//#define GMC2_MUX_PORT_SEL          (volatile unsigned long *)(GMC2_BASE+0x58)     /*RW*/

/*Common, MP4_CODEC_COMD*/
#define MP4_CODEC_COMD_CORE_RST                    0x0001
#define MP4_CODEC_COMD_ENC_RST                     0x0002
#define MP4_CODEC_COMD_DEC_RST                     0x0004
#define MP4_CODEC_COMD_ENC_START                   0x0008
#define MP4_CODEC_COMD_DEC_START                   0x0010
/*Common, MP4_VLC_DMA_COMD*/
#define MP4_VLC_DMA_COMD_STOP                      0x0001
#define MP4_VLC_DMA_COMD_RELOAD                    0x0002

/*Encoder, MP4_ENC_CODEC_CONF*/
#define MP4_ENC_CODEC_CONF_ENC		               0x0001
#define MP4_ENC_CODEC_CONF_IRQ		               0x0002
#define MP4_ENC_CODEC_CONF_DCT		               0x0004
#define MP4_ENC_CODEC_CONF_VPGOB	                  0x0008
#define MP4_ENC_CODEC_CONF_STEP	                  0x0010
#define MP4_ENC_CODEC_CONF_STEP_MASK               0x00f0
#define MP4_ENC_CODEC_CONF_HALF	                  0x0100
#define MP4_ENC_CODEC_CONF_FME		               0x0200
#define MP4_ENC_CODEC_CONF_DQUAN	                  0x0400
#define MP4_ENC_CODEC_CONF_PMV                     0x0800
#define MP4_ENC_CODEC_CONF_MC_BURST                0x1000
#define MP4_ENC_CODEC_CONF_ME_BURST                0x2000
#define MP4_ENC_CODEC_CONF_CHECK_TV                0x10000
#define MP4_ENC_CODEC_CONF_PREFETCH                0x20000   
#define MP4_ENC_CODEC_CONF_STEP_BITCNT             4
#define MP4_ENC_CODEC_CONF_STEP_COUNT              2
#define MP4_ENC_CODEC_CONF_STEP_VALUE              (MP4_ENC_CODEC_CONF_STEP_COUNT << MP4_ENC_CODEC_CONF_STEP_BITCNT)
/*Encoder, MP4_ENC_IRQ_MASK*/
#define MP4_ENC_IRQ_MASK_ENC	                     0x0001
#define MP4_ENC_IRQ_MASK_BLOCK	                  0x0002
#define MP4_ENC_IRQ_MASK_PACK    	               0x0004
#define MP4_ENC_IRQ_MASK_DMA	                     0x0008
/*Encoder, MP4_ENC_IRQ_STS*/
#define MP4_ENC_IRQ_STS_ENC	                     0x0001
#define MP4_ENC_IRQ_STS_BLOCK	                     0x0002
#define MP4_ENC_IRQ_STS_PACK    	                  0x0004
#define MP4_ENC_IRQ_STS_DMA	                     0x0008
/*Encoder, MP4_ENC_IRQ_ACK*/
#define MP4_ENC_IRQ_ACK_ENC	                     0x0001
#define MP4_ENC_IRQ_ACK_BLOCK	                     0x0002
#define MP4_ENC_IRQ_ACK_PACK    	                  0x0004
#define MP4_ENC_IRQ_ACK_DMA	                     0x0008
/*Encoder, MP4_ENC_CONF*/
#define MP4_ENC_CONF_SKIP_MASK                     0x0000003f
#define MP4_ENC_CONF_INTRA_MASK                    0x00003f00
#define MP4_ENC_CONF_PACK	                        0x00010000
#define MP4_ENC_CONF_PCAKCNT_MASK	               0x07fe0000
#define MP4_ENC_CONF_SKIP_BITCNT                   0
#define MP4_ENC_CONF_INTRA_BITCNT                  8
#define MP4_ENC_CONF_PACK_BITCNT                   16
#define MP4_ENC_CONF_PACKCNT_BITCNT              17
/*Encoder, MP4_ENC_VOP_STRUC0*/ /*MP4_VOP_STRUC0_XXX = (1 << MP4_VOP_STRUC0_XXX_BITCNT)*/
#define MP4_ENC_VOP_STRUC0_TYPE	                  0x0001
#define MP4_ENC_VOP_STRUC0_DATA	                  0x0002
#define MP4_ENC_VOP_STRUC0_RVLC      	            0x0004
#define MP4_ENC_VOP_STRUC0_SHORT                   0x0010
#define MP4_ENC_VOP_STRUC0_FCODE_MASK              0x00e0
#define MP4_ENC_VOP_STRUC0_QUANT_MASK              0x1f00
#define MP4_ENC_VOP_STRUC0_VLCTHR_MASK             0xe000
#define MP4_ENC_VOP_STRUC0_ROUND	                  0x00010000
#define MP4_ENC_VOP_STRUC0_TYPE_BITCNT	            0
#define MP4_ENC_VOP_STRUC0_DATA_BITCNT	            1
#define MP4_ENC_VOP_STRUC0_RVLC_BITCNT	            2
#define MP4_ENC_VOP_STRUC0_SHORT_BITCNT	         4
#define MP4_ENC_VOP_STRUC0_FCODE_BITCNT	         5
#define MP4_ENC_VOP_STRUC0_QUANT_BITCNT	         8
#define MP4_ENC_VOP_STRUC0_VLCTHR_BITCNT	         13
#define MP4_ENC_VOP_STRUC0_ROUND_BITCNT	         16
/*Encoder, MP4_ENC_VOP_STRUC1*/
#define MP4_ENC_VOP_STRUC1_XLIMIT_MASK	            0x001f
#define MP4_ENC_VOP_STRUC1_YLIMIT_MASK	            0x1f00
#define MP4_ENC_VOP_STRUC1_MBLENGTH_MASK	         0x000f0000
#define MP4_ENC_VOP_STRUC1_HEADERBIT_MASK          0x1f000000
#define MP4_ENC_VOP_STRUC1_XLIMIT_BITCNT	         0
#define MP4_ENC_VOP_STRUC1_YLIMIT_BITCNT	         8
#define MP4_ENC_VOP_STRUC1_MBLENGTH_BITCNT	      16
#define MP4_ENC_VOP_STRUC1_HEADERBIT_BITCNT        24
/*Encoder, MP4_ENC_VOP_STRUC2*/
#define MP4_ENC_VOP_STRUC2_START_XPOS_MASK         0x001f
#define MP4_ENC_VOP_STRUC2_START_YPOS_MASK         0x1f00
#define MP4_ENC_VOP_STRUC2_MBCOUNT_MASK	         0x01ff0000
#define MP4_ENC_VOP_STRUC2_START_XPOS_BITCNT       0
#define MP4_ENC_VOP_STRUC2_START_YPOS_BITCNT       8
#define MP4_ENC_VOP_STRUC2_MBCOUNT_BITCNT	         16
/*Encoder, MP4_ENC_VOP_STRUC3*/
#define MP4_ENC_VOP_STRUC3_CURR_XPOS_MASK          0x001f    
#define MP4_ENC_VOP_STRUC3_CURR_YPOS_MASK          0x1f00    
#define MP4_ENC_VOP_STRUC3_MBCOUNT_MASK	         0x01ff0000
#define MP4_ENC_VOP_STRUC3_CURR_XPOS_BITCNT        0
#define MP4_ENC_VOP_STRUC3_CURR_YPOS_BITCNT        8
#define MP4_ENC_VOP_STRUC3_MBCOUNT_BITCNT	         16
/*Encoder, MP4_ENC_MB_STRUC0*/
#define MP4_ENC_MB_STRUC0_CODEC	                  0x0001
#define MP4_ENC_MB_STRUC0_TYPE_MASK                0x00e0
#define MP4_ENC_MB_STRUC0_PATTERN_MASK	            0x03f0
#define MP4_ENC_MB_STRUC0_DQUANT_MASK	            0x0c00
#define MP4_ENC_MB_STRUC0_AC		                  0x1000
#define MP4_ENC_MB_STRUC0_DCVLC_MASK	            0x2000
#define MP4_ENC_MB_STRUC0_QUANTIZER_MASK	         0xc000
#define MP4_ENC_MB_STRUC0_CODEC_BITCNT	            0
#define MP4_ENC_MB_STRUC0_TYPE_BITCNT	            1
#define MP4_ENC_MB_STRUC0_PATTERN_BITCNT	         5
#define MP4_ENC_MB_STRUC0_DQUANT_BITCNT	         10
#define MP4_ENC_MB_STRUC0_AC_BITCNT		            12
#define MP4_ENC_MB_STRUC0_DCVLC_BITCNT	            13
#define MP4_ENC_MB_STRUC0_QUANTIZER_BITCNT	      14
/*Encoder, MP4_ENC_VLC_LIMIT*/
#define MP4_ENC_VLC_LIMIT_MASK                     0xffff
/*Encoder, MP4_ENC_RESYNC_CONF0*/
#define MP4_ENC_RESYNC_CONF0_PERIOD_BITS_MASK      0x3fffffff
#define MP4_ENC_RESYNC_CONF0_MODE_MB               0x40000000
#define MP4_ENC_RESYNC_CONF0_MODE_BITS             0x0
#define MP4_ENC_RESYNC_CONF0_MODE_EN               0x80000000
/*Encoder, MP4_ENC_RESYNC_CONF1*/
#define MP4_ENC_RESYNC_CONF1_PERIOD_MB_MASK        0x0000ffff
#define MP4_ENC_RESYNC_CONF1_HE_VALUE              0x00010000
/*Encoder, MP4_ENC_TIME_BASE, VTI=VOP_TIME_INCREMENT, MTB=MODULE_TIME_BASE*/
#define MP4_ENC_TIME_BASE_VTI_MASK                 0x0000ffff
#define MP4_ENC_TIME_BASE_VTI_BW                   0x000f0000
#define MP4_ENC_TIME_BASE_MTB_MASK                 0x01f00000

/*Decoder, MP4_DEC_CODEC_CONF*/
#define MP4_DEC_CODEC_CONF_ENC		               0x0001
#define MP4_DEC_CODEC_CONF_IRQ		               0x0002
#define MP4_DEC_CODEC_CONF_DCT		               0x0004
#define MP4_DEC_CODEC_CONF_VPGOB	                  0x0008
#define MP4_DEC_CODEC_CONF_STEP	                  0x0010
#define MP4_DEC_CODEC_CONF_STEP_MASK               0x00f0
#define MP4_DEC_CODEC_CONF_HALF	                  0x0100
#define MP4_DEC_CODEC_CONF_FME		               0x0200
#define MP4_DEC_CODEC_CONF_DQUAN	                  0x0400
#define MP4_DEC_CODEC_CONF_PMV                     0x0800
#define MP4_DEC_CODEC_CONF_MC_BURST                0x1000
#define MP4_DEC_CODEC_CONF_COPY_REC                0x4000
#define MP4_DEC_CODEC_CONF_DEBLOCK                 0x8000
#define MP4_DEC_CODEC_CONF_CHECK_TV                0x10000
/*Decoder, MP4_DEC_IRQ_MASK*/
#define MP4_DEC_IRQ_MASK_VLD	                     0x0001
#define MP4_DEC_IRQ_MASK_RLD	                     0x0002
#define MP4_DEC_IRQ_MASK_MARK	        	            0x0004
#define MP4_DEC_IRQ_MASK_DEC	                     0x0008
#define MP4_DEC_IRQ_MASK_BLOCK	                  0x0010
#define MP4_DEC_IRQ_MASK_DMA	                     0x0020
/*Decoder, MP4_DEC_IRQ_STS*/
#define MP4_DEC_IRQ_STS_VLD	                     0x0001
#define MP4_DEC_IRQ_STS_RLD	                     0x0002
#define MP4_DEC_IRQ_STS_MARK	        	            0x0004
#define MP4_DEC_IRQ_STS_DEC	                     0x0008
#define MP4_DEC_IRQ_STS_BLOCK	                     0x0010
#define MP4_DEC_IRQ_STS_DMA	                     0x0020
/*Decoder, MP4_DEC_IRQ_ACK*/
#define MP4_DEC_IRQ_ACK_VLD	                     0x0001
#define MP4_DEC_IRQ_ACK_RLD	                     0x0002
#define MP4_DEC_IRQ_ACK_MARK	        	            0x0004
#define MP4_DEC_IRQ_ACK_DEC	                     0x0008
#define MP4_DEC_IRQ_ACK_BLOCK	                     0x0010
#define MP4_DEC_IRQ_ACK_DMA	                     0x0020
/*Decoder, MP4_DEC_VOP_STRUC0*/ /*MP4_VOP_STRUC0_XXX = (1 << MP4_VOP_STRUC0_XXX_BITCNT)*/
#define MP4_DEC_VOP_STRUC0_TYPE	                  0x0001
#define MP4_DEC_VOP_STRUC0_DATA	                  0x0002
#define MP4_DEC_VOP_STRUC0_RVLC      	            0x0004
#define MP4_DEC_VOP_STRUC0_SHORT                   0x0010
#define MP4_DEC_VOP_STRUC0_FCODE_MASK              0x00e0
#define MP4_DEC_VOP_STRUC0_QUANT_MASK              0x1f00
#define MP4_DEC_VOP_STRUC0_VLCTHR_MASK             0xe000
#define MP4_DEC_VOP_STRUC0_ROUND	                  0x00010000
#define MP4_DEC_VOP_STRUC0_TYPE_BITCNT	            0
#define MP4_DEC_VOP_STRUC0_DATA_BITCNT	            1
#define MP4_DEC_VOP_STRUC0_RVLC_BITCNT	            2
#define MP4_DEC_VOP_STRUC0_SHORT_BITCNT	         4
#define MP4_DEC_VOP_STRUC0_FCODE_BITCNT	         5
#define MP4_DEC_VOP_STRUC0_QUANT_BITCNT	         8
#define MP4_DEC_VOP_STRUC0_VLCTHR_BITCNT	         13
#define MP4_DEC_VOP_STRUC0_ROUND_BITCNT	         16
/*Decoder, MP4_DEC_VOP_STRUC1*/
#define MP4_DEC_VOP_STRUC1_XLIMIT_MASK	            0x001f
#define MP4_DEC_VOP_STRUC1_YLIMIT_MASK	            0x1f00
#define MP4_DEC_VOP_STRUC1_MBLENGTH_MASK	         0x000f0000
#define MP4_DEC_VOP_STRUC1_HEADERBIT_MASK          0x1f000000
#define MP4_DEC_VOP_STRUC1_XLIMIT_BITCNT	         0
#define MP4_DEC_VOP_STRUC1_YLIMIT_BITCNT	         8
#define MP4_DEC_VOP_STRUC1_MBLENGTH_BITCNT	      16
#define MP4_DEC_VOP_STRUC1_HEADERBIT_BITCNT        24
/*Decoder, MP4_DEC_VOP_STRUC2*/
#define MP4_DEC_VOP_STRUC2_START_XPOS_MASK         0x001f
#define MP4_DEC_VOP_STRUC2_START_YPOS_MASK         0x1f00
#define MP4_DEC_VOP_STRUC2_MBCOUNT_MASK	         0x01ff0000
#define MP4_DEC_VOP_STRUC2_START_XPOS_BITCNT       0
#define MP4_DEC_VOP_STRUC2_START_YPOS_BITCNT       8
#define MP4_DEC_VOP_STRUC2_MBCOUNT_BITCNT	         16
/*Decoder, MP4_DEC_MB_STRUC0*/
#define MP4_DEC_MB_STRUC0_CODEC	                  0x0001
#define MP4_DEC_MB_STRUC0_TYPE_MASK                0x00e0
#define MP4_DEC_MB_STRUC0_PATTERN_MASK	            0x03f0
#define MP4_DEC_MB_STRUC0_DQUANT_MASK	            0x0c00
#define MP4_DEC_MB_STRUC0_AC		                  0x1000
#define MP4_DEC_MB_STRUC0_DCVLC_MASK	            0x2000
#define MP4_DEC_MB_STRUC0_QUANTIZER_MASK	         0x0007c000
#define MP4_DEC_MB_STRUC0_CODEC_BITCNT	            0
#define MP4_DEC_MB_STRUC0_TYPE_BITCNT	            1
#define MP4_DEC_MB_STRUC0_PATTERN_BITCNT	         5
#define MP4_DEC_MB_STRUC0_DQUANT_BITCNT	         10
#define MP4_DEC_MB_STRUC0_AC_BITCNT		            12
#define MP4_DEC_MB_STRUC0_DCVLC_BITCNT	            13
#define MP4_DEC_MB_STRUC0_QUANTIZER_BITCNT	      14
/*Decoder, MP4_DEC_VLC_LIMIT*/
#define MP4_DEC_VLC_LIMIT_MASK                     0xffff

/*Core, MP4_CORE_CODEC_CONF*/
#define MP4_CORE_CODEC_CONF_ENC		               0x0001
#define MP4_CORE_CODEC_CONF_IRQ		               0x0002
#define MP4_CORE_CODEC_CONF_DCT		               0x0004
#define MP4_CORE_CODEC_CONF_VPGOB	               0x0008
#define MP4_CORE_CODEC_CONF_STEP	                  0x0010
#define MP4_CORE_CODEC_CONF_STEP_MASK              0x00f0
#define MP4_CORE_CODEC_CONF_HALF	                  0x0100
#define MP4_CORE_CODEC_CONF_FME		               0x0200
#define MP4_CORE_CODEC_CONF_DQUAN	               0x0400
#define MP4_CORE_CODEC_CONF_PMV                    0x0800
#define MP4_CORE_CODEC_CONF_MC_BURST               0x1000
#define MP4_CORE_CODEC_CONF_ME_BURST               0x2000
#define MP4_CORE_CODEC_CONF_COPY_REC               0x4000
#define MP4_CORE_CODEC_CONF_DEBLOCK                0x8000
#define MP4_CORE_CODEC_CONF_CHECK_TV               0x10000
/*Core, MP4_CORE_CONF*/
#define MP4_CORE_ENC_CONF_SKIP_MASK                0x0000003f
#define MP4_CORE_ENC_CONF_INTRA_MASK               0x00003f00
#define MP4_CORE_ENC_CONF_PACK	                  0x00010000
#define MP4_CORE_ENC_CONF_PCAKCNT_MASK	            0x07fe0000
/*Core, MP4_CORE_VOP_STRUC0*/ /*MP4_VOP_STRUC0_XXX = (1 << MP4_VOP_STRUC0_XXX_BITCNT)*/
#define MP4_CORE_VOP_STRUC0_TYPE	                  0x0001
#define MP4_CORE_VOP_STRUC0_DATA	                  0x0002
#define MP4_CORE_VOP_STRUC0_RVLC      	            0x0004
#define MP4_CORE_VOP_STRUC0_SHORT                  0x0010
#define MP4_CORE_VOP_STRUC0_FCODE_MASK             0x00e0
#define MP4_CORE_VOP_STRUC0_QUANT_MASK             0x1f00
#define MP4_CORE_VOP_STRUC0_VLCTHR_MASK            0xe000
#define MP4_CORE_VOP_STRUC0_ROUND	               0x00010000
#define MP4_CORE_VOP_STRUC0_TYPE_BITCNT	         0
#define MP4_CORE_VOP_STRUC0_DATA_BITCNT	         1
#define MP4_CORE_VOP_STRUC0_RVLC_BITCNT	         2
#define MP4_CORE_VOP_STRUC0_SHORT_BITCNT	         4
#define MP4_CORE_VOP_STRUC0_FCODE_BITCNT	         5
#define MP4_CORE_VOP_STRUC0_QUANT_BITCNT	         8
#define MP4_CORE_VOP_STRUC0_VLCTHR_BITCNT	         13
#define MP4_CORE_VOP_STRUC0_ROUND_BITCNT	         16
/*Core, MP4_CORE_VOP_STRUC1*/
#define MP4_CORE_VOP_STRUC1_XLIMIT_MASK	         0x001f
#define MP4_CORE_VOP_STRUC1_YLIMIT_MASK	         0x1f00
#define MP4_CORE_VOP_STRUC1_MBLENGTH_MASK	         0x000f0000
#define MP4_CORE_VOP_STRUC1_HEADERBIT_MASK         0x1f000000
#define MP4_CORE_VOP_STRUC1_XLIMIT_BITCNT	         0
#define MP4_CORE_VOP_STRUC1_YLIMIT_BITCNT	         8
#define MP4_CORE_VOP_STRUC1_MBLENGTH_BITCNT	      16
#define MP4_CORE_VOP_STRUC1_HEADERBIT_BITCNT       24
/*Core, MP4_CORE_VOP_STRUC2*/
#define MP4_CORE_VOP_STRUC2_START_XPOS_MASK        0x001f
#define MP4_CORE_VOP_STRUC2_START_YPOS_MASK        0x1f00
#define MP4_CORE_VOP_STRUC2_MBCOUNT_MASK	         0x01ff0000
#define MP4_CORE_VOP_STRUC2_START_XPOS_BITCNT      0
#define MP4_CORE_VOP_STRUC2_START_YPOS_BITCNT      8
#define MP4_CORE_VOP_STRUC2_MBCOUNT_BITCNT	      16
/*Core, MP4_CORE_VOP_STRUC3*/
#define MP4_CORE_VOP_STRUC3_CURR_XPOS_MASK         0x001f    
#define MP4_CORE_VOP_STRUC3_CURR_YPOS_MASK         0x1f00    
#define MP4_CORE_VOP_STRUC3_MBCOUNT_MASK	         0x01ff0000
#define MP4_CORE_VOP_STRUC3_CURR_XPOS_BITCNT       0
#define MP4_CORE_VOP_STRUC3_CURR_YPOS_BITCNT       8
#define MP4_CORE_VOP_STRUC3_MBCOUNT_BITCNT	      16
/*Core, MP4_CORE_MB_STRUC0*/
#define MP4_CORE_MB_STRUC0_CODEC	                  0x0001
#define MP4_CORE_MB_STRUC0_TYPE_MASK               0x00e0
#define MP4_CORE_MB_STRUC0_PATTERN_MASK	         0x03f0
#define MP4_CORE_MB_STRUC0_DQUANT_MASK	            0x0c00
#define MP4_CORE_MB_STRUC0_AC		                  0x1000
#define MP4_CORE_MB_STRUC0_DCVLC_MASK	            0x2000
#define MP4_CORE_MB_STRUC0_QUANTIZER_MASK	         0xc000
#define MP4_CORE_MB_STRUC0_CODEC_BITCNT	         0
#define MP4_CORE_MB_STRUC0_TYPE_BITCNT	            1
#define MP4_CORE_MB_STRUC0_PATTERN_BITCNT	         5
#define MP4_CORE_MB_STRUC0_DQUANT_BITCNT	         10
#define MP4_CORE_MB_STRUC0_AC_BITCNT		         12
#define MP4_CORE_MB_STRUC0_DCVLC_BITCNT	         13
#define MP4_CORE_MB_STRUC0_QUANTIZER_BITCNT	      14
/*Core, MP4_CORE_VLC_LIMIT*/
#define MP4_CORE_VLC_LIMIT_MASK                    0xffff
/*Core, MP4_CORE_RESYNC_CONF0*/
#define MP4_CORE_RESYNC_CONF0_PERIOD_BITS_MASK     0x3fffffff
#define MP4_CORE_RESYNC_CONF0_MODE_MB              0x40000000
#define MP4_CORE_RESYNC_CONF0_MODE_BITS            0x0
#define MP4_CORE_RESYNC_CONF0_MODE_EN              0x80000000
/*Core, MP4_CORE_RESYNC_CONF1*/
#define MP4_CORE_RESYNC_CONF1_PERIOD_MB_MASK       0x0000ffff
#define MP4_CORE_RESYNC_CONF1_HE_VALUE             0x00010000
/*Core, MP4_CORE_TIME_BASE, VTI=VOP_TIME_INCREMENT, MTB=MODULE_TIME_BASE*/
#define MP4_CORE_TIME_BASE_VTI_MASK                0x0000ffff
#define MP4_CORE_TIME_BASE_VTI_BW                  0x000f0000
#define MP4_CORE_TIME_BASE_MTB_MASK                0x01f00000

typedef enum 
{
		SET_ENC_HAL_RST		= 1,        ///< set reset
		SET_ENC_HAL_CB		= 2,        ///< set callback
		SET_ENC_HAL_REF_BUF	= 3,        ///< set reference buffer
		SET_ENC_HAL_FRM_BUF	= 4,        ///< set frame buffer
		SET_ENC_HAL_BS_BUF	= 5,        ///< set bitstream buffer
		SET_ENC_HAL_RC		= 6,        ///< set rate control
		SET_ENC_HAL_ME		= 7,        ///< set motion estimation
		SET_ENC_HAL_ENC_PARAM	= 8,    ///< set encoding parameter
		SET_ENC_HAL_MP4_ENC_PARAM = 9,  ///< set mpeg4 encoding parameter
		SET_ENC_HAL_START	= 10,       ///< set start
		SET_ENC_HAL_INIT = 11           ///< set init
}  VideoEncHalDrvCmd;

///

typedef struct  
{
    INT32   frame_rate;             ///< frame rate
    INT32   width;                  ///< frame width
    INT32   height;                 ///< frame height
    INT32   intra_num;              ///< number of encoded I frame
    BOOL    b_short_header;         ///< coding use short header or not
    INT32   predition_type;         ///< coding in I or P frame
    /// This value represents the absolute vop_time_increment from the
    /// synchronization point marked by the modulo_time_base measured
    /// in the number of clock ticks. It can take a value in the range
    /// of [0,vop_time_increment_resolution)
    INT32   vop_time_increment;         ///< vop time increment
    INT32   fixed_vop_time_increment;   ///< fixed vop time increment
    /// vop_time_increment_resolution, indicates the number of evenly spaced subintervals,
    /// called ticks, within one modulo time. One modulo time represents the fixed interval
    /// of one second. The value zero is forbidden.
    INT32   time_increment;             ///< time increment
    UINT32 old_time;                    ///< saved time
    INT32   quantizer;                  ///< quantization parameter
    INT32   intra_threshlod;            ///< threshold to become intra conding
    INT32   fcode;                      ///< fcode, for ME
    INT32   time_base;                  ///< time base
    INT32   prev_vop_time_increment;    ///< previous vop time increment
    INT32   prev_old_time;              ///< previous saved time
    UINT32	intra_period;               ///< intra frame period
    
}m4v_enc_im;

typedef struct 
{
    UINT32  frame;                  ///< total frame 
    UINT8   *p_input_yuv_data;      ///< input frame physical address
    UINT8   *v_input_yuv_data;      ///< input frame virtual address
    UINT8   *p_bitstreams_ptr;      ///< output bitstream physical address
    UINT8   *v_bitstreams_ptr;      ///< output bitstream virtual address
    UINT32  buffer_length;          ///< buffer length
    m4v_enc_im  *im;                ///< input parameter
    BOOL     b_force_I_frame;       ///< force coding I frame
    UINT32 pkt_size_limit;          ///< packet size limit
} mp4_enc_proc_struct;

typedef struct
{
    UINT8 *va;       ///< virtual address   
    UINT32 pa;       ///< physical address   
    UINT32 size;     ///< size   
} StructMemAddr;

typedef struct
{
    UINT8   *p_int_mem_addr;    ///< initial memory address 
    UINT32  int_mem_size;       ///< init memory size
    UINT8   *v_ext_mem_addr;    ///< external memory address, virtual
    UINT32  p_ext_mem_addr;     ///< external memory address, physical
    UINT32  ext_mem_size;       ///< external memory size

    m4v_enc_im enc_im;	        ///< encoder input parameter
    UINT8 *p_mp4_enc_info;      ///< encoder information
    
    StructMemAddr enc_working_memory;	    ///< encoder working memory
    StructMemAddr enc_frame_buffer_addr;    ///< encoder frame buffer address    
    UINT32 enc_time_stamp;	                ///< time stamp
    UINT32 enc_const_delay;                 ///< const delay for time stamp(AV sync)	
    UINT32 enc_total_frame;	                ///< total frame
    
    UINT32 p_enc_input_yuv_data;            ///< input yuv data address, physical
    UINT8  *v_enc_input_yuv_data;           ///< input yuv data address, virtual
    UINT32 enc_input_yuv_data_size;         ///< input yuv data size
    
    UINT32 p_enc_bitstreams_ptr;            ///< output bistream data address, physical
    UINT8  *v_enc_bitstreams_ptr;           ///< output bistream data address, virtual
    UINT32 bitstream_limit;                 ///< output bistream data size
    // Comment by mtk01845, use pointer instead
    //UINT8 enc_vos_data[MP4_MAX_VOS_LENGTH];
    
    UINT32 enc_bit_rate;                    ///< encoding bitrate
    UINT8 *enc_vos_data;                    ///< encoding vos data
    UINT32 enc_vos_size;                    ///< encoding vos data size
    UINT32 enc_trigger_limit;               ///< encoding size limit
    UINT32 enc_toal_save_bytes;             ///< encoding total save bytes
    UINT32 frame_size;                      ///< frame size in bytes        
    BOOL b_reach_limit;                     ///< if bitstream hit limit
    BOOL b_I_frame;                         ///< is key frame
    BOOL  b_use_pmem;                       ///< use pmem or not
        
    UINT32  u4_MP4_ENC_CODEC_CONF;                 
    UINT32  u4_MP4_ENC_IRQ_MASK;     
}mp4_enc_info;

typedef struct 
{
	UINT32  u4_MP4_ENC_ADDR_VA;                 ///< reg input
    UINT32  u4_MP4_ENC_ADDR_PA;                 ///< reg input
    UINT32  u4_MP4_ENC_VOP_ADDR_VA;             ///< reg input
    UINT32  u4_MP4_ENC_VOP_ADDR_PA;             ///< reg input
    UINT32  u4_MP4_ENC_CONF;                    ///< reg input
    UINT32  u4_MP4_ENC_RESYNC_CONF0;            ///< reg input
    UINT32  u4_MP4_ENC_VOP_STRUC0;              ///< reg input
    UINT32  u4_MP4_ENC_VOP_STRUC1;              ///< reg input
    UINT32  u4_MP4_ENC_VOP_STRUC2;              ///< reg input
    UINT32  u4_MP4_ENC_VOP_STRUC3;              ///< reg input
    UINT32  u4_MP4_ENC_VLC_ADDR;                ///< reg input
    UINT32  u4_MP4_ENC_VLC_BIT;                 ///< reg input
    UINT32  u4_MP4_ENC_VLC_LIMIT;               ///< reg input
    UINT32  g_curr_vlc_addr;                    ///< current vlc address
    UINT32  g_real_bitstream_length;            ///< real bitstream length
    UINT32  g_encoded_bitstream_length;         ///< encoded bistream length
    UINT32  g_encoded_bitstream_start_addr;     ///< encoded bistream start address
    UINT32  g_mp4_enc_event_id;                 ///< event id
    UINT32  enc_toal_save_bytes;                ///< total bytes
    UINT32  frame_size;                         ///< frame size
    UINT32  enc_trigger_limit;                  ///< trigger limit
    UINT32  enc_total_frame;                    ///< total frame
    UINT32  u4_buffer_length;                   ///< buffer length
    BOOL  mp4_enc_prev_bitstream_reach_limit_flag;  ///< is previous encoding reach limit
    UINT8  *p_mp4_enc_info;                     ///< encoder information
} mp4_enc_proc_start_struct;

#endif /*__MT6516_MPEG4ENCDRV_H__*/


