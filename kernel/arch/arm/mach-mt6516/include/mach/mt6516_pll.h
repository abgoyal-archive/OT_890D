


#ifndef __MT6516_PLL__
#define __MT6516_PLL__

#include    <mach/mt6516_typedefs.h>
#include    <mach/mt6516_reg_base.h>


#define PDN_CON   	                     (PLL_BASE+0x0010)
#define CLK_CON 	                     (PLL_BASE+0x0014)
#define MPLL 	                         (PLL_BASE+0x0020)
#define MPLL2  	                         (PLL_BASE+0x0024)
#define UPLL                             (PLL_BASE+0x0030)
#define UPLL2                            (PLL_BASE+0x0034)
#define CPLL                             (PLL_BASE+0x0038)
#define CPLL2                            (PLL_BASE+0x003C)
#define TPLL                             (PLL_BASE+0x0040)
#define TPLL2                            (PLL_BASE+0x0044)
#define CPLL3                            (PLL_BASE+0x0048)
#define PLL_RES_CON0                     (PLL_BASE+0x004c)
#define PLL_BIAS                         (PLL_BASE+0x0050)
#define MCPLL                            (PLL_BASE+0x0058)
#define MCPLL2                           (PLL_BASE+0x005C)
#define CEVAPLL                          (PLL_BASE+0x0060)
#define CEVAPLL2                         (PLL_BASE+0x0064)
#define PLL_IDN                          (PLL_BASE+0x0070)
#define XOSC32_AC_CON                    (PLL_BASE+0x007C)
#define MIPITX_CON                       (PLL_BASE+0x0b00)

#define AP_SM_PAUSE_M                   (APSLP_BASE + 0x0200)
#define AP_SM_PAUSE_L                   (APSLP_BASE + 0x0204)
#define AP_SM_CLK_SETTLE                (APSLP_BASE + 0x0208)
#define AP_FINAL_PAUSE_M                (APSLP_BASE + 0x020C)
#define AP_FINAL_PAUSE_L                (APSLP_BASE + 0x0210)
#define AP_SM_CON                       (APSLP_BASE + 0x0218)
#define AP_SM_STA                       (APSLP_BASE + 0x021C)
#define AP_WAKE_PLL_SETTING             (APSLP_BASE + 0x0228)
#define AP_SM_CNF                       (APSLP_BASE + 0x022C)
#define RTCCOUNT_M                      (APSLP_BASE + 0x0230)
#define RTCCOUNT_L                      (APSLP_BASE + 0x0234)
#define WAKE_APLL_SETTING               (APSLP_BASE + 0x0238)

#define PDN_ID_MAX  128

#define CLK_ID_MAX      9
#define CLK_ID_DSP      0
#define CLK_ID_AHB      1
#define CLK_ID_USB      2
#define CLK_ID_IRDA     3
#define CLK_ID_CAM      4
#define CLK_ID_TV       5
#define CLK_ID_CEVA     6
#define CLK_ID_MCARD    7
#define CLK_ID_MIPITX   8

#define MAX_DEVICE		3
#define MAX_MOD_NAME	32

typedef enum MT6516_CLOCK_TAG {
    //PDN and Clock Gating
    MT6516_PDN_PERI_DMA	=	0,
    MT6516_PDN_PERI_USB	=	1,
    MT6516_PDN_PERI_SEJ	=	2,
    MT6516_PDN_PERI_I2C3	=	3,
    MT6516_PDN_PERI_GPT	=	4,
    MT6516_PDN_PERI_KP	    =	5,
    MT6516_PDN_PERI_GPIO	=	6,
    MT6516_PDN_PERI_UART1	=	7,
    MT6516_PDN_PERI_UART2	=	8,
    MT6516_PDN_PERI_UART3	=	9,
    MT6516_PDN_PERI_SIM	=	10,
    MT6516_PDN_PERI_PWM	=	11,
    MT6516_PDN_PERI_PWM1	=	12,
    MT6516_PDN_PERI_PWM2	=	13,
    MT6516_PDN_PERI_PWM3	=	14,
    MT6516_PDN_PERI_MSDC	=	15,
    MT6516_PDN_PERI_SWDBG	=	16,
    MT6516_PDN_PERI_NFI	=	17,
    MT6516_PDN_PERI_I2C2	=	18,
    MT6516_PDN_PERI_IRDA	=	19,
    MT6516_PDN_PERI_I2C	=	21,
    MT6516_PDN_PERI_TOUC	=	21,
    MT6516_PDN_PERI_SIM2	=	22,
    MT6516_PDN_PERI_MSDC2	=	23,
    MT6516_PDN_PERI_ADC	=	26,
    MT6516_PDN_PERI_TP	    =	27,
    MT6516_PDN_PERI_XGPT	=	28,
    MT6516_PDN_PERI_UART4	=	29,
    MT6516_PDN_PERI_MSDC3	=	30,
    MT6516_PDN_PERI_ONEWIRE	=	31,
    MT6516_PDN_PERI_CSDBG	=	32,
    MT6516_PDN_PERI_PWM0	=	33,    

    //GRAPH1SYS Clock Gating        
    MT6516_PDN_MM_GMC1	    =	64,
    MT6516_PDN_MM_G2D	    =	65,
    MT6516_PDN_MM_GCMQ	    =	66,
    MT6516_PDN_MM_BLS	    =	67,
    MT6516_PDN_MM_IMGDMA0	=	68,
    MT6516_PDN_MM_PNG	    =	69,
    MT6516_PDN_MM_DSI	    =	70,
    MT6516_PDN_RSV_71	    =	71,
    MT6516_PDN_MM_TVE	    =	72,
    MT6516_PDN_MM_TVC	    =	73,
    MT6516_PDN_MM_ISP	    =	74,
    MT6516_PDN_MM_IPP	    =	75,
    MT6516_PDN_MM_PRZ	    =	76,
    MT6516_PDN_MM_CRZ	    =	77,
    MT6516_PDN_MM_DRZ	    =	78,
    MT6516_PDN_RSV_79	    =	79,    
    MT6516_PDN_MM_WT	    =	80,
    MT6516_PDN_MM_AFE	    =	81,
    MT6516_PDN_RSV_82	    =	82,    
    MT6516_PDN_MM_SPI	    =	83,
    MT6516_PDN_MM_ASM	    =	84,
    MT6516_PDN_RSV_85	    =	85,   
    MT6516_PDN_MM_RESZLB	=	86,                    
    MT6516_PDN_MM_LCD	    =	87,                    
    MT6516_PDN_MM_DPI	    =	88,                    
    MT6516_PDN_MM_G1FAKE	=	89,       
    
    //GRAPH2SYS Clock Gating    
    MT6516_PDN_MM_GMC2	    =	96,
    MT6516_PDN_MM_IMGDMA1	=	97,
    MT6516_PDN_MM_PRZ2	    =	98,
    MT6516_PDN_MM_M3D	    =	99,
    MT6516_PDN_MM_H264	    =	100,
    MT6516_PDN_MM_DCT	    =	101,
    MT6516_PDN_MM_JPEG	    =	102,
    MT6516_PDN_MM_MP4	    =	103,
    MT6516_PDN_MM_MP4DBLK	=	104,

	//  Sleep Control Register
    MT6516_ID_CEVA		    =	108,
    MT6516_IS_MD			=	109,
    
    MT6516_CLOCK_COUNT_END,
    //Others
    MT6516_PDN0_MCU_START = MT6516_PDN_PERI_DMA,
    MT6516_PDN0_MCU_END = MT6516_PDN_PERI_ONEWIRE,
    MT6516_PDN1_MCU_START = MT6516_PDN_PERI_CSDBG,
    MT6516_PDN1_MCU_END = MT6516_PDN_PERI_PWM0,

    MT6516_GRAPH1SYS_START = MT6516_PDN_MM_GMC1,
    MT6516_GRAPH1SYS_END = MT6516_PDN_MM_G1FAKE,
    MT6516_GRAPH2SYS_START = MT6516_PDN_MM_GMC2,
    MT6516_GRAPH2SYS_END = MT6516_PDN_MM_MP4DBLK,

    //==== 2009/06/10 Shu-Hsin ====//
    MT6516_SLEEP_START = MT6516_ID_CEVA,
    MT6516_SLEEP_END = MT6516_IS_MD,
    //==== 2009/06/10 Shu-Hsin ====// 
           
} MT6516_CLOCK;

typedef enum MT6516_POWER_VOL_TAG 
{
    VOL_DEFAULT,
	VOL_0800,	 
	VOL_0850,	 
	VOL_0900,	 
	VOL_0950,	 
	VOL_1000,	 
	VOL_1050,	 
	VOL_1100,	 
	VOL_1150,	 
	VOL_1200,	 
	VOL_1250,	 		
    VOL_1300,    
	VOL_1350,	 
	VOL_1400,	 
	VOL_1450,	 
    VOL_1500,    
	VOL_1550,	 
    VOL_1800,    
    VOL_2500,    
    VOL_2800, 
    VOL_3000,
    VOL_3300,        
} MT6516_POWER_VOLTAGE;


typedef enum MT6516_POWER_TAG {
    MT6516_POWER_VRF,
    MT6516_POWER_VTCXO,
    MT6516_POWER_V3GTX,
    MT6516_POWER_V3GRX,
    MT6516_POWER_VCAM_A,
    MT6516_POWER_VWIFI3V3,
    MT6516_POWER_VWIFI2V8,
    MT6516_POWER_VSIM,
    MT6516_POWER_VUSB,
    MT6516_POWER_VBT,
    MT6516_POWER_VCAM_D,
    MT6516_POWER_VGP,
    MT6516_POWER_VGP2,
    MT6516_POWER_VSDIO,
    MT6516_POWER_VCORE2,    
    MT6516_POWER_VSIM2,
    MT6516_POWER_COUNT_END,
    MT6516_POWER_NONE = -1
} MT6516_POWER;

typedef enum MT6516_PLL_TAG {
    MT6516_PLL_UPLL,
    MT6516_PLL_DPLL,        
    MT6516_PLL_MPLL,
    MT6516_PLL_CPLL,
    MT6516_PLL_TPLL,    
    MT6516_PLL_CEVAPLL,    
    MT6516_PLL_MCPLL,
    MT6516_PLL_MIPI,    
    MT6516_PLL_COUNT_END,
} MT6516_PLL;

typedef enum MT6516_SUBSYS_TAG {
    MT6516_SUBSYS_GRAPH1SYS = 0x8,
    MT6516_SUBSYS_GRAPH2SYS = 0x10,
    MT6516_SUBSYS_CEVASYS = 0x20,
    MT6516_SUBSYS_COUNT_END,
} MT6516_SUBSYS;

typedef struct { 
    DWORD dwClockCount; 
    DWORD Clockid;
	BOOL bDefault_on;
	char mod_name[MAX_DEVICE][MAX_MOD_NAME];		
} DEVICE_POWERID;

typedef struct { 
    DWORD dwPllCount; 
	BOOL bDefault_on;
	char mod_name[MAX_DEVICE][MAX_MOD_NAME];		
} DEVICE_PLLID;

typedef struct { 
    DWORD dwPowerCount; 
	BOOL bDefault_on;
	char mod_name[MAX_DEVICE][MAX_MOD_NAME];	
} DEVICE_POWER;


typedef struct
{    
    DEVICE_POWERID device[MT6516_CLOCK_COUNT_END];    
    DEVICE_POWER Power[MT6516_POWER_COUNT_END];
    DEVICE_PLLID Pll[MT6516_PLL_COUNT_END];
    DWORD dwSubSystem_status;
    DWORD dwSubSystem_defaultOn;	
    
} ROOTBUS_HW;



extern BOOL hwEnablePLL(MT6516_PLL PllId, char *mode_name);
extern BOOL hwDisablePLL(MT6516_PLL PllId, BOOL bForce, char *mode_name);
extern BOOL hwEnableSubsys(MT6516_SUBSYS subsysId);
extern BOOL hwDisableSubsys(MT6516_SUBSYS subsysId);
extern BOOL hwEnableClock(MT6516_CLOCK clockId, char *mode_name);
extern BOOL hwDisableClock(MT6516_CLOCK clockId, char *mode_name);
extern BOOL hwPowerOn(MT6516_POWER powerId, MT6516_POWER_VOLTAGE powervol, char *mode_name);
extern BOOL hwPowerDown(MT6516_POWER powerId, char *mode_name);


#endif  




