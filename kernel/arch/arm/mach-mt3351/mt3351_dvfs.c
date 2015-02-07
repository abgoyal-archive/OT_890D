
#include    <linux/spinlock.h>
#include    <asm/io.h>
#include    <linux/delay.h>
#include    <linux/pm.h>
#include    <mach/mt3351_pdn_hw.h>
#include    <mach/mt3351_dvfs.h>
#include    <mach/mt3351_pmu_hw.h>
#include    <mach/mt3351_typedefs.h>
#include    <mach/mt3351_reg_base.h>




#define DVFS_SW_WORKAROUND      1
#define DVFS_DELAY  500



ARM_FREQUENCY DVFS_PreARMFreq  = ARM_416_or_468;
UINT32 u4OriVcore1Vol = OUTPUT_VCORE1_155;
UINT32 g_Original_MMCKSRC = 0;

static UINT32 u4CLKDLY_MAP[9] = 
{
    CLK_DLY_0, // 1.2
    CLK_DLY_1, // 1.25
    CLK_DLY_2, // 1.3
    CLK_DLY_3, // 1.35
    CLK_DLY_3, // 1.4
    CLK_DLY_4, // 1.45 
    CLK_DLY_4, // 1.5
    CLK_DLY_4, // 1.55
    CLK_DLY_4, // 1.6

};
extern DVFS_CLKSwitch_4x();
extern DVFS_CLKSwitch_2x();
extern DVFS_CLKSwitch_1x();
extern DisableIRQ();
extern EnableIRQ();
extern DisableFIQ();
extern EnableFIQ();



DVFS_STATUS DVFS_SetAsyncMode(BOOL async_en)
{
    UINT32 config = DRV_Reg32(DVFS_CON);
    if(async_en)
        config |= DVFS_ASYNC;
    else
        config &= ~ DVFS_ASYNC;
    DRV_WriteReg32(DVFS_CON,config);
    return DVFS_STATUS_OK;  
}

BOOL DVFS_GetStatus()
{
   UINT32  reg_val;

   reg_val = DRV_Reg32(DVFS_STA);

   if (reg_val & DVFS_BUSY)
      return TRUE;
   else
      return FALSE; 
}

DVFS_STATUS DVFS_SetClkTimeOut(UINT8 timeout)
{
    timeout &= DVFS_CLK_TIMEOUT_MASK;
    DRV_WriteReg32(DVFS_CLK_TMR, timeout);    
    return DVFS_STATUS_OK;  
}

DVFS_STATUS DVFS_SetPmuTimeOut(UINT16 timeout)
{
    timeout &= DVFS_PMU_TIMEOUT_MASK;
    DRV_WriteReg32(DVFS_PMU_TMR, timeout);        
    return DVFS_STATUS_OK;  
}

DVFS_STATUS DVFS_ConfigSettings
(      
    BOOL scaling, 
    BOOL IRQ_en, 
    BOOL CLK_AU, 
    BOOL PMU_AU 
)
{
    UINT32 config = 0 ;

    if(CLK_AU)
        config |= DVFS_CKDIV_AU;
    else
        config &= ~ DVFS_CKDIV_AU;      

    if(PMU_AU)
        config |= DVFS_PMU_AU;
    else
        config &= ~ DVFS_PMU_AU;

    if(IRQ_en)
        config |= DVFS_IRQ_EN;
    else
        config &= ~ DVFS_IRQ_EN;

    //config |= DVFS_ASYNC;

    DRV_WriteReg32(DVFS_CON, config);        


    if (scaling)
    {
        config |= DVFS_UP;
        config &= ~ DVFS_DOWN;
    }
    else
    {
        config &= ~ DVFS_UP;
        config |= DVFS_DOWN;
    }
    
    DRV_WriteReg32(DVFS_CON, config);        
    return DVFS_STATUS_OK;  
}




DVFS_STATUS DVFS_init(void)
{
    //UINT32  size ;
    UINT32 u4Val, u4ClkVal;

    // Get original Vcore1 voltage
    u4Val = DRV_Reg32(PMU_CON7) ;
    u4OriVcore1Vol =  u4Val & 0xF ;
    
    // Make the ARM11 bus clock enable tie at 1 in DVFS_ASYNC mode
    u4Val = DRV_Reg32(HW_MISC) ;    
    u4Val |= CKEN_HOLD_EN;
    DRV_WriteReg32(HW_MISC, u4Val);        
    	
    return DVFS_STATUS_OK;
}




DVFS_STATUS DVFS_AdjustFrequency(ARM_FREQUENCY arm_freq )
{   
    UINT32  u4ClkVal, u4CurrentVCore1, u4OriCLKDly ;
	UINT32  u4IntValL, u4IntValH, u4Step, u4Delay, u4PMUVal;

    if ((arm_freq > ARM_416_or_468) || (arm_freq < ARM_104_or_117)) 
    {
        printk("[DVFS ERR]CPU frequency out of range, arm_freq:%d", arm_freq);
        return DVFS_STATUS_FAIL;
    }    

    if (arm_freq == DVFS_PreARMFreq)
        return DVFS_STATUS_OK;

    // save Intr setting
    //u4IntValL = *MT3351_IRQ_MASKL;
    //u4IntValH = *MT3351_IRQ_MASKH;
    // mask all interrupts
    //*MT3351_IRQ_MASKL = 0xFFFFFFFF;
    //*MT3351_IRQ_MASKH = 0xFFFFFFFF;

    // CPU frequency scale up            
    if(arm_freq > DVFS_PreARMFreq)  
    {
        /*********************************************************************
         *  Scale up step1 : Set PMU VCORE1
         *********************************************************************/        
        // adjust PMU_CON7 VCORE1 voltage depend on ARM frequency
        u4PMUVal = DRV_Reg32(PMU_CON7) ;
        u4CurrentVCore1 = u4PMUVal & 0xF ;        
		printk("scale up => before DVFS PMU : 0x%x \r\n",DRV_Reg32(PMU_CON7)); 

        
        switch (arm_freq)
        {
            case ARM_416_or_468 : 
            {
                for (u4Step = (u4CurrentVCore1+1); u4Step <= u4OriVcore1Vol; u4Step++)
                {
                    // set Vcore1 voltage
                    u4PMUVal = DRV_Reg32(PMU_CON7) ;
                    u4PMUVal &= 0x00007FF0 ;
                    u4PMUVal |= u4Step ; 
                    DRV_WriteReg32(PMU_CON7, u4PMUVal);
                    printk("up step %d PMU : 0x%x \r\n",u4Step,DRV_Reg32(PMU_CON7));                           
                    // set CLKCON delay
                    for (u4Delay = 0 ; u4Delay < DVFS_DELAY; u4Delay++);                    
                    u4ClkVal = DRV_Reg32(CLKCON) ;
                    u4OriCLKDly = u4ClkVal & 0x00700000;
                    u4ClkVal &= 0xFF8FFFFF;
                    u4ClkVal |= (u4CLKDLY_MAP[u4Step-OUTPUT_VCORE1_120]);
                    DRV_WriteReg32(CLKCON, u4ClkVal);                    
            		printk("up step %d CLKCON : 0x%x " ,u4Step,DRV_Reg32(CLKCON)); 

                }
            }
            break;
            
            case ARM_208_or_234 : 
            {
                for (u4Step = (u4CurrentVCore1+1); u4Step <= OUTPUT_VCORE1_140; u4Step++)
                {
                    // set Vcore1 voltage                    
                    u4PMUVal = DRV_Reg32(PMU_CON7);
                    u4PMUVal &= 0x00007FF0 ;
                    u4PMUVal |= u4Step ; 
                    DRV_WriteReg32(PMU_CON7, u4PMUVal);
                    printk("up step %d PMU : 0x%x \r\n",u4Step,DRV_Reg32(PMU_CON7));                           
                    // set CLKCON delay
                    for (u4Delay = 0 ; u4Delay < DVFS_DELAY; u4Delay++);
                    u4ClkVal = DRV_Reg32(CLKCON) ;
                    u4OriCLKDly = u4ClkVal & 0x00700000;
                    u4ClkVal &= 0xFF8FFFFF;	
                    u4ClkVal |= (u4CLKDLY_MAP[u4Step-OUTPUT_VCORE1_120]);
                    DRV_WriteReg32(CLKCON, u4ClkVal);
            		printk("up step %d CLKCON : 0x%x " ,u4Step,DRV_Reg32(CLKCON)); 

                }
            }                            
            break;

            default : 
                return DVFS_STATUS_FAIL;
            break;
        }                
		printk("scale up => after DVFS PMU : 0x%x \r\n", DRV_Reg32(PMU_CON7)); 				

        /******************************************************************************
         * Scale up step2 : Set ARM Div
         *****************************************************************************/
        u4ClkVal = DRV_Reg32(CLKCON) ;
		printk("scale up => before DVFS CLKCON : 0x%x \r\n", DRV_Reg32(CLKCON)); 
		switch (arm_freq)
        {   
            case ARM_416_or_468 :
                // 1. disable interrupt
			    DisableIRQ();
                DVFS_CLKSwitch_4x();
				EnableIRQ();
			break;

            case ARM_208_or_234 :
				DisableIRQ();
                DVFS_CLKSwitch_2x();
				EnableIRQ();
			break;

            default : 
                return DVFS_STATUS_FAIL;
            break;
        }

		printk("scale up => after DVFS CLKCON : 0x%x \r\n", DRV_Reg32(CLKCON)); 
    }
    else //scale down
    {

        /******************************************************************************
         * Scale down step1 : Set CLK and ARM Delay
         *****************************************************************************/
		printk("scale down => before DVFS CLKCON : 0x%x \r\n", DRV_Reg32(CLKCON)); 

		switch (arm_freq)
        {   

            case ARM_208_or_234 :
				DisableIRQ();
                DVFS_CLKSwitch_2x();
				EnableIRQ();
			break;
            case ARM_104_or_117 :
				DisableIRQ();
                DVFS_CLKSwitch_1x();
				EnableIRQ();
			break;
            default : 
                return DVFS_STATUS_FAIL;
            break;
        }
		printk("scale down => after DVFS CLKCON : 0x%x \r\n", DRV_Reg32(CLKCON)); 

        /******************************************************************************
         * Scale down step2 : Set PMU VCORE1
         *****************************************************************************/
        // adjust PMU_CON7 VCORE1 voltage depend on ARM frequency
        u4PMUVal = DRV_Reg32(PMU_CON7) ;
        u4CurrentVCore1 = u4PMUVal & 0xF ;        
		printk("scale down => before DVFS PMU : 0x%x \r\n",DRV_Reg32(PMU_CON7)); 				
		
        u4PMUVal &= 0x00007FF0 ;
        switch (arm_freq)
        {
            case ARM_208_or_234 : 
            {
                for (u4Step = (u4CurrentVCore1-1); u4Step >= OUTPUT_VCORE1_140; u4Step--)
                {
                    // set Vcore1 voltage                    
                    u4PMUVal = DRV_Reg32(PMU_CON7) ;
                    u4PMUVal &= 0x00007FF0 ;
                    u4PMUVal |= u4Step ; 
                    DRV_WriteReg32(PMU_CON7, u4PMUVal); 
                    printk("down step %d PMU : 0x%x \r\n",u4Step, DRV_Reg32(PMU_CON7));            

                    // set CLKCON delay
                    for (u4Delay = 0 ; u4Delay < DVFS_DELAY; u4Delay++);                    
                    u4ClkVal = DRV_Reg32(CLKCON);
                    u4OriCLKDly = u4ClkVal & 0x00700000;
                    u4ClkVal &= 0xFF8FFFFF;
                    u4ClkVal |= (u4CLKDLY_MAP[u4Step-OUTPUT_VCORE1_120]);
                    DRV_WriteReg32(CLKCON, u4ClkVal);
            		printk("down step %d CLKCON : 0x%x ",u4Step, DRV_Reg32(CLKCON)); 
                }
            }                        
            break;
            case ARM_104_or_117 : 
            {
                for (u4Step = (u4CurrentVCore1-1); u4Step >= OUTPUT_VCORE1_125; u4Step--)
                {
                    // set Vcore1 voltage                    
                    u4PMUVal = DRV_Reg32(PMU_CON7);
                    u4PMUVal &= 0x00007FF0 ;
                    u4PMUVal |= u4Step ; 
                    DRV_WriteReg32(PMU_CON7, u4PMUVal); 
                    printk("down step %d PMU : 0x%x \r\n",u4Step, DRV_Reg32(PMU_CON7));            

                    // set CLKCON delay
                    for (u4Delay = 0 ; u4Delay < DVFS_DELAY; u4Delay++);
                    u4ClkVal = DRV_Reg32(CLKCON);
                    u4OriCLKDly = u4ClkVal & 0x00700000;
                    u4ClkVal &= 0xFF8FFFFF;	
                    u4ClkVal |= (u4CLKDLY_MAP[u4Step-OUTPUT_VCORE1_120]);
                    DRV_WriteReg32(CLKCON, u4ClkVal);
            		printk("down step %d CLKCON : 0x%x ",u4Step, DRV_Reg32(CLKCON)); 

                }
            }                        
            break;     
            default : 
                return DVFS_STATUS_FAIL;
            break;
        }

        printk("scale down => after DVFS PMU : 0x%x \r\n", DRV_Reg32(PMU_CON7)); 	
    }

    //*MT3351_IRQ_MASKL = u4IntValL;
    //*MT3351_IRQ_MASKH = u4IntValH;

    DVFS_PreARMFreq = arm_freq;

    return DVFS_STATUS_OK;
}




