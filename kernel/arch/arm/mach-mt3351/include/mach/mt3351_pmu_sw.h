


//Reference from MT3351 WinCE PMU driver
//please see mt3351_pmu_regs.h

#include    <mach/mt3351_pmu_hw.h>

#ifndef _PMU_SW_H
#define _PMU_SW_H

#define  CHR_TICKLE                     0x1000
#define  CHR_PRE                          0x1001
#define  CHR_CC                             0x1002 
#define  CHR_CV                             0x1003 
#define  CHR_BATFULL                      0x1004 
#define  CHR_ERROR                        0x1005 

#define  PMU_CHARGEOFF                   0  
#define  PMU_CHARGEON                    1   

#define CHARGER_OVER_VOL    1
#define BATTERY_UNDER_VOL       2
#define BATTERY_OVER_TEMP       3

#define OV_431V     0
#define OV_441V     1


#define FULL_BATTERY_TICKLE_CURRENT      50  
typedef struct 
{
	BOOL	bat_exist;
    BOOL	bat_full;  
    BOOL	bat_low;  
    UINT16       bat_charging_state;
	UINT16		bat_vol;			//BATSENSE
    BOOL	charger_exist;   
	UINT16		pre_charging_current;
	UINT16		charging_current;
	UINT16		charger_vol;		//CHARIN
	UINT8		charger_protect_status; 
	UINT16		ISENSE;				//ISENSE
	UINT16		ICharging;
	INT16		temperature;
    UINT32      total_charging_time;
    UINT32      CV_charging_time;
    UINT32      PRE_charging_time;
    UINT32      charger_type ;
    UINT32      PWR_SRC;
    //CEDEVICE_POWER_STATE Bat_pwr_state ;
    
} PMU_ChargerStruct;

typedef enum 
{
	PMU_STATUS_OK = 0,
	PMU_STATUS_FAIL = 1,
}PMU_STATUS;


// 20090201 Shu-Hsin Add Begin

#define OFFSET_IN_CLOSED_RANGE(x, start, end) \
    ((x) >= (start) && (x) <= (end)) ? ((x) - (start)) :

#define OFFSET_IN_CLOSED_RANGE_END (0xFFFFFFFF)

#define MT3351_CLOCK_CON_BIT_OFFSET(c)                                          \
    (OFFSET_IN_CLOSED_RANGE(c, MT3351_CLOCK_CONA_START, MT3351_CLOCK_CONA_END)  \
     OFFSET_IN_CLOSED_RANGE(c, MT3351_CLOCK_CONB_START, MT3351_CLOCK_CONB_END)  \
     OFFSET_IN_CLOSED_RANGE_END)

#define MT3351_CLOCK_CON_BIT_MASK(c)  (1 << MT3351_CLOCK_CON_BIT_OFFSET(c))

#define MT3351_CLOCK_GMC_BIT_MASK   \
    (MT3351_CLOCK_CON_BIT_MASK(MT3351_CLOCK_GMC))

typedef enum MT3351_CLOCK_TAG {
    //CONA
    MT3351_CLOCK_DMA,   
    MT3351_CLOCK_USB, 
    MT3351_CLOCK_RESERVED_0,    
    MT3351_CLOCK_SPI,
    MT3351_CLOCK_GPT,
    MT3351_CLOCK_UART0,
    MT3351_CLOCK_UART1,
    MT3351_CLOCK_UART2,
    MT3351_CLOCK_UART3,
    MT3351_CLOCK_UART4,
    MT3351_CLOCK_PWM ,
    MT3351_CLOCK_PWM0,
    MT3351_CLOCK_PWM1,
    MT3351_CLOCK_PWM2,
    MT3351_CLOCK_MSDC0,
    MT3351_CLOCK_MSDC1,
    MT3351_CLOCK_MSDC2,
    MT3351_CLOCK_NFI,
    MT3351_CLOCK_IRDA,
    MT3351_CLOCK_I2C,
    MT3351_CLOCK_AUXADC,
    MT3351_CLOCK_TOUCHPAD,
    MT3351_CLOCK_SYSROM,
    MT3351_CLOCK_KEYPAD,

    //CONB
    MT3351_CLOCK_GMC,
    MT3351_CLOCK_G2D,
    MT3351_CLOCK_GCMQ,
    MT3351_CLOCK_DDE,
    MT3351_CLOCK_IMAGE_DMA ,
    MT3351_CLOCK_IMAGE_PROCESSOR,
    MT3351_CLOCK_JPEG,           
    MT3351_CLOCK_DCT,            
    MT3351_CLOCK_ISP,            
    MT3351_CLOCK_PRZ,
    MT3351_CLOCK_CRZ,
    MT3351_CLOCK_DRZ,
    MT3351_CLOCK_MTVSPI,
    MT3351_CLOCK_ASM,
    MT3351_CLOCK_I2S,
    MT3351_CLOCK_RESIZE_LB,
    MT3351_CLOCK_LCD,
    MT3351_CLOCK_DPI,
    MT3351_CLOCK_VFE,
    MT3351_CLOCK_AFE,
    MT3351_CLOCK_BLS,

    // Other
    MT3351_CLOCK_DSP,
    MT3351_CLOCK_MSDC_UPLL, //MSDC CLOCK SOURCE CAN BE UPLL
    MT3351_CLOCK_MSDC_MM,   //MSDC CLOCK SOURCE CAN BE MM

    MT3351_CLOCKS_COUNT,
    MT3351_CLOCK_NONE = -1,
    MT3351_CLOCK_CONA_START = MT3351_CLOCK_DMA,
    MT3351_CLOCK_CONA_END = MT3351_CLOCK_KEYPAD,
    MT3351_CLOCK_CONB_START = MT3351_CLOCK_GMC,
    MT3351_CLOCK_CONB_END = MT3351_CLOCK_BLS
} MT3351_CLOCK;

typedef enum MT3351_POWER_TAG {
    MT3351_POWER_USB,
    MT3351_POWER_GP, 
    MT3351_POWER_BT,
    MT3351_POWER_GPS,
    MT3351_POWER_LCD,
    MT3351_POWER_CAMA,
    MT3351_POWER_CAMD,
    MT3351_POWER_SDIO,
    MT3351_POWER_MEMORY,
    MT3351_POWER_TCXO,
    MT3351_POWER_RTC,
    MT3351_POWER_CORE1,
    MT3351_POWER_CORE2,
    MT3351_POWER_ANALOG,
    MT3351_POWER_IO,

    MT3351_POWERS_COUNT,
    MT3351_POWER_NONE = -1
} MT3351_POWER;

typedef enum MT3351_POWER_VOL_TAG {
    VOL_DEFAULT,
    VOL_1300,    
    VOL_1500,    
    VOL_1800,    
    VOL_2500,    
    VOL_2800,    
    VOL_3300,        
} MT3351_POWER_VOLTAGE;

typedef struct
{
    //MT3351_CONFIG_REGS *pConfigRegs;
    //MT3351_PLL_REGS    *pPllRegs;
    //MT3351_PMU_REGS    *pPmuRegs;    
    //MT3351_RGU_REGS    *pRguRegs;
    
    BYTE*  pBusStatus;
    DWORD dwPowerCount[MT3351_POWERS_COUNT];
    DWORD dwClockCount[MT3351_CLOCKS_COUNT];
    DWORD dwUpllCount;
    DWORD dwMMCount;
    DWORD dwLpllCount;       

} ROOTBUS_HW;

// PDN_CONA
typedef enum
{
    PDN_CONA_DMA        = 1 << 0,
    PDN_CONA_USB        = 1 << 1,
    PDN_CONA_RESERVED0  = 1 << 2,
    PDN_CONA_SPI        = 1 << 3,
    PDN_CONA_GPT        = 1 << 4,
    PDN_CONA_UART0      = 1 << 5,
    PDN_CONA_UART1      = 1 << 6,
    PDN_CONA_UART2      = 1 << 7,
    PDN_CONA_UART3      = 1 << 8,
    PDN_CONA_UART4      = 1 << 9,
    PDN_CONA_PWM        = 1 << 10,
    PDN_CONA_PWM0CLK    = 1 << 11,
    PDN_CONA_PWM1CLK    = 1 << 12,
    PDN_CONA_PWM2CLK    = 1 << 13,
    PDN_CONA_MSDC0      = 1 << 14,
    PDN_CONA_MSDC1      = 1 << 15,
    PDN_CONA_MSDC2      = 1 << 16,
    PDN_CONA_NFI        = 1 << 17,
    PDN_CONA_IRDA       = 1 << 18,
    PDN_CONA_I2C        = 1 << 19,
    PDN_CONA_AUXADC     = 1 << 20,
    PDN_CONA_TOUCH      = 1 << 21,
    PDN_CONA_SYSROM     = 1 << 22,
    PDN_CONA_KEYPAD     = 1 << 23
} PDN_CONA_MODE;

// PDN_CONB
typedef enum
{
    PDN_CONB_GMC              = 1 << 0,
    PDN_CONB_G2D              = 1 << 1,
    PDN_CONB_GCMQ             = 1 << 2,
    PDN_CONB_RESERVED0        = 1 << 3,
    PDN_CONB_IMAGEDMA         = 1 << 4,
    PDN_CONB_IMAGEPROCESSOR   = 1 << 5,
    PDN_CONB_JPEG             = 1 << 6,
    PDN_CONB_DCT              = 1 << 7,
    PDN_CONB_ISP              = 1 << 8,
    PDN_CONB_PRZ              = 1 << 9,
    PDN_CONB_CRZ              = 1 << 10,
    PDN_CONB_DRZ              = 1 << 11,
    PDN_CONB_SPI              = 1 << 12,
    PDN_CONB_ASM              = 1 << 13,
    PDN_CONB_I2S              = 1 << 14,
    PDN_CONB_RESIZE_LB        = 1 << 15,
    PDN_CONB_LCD              = 1 << 16,
    PDN_CONB_DPI              = 1 << 17,
    PDN_CONB_VFE              = 1 << 18,
    PDN_CONB_AFE              = 1 << 19,
    PDN_CONB_BLS              = 1 << 20
} PDN_CONB_MODE;

//CGCON
typedef enum
{
    DCM_EN = 0x2,
    DSP_OFF = 0x4,
    MM_OFF = 0x8,
    EMI2X_OFF = 0x10,
    UPLL_48M_OFF = 0x80
} CGCON_MODE;
    
// 20090201 Shu-Hsin Add End

/*Function*/
PMU_STATUS PMU_EnableOSCBagBlk(BOOL bEnable);
PMU_STATUS PMU_SetOVThreshHold( UINT32 u4OV );
PMU_STATUS PMU_SetUVThreshHold( RG_UV_SEL eUVSel);
PMU_STATUS PMU_SetThermal( RG_THR_SEL eTHRSel);
PMU_STATUS PMU_GetPMURtcTmpStatus(void);
PMU_STATUS PMU_Enable32KProtect( BOOL bEnable);
PMU_STATUS PMU_EnableTempProtect( BOOL bEnable);
PMU_STATUS PMU_SetHiLoTemperature( UINT32 u4HiVal ,UINT32 u4LoVal);
PMU_STATUS PMU_SetHiLoTemperature( UINT32 u4HiVal ,UINT32 u4LoVal);
PMU_STATUS PMU_SetConstantCurrentMode( CC_MODE_CHAGE_LEVEL cc_cuerrent);
PMU_STATUS PMU_SetPreConstantCurrentMode( RG_CAL_PRECC pre_cc_cuerrent);
PMU_STATUS PMU_SwitchPowerSource( RG_PS_SOURCE power_select);
PMU_STATUS PMU_EnablePreConstantCurrentMode( RG_CAL_PRECC PreCC_Current);
PMU_STATUS PMU_EnableConstantCurrentMode( CC_MODE_CHAGE_LEVEL CC_Current);
PMU_STATUS PMU_EnableConstantVoltageMode(void);
UINT32 PMU_GetChargerStatus(void);
PMU_STATUS PMU_SetVUSBLDO( BOOL enable);
PMU_STATUS PMU_SetVCAMEDLDO( BOOL enable, RG_VCAMD_SEL VCAMD_VOL);
PMU_STATUS PMU_SetVTCXOLDO( BOOL enable);
PMU_STATUS PMU_SetVBTLDO( BOOL enable);
PMU_STATUS PMU_SetVCAMEALDO( BOOL enable , RG_VCAMA_SEL VCAMA_VOL);
PMU_STATUS PMU_SetVGPSLDO( BOOL enable);
PMU_STATUS PMU_SetVGPLDO( BOOL enable, RG_VGP_SEL RG_VGP_vol);
PMU_STATUS PMU_SetVSDIOLDO( BOOL enable,  RG_VSDIO_SEL RG_VSDIO_vol);
PMU_STATUS PMU_SetFixPWMMode( BOOL bFixPWM  );
void PMU_DumpBusPowerState(void);

BOOL hwEnableClock(MT3351_CLOCK clockId);
BOOL hwDisableClock(MT3351_CLOCK clockId);
BOOL hwPowerOn(MT3351_POWER powerId, MT3351_POWER_VOLTAGE powervol);
BOOL hwPowerDown(MT3351_POWER powerId);

void MT3351_pmu_init(void);

#endif  /*_PMU_SW_H*/


