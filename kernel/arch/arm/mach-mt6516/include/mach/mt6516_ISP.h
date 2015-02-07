
#ifndef _MT6516ISP_H
#define _MT6516ISP_H

#include <linux/ioctl.h>

#define ISP_DEV_MAJOR_NUMBER 251
#define ISPMAGIC 'k'
//IOCTRL(inode * ,file * ,cmd ,arg )
//S means "set through a ptr"
//T means "tell by a arg value"
//G means "get by a ptr"
//Q means "get by return a value"
//X means "switch G and S atomically"
//H means "switch T and Q atomically"

//#define

//Structures
//Parallel interface
typedef struct {
    u32 bVsyncPolarity:1; //
    u32 bHsyncPolarity:1;
    u32 uReserved:30;
} stPIF;
//Serial interface
typedef struct {
    u32 bCSI2Header:1;//0 : DI_Byte[7:0],WC_Num[7:0],WC_Num[15:8] ; 1 : WC_Num[15:0], DI_Byte[7:0]
    u32 bECC:1;
    u32 bEnDLane4:1;
    u32 bEnDLane2:1;
    u32 uReserved:28;
} stSIF;

typedef enum{
    MT6516INFMT_Bayer = 0,
    MT6516INFMT_YUV422 = 1,
    MT6516INFMT_RGB565 = 2,
    MT6516INFMT_YCBCR = 5
} eMT6516_InFMT;



typedef enum{
    MT6516OUTFMT_Bayer = 0,
    MT6516OUTFMT_YUV444 = 1,//To CRZ, must choose this
    MT6516OUTFMT_RGB888 = 2,
    MT6516OUTFMT_RGB565 = 3
}eMT656_OutFMT;
typedef struct {
    u32 u4InAddr;
    u32 u4OutAddr;
    u32 u4OutputClkInkHz;//clock rate
    u16 u2HOffsetInPixel;// start from 0
    u16 u2VOffsetInLine;// start from 0
    u16 u2TotalWidthInPixel;//
    u16 u2TotalHeightInLine;//
    u16 u2CropWinXStart;// start from 0
    u16 u2CropWinYStart;// start from 0
    u16 u2CropWidth;
    u16 u2CropHeight;
    u32 u4ColorOrder;
    union
    {
        stSIF stMipiIF;
        stPIF stParallelIF;
    };
    eMT6516_InFMT eInFormat;
    eMT656_OutFMT eOutFormat;
    u32 bSourceIF:1;// 0 : parallel, 1 : serial
    u32 bCapMode:1;//0 : continous mode, 1 : single shot mode
    u32 bSource:1;//0 : Sensor, 1 : Dram
    u32 bDestination:1;// 0 : To CRZ, 1 : To DRAM
//    u16 uEnBinning:1;// 0 : no binning
    u32 bSwapCbCr:1; 
    u32 bSwapY:1;     
    u32 u2CapDelay:3; 
    u32 bMasterClockSwtich:1; 
    u32 u2Reserved:22;
    u8   uDrivingCurrent; 
} stMT6516ISPIFCFG;

//TODO : Ask flare gain, and flare offset usage?
typedef struct {
    u8 uCh00Offset; // 0~-255, for example, 17 : offset is -17
    u8 uCh01Offset;
    u8 uCh10Offset;
    u8 uCh11Offset;
    u8 uRGain;
    u8 uGGain;
    u8 uBGain;
} stMT6516ISPContrastCFG;

typedef struct {
    u8 uCh00Offset;
    u8 uCh01Offset;
    u8 uCh10Offset;
    u8 uCh11Offset;
} stMT6516ISPOBLevelCFG;

#define MT6516ISP_AFWINCNT 8
#define MT6516ISP_AFCFG 5
#define MT6516ISP_AWBWINCNT 13 //Color Edge window + XY win1~12
typedef struct {
    u32 uBottom : 8;
    u32 uTop : 8;
    u32 uRight : 8;
    u32 uLeft : 8;
} stAFWinReg;
typedef union{
    stAFWinReg stAFWin;
    u32 u4RegValue;
}stAFWinCfg;

typedef struct {
    u32 uWinD : 8;
    u32 uWinU : 8;
    u32 uWinR : 8;
    u32 uWinL : 8;
} stAWBSumWinReg;
typedef union{
   stAWBSumWinReg stAWBSumWin;
   u32 u4RegValue;
} stAWBSumWinCfg;

typedef struct {
    u32 uPAXEL_YL : 8; // [00:07] PAXEL_YL
    u32 uPAXEL_RGBH : 8; // [08:15] PAXEL_RGBH
    u32 bAWBDM_DEBUG : 1; // [16:16] AWBSUM_WINR
    u32 uReserved1 : 7; // [17:23] RESERVED
    u32 uSMAREA_NO : 3; // [24:26] SMAREA_NO
    u32 uReserved2 : 1; // [27:27] RESERVED
    u32 bSMAREA_EN : 1; // [28:28] SMAREA_EN
    u32 bCOLOREDGE_EN : 1; // [29:29] COLOREDGE_EN
    u32 bNEUTRAL_EN : 1; // [30:30] NEUTRAL_EN
    u32 bAWB_EN : 1; // [31:31] AWB_EN
} stAWBCtlReg;
typedef union{
    stAWBCtlReg stAWBCtl;
    u32 u4RegValue;
} stAWBCtlCfg;

typedef struct {
    u32 u2NEUTRAL_TH : 12; // [00:11] NEUTRAL_TH
    u32 uReserved: 4;  // [12:15] RESERVED
    u32 uCEDGEY_TH : 8;  // [16:23] CEDGEY_TH
    u32 uCEDGEX_TH : 8;  // [24:31] CEDGEX_TH
} stAWBThresholdReg;
typedef union{
    stAWBThresholdReg stAWBThreshold;
    u32 u4RegValue;
} stAWBThresholdCfg;

typedef struct {
    u8 uAWBH11; // [0:7] AWBH11
    u8 uAWBH12; // [0:7] AWBH12
    u8 uAWBH21; // [0:7] AWBH21
    u8 uAWBH22; // [0:7] AWBH22
    u32 bAWBH11_SIGN : 1; // [8:8] Sign bit of AWBH12: 1 => negative
    u32 bAWBH12_SIGN : 1; // [8:8] Sign bit of AWBH12: 1 => negative
    u32 bAWBH21_SIGN : 1; // [8:8] Sign bit of AWBH12: 1 => negative
    u32 bAWBH22_SIGN : 1; // [8:8] Sign bit of AWBH12: 1 => negative
    u32 uReserved : 28;
}stAWBColorSpaceCfg;

typedef struct {
    u16 u2WinR;
    u16 u2WinL;
    u16 u2WinU;
    u16 u2WinD;
} stAWBWinCfg;

typedef struct {
    u32 bAEHistEn:1;
    u32 bAELuminanceWinEn:1;
    u32 bFlareHistEn:1;
    u32 bAFWinEn:1;
    u32 bAFFilter:1; // 0 : SMD filter, 1 : Tenengrad filter.
    u32 uReserved:27;
    u16 u2AEWinXStart;
    u16 u2AEWinYStart;
    u16 u2AEWinWidth;
    u16 u2AEWinHeight;
    u16 u2HistWinXStart;// must be 16 pixel aligned
    u16 u2HistWinYStart;// must be 8 lines aligned
    u16 u2HistWinWidth;// must be 16 pixel aligned
    u16 u2HistWinHeight;// must be 8 lines aligned
    stAFWinCfg stAFWin[MT6516ISP_AFWINCNT];
    u8 uAFThresholdCFG[MT6516ISP_AFCFG];
    stAWBSumWinCfg stAWBSumWin;
    stAWBCtlCfg stAWBCtl;
    stAWBThresholdCfg stAWBThreshold;
    stAWBColorSpaceCfg stAWBColorSpace;
    stAWBWinCfg stAWBWin[MT6516ISP_AWBWINCNT];
} stMT6516ISP3AStatisticCFG;

typedef struct {
    u16 u2RawPreRgain;// 2.7
    u16 u2RawPreGrgain;// 2.7
    u16 u2RawPreGbgain;// 2.7
    u16 u2RawPreBgain;// 2.7
}stMT6516ISPRawGainCFG;
#if 0
//...0x1000~0x1054 -- 22x4
//...0x1060~0x1324 -- 178x4(bytes)
#define MT6516ISP_3ASTATREG_CNT 200
typedef struct {
    u32 u4Result[MT6516ISP_3ASTATREG_CNT];
} stMT6516ISP3AStat;
#else
//AE statisitc result
#define AE_WIN_STAT_CNT 20
#define AE_WIN_INFO_CNT 4
#define FLARE_HISTBIN_CNT 10
#define AE_HIST_RES_NUM 64
typedef struct{
    u32 u4AEWinStat[AE_WIN_STAT_CNT];//1000h~104Ch
    u32 u4AEWinInfo[AE_WIN_INFO_CNT];    //1050h~105Ch
    u32 u4FlareHist[FLARE_HISTBIN_CNT];     //1060h~1084h
} MT6516_AESTAT_T;

typedef struct{
    struct {
        u32 u4FV[MT6516ISP_AFCFG];
    } sWin[MT6516ISP_AFWINCNT];
    u32 u4Mean[MT6516ISP_AFWINCNT];
} MT6516_AFSTAT_T;

typedef struct{
    // CAM + 1148h~1204h: AWB XY Window Result (1-12) (Paxel Count, Rsum, Gsum, Bsum)
    struct CAM_1148H
    {
        u32 PCNT;
        u32 RSUM;
        u32 GSUM;
        u32 BSUM;
    } AWBXY_RESULT[12];	// CAM + 1208h~1214h: AWB Sum Window Result (Paxel Count, Rsum, Gsum, Bsum)
    struct CAM_1208H
    {
        u32 PCNT;
        u32 RSUM;
        u32 GSUM;
        u32 BSUM;
    } AWBSUM_RESULT;    // CAM + 1218h~1224h: AWB Color Edge Window Result (Paxel Count, Rsum, Gsum, Bsum)
    struct CAM_1218H
    {
        u32 PCNT;
        u32 RSUM;
        u32 GSUM;
        u32 BSUM;
    } AWBCE_RESULT;
} MT6516_AWBSTAT_T;

typedef struct{
    u32 u4AEHistRes[AE_HIST_RES_NUM];    //1228h~1324h
}MT6516_HIST_T;

typedef struct {
    // AE statistics (part 1): 0x1000 ~ 0x1084
    MT6516_AESTAT_T rAEWinStat;
    // AF statistics: 0x1088 ~ 0x1144
    MT6516_AFSTAT_T rAFStat;
    // AWB statistics: 0x1148 ~ 0x1224
    MT6516_AWBSTAT_T rAWBStat;
    // AE statistics (part 2): 0x1228 ~ 0x1324
    MT6516_HIST_T rAEHist;
} stMT6516ISP3AStat;

#endif

typedef struct {
    u8 uDefectCorrectEn;
    u32 u4DefectTblAddr;
} stMT6516ISPBadPixelCFG;

//CAM_SHADING2
//SD_RADDR
//SD_LBLOCK
//SD_RATIO
#define MT6516ISP_SHADINGCOMP_PARAMCNT 4
typedef struct {
    u32 bShadingBlockTriggger:1;
    u32 bShadingCompEnable:1;
    u32 bRawAccuWinEn:1;
    u32 uReserved:29;
    u32 u4RawAccuWinDimension;// RAWWIN_REG
    u32 uShadingPamrams[MT6516ISP_SHADINGCOMP_PARAMCNT];
} stMT6516ISPVignettingCFG;

//NR2_CFG2~4
//NR1_CON
//NR1_DP1~4
//NR1_CT
//NR1_NR1~10
#define MT6516ISP_NR_PARAMCNT 19
typedef struct {
    u32 bYNREn:1;
    u32 bUVNREn:1;
    u32 uReserved:30;
    u32 u4NRParam[MT6516ISP_NR_PARAMCNT];
} stMT6516ISPNoiseFilterCFG;

//Pre gain will be loaded in initialization.
typedef struct {
    u16 u2RawPreGain;// 2.7 pregain in CAM_CTRL1
    u16 u2RGain; // 2.7
    u16 u2GrGain; // 2.7
    u16 u2GbGain; // 2.7
    u16 u2BGain; // 2.7
} stMT6516ISPColorGainCFG;

//CAM_CPSCON1
//CAM_INTER1
//CAM_INTER2
#define MT6516ISP_DEMOSAIC_PARAMCNT 3// 3(registers)
typedef struct {
    u32 u4Params[MT6516ISP_DEMOSAIC_PARAMCNT];
} stMT6516ISPInterpolationCFG;

//M11 M12 M13 M21 M22 M22 M23 M31 M32 M33
#define CCM_COEFF_CNT 9
typedef struct {
    u8 uCCMParam[CCM_COEFF_CNT];
    u8 uYGain;
    u8 uYOffset;
    u8 uVGain;
    u8 uUGain;
} stMT6516ISPColorCFG;

//CAM_GMA_REG1~5
#define MT6516ISP_GAMMA_PARAMCNT 5// 5(registers)
typedef struct {
    u32 u4Params[MT6516ISP_GAMMA_PARAMCNT];
} stMT6516ISPGammaCFG;

//CAM_EDGCORE
//CAM_EDGGAIN1
//CAM_EDGGAIN2
//CAM_EDGTHRE
//CAM_EDGVCON
//CAM_CPSCON2[Y_EGAIN]
#define MT6516ISP_EDGEENHANCE_PARAMCNT 9
typedef struct {
    u32 u4Params[MT6516ISP_EDGEENHANCE_PARAMCNT];
} stMT6516ISPEdgeEnhanceCFG;


typedef struct {
    u32 bGlobalShutterEn:1;
    u32 bGlobalShutterPolarity:1;
    u32 bMshutterEn:1;
//    u8 bContinousMode:1;//TBD
    u32 uReserved:29;
    u32 u4ResetTimeInus;
    u32 u4ExposureTimeInus;
    u32 u4MshutterLagInus;
    u32 u4MshutterPin0SWTiming[4];
    u32 u4MshutterPin1SWTiming[4];
} stMT6516ISPMshutCFG;

typedef struct {
    u8 YCC_ENC3;
    u8 YCC_ENC2;
    u8 YCC_ENC1;
    u8 YCC_ENY3;
    u8 YCC_ENY2;
    u8 YCC_ENY1;
    u8 YCC_H12;
    u8 YCC_H11;
    u8 YCC_MV;
    u8 YCC_MU;
    u8 YCC_Y4;
    u8 YCC_Y3;
    u8 YCC_Y2;
    u8 YCC_Y1;
    u8 YCC_G4;
    u8 YCC_G3;
    u8 YCC_G2;
    u8 YCC_G1;
    u8 YCC_OFSTV;
    u8 YCC_OFSTU;
    u8 YCC_OFSTY;
    u8 YCC_G5;
    u8 YCC_GAINY;
    u8 YCC_YBNDL;
    u8 YCC_YBNDH;
    u8 YCC_VBNDL;
    u8 YCC_VBNDH;
    u8 YCC_UBNDL;
    u8 YCC_UBNDH;
} stMT6516ISPYCCGOCFG;

#define MT6516ISP_MAX_REGIO_CNT 10
typedef struct {
    u32 pAddr[MT6516ISP_MAX_REGIO_CNT];
    u32 pVal[MT6516ISP_MAX_REGIO_CNT];
    u32 count;;
} stMT6516ISPRegIO;

//TBD
typedef struct {
} stMT6516ISPFlashCFG;



#define MT6516ISP_EXPDONEIRQ 0x1
#define MT6516ISP_REZOVERRUNIRQ 0x2
#define MT6516ISP_GMCOVERRUNIRQ 0x4
#define MT6516ISP_IDLEIRQ 0x8
#define MT6516ISP_ISPDONEIRQ 0x10
#define MT6516ISP_AEDONEIRQ 0x20
#define MT6516ISP_FLASHIRQ 0x80
#define MT6516ISP_AVSYNCRQ 0x100
typedef struct {
    u16 u2AVSyncLine;// 0 ~ 4095
    u16 u2IRQEnMask;
} stMT6516ISPIRQCfg;

#define MT6516ISP_LCE_PARAMCNT 8
typedef struct {
    u32 u4Params[MT6516ISP_LCE_PARAMCNT];
} stMT6516ISPLCECFG;

#define MT6516ISP_PreShadingTableSize_CNT 3072
#define MT6516ISP_CapShadingTableSize_CNT 3584
#define MT6516ISP_MaxShadingTableSize_CNT 3584
typedef struct {
    u32 u4SRAMOffset;
    u32 u4BufferLength;
    u8   u1Params[MT6516ISP_MaxShadingTableSize_CNT];
} stMT6516ISPKernelBufferCFG;

//Control commnad

//S means "set through a ptr"
//T means "tell by a arg value"
//G means "get by a ptr"
//Q means "get by return a value"
//X means "switch G and S atomically"
//H means "switch T and Q atomically"

//MTKISPIOC_X_IOIF : Set ISP source interface, sync signal, dimension, format, oneshot/continuous operation
//no tile mode on 6516 yet.
//aligned dimension will be updated to stMT16516ISPInfo and return.
#define MT6516ISPIOC_S_IOIF _IOW(ISPMAGIC,0,stMT6516ISPIFCFG)
//MTKISPIOC_G_IOIF : Get current ISP source IF, sync signal, dimension, format, oneshot/continuous operation
//#define MT6516ISPIOC_G_IOIF _IOR(ISPMAGIC,1,stMT6516ISPIFCFG)

//wait IRQ in IRQ mask
//This command will block by putting current thread to sleep mode,
//till one of the hardware interrupt defined with input mask comes.
#define MT6516ISPIOC_G_WAITIRQ _IOR(ISPMAGIC,2,u32)

#define MT6516ISPIOC_G_CHECKIRQ _IOR(ISPMAGIC,3,u32)

//Set MT6516 Vsync IRQ, unit is hsync line time.
#define MT6516ISPIOC_S_SETIRQ _IOW(ISPMAGIC,4,stMT6516ISPIRQCfg)

//MT6516ISPIOC_T_RUN : Tell ISP to run/stop
#define MT6516ISPIOC_T_RUN _IOW(ISPMAGIC,5,u32)
//MT6516ISPIOC_Q_RUN : Query ISP is running or stopped.
#define MT6516ISPIOC_Q_RUN _IOR(ISPMAGIC,6,u32)

//MT6516ISPIOC_S_3A_STATISTICS_CFG : Configure MT6516 ISP 3A statistic windows.
#define MT6516ISPIOC_S_3A_STATISTICS_CFG _IOW(ISPMAGIC,7,stMT6516ISP3AStatisticCFG)
//MT6516ISPIOC_G_3A_STATISTICS_CFG : Get current MT6516 ISP 3A statistic window configuration,
//!!!not statistic value.
//#define MT6516ISPIOC_G_3A_STATISTICS_CFG _IOR(ISPMAGIC,8,stMT6516ISP3AStatisticCFG)
//Call this func to update last statistic window, blocking
#define MT6516ISPIOC_T_3A_UpdatesSTATISTICS _IOR(ISPMAGIC,9,stMT6516ISP3AStat)

//MT6516ISPIOC_S_BADPIXEL_CFG : Configure MT6516 ISP bad pixel compensation.
#define MT6516ISPIOC_S_BADPIXEL_CFG _IOW(ISPMAGIC,10,stMT6516ISPBadPixelCFG)
//MT6516ISPIOC_G_BADPIXEL_CFG : Get current MT6516 ISP bad pixel compensation configuration.
//#define MT6516ISPIOC_G_BADPIXEL_CFG _IOR(ISPMAGIC,11,stMT6516ISPBadPixelCFG)

//MT6516ISPIOC_S_CONTRAST : Configure MT6516 ISP contrast
#define MT6516ISPIOC_S_CONTRAST _IOW(ISPMAGIC,12,stMT6516ISPContrastCFG)
//MT6516ISPIOC_G_OB : Get current MT6516 ISP contrast configuration.
//#define MT6516ISPIOC_G_CONTRAST _IOR(ISPMAGIC,13,stMT6516ISPContrastCFG)

//MT6516ISPIOC_S_VIGNETTING_CFG : Configure MT6516 ISP vignetting compensation.
#define MT6516ISPIOC_S_VIGNETTING_CFG _IOW(ISPMAGIC,14,stMT6516ISPVignettingCFG)
//MT6516ISPIOC_G_VIGNETTING_CFG : Get current MT6516 ISP vignetting compensation configuration.
//#define MT6516ISPIOC_G_VIGNETTING_CFG _IOR(ISPMAGIC,15,stMT6516ISPVignettingCFG)

//MT6516ISPIOC_S_COLORGAIN_CFG : Configure MT6516 ISP color gain.
#define MT6516ISPIOC_S_COLORGAIN_CFG _IOW(ISPMAGIC,16,stMT6516ISPColorGainCFG)
//MT6516ISPIOC_G_COLORGAIN_CFG : Get current MT6516 ISP color gain setting.
//#define MT6516ISPIOC_G_COLORGAIN_CFG _IOR(ISPMAGIC,17,stMT6516ISPColorGainCFG)

//MT6516ISPIOC_S_INTERPOLATION_CFG : Configure MT6516 ISP interpolation.
#define MT6516ISPIOC_S_INTERPOLATION_CFG _IOW(ISPMAGIC,18,stMT6516ISPInterpolationCFG)
//MT6516ISPIOC_G_INTERPOLATION_CFG : Get current MT6516 ISP interpolation configuration.
//#define MT6516ISPIOC_G_INTERPOLATION_CFG _IOR(ISPMAGIC,19,int)

//MT6516ISPIOC_S_NoiseFilter_CFG : Configure MT6516 ISP noise filter.
#define MT6516ISPIOC_S_NOISEFILTER_CFG _IOW(ISPMAGIC,20,stMT6516ISPNoiseFilterCFG)
//MT6516ISPIOC_G_NoiseFilter_CFG : Get current MT6516 ISP noise filter configuration.
//#define MT6516ISPIOC_G_NOISEFILTER_CFG _IOR(ISPMAGIC,21,stMT6516ISPNoiseFilterCFG)

//MT6516ISPIOC_S_COLORPROCESS_CFG : Configure MT6516 ISP color process.
#define MT6516ISPIOC_S_COLORPROCESS_CFG _IOW(ISPMAGIC,22,stMT6516ISPColorCFG)
//MT6516ISPIOC_G_COLORPROCESS_CFG : Get current MT6516 ISP color configuration.
//#define MT6516ISPIOC_G_COLORPROCESS_CFG _IOR(ISPMAGIC,23,stMT6516ISPColorCFG)

//MT6516ISPIOC_S_GAMMA_CFG : Configure MT6516 ISP gamma curve.
#define MT6516ISPIOC_S_GAMMA_CFG _IOW(ISPMAGIC,24,stMT6516ISPGammaCFG)
//MT6516ISPIOC_G_GAMMA_CFG : Get current MT6516 ISP gamma curve.
//#define MT6516ISPIOC_G_GAMMA_CFG _IOR(ISPMAGIC,25,int)

//MT6516ISPIOC_S_EE_CFG : Configure MT6516 ISP edge enhancement.
#define MT6516ISPIOC_S_EDGEENHANCE_CFG _IOW(ISPMAGIC,26,stMT6516ISPEdgeEnhanceCFG)
//MT6516ISPIOC_G_EE_CFG : Get current MT6516 ISP edge enhancement configuration,
//#define MT6516ISPIOC_G_EDGEENHANCE_CFG _IOR(ISPMAGIC,27,int)

//MT6516ISPIOC_S_LCE_CFG : Configure MT6516 ISP LCE configuration.
#define MT6516ISPIOC_S_LCE_CFG _IOW(ISPMAGIC,28,stMT6516ISPLCECFG)
//MT6516ISPIOC_G_LCE_CFG : Get current MT6516 ISP LCE configuration,
//#define MT6516ISPIOC_G_LCE_CFG _IOR(ISPMAGIC,29,MT6516ISPLCECFG)

//MT6516ISPIOC_S_MSHUTTER_CFG : Configure MT6516 ISP mechanical shutter control.
#define MT6516ISPIOC_S_MSHUT_CFG _IOW(ISPMAGIC,30,stMT6516ISPMshutCFG)
//MT6516ISPIOC_G_MSHUTTER_CFG : Get current MT6516 ISP mechanical shutter control configuration,
//#define MT6516ISPIOC_G_MSHUT_CFG _IOR(ISPMAGIC,31,stMT6516ISPMshutCFG)

//MT6516ISPIOC_S_FLASH_CFG : Configure MT6516 ISP flash control.
#define MT6516ISPIOC_S_FLASH_CFG _IOW(ISPMAGIC,32,stMT6516ISPFlashCFG)
//MT6516ISPIOC_G_FLASH_CFG : Get current MT6516 ISP flash control configuration,
//#define MT6516ISPIOC_G_FLASH_CFG _IOR(ISPMAGIC,33,stMT6516ISPFlashCFG)

//To ensure all image related setting are applied in the same frame.
//Bad pixel, Contrast, Vignetting, noise filter, edge enhancement, interpolation, color gain, color process.
#define MT6516ISPIOC_T_APPLY_CFG _IO(ISPMAGIC,34)

//Dump ISP registers , for debug usage
#define MT6516ISPIOC_T_DUMPREG _IO(ISPMAGIC,35)

#define MT6516ISPIOC_S_RAWGAIN _IOW(ISPMAGIC,36,stMT6516ISPRawGainCFG)

//Set OB Level
#define MT6516ISPIOC_S_OBLEVEL _IOW(ISPMAGIC, 38, stMT6516ISPOBLevelCFG)

//set single isp register
#define MT6516ISPIOC_S_REGISTER _IOW(ISPMAGIC, 40,stMT6516ISPRegIO)

//get single isp register
#define MT6516ISPIOC_G_REGISTER _IOR(ISPMAGIC, 42,stMT6516ISPRegIO)

//MT6516ISPIOC_S_YCCGO : Configure MT6516 ISP YCCGO
#define MT6516ISPIOC_S_YCCGO _IOW(ISPMAGIC,44,stMT6516ISPYCCGOCFG)
//MT6516ISPIOC_G_YCCGO : Get current MT6516 ISP YCCGO configuration.
//#define MT6516ISPIOC_G_YCCGO _IOR(ISPMAGIC,45,stMT6516ISPYCCGOCFG)

#define MT6516ISPIOC_S_KernelBuffer_CFG _IOW(ISPMAGIC,46,stMT6516ISPKernelBufferCFG)

//include storbe IF
ssize_t strobe_StillExpCfgStart(void);
ssize_t strobe_StillExpEndIrqCbf(void);

#else
#endif
