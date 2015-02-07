

#include <linux/autoconf.h>
#include <mach/mt3351_typedefs.h>
#include <mach/mt3351_reg_base.h>
#include <mach/mt3351_pll.h>
#include <mach/mt3351_pdn_sw.h>
#include <mach/mt3351_pdn_hw.h>

#define PLL_SETTING_DELAY           1000


#if defined(CONFIG_MT3351_CPU_416MHZ_MCU_104MHZ)
const EMI_1X_PLL g_emi1xPLL = EMI_1X_PLL_416MHZ;
#elif defined(CONFIG_MT3351_CPU_468MHZ_MCU_117MHZ)
const EMI_1X_PLL g_emi1xPLL = EMI_1X_PLL_468MHZ;
#elif defined(CONFIG_MT3351_CPU_208MHZ_MCU_104MHZ)
const EMI_1X_PLL g_emi1xPLL = EMI_1X_PLL_208MHZ;
#else
const EMI_1X_PLL g_emi1xPLL = EMI_1X_PLL_52MHZ;
#endif


extern UINT32 g_HW_Ver;
extern UINT32 g_FW_Ver;
extern UINT32 g_HW_Code;    


// MPLL

void MPLL_Power_UP(kal_bool enable)
{
	kal_uint16 tmp = DRV_Reg(PDN_CON);
	if(enable) // power up MPLL
	    tmp &= ~MPLL_PWDB;
	else //power down MPLL
	    tmp |= MPLL_PWDB;	    	    
	DRV_WriteReg(PDN_CON,tmp);
	return ;
}

void MPLL_Reset(void)
{
	kal_uint16 tmp = DRV_Reg(MPLL_CON);
	tmp |= MPLL_RST;
	DRV_WriteReg(MPLL_CON,tmp);
	return ;
}

// APLL

void APLL_Reset(void)
{
	kal_uint16 tmp = DRV_Reg(APLL_CON0);
	tmp |= APPLL_RST;
	DRV_WriteReg(APLL_CON0,tmp);
	return ;
}

void APLL_set_CLK(APLL_FREQ APLL_CLK)
{
	kal_uint16 tmp = DRV_Reg(APLL_CON0);
	tmp &= ~APPLL_DIV_CTRL_MASK;
	tmp |= (APLL_CLK << 5);
	DRV_WriteReg(APLL_CON0,tmp);
	return ;
}

void APLL_Power_UP(kal_bool enable)
{
	kal_uint16 tmp = DRV_Reg(APLL_CON1);
	if(enable) // power APLL
	    tmp &= ~APLL_PWDB;
	else //power down APLL
	    tmp |= APLL_PWDB;	    	    
	DRV_WriteReg(APLL_CON1,tmp);
	return ;
}

// LPLL

void LPLL_Reset(void)
{
	kal_uint16 tmp = DRV_Reg(LPLL_CON0);
	tmp |= LPLL_RST;
	DRV_WriteReg(LPLL_CON0,tmp);
	return ;
}

void LPLL_Power_UP(kal_bool enable)
{
	kal_uint16 tmp = DRV_Reg(LPLL_CON1);
	if(enable) // power up LPLL
	    tmp &= ~LPLL_PWDB;
	else //power down LPLL
	    tmp |= LPLL_PWDB;	    	    
	DRV_WriteReg(LPLL_CON1,tmp);
	return ;
}

// UPLL

void UPLL_Power_UP(kal_bool enable)
{
	kal_uint16 tmp = DRV_Reg(PDN_CON);
	if(enable) // power up UPLL
	    tmp &= ~UPLL_PWDB;
	else //power down UPLL
	    tmp |= UPLL_PWDB;	    	    
	DRV_WriteReg(PDN_CON,tmp);
	return ;
}

void UPLL_Reset(void)
{
	kal_uint16 tmp = DRV_Reg(UPLL_CON);
	tmp |= UPLL_RST;
	DRV_WriteReg(UPLL_CON,tmp);
	return ;
}


void Config_PLL
(
	MCU_MODE 			eMode , 
	ARM_CLK_DIVISOR 	eARMCLK ,
	EMI_CLK_DIVISOR 	eEMICLK ,
	MCU_CLK_SRC 		eCLKSRC ,
	APLL_FREQ			eAPLLFrequency
)
{
	UINT32 u4Freq = 0, u4EMIDiv = 0, u4ARMdiv = 0, u4Val = 0;

    if (g_HW_Ver >= B68351B)
    {
        u4Val = DRV_Reg32(HW_MISC);
        // Make the ARM11 bus clock enable tie at 1 in DVFS_ASYNC mode
        u4Val |= ( CKEN_HOLD_EN | FAST_EN );
    	DRV_WriteReg32(HW_MISC, u4Val);
    }	
	if((eEMICLK == EMI_DIV_1) && (eARMCLK == ARM_DIV_4))
	{
		//dbg_print ("Invalid ARM and EMI divisor setting \n\r");
		while(1);
	}	
	if(eMode == MCU_ASYNC_MODE)
	{
	    //MCU_CurrentSettings.u4MCUMode = MCU_ASYNC_MODE;
		if(eCLKSRC == MCU_CLK_APLL)
		{
			switch(eAPLLFrequency)
			{
				case APLL_312: u4Freq = APLL_312MHZ; break;
				case APLL_338: u4Freq = APLL_338MHZ; break;
				case APLL_364: u4Freq = APLL_364MHZ; break;
				case APLL_390: u4Freq = APLL_390MHZ; break;
				case APLL_416: u4Freq = APLL_416MHZ; break;
				case APLL_442: u4Freq = APLL_442MHZ; break;
				case APLL_468: u4Freq = APLL_468MHZ; break;
				case APLL_494: u4Freq = APLL_494MHZ; break;
				case APLL_520: u4Freq = APLL_520MHZ; break;
				default :
					//dbg_print ("Invalid setting..APLL setting wrong \n\r");
					while(1);
				break;
			}
		}
		switch(eEMICLK)
		{
			case EMI_DIV_1: u4EMIDiv = 1; break;
			case EMI_DIV_2: u4EMIDiv = 2; break;
			case EMI_DIV_4: u4EMIDiv = 4; break;					
		}				
		switch(eARMCLK)
		{
			case ARM_DIV_1: u4ARMdiv = 1; break;
			case ARM_DIV_2: u4ARMdiv = 2; break;
			case ARM_DIV_1_3: u4ARMdiv = 3; break; // 3x, basex3
			case ARM_DIV_4: u4ARMdiv = 4; break;					
		}		
		switch (eCLKSRC)
		{
			case MCU_CLK_APLL :
			{

				//compare MCU clock and MPLL/2
				if ((u4Freq/eEMICLK/2) < MPLL_208MHZ/2)
				{
					//dbg_print ("Invalid setting..MCU clock cannot lower than MPLL/2 in ASYNC mode 
					// \n\r");
					while(1);
					return;
				}
				// 1. enable APLL and MPLL, wait PLL stable
				ConfigGMCAsyncMode();
				MPLL_Power_UP(KAL_TRUE);	
				APLL_Power_UP(KAL_TRUE);
				APLL_set_CLK(eAPLLFrequency);
				// 2. Set Async mode
				DVT_DELAYMACRO(PLL_SETTING_DELAY);
				Set_MCU_MM_Mode(eMode);
				// 3. set EMI and ARM divisor
				//DVT_DELAYMACRO(PLL_SETTING_DELAY);
				// 3. set EMI and ARM divisor
				Set_CLK_CONTROL(eEMICLK , eARMCLK);
				// 4. choose ARM and MM clock source
				//DVT_DELAYMACRO(PLL_SETTING_DELAY);				
				Choose_ARM_CLK_SRC(eCLKSRC);
				DVT_DELAYMACRO(PLL_SETTING_DELAY);
				Choose_MM_CLK_SRC(MPLL_DIVIDE_2);
                // Set APLL_DCC_EN
				// Suggest when CPU use clock >=416 MHz, turn it on
				// This will improve the clock quality of the ARM11 
				// and may help the ARM11 runs more stable @ high frequency.
                if (eAPLLFrequency >= APLL_416)
                {
                    u4Val = DRV_Reg(APLL_CON1);
                    u4Val |= APLL_DCC_EN;	    	    
                	DRV_WriteReg(APLL_CON1,u4Val);
                }
			}
			break;

			case MCU_CLK_MPLL :
				if ((eEMICLK == EMI_DIV_1) && (eARMCLK == ARM_DIV_1))
				{
					u4Freq = MPLL_208MHZ;
					u4EMIDiv = 1;
					// 1. enable APLL and MPLL, wait PLL stable
					ConfigGMCAsyncMode();
					MPLL_Power_UP(KAL_TRUE);	
					APLL_Power_UP(KAL_TRUE);				
					// 2. Set Async mode
					DVT_DELAYMACRO(PLL_SETTING_DELAY);
					Set_MCU_MM_Mode(eMode);
					// 3. set EMI and ARM divisor
					Set_CLK_CONTROL(eEMICLK , eARMCLK);
					// 4. choose ARM and MM clock source
					Choose_ARM_CLK_SRC(eCLKSRC);
					Choose_MM_CLK_SRC(MPLL_DIVIDE_2);								
				}
				else
				{
					//dbg_print ("Invalid setting..use MPLL at ASYNC mode, ARM and EMI div must set div1 \n\r");
					while(1);
					return;
				}
			break;
			
			case MCU_CLK_UPLL :
				//dbg_print ("Invalid setting..can't use UPLL at ASYNC mode!! \n\r");
				while(1);
				return;
			break;

			case MCU_CLK_EXTERNAL :
				//dbg_print ("Invalid setting..can't use External at ASYNC mode!! 
				// \n\r");
				while(1);
				return;
			break;

			default:
			break;
		}

	}
	else // SYNC mode
	{
		if(eCLKSRC == MCU_CLK_APLL)
		{
			switch(eAPLLFrequency)
			{
				case APLL_312: u4Freq = APLL_312MHZ; break;
				case APLL_338: u4Freq = APLL_338MHZ; break;
				case APLL_364: u4Freq = APLL_364MHZ; break;
				case APLL_390: u4Freq = APLL_390MHZ; break;
				case APLL_416: u4Freq = APLL_416MHZ; break;
				default :
					//dbg_print ("Invalid setting..APLL setting wrong \n\r");
					while(1);
				break;
			}
		}
		switch(eEMICLK)
		{
			case EMI_DIV_1: u4EMIDiv = 1; break;
			case EMI_DIV_2: u4EMIDiv = 2; break;
			case EMI_DIV_4: u4EMIDiv = 4; break;					
		}				
		switch(eARMCLK)
		{
			case ARM_DIV_1: u4ARMdiv = 1; break;
			case ARM_DIV_2: u4ARMdiv = 2; break;
			case ARM_DIV_1_3: u4ARMdiv = 3; break;			
			case ARM_DIV_4: u4ARMdiv = 4; break;					
		}		
			
		switch (eCLKSRC)
		{
			case MCU_CLK_APLL :
				APLL_Power_UP(KAL_TRUE);
				MPLL_Power_UP(KAL_TRUE);	
				APLL_set_CLK(eAPLLFrequency);
				// 2. Set Async mode
				DVT_DELAYMACRO(PLL_SETTING_DELAY);
				Set_MCU_MM_Mode(eMode);
				DVT_DELAYMACRO(PLL_SETTING_DELAY);
				// set EMI and ARM divisor
				Set_CLK_CONTROL(eEMICLK , eARMCLK);
				// choose clock source
				DVT_DELAYMACRO(PLL_SETTING_DELAY);
				Choose_ARM_CLK_SRC(eCLKSRC);
				DVT_DELAYMACRO(PLL_SETTING_DELAY);
				Choose_MM_CLK_SRC(MCU_CLK_SOURCE);				
			break;

			case MCU_CLK_MPLL :
				u4Freq = MPLL_208MHZ;
				MPLL_Power_UP(KAL_TRUE);	
				// 2. Set Async mode
				DVT_DELAYMACRO(PLL_SETTING_DELAY);
				Set_MCU_MM_Mode(eMode);
				// set EMI and ARM divisor
				Set_CLK_CONTROL(eEMICLK , eARMCLK);
				// choose clock source
				Choose_ARM_CLK_SRC(eCLKSRC);
				Choose_MM_CLK_SRC(MCU_CLK_SOURCE);				
			break;

			case MCU_CLK_UPLL :
				u4Freq = UPLL_48MHZ;
				UPLL_Power_UP(KAL_TRUE);
				DVT_DELAYMACRO(PLL_SETTING_DELAY);
				Set_MCU_MM_Mode(eMode);
				// set EMI and ARM divisor
				Set_CLK_CONTROL(eEMICLK , eARMCLK);
				// choose clock source
				Choose_ARM_CLK_SRC(eCLKSRC);
				Choose_MM_CLK_SRC(MCU_CLK_SOURCE);				
				// AHB and APB run the same clk at UPLL clk mode		
			break;

			case MCU_CLK_EXTERNAL :
				u4Freq = EXT_26MHZ;
				Set_MCU_MM_Mode(eMode);
				// set EMI and ARM divisor
				Set_CLK_CONTROL(eEMICLK , eARMCLK);
				// choose clock source
				Choose_ARM_CLK_SRC(eCLKSRC);
				Choose_MM_CLK_SRC(MCU_CLK_SOURCE);				
				MPLL_Power_UP(KAL_FALSE);	
				APLL_Power_UP(KAL_FALSE);				
				// AHB and APB run the same clk at external clk mode
			break;

		}
	}
	return ;
}


void MT3351_PLL_Init(void)
{
	UINT32 i;
	UINT16 tmp;

    // Use the default settings if EMI PLL is 13MHz
    //
    if (g_emi1xPLL == EMI_1X_PLL_13MHZ) return;

    // 1. we start up using SYNC mode, enable APLL and MPLL
    MPLL_Power_UP(KAL_TRUE);    
    APLL_Power_UP(KAL_TRUE);

    // 2. wait PLL stable
    for (i = 0; i < 1000; ++i) ;

	// 3. set EMI / ARM divisor and choose the clock source
    switch(g_emi1xPLL)
    {
    case EMI_1X_PLL_52MHZ:
        Set_CLK_CONTROL(EMI_DIV_2 , ARM_DIV_1);
        Choose_ARM_CLK_SRC(MCU_CLK_MPLL);
        break;
    case EMI_1X_PLL_78MHZ:
        Set_CLK_CONTROL(EMI_DIV_1 , ARM_DIV_1);
        DRV_WriteReg(0x80060000,0xA);   
        Choose_ARM_CLK_SRC(MCU_CLK_MPLL);
        break;
    case EMI_1X_PLL_104MHZ:
        Set_CLK_CONTROL(EMI_DIV_1 , ARM_DIV_1);
        Choose_ARM_CLK_SRC(MCU_CLK_MPLL);
        break;
    case EMI_1X_PLL_117MHZ:
        Set_CLK_CONTROL(EMI_DIV_2 , ARM_DIV_2);
        tmp = DRV_Reg(APLL_CON0);
        tmp = (tmp & ~APPLL_DIV_CTRL_MASK) | 0x200;
        DRV_WriteReg(APLL_CON0,tmp);	
        Choose_ARM_CLK_SRC(MCU_CLK_APLL);
        break;
    /* Infinity, 20080905, FIXME. Temporary Solution { */       
	case EMI_1X_PLL_208MHZ:
        Config_PLL(MCU_SYNC_MODE, ARM_DIV_2, EMI_DIV_2, MCU_CLK_APLL,
                   APLL_416);		
		break;
	case EMI_1X_PLL_416MHZ:
        Config_PLL(MCU_SYNC_MODE, ARM_DIV_1, EMI_DIV_2, MCU_CLK_APLL,
                   APLL_416);	
		break;
	/* Kelvin, 20081209 ASYNC 468*/	
	case EMI_1X_PLL_468MHZ:
        Config_PLL(MCU_ASYNC_MODE, ARM_DIV_1, EMI_DIV_2, MCU_CLK_APLL,
                   APLL_468);	
		break;
		
    /* Infinity, 20080905, FIXME. Temporary Solution } */
    default:
        while(1);
    }

	return;
}

