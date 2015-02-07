

#include <linux/autoconf.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/module.h>

#include <mach/mt6516_typedefs.h>
#include <mach/mt6516_pll.h>
#include <mach/mt6516_ap_config.h>
#include <mach/mt6516_apmcusys.h>
#include <mach/mt6516_graphsys.h>


//#define ENABLE_SLOWIDLE     1
static spinlock_t APMCUSYS_PDN0lock = SPIN_LOCK_UNLOCKED, APMCUSYS_PDN1lock = SPIN_LOCK_UNLOCKED;
static spinlock_t GRAPH1SYS_PDNlock = SPIN_LOCK_UNLOCKED, GRAPH2SYS_PDNlock = SPIN_LOCK_UNLOCKED;


extern UINT32 g_ChipVer;
extern void MT6516_CPUIdle(UINT32 SlowIdle, UINT32 AHB_ADDR);
extern unsigned long count_timer_tick ;
extern ssize_t	mt6326_VCORE_1_set_1_10(void);
extern ssize_t	mt6326_VCORE_1_set_1_3(void);



BOOL MT6516_APMCUSYS_GetPDN0Status (APMCUSYS_PDNCONA0_MODE mode)
{
	spin_lock(&APMCUSYS_PDN0lock);
	
	if(DRV_Reg32(APMCUSYS_PDN_CON0) & (1 << mode))
	{      
	    spin_unlock(&APMCUSYS_PDN0lock);
		return TRUE;    // this module is POWER DOWN
	}
	else
	{     
	    spin_unlock(&APMCUSYS_PDN0lock);
		return FALSE;   // this module is POWER UP
	}
	
}

BOOL MT6516_APMCUSYS_GetPDN1Status (APMCUSYS_PDNCONA1_MODE mode)
{
	spin_lock(&APMCUSYS_PDN1lock);
	if(DRV_Reg32(APMCUSYS_PDN_CON1) & (1 << mode))
	{     
	    spin_unlock(&APMCUSYS_PDN1lock);
	    return TRUE;    // this module is POWER DOWN
	}
	else
	{     
	    spin_unlock(&APMCUSYS_PDN1lock);
		return FALSE;   // this module is POWER UP
	}	
	
}

void MT6516_APMCUSYS_EnablePDN0 (APMCUSYS_PDNCONA0_MODE mode)
{
    spin_lock(&APMCUSYS_PDN0lock);
    DRV_SetReg32(APMCUSYS_PDN_SET0, (1 << mode)); // power down
    spin_unlock(&APMCUSYS_PDN0lock);
}

void MT6516_APMCUSYS_EnablePDN1 (APMCUSYS_PDNCONA1_MODE mode)
{
    spin_lock(&APMCUSYS_PDN1lock);
    DRV_SetReg32(APMCUSYS_PDN_SET1, (1 << mode)); // power down
    spin_unlock(&APMCUSYS_PDN1lock);
}

void MT6516_APMCUSYS_DisablePDN0 (APMCUSYS_PDNCONA0_MODE mode)
{
    spin_lock(&APMCUSYS_PDN0lock);
    DRV_SetReg32(APMCUSYS_PDN_CLR0, (1 << mode)); // power up
    spin_unlock(&APMCUSYS_PDN0lock);
}

void MT6516_APMCUSYS_DisablePDN1 (APMCUSYS_PDNCONA1_MODE mode)
{
    spin_lock(&APMCUSYS_PDN1lock);
    DRV_SetReg32(APMCUSYS_PDN_CLR1, (1 << mode)); // power up
    spin_unlock(&APMCUSYS_PDN1lock);
}




BOOL MT6516_GRAPH1SYS_GetPDNStatus (GRAPH1SYS_PDN_MODE mode)
{
	spin_lock(&GRAPH1SYS_PDNlock);
	
	if(DRV_Reg32(GRAPH1SYS_CG_CON) & (1 << mode))
	{      
	    spin_unlock(&GRAPH1SYS_PDNlock);
		return TRUE;    // this module is POWER DOWN
	}
	else
	{     
	    spin_unlock(&GRAPH1SYS_PDNlock);
		return FALSE;   // this module is POWER UP
	}
	
}

void MT6516_GRAPH1SYS_EnablePDN (GRAPH1SYS_PDN_MODE mode)
{
    spin_lock(&GRAPH1SYS_PDNlock);
    DRV_SetReg32(GRAPH1SYS_CG_SET, (1 << mode)); // power down
    spin_unlock(&GRAPH1SYS_PDNlock);
}


void MT6516_GRAPH1SYS_DisablePDN (GRAPH1SYS_PDN_MODE mode)
{
    spin_lock(&GRAPH1SYS_PDNlock);
    DRV_SetReg32(GRAPH1SYS_CG_CLR, (1 << mode)); // power up
    spin_unlock(&GRAPH1SYS_PDNlock);
}

BOOL MT6516_GRAPH2SYS_GetPDNStatus (GRAPH2SYS_PDN_MODE mode)
{
	spin_lock(&GRAPH2SYS_PDNlock);
	
	if(DRV_Reg32(GRAPH2SYS_CG_CON) & (1 << mode))
	{      
	    spin_unlock(&GRAPH2SYS_PDNlock);
		return TRUE;    // this module is POWER DOWN
	}
	else
	{     
	    spin_unlock(&GRAPH2SYS_PDNlock);
		return FALSE;   // this module is POWER UP
	}
	
}

void MT6516_GRAPH2SYS_EnablePDN (GRAPH2SYS_PDN_MODE mode)
{
    spin_lock(&GRAPH2SYS_PDNlock);
    DRV_SetReg32(GRAPH2SYS_CG_SET, (1 << mode)); // power down
    spin_unlock(&GRAPH2SYS_PDNlock);
}


void MT6516_GRAPH2SYS_DisablePDN (GRAPH2SYS_PDN_MODE mode)
{
    spin_lock(&GRAPH2SYS_PDNlock);
    DRV_SetReg32(GRAPH2SYS_CG_CLR, (1 << mode)); // power up
    spin_unlock(&GRAPH2SYS_PDNlock);
}





static spinlock_t AP_HWMISClock = SPIN_LOCK_UNLOCKED;

 /******************************************************************************
 * MT6516_AP_SetHWMISC
 * 
 * DESCRIPTION:
 *   set some field of  HW_MISC as 1
 *
 * PARAMETERS: 
 *   e.g USB_SEL | GMC_AUTOCG | UART1_SEL ....
 * 
 * RETURNS: 
 *   None
 * 
 ******************************************************************************/
void MT6516_AP_EnableHWMISC (UINT32 value)
{
      spin_lock(&AP_HWMISClock);
      DRV_SetReg32(HW_MISC,value);
      spin_unlock(&AP_HWMISClock);      
}

 /******************************************************************************
 * MT6516_AP_SetHWMISC
 * 
 * DESCRIPTION:
 *   set some field of  HW_MISC as 0
 *
 * PARAMETERS: 
 *   e.g USB_SEL | GMC_AUTOCG | UART1_SEL ....
 * 
 * RETURNS: 
 *   None
 * 
 ******************************************************************************/
void MT6516_AP_DisableHWMISC (UINT32 value)
{
      spin_lock(&AP_HWMISClock);
      DRV_ClrReg32(HW_MISC,value);
      spin_unlock(&AP_HWMISClock);            
}

static spinlock_t AP_MCUMEMPDNlock = SPIN_LOCK_UNLOCKED;


void MT6516_AP_EnableMCUMEMPDN (MCUMEM_PDNCONA_MODE mode)
{
      spin_lock(&AP_MCUMEMPDNlock);
      DRV_SetReg32(MCU_MEM_PDN, (1 << mode));
      spin_unlock(&AP_MCUMEMPDNlock);
}

void MT6516_AP_DisableMCUMEMPDN (MCUMEM_PDNCONA_MODE mode)
{
      spin_lock(&AP_MCUMEMPDNlock);
      DRV_ClrReg32(MCU_MEM_PDN, (1 << mode));
      spin_unlock(&AP_MCUMEMPDNlock);
}

static spinlock_t AP_G1MEMPDNlock = SPIN_LOCK_UNLOCKED;

void MT6516_AP_EnableG1MEMPDN (G1_PDNCONA_MODE mode)
{
      spin_lock(&AP_G1MEMPDNlock);
      DRV_SetReg32(G1_MEM_PDN, (1 << mode));
      spin_unlock(&AP_G1MEMPDNlock);
}

void MT6516_AP_DisableG1MEMPDN (G1_PDNCONA_MODE mode)
{
      spin_lock(&AP_G1MEMPDNlock);
      DRV_ClrReg32(G1_MEM_PDN, (1 << mode));
      spin_unlock(&AP_G1MEMPDNlock);
}

static spinlock_t AP_G2MEMPDNlock = SPIN_LOCK_UNLOCKED;

void MT6516_AP_EnableG2MEMPDN (G2_PDNCONA_MODE mode)
{
      spin_lock(&AP_G2MEMPDNlock);
      DRV_SetReg32(G2_MEM_PDN, (1 << mode));
      spin_unlock(&AP_G2MEMPDNlock);
}

void MT6516_AP_DisableG2MEMPDN (G2_PDNCONA_MODE mode)
{
      spin_lock(&AP_G2MEMPDNlock);
      DRV_ClrReg32(G2_MEM_PDN, (1 << mode));
      spin_unlock(&AP_G2MEMPDNlock);
}

static spinlock_t AP_CEVAMEMPDNlock = SPIN_LOCK_UNLOCKED;

void MT6516_AP_EnableCEVAMEMPDN (CEVA_PDNCONA_MODE mode)
{
      spin_lock(&AP_CEVAMEMPDNlock);
      DRV_SetReg32(CEVA_MEM_PDN, (1 << mode));
      spin_unlock(&AP_CEVAMEMPDNlock);
}

void MT6516_AP_DisableCEVAMEMPDN (CEVA_PDNCONA_MODE mode)
{
      spin_lock(&AP_CEVAMEMPDNlock);
      DRV_ClrReg32(CEVA_MEM_PDN, (1 << mode));
      spin_unlock(&AP_CEVAMEMPDNlock);
}

static spinlock_t AP_ARMFreqlock = SPIN_LOCK_UNLOCKED;

void MT6516_AP_SetARMFreq (ARM9_FREQ_DIV_ENUM freq)
{
      spin_lock(&AP_ARMFreqlock);
      DRV_WriteReg32(ARM9_FREQ_DIV, freq);
      spin_unlock(&AP_ARMFreqlock);
}

ARM9_FREQ_DIV_ENUM MT6516_AP_GetARMFreq (void)
{
      return DRV_Reg32(ARM9_FREQ_DIV);
}

static spinlock_t AP_SLEEPCONlock = SPIN_LOCK_UNLOCKED;

void MT6516_APMCUSYS_EnableSLEEPCON (UINT32 value)
{
	spin_lock(&AP_SLEEPCONlock);	
	DRV_SetReg32(SLEEP_CON,value);
	spin_unlock(&AP_SLEEPCONlock);
}

void MT6516_APMCUSYS_DisableSLEEPCON (UINT32 value)
{
	spin_lock(&AP_SLEEPCONlock);	
	DRV_ClrReg32(SLEEP_CON,value);
	spin_unlock(&AP_SLEEPCONlock);
}

static spinlock_t AP_MCUFreqlock = SPIN_LOCK_UNLOCKED;

void MT6516_AP_SetMCUFreq (MCU_FSEL_ENUM freq)
{
      spin_lock(&AP_MCUFreqlock);
      DRV_WriteReg32(MCUCLK_CON, freq);
      spin_unlock(&AP_MCUFreqlock);
}

MCU_FSEL_ENUM MT6516_AP_GetMCUFreq (void)
{
      return DRV_Reg32(MCUCLK_CON);
}

static spinlock_t AP_EMIFreqlock = SPIN_LOCK_UNLOCKED;

void MT6516_AP_SetEMIFreq (EMI_FREQ freq)
{
      spin_lock(&AP_EMIFreqlock);
      DRV_WriteReg32(EMICLK_CON, freq);
      spin_unlock(&AP_EMIFreqlock);
}

EMI_FREQ MT6516_AP_GetEMIFreq (void)
{
      return DRV_Reg32(EMICLK_CON);
}

static spinlock_t AP_APBReadCyclelock = SPIN_LOCK_UNLOCKED;


void MT6516_AP_SetAPBReadCycle (APBR readcycle)
{
      spin_lock(&AP_APBReadCyclelock);
      DRV_ClrReg32(APB_CON, 0xff);
      DRV_SetReg32(APB_CON, readcycle);
      spin_unlock(&AP_APBReadCyclelock);
}

APBR MT6516_AP_GetAPBReadCycle (void)
{
      return (DRV_Reg32(APB_CON) & (0x00FF));
}

static spinlock_t AP_APBWriteCyclelock = SPIN_LOCK_UNLOCKED;

void MT6516_AP_SetAPBWriteCycle (APBW writecycle)
{
      spin_lock(&AP_APBWriteCyclelock);
      DRV_ClrReg32(APB_CON, 0xff00);
      DRV_SetReg32(APB_CON, writecycle);
      spin_unlock(&AP_APBWriteCyclelock);
}

APBW MT6516_AP_GetAPBWriteCycle (void)
{
      return (DRV_Reg32(APB_CON) >> 8);
}

static spinlock_t AP_ICSIZElock = SPIN_LOCK_UNLOCKED;

void MT6516_AP_SetICSIZE_32KB (void)
{
      spin_lock(&AP_ICSIZElock);
      DRV_ClrReg32(IC_SIZE,IC_SIZE_16KB);
      spin_unlock(&AP_ICSIZElock);
}

void MT6516_AP_SetICSIZE_16KB (void)
{
      spin_lock(&AP_ICSIZElock);
      DRV_WriteReg32(IC_SIZE,IC_SIZE_16KB);
      spin_unlock(&AP_ICSIZElock);
}

UINT32 MT6516_AP_GetICSIZE (void)
{
      return DRV_Reg32(IC_SIZE);
}

static spinlock_t AP_DCSIZElock = SPIN_LOCK_UNLOCKED;

void MT6516_AP_SetDCSIZE_32KB (void)
{
      spin_lock(&AP_DCSIZElock);
      DRV_ClrReg32(DC_SIZE,DC_SIZE_16KB);
      spin_unlock(&AP_DCSIZElock);
}

void MT6516_AP_SetDCSIZE_16KB (void)
{
      spin_lock(&AP_DCSIZElock);
      DRV_WriteReg32(DC_SIZE,DC_SIZE_16KB);
      spin_unlock(&AP_DCSIZElock);
}

UINT32 MT6516_AP_GetDCSIZE (void)
{
      return DRV_Reg32(DC_SIZE);
}

EXPORT_SYMBOL(MT6516_APMCUSYS_GetPDN0Status);
EXPORT_SYMBOL(MT6516_APMCUSYS_GetPDN1Status);
EXPORT_SYMBOL(MT6516_APMCUSYS_DisablePDN0);
EXPORT_SYMBOL(MT6516_APMCUSYS_DisablePDN1);

EXPORT_SYMBOL(MT6516_GRAPH1SYS_GetPDNStatus);
EXPORT_SYMBOL(MT6516_GRAPH1SYS_EnablePDN);
EXPORT_SYMBOL(MT6516_GRAPH1SYS_DisablePDN);
EXPORT_SYMBOL(MT6516_GRAPH2SYS_GetPDNStatus);
EXPORT_SYMBOL(MT6516_GRAPH2SYS_EnablePDN);
EXPORT_SYMBOL(MT6516_GRAPH2SYS_DisablePDN);


void MT6516_PLL_init(void)
{
    UINT32 u4Delay = 0;

    //Disable watchdog
    DRV_WriteReg32((RGU_BASE+0x0), 0x2220); 
    //Power on MPLL and UPLL
    DRV_WriteReg32(PDN_CON, 0x1E);
    //Power on CEVAPLL
    DRV_WriteReg32(CEVAPLL2, 0x1E);
    //switch to 13MHZ for PLL input frequency
    DRV_WriteReg32(CLK_CON, 0x83);

    for (u4Delay = 0; u4Delay < 0xFF; u4Delay++ );
    //Reset UPLL
    DRV_WriteReg32(UPLL, 0x80);
    //Reset MPLL
    DRV_WriteReg32(MPLL, 0x80);
    //Reset CEVAPLL
    DRV_WriteReg32(CEVAPLL, 0x800);
    //Release UPLL Reset
    DRV_WriteReg32(UPLL, 0x0);
    //Release MPLL Reset
    DRV_WriteReg32(MPLL, 0x0);
    //Release CEVA Reset
    DRV_WriteReg32(CEVAPLL, 0x0);
    //Set FMCU_DIV_EN and FMCU_2X_DIV_EN, see spec page 65
    DRV_SetReg32(SLEEP_CON,0x0300); 
    //Polling until these two bits become 1
    while ((DRV_Reg32(SLEEP_CON)&0x0300)!= 0x0300);

    for (u4Delay = 0; u4Delay < 0xFF; u4Delay++ );
    //enable PLL output
    DRV_WriteReg32(CLK_CON, 0xF3);
}

UINT32 SetARM9Freq(ARM9_FREQ_DIV_ENUM ARM9_div)
{
    ARM9_FREQ_DIV_ENUM Old_ARM9_div;

    Old_ARM9_div = DRV_Reg32(ARM9_FREQ_DIV);

    DRV_WriteReg32(ARM9_FREQ_DIV,ARM9_div);

    while (!(ARM9_div == DRV_Reg32(ARM9_FREQ_DIV) ) ); 
    return Old_ARM9_div ;
}

void MT6516_Idle(void)
{    
    UINT32 u4PreVious_ARM9Freq;
    if (g_ChipVer != CHIP_VER_ECO_1)
    {
    	if(jiffies >= 12000)
    	{
	        u4PreVious_ARM9Freq = SetARM9Freq(DIV_4_104) ; 
	        MT6516_CPUIdle (0, 0xF0001204);
	        SetARM9Freq (u4PreVious_ARM9Freq); 
    	}
		else			
        	MT6516_CPUIdle (0, 0xF0001204);
    }
    else
    {
        //ECO 0, only enter Idle
        MT6516_CPUIdle (0, 0xF0001204);
    }
}

EXPORT_SYMBOL(MT6516_Idle);
