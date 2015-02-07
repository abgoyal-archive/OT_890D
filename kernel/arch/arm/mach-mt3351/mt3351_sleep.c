
#include    <mach/mt3351_sleep.h>
#include    <mach/mt3351_typedefs.h>
#include    <mach/mt3351_reg_base.h>
#include    <linux/spinlock.h>
#include    <asm/io.h>
#include    <linux/delay.h>
#include    <linux/pm.h>

BOOL
SLPCTL_Set_Sleep_Pause_Duration(UINT32 millisec)
{
	UINT32 uints, u4Val;
    //20090204, The original is list below, we remove floating operation
    //uints = (UINT32)((millisec * 1000) / 30.5);
	uints = (UINT32)((millisec * 10000) / 305);
	if (uints > 0x7FFFF)
		uints = 0x7FFFF;
	DRV_WriteReg(SLP_PAUSE_H, (uints >> 16));	
	DRV_WriteReg(SLP_PAUSE_L, (uints & SLP_PAUSE_L_MASK));
	
	if (millisec == 0) 
	{
    	u4Val = DRV_Reg(SLP_CFG);
    	u4Val &= ~SLEEP_CNT_EN;
    	DRV_WriteReg(SLP_CFG, u4Val);	    
	}
	else 
	{
    	u4Val = DRV_Reg(SLP_CFG);
    	u4Val |= SLEEP_CNT_EN;
    	DRV_WriteReg(SLP_CFG, u4Val);	    
	}

	return TRUE;
}

BOOL
SLPCTL_Set_Ext_CLK_Settle_Time(UINT32 units)
{
	if (units > SLP_ECLK_SETTLE_MASK)
		units = SLP_ECLK_SETTLE_MASK;

	DRV_WriteReg(SLP_ECLK_SETTLE, units);

	return TRUE;
}

BOOL
SLPCTL_Set_Final_Sleep_Pause_Counter(UINT32 uints)
{
	uints &= (SLP_FPAUSE_L_MASK  | (SLP_FPAUSE_H_MASK << 16) );

	DRV_WriteReg(SLP_FPAUSE_H, (uints >> 16));
	DRV_WriteReg(SLP_FPAUSE_L, (uints & SLP_FPAUSE_L));
	
	return TRUE;
}

void SLPCTL_Set_Pause_Start(BOOL start)
{  
	if (start)
		DRV_WriteReg(SLP_CNTL, SLP_CNTL_START);
	else
		DRV_WriteReg(SLP_CNTL, ~SLP_CNTL_START);		

	return;
}


UINT16
SLPCTL_Get_Pause_Status()
{
	UINT16 status;	
  	status = DRV_Reg(SLP_STAT);	
	return status;
}

void
SLPCTL_Reset_WakeUp_Source()
{
	if (DRV_Reg(SLP_CFG) & SLEEP_CNT_EN) 
	{
		DRV_WriteReg(SLP_CFG, 0);
		DRV_WriteReg(SLP_CFG, SLEEP_CNT_EN);
	}
	else 
	{
		DRV_WriteReg(SLP_CFG, 0);				
	}
}

void
SLPCTL_Set_WakeUp_Source(WAKE_SOURCE Source)
{	
    UINT16 status;
    status = DRV_Reg(SLP_CFG);
    DRV_WriteReg(SLP_CFG, status|Source);    
}

UINT16
SLPCTL_Get_WakeUp_Source()
{	
	return (UINT16)DRV_Reg(SLP_CFG);
}

void
SLPCTL_Clear_WakeUp_Source(WAKE_SOURCE Source)
{	
    UINT16 status;
    status = DRV_Reg(SLP_CFG);
    status &= ~Source;
    DRV_WriteReg(SLP_CFG, status);    
}

void 
SLPCTL_Set_PLL_WakeUp_Reset(BOOL enable, UINT32 units)
{
	UINT16 config;
	
	if (units > PLL_RESET_WIDTH_MASK)
		units = PLL_RESET_WIDTH_MASK;

	config = units;
	if(enable)
		config |= PLL_RESET_EN;
	else
		config &= ~ PLL_RESET_EN;
    DRV_WriteReg(WAKE_PLL, config);    
	
}
