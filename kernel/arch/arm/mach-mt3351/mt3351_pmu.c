
#include    <mach/mt3351_typedefs.h>
#include    <mach/mt3351_reg_base.h>
#include    <mach/mt3351_pmu_hw.h>
#include    <mach/mt3351_pmu_sw.h>
#include    <mach/mt3351_pdn_hw.h>
#include    <mach/mt3351_pdn_sw.h>
#include    <mach/mt3351_sleep.h>
#include    <mach/mt3351_dvfs.h>
#include    <mach/irqs.h>
#include    <linux/spinlock.h>
#include    <linux/suspend.h>
#include    <mach/mt3351_wdt.h>
#include    <asm/io.h>
#include    <linux/delay.h>
#include    <linux/pm.h>
#include    <mach/mt3351_battery.h>

#define MAX_WAKEUP_SOURCE_INTR   10
#define PMU_CALIBRATION_MASK     0x00000010
#define PMU_CALIBRATION_OFFSET   4

UINT32 g_oalIrq2Wake[MAX_WAKEUP_SOURCE_INTR][2] =
{
  /* IRQ, Wakeup Source*/
  {MT3351_SLEEPCTRL_IRQ_CODE,      WAKE_INT_EN},   // SLEEPCTRL
  {MT3351_KPAD_IRQ_CODE,           WAKE_KP},       // KP
  {MT3351_EIT_IRQ_CODE,            WAKE_EINT},     // EXT_INT
  {MT3351_RTC_IRQ_CODE,            WAKE_RTC},      // RTC
  {MT3351_MSDC_EVENT_IRQ_CODE,     WAKE_MSDC},     // MSDC0_EVENT
  {MT3351_MSDC2_EVENT_IRQ_CODE,     WAKE_MSDC},     // MSDC1_EVENT
  {MT3351_MSDC3_EVENT_IRQ_CODE,     WAKE_MSDC},     // MSDC2_EVENT
  {MT3351_GPT_IRQ_CODE,            WAKE_TMR},      // GPT
  {MT3351_TS_IRQ_CODE,             WAKE_TP},       // TOUCH_SCREEN
  {MT3351_LOW_BAT_IRQ_CODE,        WAKE_LOWBAT},   // LOW_BAT

};

BOOL g_pmu_cali = FALSE ;
PMU_ChargerStruct BMT_status;
UINT32 g_HW_Ver;
UINT32 g_FW_Ver;
UINT32 g_HW_Code;  
UINT32 g_oalWakeSource = WAKE_REASON_UNKNOWN;

static ROOTBUS_HW g_BusHW = {0};
static spinlock_t csClock_lock = SPIN_LOCK_UNLOCKED;
static spinlock_t csPower_lock = SPIN_LOCK_UNLOCKED;

extern void PMU_CPUPowerOff(void);
//extern DisableIRQ();
//extern EnableIRQ();
//extern DisableFIQ();
//extern EnableFIQ();
extern void MT3351_IRQUnmask(unsigned int line);
extern PMU_STATUS PMU_EnableCharger( BOOL enable);
extern void CONFIG_DCM(kal_bool enable, DCM_FSEL sel);
extern DVFS_STATUS DVFS_init(void);

PMU_STATUS PMU_EnableOSCBagBlk(BOOL bEnable)
{
    UINT32 u4Val;
    
	DRV_WriteReg32(PMU_CON26, 0);
	DRV_WriteReg32(PMU_CON26, RG_CHR_EN_CODE);
	u4Val = DRV_Reg32(PMU_CON1F);
	
	// 0:enable 1:disable 
    if(bEnable)
        u4Val &= ~RG_OSC_ENB;
    else
        u4Val |= RG_OSC_ENB;
	DRV_WriteReg32(PMU_CON1F, u4Val);    
    return PMU_STATUS_OK;

}    


PMU_STATUS PMU_SetOVThreshHold( UINT32 u4OV )
{
    UINT32 u4Val;
	DRV_WriteReg32(PMU_CON26, 0);
	DRV_WriteReg32(PMU_CON26, RG_CHR_EN_CODE);
	u4Val = DRV_Reg32(PMU_CON1E);
	
    if (u4OV == OV_441V)
        u4Val = ((u4Val & 0x3FFF) | (0x3 << 14)) ;
    else
        u4Val = ((u4Val & 0x3FFF) | (0x1 << 14)) ;
	DRV_WriteReg32(PMU_CON1E, u4Val);    
	
    return PMU_STATUS_OK;
}

PMU_STATUS PMU_SetUVThreshHold( RG_UV_SEL eUVSel)
{
    UINT32 u4Val;
	DRV_WriteReg32(PMU_CON26, 0);
	DRV_WriteReg32(PMU_CON26, RG_CHR_EN_CODE);

	u4Val = DRV_Reg32(PMU_CON1F);
    u4Val = ((u4Val & 0xFFFC) | eUVSel) ;
	DRV_WriteReg32(PMU_CON1F, u4Val);    
    return PMU_STATUS_OK;
}


PMU_STATUS PMU_SetThermal( RG_THR_SEL eTHRSel)
{
    UINT32 u4Val;
	DRV_WriteReg32(PMU_CON26, 0);
	DRV_WriteReg32(PMU_CON26, RG_CHR_EN_CODE);

	u4Val = DRV_Reg32(PMU_CON1F);
    u4Val = ((u4Val & 0xCFFF) | eTHRSel) ;
    DRV_WriteReg32(PMU_CON1F, u4Val); 
	return PMU_STATUS_OK;

}


PMU_STATUS PMU_GetPMURtcTmpStatus()
{
	UINT32  u4Val ;
	u4Val = DRV_Reg32(PMU_CON2A);
	return u4Val;
}



PMU_STATUS PMU_Enable32KProtect( BOOL bEnable)
{
	PMU_EnableCharger(FALSE);
	DRV_WriteReg32(PMU_CON26, 0);
	DRV_WriteReg32(PMU_CON26, RG_CHR_EN_CODE);
    if(bEnable)
    	DRV_WriteReg32(PMU_F32K_CON0, 0);
    else    
      	DRV_WriteReg32(PMU_F32K_CON0, F32K_PROTECT_CODE);

	return PMU_STATUS_OK;

}

PMU_STATUS PMU_EnableTempProtect( BOOL bEnable)
{
	PMU_EnableCharger( FALSE);
	DRV_WriteReg32(PMU_CON26, 0);
	DRV_WriteReg32(PMU_CON26, RG_CHR_EN_CODE);
    if(bEnable)
    	DRV_WriteReg32(PMU_TEMPDET_CON0, 0);
    else
    	DRV_WriteReg32(PMU_TEMPDET_CON0, TEMP_PROTECT_CODE);        
	return PMU_STATUS_OK;
}

PMU_STATUS PMU_SetHiLoTemperature( UINT32 u4HiVal ,UINT32 u4LoVal)
{
    UINT32 u4Val = 0;
	u4Val = DRV_Reg32(PMU_TEMPDET_CON1);
    
    u4Val =  (u4HiVal|(u4LoVal<<8));
	DRV_WriteReg32(PMU_TEMPDET_CON0, TEMP_SET_CODE);    
	DRV_WriteReg32(PMU_TEMPDET_CON1, u4Val);

	return PMU_STATUS_OK;
}


PMU_STATUS PMU_EnableCharger( BOOL enable)
{
	UINT32 u4Val;

	DRV_WriteReg32(PMU_CON26, RG_CHR_EN_CODE);
	
	u4Val = DRV_Reg32(PMU_CON1E);
	if (enable)
		u4Val |= RG_CR_EN;
	else
		u4Val &= ~RG_CR_EN;
	DRV_WriteReg32(PMU_CON1E, u4Val);
	

	return PMU_STATUS_OK;
}

PMU_STATUS PMU_SetConstantCurrentMode( CC_MODE_CHAGE_LEVEL cc_cuerrent)
{
	UINT32  u4Val ;
	u4Val = DRV_Reg32(PMU_CON1E);
	u4Val &= ~RG_CLASS_D_MASK;
	u4Val |= cc_cuerrent;
	PMU_EnableCharger(FALSE);

	DRV_WriteReg32(PMU_CON26, 0);
	DRV_WriteReg32(PMU_CON26, RG_CHR_EN_CODE);
    DRV_WriteReg32(PMU_CON1E, u4Val);	
	BMT_status.charging_current = cc_cuerrent;
	return PMU_STATUS_OK;
}


//////////////////////////////////////////////////////////////////////////////

PMU_STATUS PMU_SetPreConstantCurrentMode( RG_CAL_PRECC pre_cc_cuerrent)
{
	UINT32  u4Val ;
	u4Val = DRV_Reg32(PMU_CON24);
	u4Val &= 0xFFFC;
	u4Val |= pre_cc_cuerrent;
	PMU_EnableCharger( FALSE);
	DRV_WriteReg32(PMU_CON26, 0);
    DRV_WriteReg32(PMU_CON26, RG_CHR_EN_CODE);	
	DRV_WriteReg32(PMU_CON24, u4Val);
	BMT_status.pre_charging_current = pre_cc_cuerrent;	
	return PMU_STATUS_OK;	
}

PMU_STATUS PMU_SwitchPowerSource( RG_PS_SOURCE power_select)
{
    UINT32  u4Val ;
    PMU_EnableCharger( FALSE);
    u4Val = DRV_Reg32(PMU_CON24);
  
    if(power_select == PS_SET_AUTO)
    {         
        u4Val &= ~0x40;               
    }
    else
    {
        u4Val |= RG_PS_SET ;    
        //BAT
        if(power_select == PS_SET_VBAT)
            u4Val &= ~0x80 ;
        //AC
        else 
            u4Val |= 0x80 ;    	
    }
       
    DRV_WriteReg32(PMU_CON24, u4Val) ;	
    BMT_status.PWR_SRC = power_select;
       
    return PMU_STATUS_OK;	
}


PMU_STATUS PMU_EnablePreConstantCurrentMode( RG_CAL_PRECC PreCC_Current)
{
    // Vsense/Rsense = 10mV/0.2 = 50mA
	PMU_SetPreConstantCurrentMode( PreCC_Current);
	PMU_EnableCharger(TRUE);
	BMT_status.bat_charging_state = CHR_PRE ;
	return PMU_STATUS_OK;	

}

PMU_STATUS PMU_EnableConstantCurrentMode( CC_MODE_CHAGE_LEVEL CC_Current)
{
	PMU_SetConstantCurrentMode(CC_Current);
	PMU_EnableCharger(TRUE);
	BMT_status.bat_charging_state = CHR_CC ;
	return PMU_STATUS_OK;	

	
}

PMU_STATUS PMU_EnableConstantVoltageMode()
{
	PMU_EnableCharger(TRUE);
	BMT_status.bat_charging_state = CHR_CV ;
	return PMU_STATUS_OK;	

}


UINT32 PMU_GetChargerStatus()
{
	UINT32  u4Val ;
	u4Val = DRV_Reg32(PMU_CON28);
	return u4Val;
}




// enable or disable only
PMU_STATUS PMU_SetVUSBLDO( BOOL enable)
{  
	UINT32 u4Val;

	u4Val = DRV_Reg32(PMU_CON16);
	if (enable)
		u4Val |= (RG_VUSB_EN);
	else
		u4Val &= ~RG_VUSB_EN;
	DRV_WriteReg32(PMU_CON16, u4Val) ;

	return PMU_STATUS_OK;	
}

// set enable and voltage
PMU_STATUS PMU_SetVCAMEDLDO( BOOL enable, RG_VCAMD_SEL VCAMD_VOL)
{  
	UINT32 u4Val;

	u4Val = DRV_Reg32(PMU_CON17);
	if (enable)
	{
		u4Val &= ~RG_VCAMD_SEL_MASK;
		u4Val |= ( VCAMD_VOL | RG_VCAMD_EN);
	}
	else
		u4Val &= ~RG_VCAMD_EN;

	DRV_WriteReg32(PMU_CON17, u4Val) ;

	return PMU_STATUS_OK;	
}

// Set enable only
PMU_STATUS PMU_SetVTCXOLDO( BOOL enable)
{  
	UINT32 u4Val;
	u4Val = DRV_Reg32(PMU_CON18);

	if (enable)
		u4Val |= (RG_VTCXO_EN);
	else
		u4Val &= ~RG_VTCXO_EN;
	DRV_WriteReg32(PMU_CON18, u4Val) ;

	return PMU_STATUS_OK;	
}

// Set enable only
PMU_STATUS PMU_SetVBTLDO( BOOL enable)
{  
	UINT32 u4Val;
	u4Val = DRV_Reg32(PMU_CON19);
	if (enable)
		u4Val |= (RG_VBT_EN);
	else
		u4Val &= ~RG_VBT_EN;
	DRV_WriteReg32(PMU_CON19, u4Val) ;
	return PMU_STATUS_OK;	
}

// set enable and voltage
PMU_STATUS PMU_SetVCAMEALDO( BOOL enable , RG_VCAMA_SEL VCAMA_VOL)
{  
	UINT32 u4Val;

	u4Val = DRV_Reg32(PMU_CON1A);
	if (enable)
	{
		u4Val &= ~RG_VCAMA_SEL_MASK;
		u4Val |= (VCAMA_VOL | RG_VCAMA_EN);
	}
	else
		u4Val &= ~RG_VCAMA_EN;
	DRV_WriteReg32(PMU_CON1A, u4Val) ;

	return PMU_STATUS_OK;	
}

// set enable only
PMU_STATUS PMU_SetVGPSLDO( BOOL enable)
{  
	UINT32 u4Val;
	u4Val = DRV_Reg32(PMU_CON1B);
	if (enable)
		u4Val |= (RG_VGPS_EN);
	else
		u4Val &= ~RG_VGPS_EN;
	DRV_WriteReg32(PMU_CON1B, u4Val) ;
	return PMU_STATUS_OK;	
}

// set enable and voltage
PMU_STATUS PMU_SetVGPLDO( BOOL enable, RG_VGP_SEL RG_VGP_vol)
{  
	UINT32 u4Val;
	
	//PMU_CON1C, VGP
	u4Val = DRV_Reg32(PMU_CON1C);
	if (enable)
	{
		u4Val &= ~RG_VGP_SEL_MASK;	
		u4Val |= (RG_VGP_vol | RG_VGP_EN);
	}
	else
		u4Val &= ~RG_VGP_EN;
	DRV_WriteReg32(PMU_CON1C, u4Val) ;
	
	return PMU_STATUS_OK;	
}

// set enable and voltage
PMU_STATUS PMU_SetVSDIOLDO( BOOL enable,  RG_VSDIO_SEL RG_VSDIO_vol)
{
	UINT32 u4Val;

	u4Val = DRV_Reg32(PMU_CON1D);
	if (enable)
	{
		u4Val &= ~RG_VSDIO_SEL_MASK;	
		u4Val |= (RG_VSDIO_vol | RG_VSDIO_EN);
	}
	else
		u4Val &= ~RG_VSDIO_EN;
	DRV_WriteReg32(PMU_CON1D, u4Val) ;
	return PMU_STATUS_OK;	
}

PMU_STATUS PMU_SetFixPWMMode( BOOL bFixPWM  )
{
	UINT32 u4Val;

	u4Val = DRV_Reg32(PMU_CONB);
	if (bFixPWM)
	{
        // PMU_CONB[15](RG_MODEST_VBAT) = 0
        // PMU_CONB[13:12](RG_MODEEN_VBAT) = 10
        u4Val &= 0x4FFF ;
        u4Val |= 0x2000;
	}
	else
	{
        // PMU_CONB[15](RG_MODEST_VBAT) = 1
        // PMU_CONB[13:12](RG_MODEEN_VBAT) = 01
        u4Val &= 0x4FFF ;
        u4Val |= 0x9000;
	}	    
	DRV_WriteReg32(PMU_CONB, u4Val) ;
	return PMU_STATUS_OK;	
}

void PMU_DumpBusPowerState(void)
{
    //UINT32 u4Index;
    printk("=======PLL reference status======\n\r");    
    printk("MM subsystem Count = %d\n\r", g_BusHW.dwMMCount);
    printk("UPLL Count = %d\n\r", g_BusHW.dwUpllCount);
    printk("LPLL Count = %d\n\r", g_BusHW.dwLpllCount);

    printk("=======LDO reference status======\n\r");    
    printk("LDO MT3351_POWER_USB count = %d\n\r", g_BusHW.dwPowerCount[MT3351_POWER_USB]);
    printk("LDO MT3351_POWER_GP count = %d\n\r", g_BusHW.dwPowerCount[MT3351_POWER_GP]);
    printk("LDO MT3351_POWER_BT count = %d\n\r", g_BusHW.dwPowerCount[MT3351_POWER_BT]);
    printk("LDO MT3351_POWER_GPS count = %d\n\r", g_BusHW.dwPowerCount[MT3351_POWER_GPS]);
    printk("LDO MT3351_POWER_CAMA count = %d\n\r", g_BusHW.dwPowerCount[MT3351_POWER_CAMA]);
    printk("LDO MT3351_POWER_CAMD count = %d\n\r", g_BusHW.dwPowerCount[MT3351_POWER_CAMD]);
    printk("LDO MT3351_POWER_SDIO count = %d\n\r", g_BusHW.dwPowerCount[MT3351_POWER_SDIO]);    

    printk("=======MCU CLK reference status======\n\r");    
    printk("CLK MT3351_CLOCK_DMA count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_DMA]);    
    printk("CLK MT3351_CLOCK_USB count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_USB]);    
    printk("CLK MT3351_CLOCK_RESERVED_0 count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_RESERVED_0]);    
    printk("CLK MT3351_CLOCK_SPI count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_SPI]);    
    printk("CLK MT3351_CLOCK_GPT count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_GPT]);    
    printk("CLK MT3351_CLOCK_UART0 count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_UART0]);    
    printk("CLK MT3351_CLOCK_UART1 count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_UART1]);    
    printk("CLK MT3351_CLOCK_UART2 count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_UART2]);    
    printk("CLK MT3351_CLOCK_UART3 count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_UART3]);    
    printk("CLK MT3351_CLOCK_UART4 count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_UART4]);    
    printk("CLK MT3351_CLOCK_PWM count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_PWM]);    
    printk("CLK MT3351_CLOCK_PWM0 count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_PWM0]);    
    printk("CLK MT3351_CLOCK_PWM1 count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_PWM1]);    
    printk("CLK MT3351_CLOCK_PWM2 count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_PWM2]);    
    printk("CLK MT3351_CLOCK_MSDC0 count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_MSDC0]);    
    printk("CLK MT3351_CLOCK_MSDC1 count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_MSDC1]);    
    printk("CLK MT3351_CLOCK_MSDC2 count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_MSDC2]);    
    printk("CLK MT3351_CLOCK_NFI count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_NFI]);    
    printk("CLK MT3351_CLOCK_IRDA count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_IRDA]);    
    printk("CLK MT3351_CLOCK_I2C count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_I2C]);    
    printk("CLK MT3351_CLOCK_AUXADC count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_AUXADC]);    
    printk("CLK MT3351_CLOCK_TOUCHPAD count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_TOUCHPAD]);    
    printk("CLK MT3351_CLOCK_SYSROM count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_SYSROM]);    
    printk("CLK MT3351_CLOCK_KEYPAD count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_KEYPAD]);    

    printk("=======MM CLK reference status======\n\r");    
    printk("CLK MT3351_CLOCK_GMC count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_GMC]);    
    printk("CLK MT3351_CLOCK_G2D count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_G2D]);    
    printk("CLK MT3351_CLOCK_GCMQ count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_GCMQ]);    
    printk("CLK MT3351_CLOCK_DDE count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_DDE]);    
    printk("CLK MT3351_CLOCK_IMAGE_DMA count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_IMAGE_DMA]);    
    printk("CLK MT3351_CLOCK_IMAGE_PROCESSOR count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_IMAGE_PROCESSOR]);    
    printk("CLK MT3351_CLOCK_JPEG count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_JPEG]);    
    printk("CLK MT3351_CLOCK_DCT count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_DCT]);    
    printk("CLK MT3351_CLOCK_ISP count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_ISP]);    
    printk("CLK MT3351_CLOCK_PRZ count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_PRZ]);    
    printk("CLK MT3351_CLOCK_CRZ count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_CRZ]);    
    printk("CLK MT3351_CLOCK_DRZ count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_DRZ]);    
    printk("CLK MT3351_CLOCK_MTVSPI count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_MTVSPI]);    
    printk("CLK MT3351_CLOCK_ASM count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_ASM]);    
    printk("CLK MT3351_CLOCK_I2S count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_I2S]);    
    printk("CLK MT3351_CLOCK_RESIZE_LB count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_RESIZE_LB]);    
    printk("CLK MT3351_CLOCK_LCD count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_LCD]);    
    printk("CLK MT3351_CLOCK_DPI count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_DPI]);    
    printk("CLK MT3351_CLOCK_VFE count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_VFE]);    
    printk("CLK MT3351_CLOCK_AFE count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_AFE]);    
    printk("CLK MT3351_CLOCK_BLS count = %d\n\r", g_BusHW.dwClockCount[MT3351_CLOCK_BLS]);    

}    

void ReadVersion(void)
{
    g_HW_Ver  = DRV_Reg32(0xF0001000);
    g_FW_Ver  = DRV_Reg32(0xF0001004);    
    g_HW_Code = DRV_Reg32(0xF0001008);        
}

static int mt3351_pm_prepare(void)
{
    printk("mt3351_pm_prepare\n\r");        
	return 0;
}

void PMU_SetWakeupSource(void)
{
    UINT32 j;

#if 0
    // Clear all wakeup source
    SLPCTL_Reset_WakeUp_Source(pSlpCtlReg);
#endif

    for(j = 0; j < MAX_WAKEUP_SOURCE_INTR; j++) 
    {
        // enable wanted wakeup source
        SLPCTL_Set_WakeUp_Source(g_oalIrq2Wake[j][1]);
        // enable wanted interrupt then
	    MT3351_IRQUnmask(g_oalIrq2Wake[j][0]);
    }
           
}


void PMU_SuspendEnter(void)
{
    UINT32 slpSta, slpCfg;
    UINT32 irqMask_L, irqMask_H;

    // Save interrupt masks
    irqMask_L = *MT3351_IRQ_MASKL;
    irqMask_H = *MT3351_IRQ_MASKH;

    // mask all interrupts
    *MT3351_IRQ_MASKL = 0xFFFFFFFF;
    *MT3351_IRQ_MASKH = 0xFFFFFFFF;

    // Setup wakeup source    
    PMU_SetWakeupSource();

    // Set Sleep Pause Duration, 5 second
    SLPCTL_Set_Sleep_Pause_Duration(5000);

    SLPCTL_Set_PLL_WakeUp_Reset(TRUE, 0x200);

    SLPCTL_Set_Ext_CLK_Settle_Time(0x400);

    // (1) disable IRQ
    //DisableFIQ();
    //DisableIRQ();

   
    // Power off CPU, 
    // see mt3351_slpctrl.s
    PMU_CPUPowerOff();

    // Get sleep control status
    slpSta = SLPCTL_Get_Pause_Status();
    slpCfg = SLPCTL_Get_WakeUp_Source();

    // enable IRQ
    //EnableIRQ();
    //EnableFIQ();	


    if (slpSta & PAUSE_INT) 
    {
        //wakeup by wakeup source
        g_oalWakeSource = WAKE_REASON_SOURCE;
    }
    else if (slpSta & PAUSE_CPL) 
    {
        if (slpCfg & WAKE_INT_EN) 
        {
            //wakeup by pause period completed
            g_oalWakeSource = WAKE_REASON_TMR;
        }
        else 
        {
            //a fake CPL wakeup
            g_oalWakeSource = WAKE_REASON_TMR;
        }
    }
    else 
    {
        // wakeup by unknown reason
        g_oalWakeSource = WAKE_REASON_UNKNOWN;
    }
    SLPCTL_Set_Pause_Start(FALSE);
    // Restore interrupt masks
    *MT3351_IRQ_MASKL = irqMask_L;
    *MT3351_IRQ_MASKH = irqMask_H;
}



//# echo -n standby > /sys/power/state
//# echo -n mem > /sys/power/state
static int mt3351_pm_enter(suspend_state_t state)
{
	/* ensure the debug is initialised (if enabled) */
	switch (state) 
	{
    	case PM_SUSPEND_ON:
    		printk("mt3351_pm_enter PM_SUSPEND_ON\n\r");
    		break;
    	case PM_SUSPEND_STANDBY:
    		printk("mt3351_pm_enter PM_SUSPEND_STANDBY\n\r");        
    		break;
    	case PM_SUSPEND_MEM:
    		printk("mt3351_pm_enter PM_SUSPEND_MEM\n\r");        
	        PMU_SuspendEnter();    		
    		break;
        /*
    	case PM_SUSPEND_DISK:
    		printk("mt3351_pm_enter PM_SUSPEND_DISK\n\r");        
    		printk("Not support for MT3351\n\r");            		
    		break;
    	*/
    	case PM_SUSPEND_MAX:
    		printk("mt3351_pm_enter PM_SUSPEND_MAX\n\r");        
    		printk("Not support for MT3351\n\r");            		
    		break;
    		
    	default:
    	    printk("mt3351_pm_enter Error state\n\r");
    		break;
	}
	return 0;
}

static void mt3351_pm_finish(void)
{
    printk("mt3351_pm_finish\n\r");        
}

static struct platform_suspend_ops mt3351_pm_ops = {
	//.pm_disk_mode	= PM_DISK_FIRMWARE,
	.prepare	= mt3351_pm_prepare,
	.enter		= mt3351_pm_enter,
	.finish		= mt3351_pm_finish,
};


// 

BOOL CONFIG_PowerOn_CONA(PDN_CONA_MODE mode)
{
    __raw_writel(mode,PDNCLRA);        
    return TRUE;
}

BOOL CONFIG_PowerOn_CONB(PDN_CONB_MODE mode)
{
    __raw_writel(mode,PDNCLRB);        
    return TRUE;
}


BOOL CONFIG_CGCON(CGCON_MODE mode, BOOL bEnabled)
{
    if (bEnabled)
    {   
	    __raw_writel( (mode | __raw_readl(CGCON)) ,CGCON);
	}
    else
    {   
    	__raw_writel( ( (~mode) & __raw_readl(CGCON)) ,CGCON);
	}
    return TRUE;
}

BOOL CONFIG_PowerOff_CONB(PDN_CONB_MODE mode)
{
    __raw_writel(mode,PDNSETB);
    return TRUE;
}

BOOL CONFIG_PowerOff_CONA(PDN_CONA_MODE mode)
{
    __raw_writel(mode,PDNSETA);
    return TRUE;
}


BOOL hwEnableClock(MT3351_CLOCK clockId)
{

	if ((clockId >= MT3351_CLOCKS_COUNT) || (clockId == MT3351_CLOCK_NONE))
	{
		printk("[BUS] Error!! clockId is wrong\r\n");
 		return FALSE;
 	}

	spin_lock(&csClock_lock);
	
	g_BusHW.dwClockCount[clockId]++;

	if(g_BusHW.dwClockCount[clockId] > 1)
	{
		spin_unlock(&csClock_lock);
		return TRUE;
	}

    if (clockId >= MT3351_CLOCK_CONA_START && clockId <= MT3351_CLOCK_CONA_END)
    {
        PDN_CONA_MODE conaMode;
        conaMode = (PDN_CONA_MODE) (1 << (clockId - MT3351_CLOCK_CONA_START));
        CONFIG_PowerOn_CONA(conaMode);

    }
    else if (clockId >= MT3351_CLOCK_CONB_START && clockId <= MT3351_CLOCK_CONB_END)
    {
        PDN_CONB_MODE conbMode;

        // Because it automatically turn on the GMC clock
        // Skip GMC_CLOCK
        //

        if (__raw_readl(PDNCONB) == (0xFFFFFFFF & ~MT3351_CLOCK_GMC_BIT_MASK)
            && g_BusHW.dwMMCount++ == 0)        
        {
            printk("[BUS] 1. MM_OFF set to FALSE by %d, dwUpllCount=%d\r\n",clockId, g_BusHW.dwMMCount);
            CONFIG_CGCON(MM_OFF, FALSE);
        }
        
        conbMode = (PDN_CONB_MODE)(1 << (clockId - MT3351_CLOCK_CONB_START));
        CONFIG_PowerOn_CONB(conbMode);

        // To check if turn on LPLL?
        if (MT3351_CLOCK_DPI == clockId && g_BusHW.dwLpllCount++ == 0)
        {
            // power up LPLL
            __raw_writel((__raw_readl(LPLL_CON1) & ~LPLL_PWDB),LPLL_CON1);     
            __raw_writel(0x13,LPLL_CON0);     
            
        }
    }
    else if (MT3351_CLOCK_DSP == clockId)
    {
        CONFIG_CGCON(DSP_OFF, FALSE);
    }
    else if (MT3351_CLOCK_MSDC_MM == clockId && g_BusHW.dwMMCount++ == 0)
    {
		printk("[BUS] 2. MM_OFF set to FALSE by %d, dwMMCount=%d\r\n",clockId, g_BusHW.dwMMCount);
        CONFIG_CGCON(MM_OFF, FALSE);
    }

    // power up UPLL
    if (MT3351_CLOCK_USB == clockId ||MT3351_CLOCK_MSDC_UPLL == clockId)
    {
        if (g_BusHW.dwUpllCount++ == 0)
        {
            printk("[BUS] UPLL_48M_OFF set to FALSE by %d, dwUpllCount=%d\r\n",clockId, g_BusHW.dwUpllCount);
            printk("[BUS] UPLL_PWDB set to FALSE by %d\r\n",clockId);

            CONFIG_CGCON(UPLL_48M_OFF, FALSE);
            
            __raw_writel((__raw_readl(PDN_CON) & ~UPLL_PWDB),PDN_CON);     
        }
    }

	spin_unlock(&csClock_lock);
	
   	return TRUE;
}


BOOL hwDisableClock(MT3351_CLOCK clockId)
{

	if ((clockId >= MT3351_CLOCKS_COUNT) || (clockId == MT3351_CLOCK_NONE))
	{
		printk("%s:%s:%d clockId:%d is wrong\r\n",__FILE__,__FUNCTION__,
		    __LINE__ , clockId);
		return FALSE;
	}

	spin_lock(&csClock_lock);
	
	if(g_BusHW.dwClockCount[clockId] == 0)
	{
		//ERROR
		spin_unlock(&csClock_lock);
		printk("%s:%s:%d clockId:%d (g_BusHW.dwClockCount[clockId] = 0)\r\n",
		    __FILE__,__FUNCTION__,__LINE__ ,clockId);
		return FALSE;
	}

	g_BusHW.dwClockCount[clockId] --;
	if(g_BusHW.dwClockCount[clockId] > 0)
	{
		spin_unlock(&csClock_lock);		
		return TRUE;
	}
	
    if (clockId >= MT3351_CLOCK_CONA_START && clockId <= MT3351_CLOCK_CONA_END)
    {
        PDN_CONA_MODE conaMode;
        conaMode = (PDN_CONA_MODE) (1 << (clockId - MT3351_CLOCK_CONA_START));
        CONFIG_PowerOff_CONA(conaMode);
    }
    else if (clockId >= MT3351_CLOCK_CONB_START && clockId <= MT3351_CLOCK_CONB_END)
    {
        PDN_CONB_MODE conbMode;
       
        conbMode = (PDN_CONB_MODE) (1 << (clockId - MT3351_CLOCK_CONB_START));
        CONFIG_PowerOff_CONB(conbMode);

        // Because it automatically turn on the GMC clock
        // Skip GMC_CLOCK
        //
        if (__raw_readl(PDNCONB) == (0xFFFFFFFF & ~MT3351_CLOCK_GMC_BIT_MASK)
            && g_BusHW.dwMMCount-- == 1)        
        {
            // Stall 5us here for GMC to finish all transactions with EMI
            // before turn off MM clock
            //
            udelay(5);            
            CONFIG_CGCON(MM_OFF, TRUE);
        }

        // TO check if turn off LPLL?
        if (MT3351_CLOCK_DPI == clockId && g_BusHW.dwLpllCount-- == 1)        
        {
            // power down LPLL
            __raw_writel((__raw_readl(LPLL_CON1) | LPLL_PWDB),LPLL_CON1);     

            // SW Reset DPI
            mt3351_wdt_SW_MMPeripheralReset(MT3351_MM_PERI_DPI);
        }
    }
    else if (MT3351_CLOCK_DSP == clockId)
    {
        CONFIG_CGCON(DSP_OFF, TRUE);
    }
    else if (MT3351_CLOCK_MSDC_MM == clockId && g_BusHW.dwMMCount-- == 1)
    {
        
        CONFIG_CGCON(MM_OFF, TRUE);
    }

    // power down UPLL
    if (MT3351_CLOCK_USB == clockId || MT3351_CLOCK_MSDC_UPLL == clockId)
    {
        if (g_BusHW.dwUpllCount-- == 1)
        {            
            CONFIG_CGCON(UPLL_48M_OFF, TRUE);            
            __raw_writel( (__raw_readl(PDN_CON)|UPLL_PWDB) ,PDN_CON);     
        }
    }

	spin_unlock(&csClock_lock);

    return TRUE;
}


BOOL hwPowerOn(MT3351_POWER powerId, MT3351_POWER_VOLTAGE powervol)
{
    if(powerId >= MT3351_POWERS_COUNT)
    {
		printk("[BUS] Error!! powerId is wrong\r\n");
        return FALSE;
    }

	spin_lock(&csPower_lock);
	
	g_BusHW.dwPowerCount[powerId] ++;

    //We've already enable this LDO before
	if(g_BusHW.dwPowerCount[powerId] > 1)
	{
		spin_unlock(&csPower_lock);
		printk("[BUS] Error!! (g_BusHW.dwPowerCount[powerId] > 1)\r\n");
		return TRUE;
	}
    
    switch (powerId)
    {
    case MT3351_POWER_USB:
        PMU_SetVUSBLDO(TRUE);
        break;
    case MT3351_POWER_GP:
        switch (powervol)
        {
            case VOL_1500:
                PMU_SetVGPLDO(TRUE, RG_VGP_SEL_1_5V);
                break;
            case VOL_1800:
                PMU_SetVGPLDO(TRUE, RG_VGP_SEL_1_8V); 
                break;
            case VOL_2800:
                PMU_SetVGPLDO(TRUE, RG_VGP_SEL_2_8V); 
                break;
            case VOL_3300:
                PMU_SetVGPLDO(TRUE, RG_VGP_SEL_3_3V);
                break;
            default :
                PMU_SetVGPLDO(TRUE, RG_VGP_SEL_3_3V);                
                break;
        }
        break;
    case MT3351_POWER_BT:
        PMU_SetVBTLDO(TRUE);
        break;
    case MT3351_POWER_GPS:
        PMU_SetVGPSLDO(TRUE);
        break;
    case MT3351_POWER_LCD:
        printk("PowerOn MT3351_POWER_LCD isn't implemented\r\n");
        break;
    case MT3351_POWER_CAMA:
        switch (powervol)
        {
            case VOL_1500:
                PMU_SetVCAMEALDO(TRUE, RG_VCAMA_SEL_1_5V);
                break;
            case VOL_1800:
                PMU_SetVCAMEALDO(TRUE, RG_VCAMA_SEL_1_8V); 
                break;
            case VOL_2500:
                PMU_SetVCAMEALDO(TRUE, RG_VCAMA_SEL_2_5V); 
                break;
            case VOL_2800:
                PMU_SetVCAMEALDO(TRUE, RG_VCAMA_SEL_2_8V);
                break;
            default :
                PMU_SetVCAMEALDO(TRUE, RG_VCAMA_SEL_1_5V);                
                break;
        }
        break;
    case MT3351_POWER_CAMD:
        switch (powervol)
        {
            case VOL_1300:
                PMU_SetVCAMEDLDO(TRUE, RG_VCAMD_SEL_1_3V);
                break;
            case VOL_1500:
                PMU_SetVCAMEDLDO(TRUE, RG_VCAMD_SEL_1_5V); 
                break;
            case VOL_1800:
                PMU_SetVCAMEDLDO(TRUE, RG_VCAMD_SEL_1_8V); 
                break;
            case VOL_2800:
                PMU_SetVCAMEDLDO(TRUE, RG_VCAMD_SEL_2_8V);
                break;
            default :
                PMU_SetVCAMEDLDO(TRUE, RG_VCAMD_SEL_2_8V);                
                break;
        }        
        break;
    case MT3351_POWER_SDIO:
        switch (powervol)
        {
            case VOL_1500:
                PMU_SetVSDIOLDO(TRUE, RG_VSDIO_SEL_1_5V);
                break;
            case VOL_1800:
                PMU_SetVSDIOLDO(TRUE, RG_VSDIO_SEL_1_8V); 
                break;
            case VOL_2800:
                PMU_SetVSDIOLDO(TRUE, RG_VSDIO_SEL_2_8V); 
                break;
            case VOL_3300:
                PMU_SetVSDIOLDO(TRUE, RG_VSDIO_SEL_3_3V);
                break;
            default :
                PMU_SetVSDIOLDO(TRUE, RG_VSDIO_SEL_3_3V);                
                break;
        }                
        break;
    case MT3351_POWER_MEMORY:
        printk("PowerOn Can't Open/Close MT3351_POWER_MEMORY\r\n");
        break;
    case MT3351_POWER_TCXO:
        printk("PowerOn Can't Open/Close MT3351_POWER_TCXO\r\n");
        break;
    case MT3351_POWER_RTC:
        printk("PowerOn Can't Open/Close MT3351_POWER_RTC\r\n");
        break;
    case MT3351_POWER_CORE1:
        printk("PowerOn Can't Open/Close MT3351_POWER_CORE1\r\n");
        break;
    case MT3351_POWER_CORE2:
        printk("PowerOn Can't Open/Close MT3351_POWER_CORE2\r\n");
        break;
    case MT3351_POWER_ANALOG:
        printk("PowerOn Can't Open/Close MT3351_POWER_ANALOG\r\n");
        break;
    case MT3351_POWER_IO:
        printk("PowerOn Can't Open/Close MT3351_POWER_IO\r\n");
        break;
    default:
        printk("PowerOn the power setting can't be recongnize\r\n");
        break;
    }

	spin_unlock(&csPower_lock);

    return TRUE;    
}


BOOL hwPowerDown(MT3351_POWER powerId)
{

    if(powerId >= MT3351_POWERS_COUNT)
    {
		printk("%s:%s:%d powerId:%d is wrong\r\n",__FILE__,__FUNCTION__,
		    __LINE__ , powerId);
        return FALSE;
    }

	spin_lock(&csPower_lock);
	
	if(g_BusHW.dwPowerCount[powerId] == 0)
	{
		spin_unlock(&csPower_lock);
		printk("%s:%s:%d powerId:%d (g_BusHW.dwPowerCount[powerId] = 0)\r\n",
		    __FILE__,__FUNCTION__,__LINE__ ,powerId);
		return FALSE;
	}

	g_BusHW.dwPowerCount[powerId] --;
	if(g_BusHW.dwPowerCount[powerId] > 0)
	{
		spin_unlock(&csPower_lock);
		return TRUE;
	}

    switch (powerId)
    {
    case MT3351_POWER_USB:
        PMU_SetVUSBLDO(FALSE);
        break;
    case MT3351_POWER_GP:
        PMU_SetVGPLDO(FALSE, RG_VGP_SEL_3_3V); 
        break;
    case MT3351_POWER_BT:
        PMU_SetVBTLDO(FALSE);
        break;
    case MT3351_POWER_GPS:
        PMU_SetVGPSLDO(FALSE);
        break;
    case MT3351_POWER_LCD:
        printk("PowerOFF MT3351_POWER_LCD isn't implemented\r\n");
        break;
    case MT3351_POWER_CAMA:
        PMU_SetVCAMEALDO(FALSE, RG_VCAMA_SEL_1_5V);       
        break;
    case MT3351_POWER_CAMD:
        PMU_SetVCAMEDLDO(FALSE, RG_VCAMD_SEL_2_8V); 	   
        break;
    case MT3351_POWER_SDIO:
		PMU_SetVSDIOLDO(FALSE , RG_VSDIO_SEL_3_3V ); 
        break;
    case MT3351_POWER_MEMORY:
    case MT3351_POWER_TCXO:
    case MT3351_POWER_RTC:
    case MT3351_POWER_CORE1:
    case MT3351_POWER_CORE2:
    case MT3351_POWER_ANALOG:
    case MT3351_POWER_IO:
    default:
        printk("PowerOFF won't be modified(powerId = %d)\r\n", powerId);
        break;
    }

    spin_unlock(&csPower_lock);
	
    return TRUE;    
}


void MT3351_pmu_init(void)
{  
    UINT32 u4Val ;
	volatile UINT32 Efuse_d3=0; 

    //VDD = 1.2V, VCCQ = 3.3V, FSOURCE = Floating or Ground 
    DRV_WriteReg32(EFUSEC_CON, EFUSEC_CON_READ);

    while(DRV_Reg32(EFUSEC_CON) & EFUSEC_CON_BUSY); 
    Efuse_d3 = DRV_Reg32(EFUSEC_D3);        

    ReadVersion();
    
    // (adjust reference voltage to 1.17V and set PID initial value to default)
    DRV_WriteReg(PMU_CON3, 0x144C);
    // (adjust PID D gain to default+2)
    DRV_WriteReg(PMU_CON4, 0x7380);
    // ((auto mode, adjust  PID P/I gain to default+2)
    //DRV_WriteReg(PMU_CON5, 0x64F7);
    // VCORE1 Vcore1/2/Vm slew-rate the slowest value
    u4Val = Efuse_d3 & PMU_CALIBRATION_MASK;
	u4Val = u4Val >> PMU_CALIBRATION_OFFSET;
	if(u4Val==1)
	{	DRV_WriteReg(PMU_CON7, 0x781F);
		g_pmu_cali = TRUE;
	}
	else
	{	DRV_WriteReg(PMU_CON7, 0x781E);
		g_pmu_cali = FALSE;
	}

    //PMU_CON9, Vcore2 CLK select
    u4Val = DRV_Reg(PMU_CON9);
    u4Val |= RG_DCVCKSEL_VCORE2;
    //VCORE1/2 and VM's DCVTRIM => 3'b000(bandgap issue)    
    u4Val &= RG_DCVTRIM_VBAT_VCORE2_MASK;    
    DRV_WriteReg(PMU_CON9, u4Val);

    //PMU_CONA
    //enable Vcore2 local sense, reference from external
    u4Val = DRV_Reg(PMU_CONA);
    u4Val |= 0x0001;
    DRV_WriteReg(PMU_CONA, u4Val);

	//VCore 2, PMU_CONB, fix PWM mode, [13:12](RG_MODEEN_VBAT)=10, [15](RG_MODEST_VBAT)=0
	u4Val = DRV_Reg(PMU_CONB);
	u4Val &= 0x4FFF ;
	u4Val |= 0x2000;
	DRV_WriteReg(PMU_CONB, u4Val);


    //PMU_COND, set Vcore2 1.2V
    u4Val = DRV_Reg(PMU_COND);
    u4Val &= 0xFFF0 ;
    u4Val |= OUTPUT_VCORE2_130 ;
    // Vcore1/2/Vm slew-rate the slowest value
    u4Val |= 0x6000 ;
    // Remote sense enable
    // u4Val |= 0x1000 ;       
    DRV_WriteReg(PMU_COND, u4Val);


    //PMU_CONF, Set VM
    u4Val = DRV_Reg(PMU_CONF);
    u4Val |= RG_DCVCKSEL_VM;
    //VCORE1/2 and VM's DCVTRIM => 3'b000(bandgap issue)    
    u4Val &= RG_DCVTRIM_VBAT_VM_MASK;        
    DRV_WriteReg(PMU_CONF, u4Val);

	//PMU_CON10, Set VM
	DRV_WriteReg(PMU_CON10, 0x0380);

	//PMU_CON11, Set VM
	DRV_WriteReg(PMU_CON11, 0xD484);

    //PMU_CON13
    //Vcore1/2/Vm slew-rate the slowest value
    u4Val = DRV_Reg(PMU_CON13);
    u4Val |= 0x6000;
    DRV_WriteReg(PMU_CON13, u4Val);
    
    //PMU_CON17, VCAMD, 1.5V
    hwPowerOn(MT3351_POWER_CAMD, VOL_1500);

    //PMU_CON1A, VCAMA, 2.8V
    hwPowerOn(MT3351_POWER_CAMA, VOL_2800);

    //PMU_CON1C, VGP
    hwPowerOn(MT3351_POWER_GP, VOL_3300);    

    //PMU_CON1D, VSDIO
    //hwPowerOn(MT3351_POWER_SDIO, VOL_3300);        

	//PMU_CON1F, Set UVLO threashold
	DRV_WriteReg(PMU_CON26, 0);
	DRV_WriteReg(PMU_CON26, RG_CHR_EN_CODE);	
	u4Val = DRV_Reg(PMU_CON1F);
	u4Val = ((u4Val & 0xFFFC) | UV_SEL_2600 );
	DRV_WriteReg(PMU_CON1F, u4Val);


    //PMU_CON22, Enable BATSNS/ISENSE voltage divider 2
    u4Val = DRV_Reg(PMU_CON22);
    u4Val |= (RG_ISENSE_OUT_EN | RG_VBAT_OUT_EN);
    DRV_WriteReg(PMU_CON22, u4Val);

    //PMU_CON26, fine tune CV calibration gain voltage according to Vref
    DRV_WriteReg(PMU_CON26, RG_CHR_EN_CODE);
    u4Val = DRV_Reg(PMU_CON1E);
    u4Val |= 0x0A00;
    DRV_WriteReg(PMU_CON1E, u4Val);

    // Init SRAM settings
    DRV_WriteReg32(0xF0001600, 0x1540540c);

    u4Val = DRV_Reg32(0xF002C07c);
    u4Val |= 0x10;
    DRV_WriteReg(0xF002C07c, u4Val);
    //dbg_print("Set PMU_CTL for activing the core power.\r\n\n");
    
    // Set ARM and Bus delay, depending on VCORE1 and VCORE2 voltage
    // For ES, 416 or 468, VCORE1/VCORE2 = 1.5/1.2V, Set delay 7 clocks
    // For MP, 416 or 468, VCORE1/VCORE2 = 1.5/1.2V, Set delay 5 clocks
    // For MP, 416 or 468, VCORE1/VCORE2 = 1.5/1.3V, Set delay 6 clocks
    switch (g_HW_Ver)
    {
        case A68351B :
            Set_CLK_DLY(7);
        break;    

        case B68351B :
            Set_CLK_DLY(5);
        break;    

        case B68351D :
            Set_CLK_DLY(4);
        break;    

        case B68351E :
            Set_CLK_DLY(4);
        break;    

        default:
            ASSERT(0);
    }    

    
    // enable ARM clock gating, save power when bus is idle
    ARM_CLK_GATE_Enable(KAL_TRUE);
    
    DVT_DELAYMACRO(1000);
    // init Linux power management
	suspend_set_ops(&mt3351_pm_ops);    
    // init ARM clock gating for power saving
    ARM_CLK_GATE_Enable(TRUE);
    // init AHB and APB DCM for power saving    
    CONFIG_DCM(TRUE, AHB_DIVIDE_1_APB_DIVIDE_1);
    //DVFS init
    DVFS_init();
}   

