
#ifndef _MT6516IDP_H
#define _MT6516IDP_H

#include <linux/ioctl.h>

#define IDP_DEV_MAJOR_NUMBER 252
#define IDPMAGICNO 'g'
//IOCTRL(inode * ,file * ,cmd * ,arg * )

#define CRZ_IRQ 0x1
#define PRZ_IRQ 0x2
#define DRZ_IRQ 0x4
//#define IPP_IRQ 0x8
//#define R2Y0_IRQ 0x10
//#define Y2R1_IRQ 0x20
#define IBW1_IRQ 0x40
#define IBW2_IRQ 0x80
#define IBR1_IRQ 0x100
#define OVL_IRQ 0x200
#define IRT0_IRQ 0x400
#define IRT1_IRQ 0x800
#define IRT3_IRQ 0x1000
#define JPEGDMA_IRQ 0x2000
#define VDOENCDMA_IRQ 0x20000
#define VDODECDMA_IRQ 0x80000
#define MP4DBLK_IRQ 0x4000

#define MT6516IDP_RES_CRZ 0x1
#define MT6516IDP_RES_PRZ 0x2
#define MT6516IDP_RES_DRZ 0x4
#define MT6516IDP_RES_IPP 0x8
#define MT6516IDP_RES_R2Y0 0x10
#define MT6516IDP_RES_Y2R1 0x20
#define MT6516IDP_RES_IBW1 0x40
#define MT6516IDP_RES_IBW2 0x80
#define MT6516IDP_RES_IBR1 0x100
#define MT6516IDP_RES_OVL 0x200
#define MT6516IDP_RES_IRT0 0x400
#define MT6516IDP_RES_IRT1 0x800
#define MT6516IDP_RES_IRT3 0x1000
#define MT6516IDP_RES_JPEGDMA 0x2000
#define MT6516IDP_RES_VDOENCDMA 0x20000
#define MT6516IDP_RES_VDODECDMA 0x80000
#define MT6516IDP_RES_MP4DBLK 0x4000

#define CRZ_CUST
typedef enum {
    IDP_CRZ = 0,
    IDP_PRZ,
    IDP_DRZ,
    IDP_IPP1_Y2R0_IPP2,
    IDP_R2Y0,
    IDP_Y2R1,
    IDP_IBW1,
    IDP_IBW2,
    IDP_IBR1,
    IDP_OVL,
    IDP_IRT0,
    IDP_IRT1,
    IDP_IRT3,
    IDP_JPEGDMA,
    IDP_VDOENCDMA,
    IDP_VDODECDMA,
    IDP_MP4DBLK,
    IDP_ConfigFuncCnt
//    IDP_BUTT = 0xFFFFFFFF
}eMT6516IDP_NODES;

//Structure list
typedef enum{
    CRZSRC_ISP = 0,
    CRZSRC_IRT0,
    CRZSRC_MP4,
    CRZSRC_PRZ,
    CRZSRC_R2Y0
//    CRZSRC_BUTT = 0xFFFFFFFF
} MT6516CRZ_SRC;

typedef struct {
    MT6516CRZ_SRC eSrc;
    u32 bToOVL:1;
    u32 bToIPP1:1;
    u32 bToY2R1:1;
    u32 bContinousRun:1;//single mode : 0, continous mode : 1.
#ifdef CRZ_CUST
    u32 uDownsampleCoeff:4;//0~12
    u32 uUpsampleCoeff:4;//0~12
    u32 uReserved:20;
#else
    u32 uReserved:28;
#endif
    u16 u2SrcImgWidth;// 3~2688
    u16 u2SrcImgHeight;// 3~2688
    u16 u2DestImgWidth;// 3~2688 
    u16 u2DestImgHeight;// 3~2688
} stCRZCfg;

typedef enum {
    PRZSRC_IRT0 = 1,
    PRZSRC_MP4,
    PRZSRC_IBW4,
    PRZSRC_R2Y0,
    PRZSRC_JPGDEC
//    PRZSRC_BUTT = 0xFFFFFFFF
}MT6516PRZ_SRC;

typedef struct{
    MT6516PRZ_SRC eSrc;
    u32 bContinousRun:1;//single mode : 0, continous mode : 1.
    u32 bToCRZ:1;//Only for source is JPG decoder.
    u32 bToIPP1:1;// mutual exclusive to CRZ
    u32 bToY2R1:1;// mutual exclusive to CRZ
    u32 uReserved:28;
    u16 u2SrcImgWidth;
    u16 u2SrcImgHeight;
    u16 u2DestImgWidth;
    u16 u2DestImgHeight;
    u16 u2SampleFactor;//0 :1 , 1 : 1/2, 2 : 1/4, 3 : No Y component
} stPRZCfg;

typedef struct{
    u8 bContinousRun;//single mode : 0, continous mode : 1.
    u16 u2SrcImgWidth;
    u16 u2SrcImgHeight;
    u16 u2DestImgWidth;
    u16 u2DestImgHeight;
} stDRZCfg;

typedef enum {
    IPPSRC_CRZ = 1,
    IPPSRC_PRZ = 2,
    IPPSRC_MP4 = 4,
    IPPSRC_IRT0 = 8
//    IPPSRC_BUTT = 0xFFFFFFFF
}MT6516IPP_SRC;

#define MT6516IPP_GAMMA_COEFF_CNT 8
#define MT6516IPP_COLOR_OFF_COEFF_CNT 12
#define MT6516IPP_COLOR_SLP_COEFF_CNT 9
typedef struct{
    MT6516IPP_SRC eSrc;
    u32 bToOVL:1;
    u32 bToIBW2:1;
    u32 bOverlap:1;
    u32 bRGBDetectNReplace:1;//enable RGB detect and replace
    u32 bSDT1:1;//enable RGB domain dithering
    u32 bColorInverse:1;//Enable color inverse
    u32 bGammaCorrect:1;//Enable Gamma correction
    u32 bGammaGreatThanOne:1;// 1: if Gamma > 1
    u32 bColorAdj:1;//Enable color adjustment
    u32 bY2R0Round:1;//Enable YUV to RGB rounding option
    u32 bY2R0:1;//Enable Y2R0
    u32 bSDT0:1;//Enable YUV domain dithering.
    u32 bColorize:1;//Enable colorize
    u32 bSAT:1;//enable saturation adj
    u32 bHue:1;//enable hue adj
    u32 bContrastNBrightness:1;//Contrast and brightness
    u32 u2Reserved:16;
    u8 uBD1;
    u8 uBD2;
    u8 uBD3;
    u8 uSeed1;
    u8 uSeed2;
    u8 uSeed3;
    u8 uHueC11;
    u8 uHueC12;
    u8 uHueC21;
    u8 uHueC22;
    u8 uSaturation;
    u8 uBrightCoeffB1;
    u8 uBrightCoeffB2;
    u8 uContrast;
    u8 uColorizeU;
    u8 uColorizeV;
    u8 uGammaOffset[MT6516IPP_GAMMA_COEFF_CNT];
    u8 uGammaSlope[MT6516IPP_GAMMA_COEFF_CNT];
    u8 uColorAdjOffet[MT6516IPP_COLOR_OFF_COEFF_CNT];
    u8 uColorAdjSlope[MT6516IPP_COLOR_SLP_COEFF_CNT];
    u8 uRGBDetect[3];//[0]-R, [1]-G, [2]-B
    u8 uRGBReplace[3];//[0]-R, [1]-G, [2]-B
} stIPP1Y2R0IPP2Cfg;

typedef enum {
    R2Y0SRC_IBW2 = 1,
    R2Y0SRC_IBR1 = 2
//    R2Y0SRC_BUTT = 0xFFFFFFFF
}MT6516R2Y0_SRC;

typedef struct{
    MT6516R2Y0_SRC eSrc;
    u32 bToCRZ:1;
    u32 bToPRZ:1;
    u32 bR2Y:1;//Enable rgb 2 yuv 
    u32 bR2Y0Round:1;
    u32 uReserved:28;
} stR2Y0Cfg;

typedef enum {
    Y2R1SRC_CRZ = 1,
    Y2R1SRC_PRZ = 2,
    Y2R1SRC_DRZ = 4
//    Y2R1SRC_BUTT = 0xFFFFFFFF
}MT6516Y2R1_SRC;

typedef struct{
    MT6516Y2R1_SRC eSrc;
    u32 bRGBDetectNReplace:1;//enable RGB detect and replace
    u32 bSDT3:1;//RGB domain dithering
    u32 bSDT2:1;//YUV domain dithering
    u32 bRoundY2R1:1;//
    u32 bY2R1:1;// Enable yuv 2 rgb
    u32 uReserved:27;//
    u8 uBD1;
    u8 uBD2;
    u8 uBD3;
    u8 uSeed1;
    u8 uSeed2;
    u8 uSeed3;
    u8 uRGBDetect[3];//[0]-R, [1]-G, [2]-B
    u8 uRGBReplace[3];//[0]-R, [1]-G, [2]-B
} stY2R1Cfg;

typedef enum {
    MT6516IBW1_OUTFMT_RGB565 = 0,
    MT6516IBW1_OUTFMT_RGB888,
    MT6516IBW1_OUTFMT_ARGB8888
//    MT6516IBW1_OUTFMT_BUTT = 0xFFFFFFFF
}MT6516IBW1_OUTFMT;

typedef struct{
    MT6516IBW1_OUTFMT eFmt;
    u8 uAlpha;// Alpha value
    u16 u2SrcImgWidth;// Source image width
    u16 u2SrcImgHeight;// Source image height
    u16 u2ClipStartX;// Clip window
    u16 u2ClipEndX;
    u16 u2ClipStartY;
    u16 u2ClipEndY;
    u16 u2PitchImg0Width;
    u16 u2PitchImg1Width;
    u16 u2PitchImg2Width;
    u32 bInterrupt:1;// 1: enable interrupt.
    u32 bInformLCDDMA:1;// 1: enable inform LCD DMA when end of frame comes.
    u32 bAutoRestart:1;// 1: continous mode
    u32 bTripleBuffer:1;// 0 : double buffer, 1:triple buffer.
    u32 bPitch:1;// 1: enable pitch
    u32 bClip:1;// 1: enable Clip
    u32 uReserved:26;
    u32 u4DestBufferPhysAddr0;//image buffer address
    u32 u4DestBufferPhysAddr1;//image buffer address (double and tripple)
    u32 u4DestBufferPhysAddr2;//image buffer address (tripple)
} stIBW1Cfg;

typedef struct{
    u8 uAlpha;// Alpha value
    u16 u2SrcImgWidth;// Source image width
    u16 u2SrcImgHeight;// Source image height
    u16 u2ClipStartX;// Clip window
    u16 u2ClipEndX;
    u16 u2ClipStartY;
    u16 u2ClipEndY;
    u32 bInterrupt:1;// 1: enable interrupt.
    u32 bInformLCDDMA:1;// 1: enable inform LCD DMA when end of frame comes.
    u32 bClip:1;// 1: enable Clip
    u32 bToLCDDMA:1;
    u32 bAutoRestart:1;// 1: continous mode
    u32 bToIRT1:1;
    u32 bToR2Y0:1;
    u32 bECOVideoRecMode:1; // set 1 when Video recording mode.
    u32 uReserved:24;
} stIBW2Cfg;

typedef enum {
    MT6516IBR1_OUTFMT_RGB565 = 0,
    MT6516IBR1_OUTFMT_BGR888 = 1,
    MT6516IBR1_OUTFMT_RGB888 = 3
//    MT6516IBR1_OUTFMT_BUTT = 0xFFFFFFFF
}MT6516IBR1_OUTFMT;

typedef struct{
    u8 bInterrupt;// 1: enable interrupt.
    MT6516IBR1_OUTFMT eFmt;
    u32 u4SrcBufferPhysAddr;//image source address
    u32 u4SrcImgPixelCnt;
} stIBR1Cfg;

typedef enum {
    OVLSRC_IPP1 = 0,
    OVLSRC_CRZ
//    OVLSRC_OUTFMT_BUTT = 0xFFFFFFFF
}MT6516OVL_SRC;

typedef struct{
    MT6516OVL_SRC eSrc;
    u8 uMaskDataDepth;// 0 : 1bit, 1 : 2 bit, 2: 4 bit, 3 : 8bit
    u8 uColorKey;
    u8 uHRatio;
    u8 uVRatio;
    u16 u2MaskImgWidth;
    u16 u2MaskImgHeight;
    u32 bOverlay:1;// 0 : disable
    u32 bToY2R0:1;
    u32 bToDRZ:1;
    u32 bToJPEGDMA:1;
    u32 bToVDOENC:1;
    u32 bToPRZ:1;
    u32 bReserved:26;
    u32 u4SrcImgPhyAddr;//8x aligned
    u32  * pu4Palette;
} stOVLCfg;

typedef enum {
    VDOENC_RDMA = 0,
    VDODEC_BLOCK,
    VDODEC_SCANLINE
//    VDODEC_BUTT = 0xFFFFFFFF
}MT6516IRT0_SRC;

typedef struct{
    MT6516IRT0_SRC eSrc;
    u32 bInterrupt:1;// 1: enable interrupt.
    u32 bToPRZ:1;
    u32 bToMP4DeBlk:1;
    u32 bToCRZ:1;
    u32 bToIPP1:1;
    u32 bAutoRestart:1;// 1: continous mode
    u32 bRotate:2;// 0:no rotate , 1:90 , 2:180, 3:270
    u32 bFlip:1;// 1:flip after rotation
    u32 uReserved:23;
    u16 u2SrcImgWidth;// Source image width, < 4095
    u16 u2SrcImgHeight;// Source image height, <4095
} stIRT0Cfg;

typedef enum {
    MT6516IRT1_OUTFMT_RGB565 = 0,
    MT6516IRT1_OUTFMT_RGB888,
    MT6516IRT1_OUTFMT_ARGB888FromIBW2,
    MT6516IRT1_OUTFMT_ARGB888FromIRT1
//    MT6516IRT1_OUTFMT_BUTT = 0xFFFFFFFF
}MT6516IRT1_OUTFMT;

typedef struct{
    u32 bInterrupt:1;
    u32 bTripleBuffer:1;// 0 : double buffer, 1:triple buffer.
    u32 bAutoRestart:1;// 1 : continous mode.
//    u8 bInformLCDDMA:1;// 
    u32 bPitch:1;// 1: enable pitch
    u32 bRotate:2;// 0:no rotate , 1:90 , 2:180, 3:270
    u32 bFlip:1;// 1:flip after rotation
    u32 uReserved:25;
    MT6516IRT1_OUTFMT eFmt;
    u8 uAlpha;// Alpha value
    u16 u2SrcImgWidth;// Source image width
    u16 u2SrcImgHeight;// Source image height
    u32 u4DestBufferPhysAddr0;//image buffer address
    u32 u4DestBufferPhysAddr1;//image buffer address (double and tripple)
    u32 u4DestBufferPhysAddr2;//image buffer address (tripple)
    u16 u2PitchImg0Width;
    u16 u2PitchImg1Width;
    u16 u2PitchImg2Width;
} stIRT1Cfg;

typedef enum {
    MT6516IRT3_OUTFMT_RGB565 = 0,
    MT6516IRT3_OUTFMT_RGB888,
    MT6516IRT3_OUTFMT_ARGB888FromLCD,
    MT6516IRT3_OUTFMT_ARGB888FromIRT3
//    MT6516IRT3_OUTFMT_BUTT = 0xFFFFFFFF
}MT6516IRT3_OUTFMT;

typedef struct{
    u32 bTripleBuffer:1;// 0 : double buffer, 1:triple buffer.
    u32 bAutoRestart:1;// 1 : continous mode.
    u32 bPitch:1;// 1: enable pitch
    u32 bRotate:2;// 0:no rotate , 1:90 , 2:180, 3:270
    u32 bFlip:1;// 1:flip after rotation
    u32 uReserve:26;
    MT6516IRT3_OUTFMT eFmt;
    u8 uAlpha;// Alpha value
    u16 u2SrcImgWidth;// Source image width
    u16 u2SrcImgHeight;// Source image height
    u32 u4DestBufferPhysAddr0;//image buffer address
    u32 u4DestBufferPhysAddr1;//image buffer address (double and tripple)
    u32 u4DestBufferPhysAddr2;//image buffer address (tripple)
    u16 u2PitchImg0Width;
    u16 u2PitchImg1Width;
    u16 u2PitchImg2Width;
} stIRT3Cfg;

typedef enum {
    YUV422 = 0,
    GRAY,
    YUV420,
    YUV411
//    MT6516JPEGENC_INFMT_BUTT = 0xFFFFFFFF
}MT6516JPEGENC_INFMT;

typedef struct{
    MT6516JPEGENC_INFMT eFmt;
    u8 bAutoRestart;// 1 : continous mode.
    u16 u2SrcImgWidth;// Source image width
    u16 u2SrcImgHeight;// Source image height    
} stJPEGDMACfg;

#define MT6516IDP_VDOENCDMA_MAXBUFF_CNT 20
typedef struct{
    u32 bWriteTriggerRead:1;//Write DMA trigger read DMA
    u32 bAutoRestart:1;// 1 : continous mode
//    u32 bInterrupt:1;
//    u32 bReadIntEn:1;
    u32 bRotate:2; // 0:no rotate , 1:90 , 2:180, 3:270
    u32 bFlip:1;// 1:flip after rotation
    u32 bStallRingWhenCatchup : 1;
    u32 uReserved:26;
    u16 u2SrcImgWidth;// Source image width
    u16 u2SrcImgHeight;// Source image height
    u32 u4BufferCnt;//ring buffer count
    unsigned long pu4DestYBuffPAArray[MT6516IDP_VDOENCDMA_MAXBUFF_CNT];
    unsigned long pu4DestUBuffPAArray[MT6516IDP_VDOENCDMA_MAXBUFF_CNT];
    unsigned long pu4DestVBuffPAArray[MT6516IDP_VDOENCDMA_MAXBUFF_CNT];
//    u32 u4DestYBufferPhyAddr1;
//    u32 u4DestUBufferPhyAddr1;
//    u32 u4DestVBufferPhyAddr1;
} stVDOENCDMACfg;

typedef struct{
    u32 bRotate:2; // 0:no rotate , 1:90 , 2:180, 3:270
    u32 bFlip:1;// 1:flip after rotation
    u32 bScanMode:1;// 0: 4x4 block mode, 1 : line scan
    u32 uReserved:28;
    u16 u2SrcImgWidth;// Source image width, must be 16 pixels aligned
    u16 u2SrcImgHeight;// Source image height, must be 16 pixels aligned 
    u32 u4SrcYBufferPhyAddr;// must be 8 bytes aligned.
    u32 u4SrcUBufferPhyAddr;// must be 8 bytes aligned.
    u32 u4SrcVBufferPhyAddr;// must be 8 bytes aligned.
} stVDODECDMACfg;

typedef struct{
    u32 bToCRZ:1;
    u32 bToPRZ:1;
    u32 bToIPP:1;
    u32 bFlip:1;
    u32 bInterrupt:1;
    u32 uRotate:2;// 0 : no rotation, 1:90 , 2:180, 3:270
    u32 uReserved:25;
    u16 u2Width;
    u16 u2Height;
    u32 u4QuantizerAddr;
}stMP4DBLKCfg;

typedef struct stNodeCfg_t {
    eMT6516IDP_NODES eNodeType;
    void * pNodeCfg;
    struct stNodeCfg_t *pNextCfg;
} stNodeCfg;

typedef struct{
    u32 u4Length;
    eMT6516IDP_NODES *pEnableSeq;
} stEnableSeq;

typedef struct{
    unsigned long u4OutBuffNo;
    unsigned long u4VDODMAReadyBuffCount;//Indicates VDODMA ready buffer
    unsigned long u4VDODMADispBuffNo;//Indicates VDODMA display buffer number
    unsigned long u4IRT1BuffNo;//Indicates VDODMA display buffer number
    //Indicates the time interval between current to last frame
    unsigned long pu4TSec[MT6516IDP_VDOENCDMA_MAXBUFF_CNT];
    unsigned long pu4TuSec[MT6516IDP_VDOENCDMA_MAXBUFF_CNT];
} stWaitBuff;

#define LOCK_MODE_MASK 0x000FFFFF
typedef struct{
    unsigned int  u4Mode;
    unsigned int  u4WaitFlag;
    unsigned int* pNowMode;
}stModeParam;

void MT6516_DCT_Reset(void);

//IOCTL commnad
//Lock NODES
#define MTK6516IDP_T_LOCK _IOW(IDPMAGICNO,0,int)
//Unlock NODES
#define MTK6516IDP_T_UNLOCK _IOW(IDPMAGICNO,1,int)

//Config and check(debug mode only) NODES
#define MTK6516IDP_S_CONFIG _IOW(IDPMAGICNO,2, stNodeCfg)

//Enable
#define MTK6516IDP_S_ENABLE _IOW(IDPMAGICNO,3, stEnableSeq)

//Disable path
#define MTK6516IDP_S_DISABLE _IOW(IDPMAGICNO,4, stEnableSeq)

//wait IRQ in IRQ mask and error handling(debug mode only)
#define MT6516IDP_X_WAITIRQ _IOW(IDPMAGICNO,5,int)

//check IRQ in IRQ mask and error handling(debug mode only)
#define MT6516IDP_X_CHECKIRQ _IOR(IDPMAGICNO,6,int)

#define MT6516IDP_T_DUMPREG _IO(IDPMAGICNO,7)

#define MT6516IDP_M_LOCKMODE _IOW(IDPMAGICNO, 8, stModeParam)

#define MT6516IDP_M_UNLOCKMODE _IOW(IDPMAGICNO, 9, unsigned int)

#define MT6516IDP_X_WAITBUFF _IOW(IDPMAGICNO, 10 , stWaitBuff)

#define MTK6516IDP_S_DIRECT_LINK _IOW(IDPMAGICNO, 11, stEnableSeq)

#else
#endif
