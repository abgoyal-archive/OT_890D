
   
#include <mach/mt3351_reg_base.h>
#include <mach/irqs.h>
#include <asm/io.h>
#include <mach/dma.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/module.h>

#include <mach/mt3351_gpt_sw.h>
#include <mach/mt3351_pdn_sw.h>
#include <mach/mt3351_pmu_sw.h>
#include <mach/mt3351_wdt_hw.h>
#include <mach/mt3351_wdt_sw.h>
#include <mach/mt3351_reg_base.h>


#define GPT_IRQEN	 (GPT_BASE+0x00)
#define GPT_IRQSTA       (GPT_BASE+0x04)
#define GPT_IRQACK       (GPT_BASE+0x08)
#define GPT0_CON         (GPT_BASE+0x10)
#define GPT0_CLK         (GPT_BASE+0x14)
#define GPT0_COUNT       (GPT_BASE+0x18)
#define GPT0_COMPARE     (GPT_BASE+0x1c)
#define GPT1_CON         (GPT_BASE+0x20)
#define GPT1_CLK         (GPT_BASE+0x24)
#define GPT1_COUNT       (GPT_BASE+0x28)
#define GPT1_COMPARE     (GPT_BASE+0x2c)
#define GPT2_CON         (GPT_BASE+0x30)
#define GPT2_CLK         (GPT_BASE+0x34)
#define GPT2_COUNT       (GPT_BASE+0x38)
#define GPT2_COMPARE     (GPT_BASE+0x3c)
#define GPT3_CON         (GPT_BASE+0x40)
#define GPT3_CLK         (GPT_BASE+0x44)
#define GPT3_COUNT       (GPT_BASE+0x48)
#define GPT3_COMPARE     (GPT_BASE+0x4c)
#define GPT4_CON         (GPT_BASE+0x50)
#define GPT4_CLK         (GPT_BASE+0x54)
#define GPT4_COUNT       (GPT_BASE+0x58)
#define GPT4_COMPARE     (GPT_BASE+0x5c)
#define GPT5_CON         (GPT_BASE+0x60)
#define GPT5_CLK         (GPT_BASE+0x64)
#define GPT5_COUNTL      (GPT_BASE+0x68)
#define GPT5_COMPAREL    (GPT_BASE+0x6c)
#define GPT5_COUNTH      (GPT_BASE+0x78)
#define GPT5_COMPAREH    (GPT_BASE+0x7c)


extern UINT32 g_HW_Ver;


const UINT32 GPT_CON_ENABLE = 0x0001;
const UINT32 GPT_CON_CLEAR = 0x0002;    
const UINT32 GPT_CLK_DIVISOR_MASK = 0x00000007;
const UINT32 GPT_CLK_SOURCE_MASK = 0x00000030;
const UINT32 GPT_CLK_SOURCE_SHIFT_BITS = 4;

const UINT32 g_u4GPTMask[GPT_TOTAL_COUNT] = { 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020};
const UINT32 g_u4AddrCON[GPT_TOTAL_COUNT] = { GPT0_CON, GPT1_CON, GPT2_CON, GPT3_CON, GPT4_CON, GPT5_CON};
const UINT32 g_u4AddrClk[GPT_TOTAL_COUNT] = { GPT0_CLK, GPT1_CLK, GPT2_CLK, GPT3_CLK, GPT4_CLK, GPT5_CLK};
const UINT32 g_u4AddrCnt[GPT_TOTAL_COUNT] = { GPT0_COUNT, GPT1_COUNT, GPT2_COUNT, GPT3_COUNT, GPT4_COUNT, GPT5_COUNTL};
const UINT32 g_u4AddrCompare[GPT_TOTAL_COUNT] = { GPT0_COMPARE, GPT1_COMPARE, GPT2_COMPARE, GPT3_COMPARE, GPT4_COMPARE, GPT5_COMPAREL};

//GPT timer start number
static const GPT_NUM GPT_TestStart = GPT1;

static spinlock_t gpt_lock = SPIN_LOCK_UNLOCKED;
unsigned long gpt_flags;




// GPT1~5 Handle Function
GPT_Func GPT_FUNC;

// tasklet
struct tasklet_struct GPT1_tlet;  
struct tasklet_struct GPT2_tlet;  
struct tasklet_struct GPT3_tlet;  
struct tasklet_struct GPT4_tlet;  
struct tasklet_struct GPT5_tlet;  


 /******************************************************************************
 * GPT_EnableIRQ
 * 
 * DESCRIPTION:
 *   Enable Interrupt of GPT
 *
 * PARAMETERS: 
 *   GPT Number (GPT1~5)
 * 
 * RETURNS: 
 *   None
 * 
 * NOTES: 
 *   None
 * 
 ******************************************************************************/
void GPT_EnableIRQ(GPT_NUM numGPT)
{
	spin_lock_irqsave(&gpt_lock,gpt_flags);	
	
    if(GPT0 != numGPT)
    	DRV_SetReg32(GPT_IRQEN, g_u4GPTMask[numGPT]);

    spin_unlock_irqrestore(&gpt_lock,gpt_flags);	
}

 /******************************************************************************
 * GPT_DisableIRQ
 * 
 * DESCRIPTION:
 *   Disable Interrupt of GPT
 *
 * PARAMETERS: 
 *   GPT Number (GPT1~5)
 * 
 * RETURNS: 
 *   None
 * 
 * NOTES: 
 *   None
 * 
 ******************************************************************************/
void GPT_DisableIRQ(GPT_NUM numGPT)
{
	spin_lock_irqsave(&gpt_lock,gpt_flags);	
	
    if(GPT0 != numGPT)	
    	DRV_ClrReg32(GPT_IRQEN, g_u4GPTMask[numGPT]);

    spin_unlock_irqrestore(&gpt_lock,gpt_flags);	
}


 /******************************************************************************
 * GPT_IsIRQEnable
 * 
 * DESCRIPTION:
 *   Check GPT Interrupt is enable or disable
 *
 * PARAMETERS: 
 *   GPT Number (GPT0~5)
 * 
 * RETURNS: 
 *   TRUE : Interrupt is enable
 *   FALSE: Interrupt is disbale
 * 
 * NOTES: 
 *   None
 * 
 ******************************************************************************/
BOOL GPT_IsIRQEnable(GPT_NUM numGPT)
{
    return (__raw_readl(GPT_IRQEN) & g_u4GPTMask[numGPT])? TRUE : FALSE;
}


 /******************************************************************************
 * GPT_Get_IRQSTA
 * 
 * DESCRIPTION:
 *   This function is to read IRQ status. if none of gpt channel has issued an interrupt
 *   ,then the return value is GPT_TOTAL_COUNT. 
 *   Otherwise, the return value would be the number of gpt has issued an interrupt
 *
 * PARAMETERS: 
 *   None
 * 
 * RETURNS: 
 *   numGPT : GPT Number 
 *           (GPT1,GPT2,GPT3,GPT4,GPT5)

 * 
 * NOTES: 
 *   None
 * 
 ******************************************************************************/
GPT_NUM GPT_Get_IRQSTA(void)
{
    BOOL sta;
    UINT32 numGPT;

    sta = __raw_readl(GPT_IRQSTA);
          
    for (numGPT = GPT1; numGPT < GPT_TOTAL_COUNT; numGPT++)
    {
        if (sta & g_u4GPTMask[numGPT])
            break;
    }    
    return numGPT;
}


 /******************************************************************************
 * GPT_Check_IRQSTA
 * 
 * DESCRIPTION:
 *   This function is used to check GPT's IRQ status 
 *
 * PARAMETERS: 
 *   GPT number (GPT0~5)
 * 
 * RETURNS: 
 *   TRUE : gpt channel has issued an interrupt. 
 *   FALSE: gpt channel hasn't issued an interrupt. 
 * 
 * NOTES: 
 *   None
 * 
 ******************************************************************************/
BOOL GPT_Check_IRQSTA(GPT_NUM numGPT)
{
    BOOL sta;

    sta = __raw_readl(GPT_IRQSTA);

    return (sta & g_u4GPTMask[numGPT]);

}

 /******************************************************************************
 * GPT_AckIRQ
 * 
 * DESCRIPTION:
 *   Clean IRQ status bit by writing IRQ ack bit
 *
 * PARAMETERS: 
 *   GPT number
 * 
 * RETURNS: 
 *   None
 * 
 * NOTES: 
 *   None
 * 
 ******************************************************************************/
void GPT_AckIRQ(GPT_NUM numGPT)
{
	spin_lock_irqsave(&gpt_lock,gpt_flags);	

    if(GPT0 != numGPT)		
    	DRV_SetReg32(GPT_IRQACK, g_u4GPTMask[numGPT]);

    spin_unlock_irqrestore(&gpt_lock,gpt_flags);	
}


 /******************************************************************************
 * GPT_Start
 * 
 * DESCRIPTION:
 *   Start GPT Timer
 *
 * PARAMETERS: 
 *   GPT number
 * 
 * RETURNS: 
 *   None
 * 
 * NOTES: 
 *   None
 * 
 ******************************************************************************/
void GPT_Start(GPT_NUM numGPT)
{
	spin_lock_irqsave(&gpt_lock,gpt_flags);	

    // For mt3351 e2 version, the clear bit must come with enable bit
    // When eco, the cleart bit will be removed.
    if (g_HW_Ver >= B68351B)
        DRV_SetReg32(g_u4AddrCON[numGPT] , GPT_CON_ENABLE);     
    else
        DRV_SetReg32(g_u4AddrCON[numGPT] , GPT_CON_ENABLE|GPT_CON_CLEAR);     
    

    spin_unlock_irqrestore(&gpt_lock,gpt_flags);	
}

 /******************************************************************************
 * GPT_Stop
 * 
 * DESCRIPTION:
 *   Stop GPT Timer
 *
 * PARAMETERS: 
 *   GPT number (GPT1~5)
 * 
 * RETURNS: 
 *   None
 * 
 * NOTES: 
 *   None
 * 
 ******************************************************************************/
void GPT_Stop(GPT_NUM numGPT)
{
	spin_lock_irqsave(&gpt_lock,gpt_flags);	

    if(GPT0 != numGPT)
    	DRV_ClrReg32(g_u4AddrCON[numGPT] , GPT_CON_ENABLE);

    spin_unlock_irqrestore(&gpt_lock,gpt_flags);	
}

 /******************************************************************************
 * GPT_IsStart
 * 
 * DESCRIPTION:
 *   Check whether GPT is running or not
 *
 * PARAMETERS: 
 *   GPT number
 * 
 * RETURNS: 
 *   TRUE : running
 *   FALSE: stop
 * 
 * NOTES: 
 *   None
 * 
 ******************************************************************************/
BOOL GPT_IsStart(GPT_NUM numGPT)
{
    return (__raw_readl(g_u4AddrCON[numGPT] ) & GPT_CON_ENABLE)? TRUE : FALSE;
}


 /******************************************************************************
 * GPT_ClearCount
 * 
 * DESCRIPTION:
 *   This function is used to clean GPT counter value
 *   If it has been started, stop the timer. 
 *   After cleaning GPT counter value, restart this GPT counter again
 *
 * PARAMETERS: 
 *   GPT number
 * 
 * RETURNS: 
 *   None
 * 
 * NOTES: 
 *   None
 * 
 ******************************************************************************/
void GPT_ClearCount(GPT_NUM numGPT)
{
    // For mt3351 e2 version, the clear bit must come with enable bit
    // When eco, the clear bit will be removed.
    if (g_HW_Ver >= B68351B)
    {
    	
        BOOL bIsStarted;

        //In order to get the counter as 0 after the clear command
        //Stop the timer if it has been started.
        //after get the counter as 0, restore the timer status
        bIsStarted = GPT_IsStart(numGPT);
        if (bIsStarted)
            GPT_Stop(numGPT);

		spin_lock_irqsave(&gpt_lock,gpt_flags);	

        DRV_SetReg32(g_u4AddrCON[numGPT] , GPT_CON_CLEAR);

        spin_unlock_irqrestore(&gpt_lock,gpt_flags);	

        //After setting the clear command, it needs to wait one cycle time of the clock source to become 0
        while (GPT_GetCounterL32(numGPT) != 0);
        if (bIsStarted)
            GPT_Start(numGPT);    	
        
    }
    else
    {
    	/* A68351B 
        BOOL bIsStarted;

        //In order to get the counter as 0 after the clear command
        //Stop the timer if it has been started.
        //after get the counter as 0, restore the timer status
        bIsStarted = GPT_IsStart(numGPT);
        val = __raw_readl(g_u4AddrCON[numGPT]);
        
        if (bIsStarted)
            GPT_Stop(numGPT);

        //BUG?
        __raw_writel(0x11,g_u4AddrCON[numGPT]); 
        //Delay for a while
        for(i=0;i<5500;i++) {}        
        __raw_writel(GPT_CON_CLEAR,g_u4AddrCON[numGPT]); 
        

        //After setting the clear command, it needs to wait one cycle time of the clock source to become 0
        while (GPT_GetCounterL32(numGPT) != 0);
        
        __raw_writel(val,g_u4AddrCON[numGPT]);
        */
    }
}

 /******************************************************************************
 * GPT_SetOpMode
 * 
 * DESCRIPTION:
 *   This function is used to set GPT mode
 *   (GPT_ONE_SHOT,GPT_REPEAT,GPT_KEEP_GO,GPT_FREE_RUN)
 *
 * PARAMETERS: 
 *   GPT number (GPT1~5) and GPT Mode
 * 
 * RETURNS: 
 *   None
 * 
 * NOTES: 
 *   None
 * 
 ******************************************************************************/
void GPT_SetOpMode(GPT_NUM numGPT, GPT_CON_MODE mode)
{
    UINT32 gptMode;
    
    if(GPT0 != numGPT)
    {
		gptMode = __raw_readl(g_u4AddrCON[numGPT]) & (~GPT_FREE_RUN);
		gptMode |= mode;

		spin_lock_irqsave(&gpt_lock,gpt_flags);			

        __raw_writel(gptMode,g_u4AddrCON[numGPT]);

        spin_unlock_irqrestore(&gpt_lock,gpt_flags);	
    }
}

 /******************************************************************************
 * GPT_GetOpMode
 * 
 * DESCRIPTION:
 *   This function is used to get GPT mode
 *   (GPT_ONE_SHOT,GPT_REPEAT,GPT_KEEP_GO,GPT_FREE_RUN)
 *
 * PARAMETERS: 
 *   GPT number (GPT0~5)
 * 
 * RETURNS: 
 *   GPT Mode
 * 
 * NOTES: 
 *   None
 * 
 ******************************************************************************/
GPT_CON_MODE GPT_GetOpMode(GPT_NUM numGPT)
{
    UINT32 u4Con;
    GPT_CON_MODE mode = GPT_ONE_SHOT;
    
    u4Con = __raw_readl(g_u4AddrCON[numGPT]) & GPT_FREE_RUN;
    if (u4Con == GPT_FREE_RUN)
        mode = GPT_FREE_RUN;
    else if (u4Con == GPT_KEEP_GO)
        mode = GPT_KEEP_GO;
    else if (u4Con == GPT_REPEAT)
        mode = GPT_REPEAT;

    return mode;
}

 /******************************************************************************
 * GPT_SetClkDivisor
 * 
 * DESCRIPTION:
 *   This function is used to set GPT clock divisor 
 *   (GPT_CLK_DIV_1,GPT_CLK_DIV_2,GPT_CLK_DIV_4,GPT_CLK_DIV_8,
 *    GPT_CLK_DIV_16,GPT_CLK_DIV_32,GPT_CLK_DIV_64,GPT_CLK_DIV_128)
 *
 * PARAMETERS: 
 *   GPT number (GPT1~5) and clock divisor
 * 
 * RETURNS: 
 *   None
 * 
 * NOTES: 
 *   None
 * 
 ******************************************************************************/
void GPT_SetClkDivisor(GPT_NUM numGPT, GPT_CLK_DIV clkDiv)
{
    UINT32 div;
    
    if(GPT0 != numGPT)
    {
	div = __raw_readl(g_u4AddrClk[numGPT]) & (~GPT_CLK_DIVISOR_MASK);
	div |= clkDiv;

	spin_lock_irqsave(&gpt_lock,gpt_flags);		

	__raw_writel(div,g_u4AddrClk[numGPT]);

    spin_unlock_irqrestore(&gpt_lock,gpt_flags);	
    }
}

 /******************************************************************************
 * GPT_GetClkDivisor
 * 
 * DESCRIPTION:
 *   This function is used to get GPT clock divisor 
 *   (GPT_CLK_DIV_1,GPT_CLK_DIV_2,GPT_CLK_DIV_4,GPT_CLK_DIV_8,
 *    GPT_CLK_DIV_16,GPT_CLK_DIV_32,GPT_CLK_DIV_64,GPT_CLK_DIV_128)
 *
 * PARAMETERS: 
 *   GPT number (GPT0~5)
 * 
 * RETURNS: 
 *   GPT clock divisor
 * 
 * NOTES: 
 *   None
 * 
 ******************************************************************************/
GPT_CLK_DIV GPT_GetClkDivisor(GPT_NUM numGPT)
{
    return __raw_readl(g_u4AddrClk[numGPT]) & GPT_CLK_DIV_128;
}

 /******************************************************************************
 * GPT_SetClkSource
 * 
 * DESCRIPTION:
 *   This function is used to set GPT clock source
 *   1.GPT_CLK_SYS_CRYSTAL: 13M or 26M Hz
 *   2.GPT_CLK_APB_BUS    : APB bus frequency
 *   3.GPT_CLK_RTC        : 32768Hz
 *
 * PARAMETERS: 
 *   GPT number (GPT1~5) and clock source
 * 
 * RETURNS: 
 *   None
 * 
 * NOTES: 
 *   None
 * 
 ******************************************************************************/
void GPT_SetClkSource(GPT_NUM numGPT, GPT_CLK_SOURCE clkSrc)
{
    UINT32 src;

    if(GPT0 != numGPT)
    {    
        src = __raw_readl(g_u4AddrClk[numGPT]) & (~GPT_CLK_SOURCE_MASK);
        src |= clkSrc << GPT_CLK_SOURCE_SHIFT_BITS;

		spin_lock_irqsave(&gpt_lock,gpt_flags);	
        __raw_writel(src,g_u4AddrClk[numGPT]);
        spin_unlock_irqrestore(&gpt_lock,gpt_flags);	
    }
}


 /******************************************************************************
 * GPT_GetClkSource
 * 
 * DESCRIPTION:
 *   This function is used to get GPT clock source
 *   1.GPT_CLK_SYS_CRYSTAL: 13M or 26M Hz
 *   2.GPT_CLK_APB_BUS    : APB bus frequency
 *   3.GPT_CLK_RTC        : 32768Hz
 *
 * PARAMETERS: 
 *   GPT number (GPT0~5)
 * 
 * RETURNS: 
 *   GPT clock source
 * 
 * NOTES: 
 *   None
 * 
 ******************************************************************************/
GPT_CLK_SOURCE GPT_GetClkSource(GPT_NUM numGPT)
{
    UINT32 u4Div;
    GPT_CLK_SOURCE clkSrc = GPT_CLK_RTC;
    
    u4Div = __raw_readl(g_u4AddrClk[numGPT])>>GPT_CLK_SOURCE_SHIFT_BITS;
    if (u4Div == GPT_CLK_APB_BUS)
        clkSrc = GPT_CLK_APB_BUS;
    else if (u4Div == GPT_CLK_SYS_CRYSTAL)
        clkSrc = GPT_CLK_SYS_CRYSTAL;
    
    return clkSrc;
}

 /******************************************************************************
 * GPT_GetCounterL32
 * 
 * DESCRIPTION:
 *   This function is used to get low 32 bit of GPT counter
 *
 * PARAMETERS: 
 *   GPT number (GPT0~5)
 * 
 * RETURNS: 
 *   Low 32 bit of GPT counter value
 * 
 * NOTES: 
 *   None
 * 
 ******************************************************************************/
UINT32 GPT_GetCounterL32(GPT_NUM numGPT)
{  
    return __raw_readl(g_u4AddrCnt[numGPT]);      
}

 /******************************************************************************
 * GPT_GetCounterL32
 * 
 * DESCRIPTION:
 *   This function is used to get high 32 bit of GPT counter value
 *
 * PARAMETERS: 
 *   GPT number (only GPT5 is accepted)
 * 
 * RETURNS: 
 *   High 32 bit of GPT5 counter value
 * 
 * NOTES: 
 *   None
 * 
 ******************************************************************************/
UINT32 GPT_GetCounterH32(GPT_NUM numGPT)
{
    if (numGPT < GPT5)
    {
        //ASSERT(0);
        return 0;
    }
    
    return __raw_readl(GPT5_COUNTH);
}

 /******************************************************************************
 * GPT_SetCompareL32
 * 
 * DESCRIPTION:
 *   This function is used to set low 32 bit of GPT compare
 *
 * PARAMETERS: 
 *   GPT number (GPT1~5)
 * 
 * RETURNS: 
 *   None
 * 
 * NOTES: 
 *   None
 * 
 ******************************************************************************/
void GPT_SetCompareL32(GPT_NUM numGPT, UINT32 u4Compare)
{
    if(GPT0 != numGPT)
    {    	

		spin_lock_irqsave(&gpt_lock,gpt_flags);	
    	__raw_writel(u4Compare,g_u4AddrCompare[numGPT]);    
	    spin_unlock_irqrestore(&gpt_lock,gpt_flags);	    	
    }
}

 /******************************************************************************
 * GPT_SetCompareH32
 * 
 * DESCRIPTION:
 *   This function is used to set high 32 bit of GPT compare
 *
 * PARAMETERS: 
 *   GPT number (only GPT5 is accepted)
 * 
 * RETURNS: 
 *   None
 * 
 * NOTES: 
 *   None
 * 
 ******************************************************************************/
void GPT_SetCompareH32(GPT_NUM numGPT, UINT32 u4Compare)
{
    if (numGPT < GPT5)
    {
        //ASSERT(0);
        return;
    }

	spin_lock_irqsave(&gpt_lock,gpt_flags);	
    __raw_writel(u4Compare,GPT5_COMPAREH);    
    spin_unlock_irqrestore(&gpt_lock,gpt_flags);	    
}

 /******************************************************************************
 * GPT_GetCompareL32
 * 
 * DESCRIPTION:
 *   This function is used to get low 32 bit of GPT compare
 *
 * PARAMETERS: 
 *   GPT number (GPT0~5)
 * 
 * RETURNS: 
 *   Low 32 bit of GPT counter value
 * 
 * NOTES: 
 *   None
 * 
 ******************************************************************************/
UINT32 GPT_GetCompareL32(GPT_NUM numGPT)
{
    return __raw_readl(g_u4AddrCompare[numGPT]);
}

 /******************************************************************************
 * GPT_GetCompareH32
 * 
 * DESCRIPTION:
 *   This function is used to get high 32 bit of GPT compare
 *
 * PARAMETERS: 
 *   GPT number (only GPT5)
 * 
 * RETURNS: 
 *   High 32 bit of GPT counter value
 * 
 * NOTES: 
 *   None
 * 
 ******************************************************************************/
UINT32 GPT_GetCompareH32(GPT_NUM numGPT)
{
    if (numGPT < GPT5)
    {
        //ASSERT(0);
        return 0;
    }
    
    return __raw_readl(GPT5_COMPAREH);
}

 /******************************************************************************
 * GPT_Reset
 * 
 * DESCRIPTION:
 *   This function is used to reset GPT register
 *
 * PARAMETERS: 
 *   GPT number (GPT1~5)
 * 
 * RETURNS: 
 *   None
 * 
 * NOTES: 
 *   None
 * 
 ******************************************************************************/
void GPT_Reset(GPT_NUM numGPT)
{

    // cannot reset GPT0
    // Reset GPT0 will cause sysmten timer fail
    if(GPT0 == numGPT)
    {	return;
    }
     
    // (1) stop GPT
    GPT_Stop(numGPT);
    
    // (2) Clean GPT counter
    GPT_ClearCount(numGPT);
    
    // (3) Clean GPT Comparator
    GPT_SetCompareL32(numGPT,0);
    if(GPT5 == numGPT)
    {	GPT_SetCompareH32(GPT5,0);
    }

    // (4) Clean GPT Counter
    GPT_ClearCount(numGPT);
    
    // (5) Disbale Interrupt
    GPT_DisableIRQ(numGPT);
    
    // (6) Default mode should be one shot
    GPT_SetOpMode(numGPT,GPT_ONE_SHOT);
    
    // (7) Default Clock source should be GPT_CLK_RTC
    GPT_SetClkSource(numGPT,GPT_CLK_RTC);
    
    // (8) Default Clock Divisor should be GPT_CLK_DIV_1
    GPT_SetClkDivisor(numGPT,GPT_CLK_DIV_1);
    
    
}

 /******************************************************************************
 * GPT_Config
 * 
 * DESCRIPTION:
 *   This function is used to config GPT register
 *   (1) mode
 *   (2) clkSrc
 *   (3) clkDiv
 *   (4) bIrqEnable
 *   (5) u4CompareL
 *   (6) u4CompareH
 *
 * PARAMETERS: 
 *   GPT configuration data structure
 * 
 * RETURNS: 
 *   TRUE : config GPT pass
 *   FALSE: config GPT fail
 * 
 * NOTES: 
 *   None
 * 
 ******************************************************************************/
BOOL GPT_Config(GPT_CONFIG config)
{
    GPT_NUM num;
    GPT_CON_MODE mode;
    GPT_CLK_SOURCE clkSrc;
    GPT_CLK_DIV clkDiv;
    BOOL bIrqEnable;
    UINT32 u4CompareL;
    UINT32 u4CompareH;
    
    BOOL bRet = TRUE;

    num = config.num;
    mode = config.mode;
    clkSrc = config.clkSrc;
    clkDiv = config.clkDiv;
    bIrqEnable = config.bIrqEnable;
    u4CompareL = config.u4CompareL;
    u4CompareH = config.u4CompareH;    

    GPT_SetOpMode(num, mode);    

    if (GPT_GetOpMode(num) != mode)
    {
        //dbg_print("[GPT%d] GPT_SetOpMode failed\r\n", num);
        bRet = FALSE;
    }   

    GPT_SetClkSource(num, clkSrc);
    if (GPT_GetClkSource(num) != clkSrc)
    {
        //dbg_print("[GPT%d] GPT_SetClkSource failed\r\n", num);
        bRet = FALSE;
    }            

    GPT_SetClkDivisor(num, clkDiv);
    if (GPT_GetClkDivisor(num) != clkDiv)
    {
        //dbg_print("[GPT%d] GPT_SetClkDivisor failed\r\n", num);
        bRet = FALSE;
    }

    if (bIrqEnable)
        GPT_EnableIRQ(num);
    else
        GPT_DisableIRQ(num);

    GPT_SetCompareL32(num, u4CompareL);
    if (GPT_GetCompareL32(num) != u4CompareL)
    {
        //dbg_print("[GPT%d] GPT_GetCompareL32 isn't equal to the expected value\r\n", num);
        bRet = FALSE;
    }    

    if (num == GPT5)
    {
        GPT_SetCompareH32(num, u4CompareH);
        if (GPT_GetCompareH32(num) != u4CompareH)
        {
            //dbg_print("[GPT%d] GPT_GetCompareH32 isn't equal to the expected value\r\n", num);
            bRet = FALSE;
        }    
    }
    
    return bRet;
}


 /******************************************************************************
 * GPT1_Tasklet ~ GPT5_Tasklet
 * 
 * DESCRIPTION:
 *   Tasklet of GPT1~5 that will call user registered call-back function
 *
 * PARAMETERS: 
 *   None
 * 
 * RETURNS: 
 *   None
 * 
 * NOTES: 
 *   None
 * 
 ******************************************************************************/

void GPT1_Tasklet(unsigned long arg)
{
    if (GPT_FUNC.gpt1_func != NULL)
    {    	
        GPT_FUNC.gpt1_func(GPT1);
    }	
}

void GPT2_Tasklet(unsigned long arg)
{
    if (GPT_FUNC.gpt2_func != NULL)
    {
        GPT_FUNC.gpt2_func(GPT2);
    }	
}

void GPT3_Tasklet(unsigned long arg)
{
    if (GPT_FUNC.gpt3_func != NULL)
    {
        GPT_FUNC.gpt3_func(GPT3);
    }	
}

void GPT4_Tasklet(unsigned long arg)
{
    if (GPT_FUNC.gpt4_func != NULL)
    {
        GPT_FUNC.gpt4_func(GPT4);
    }  	
}

void GPT5_Tasklet(unsigned long arg)
{
    if (GPT_FUNC.gpt5_func != NULL)
    {
        GPT_FUNC.gpt5_func(GPT5);
    }   	
}

 /******************************************************************************
 * GPT_LISR
 * 
 * DESCRIPTION:
 *   GPT LISR to handle GPT1~5 interrupt
 *   In this function, it will schedule corresonding GPT tasklet
 *
 * PARAMETERS: 
 *   None
 * 
 * RETURNS: 
 *   None
 * 
 * NOTES: 
 *   None
 * 
 ******************************************************************************/
void GPT_LISR()
{
    GPT_NUM numGPT;

    numGPT = GPT_Get_IRQSTA();

    switch(numGPT)
    {	case GPT1:
    	     tasklet_schedule(&GPT1_tlet);	    
    	     break;
    	case GPT2:
    	     tasklet_schedule(&GPT2_tlet);	    
    	     break;
    	case GPT3:
    	     tasklet_schedule(&GPT3_tlet);	    
    	     break;
    	case GPT4:
    	     tasklet_schedule(&GPT4_tlet);	    
    	     break;
    	case GPT5:
    	     tasklet_schedule(&GPT5_tlet);	    
    	     break; 
    	case GPT0:
    	default:
    	     break;
    }

    GPT_AckIRQ(numGPT); 
}


 /******************************************************************************
 * GPT_Init
 * 
 * DESCRIPTION:
 *   Register GPT1~5 interrupt handler (call back function)
 *
 * PARAMETERS: 
 *   GPT number (GPT1~5) and handler
 * 
 * RETURNS: 
 *   None
 * 
 * NOTES: 
 *   None
 * 
 ******************************************************************************/

void GPT_Init(GPT_NUM timerNum, void (*GPT_Callback)(UINT16))
{

    if (timerNum == GPT1)
    {
    	GPT_FUNC.gpt1_func = GPT_Callback;
    }
    if (timerNum == GPT2)
    {
    	GPT_FUNC.gpt2_func = GPT_Callback;
    }
    if (timerNum == GPT3)
    {
    	GPT_FUNC.gpt3_func = GPT_Callback;
    }    
    if (timerNum == GPT4)
    {
    	GPT_FUNC.gpt4_func = GPT_Callback;
    }
    if (timerNum == GPT5)
    {
    	GPT_FUNC.gpt5_func = GPT_Callback;
    }

    //power up GPT
    if (PDN_Get_Peri_Status(PDN_PERI_GPT))
        hwEnableClock(MT3351_CLOCK_GPT);
        //PDN_Power_CONA_DOWN(PDN_PERI_GPT, KAL_FALSE);
}

 /******************************************************************************
 * mt_init_gpt
 * 
 * DESCRIPTION:
 *   Initialize GPT Module
 *
 * PARAMETERS: 
 *   None
 * 
 * RETURNS: 
 *   None
 * 
 * NOTES: 
 *   None
 * 
 ******************************************************************************/
static int __init mt_init_gpt(void){
    

    /*
     * Five 32-bit timers (GPT0~GPT4) and one 64-bit timer (GPT5)
     * Each timer operates on one of the following modes
     * (a) ONE-SHOT
     * (b) REPEAT
     * (c) KEEP-GO
     * (d) FREERUN
     */

    /* register the tasklet */
    tasklet_init(&GPT1_tlet,GPT1_Tasklet, 0);
    tasklet_init(&GPT2_tlet,GPT2_Tasklet, 0);
    tasklet_init(&GPT3_tlet,GPT3_Tasklet, 0);
    tasklet_init(&GPT4_tlet,GPT4_Tasklet, 0);
    tasklet_init(&GPT5_tlet,GPT5_Tasklet, 0);                
    printk("MT3351 GPT is intialized!!\n");

    return 0;
}

arch_initcall(mt_init_gpt);

EXPORT_SYMBOL(GPT_EnableIRQ);
EXPORT_SYMBOL(GPT_DisableIRQ);
EXPORT_SYMBOL(GPT_IsIRQEnable);
EXPORT_SYMBOL(GPT_Get_IRQSTA);
EXPORT_SYMBOL(GPT_AckIRQ);
EXPORT_SYMBOL(GPT_Start);
EXPORT_SYMBOL(GPT_Stop);
EXPORT_SYMBOL(GPT_IsStart);
EXPORT_SYMBOL(GPT_ClearCount);
EXPORT_SYMBOL(GPT_SetOpMode);
EXPORT_SYMBOL(GPT_GetOpMode);
EXPORT_SYMBOL(GPT_SetClkDivisor);
EXPORT_SYMBOL(GPT_GetClkDivisor);
EXPORT_SYMBOL(GPT_SetClkSource);
EXPORT_SYMBOL(GPT_GetClkSource);

EXPORT_SYMBOL(GPT_GetCounterL32);
EXPORT_SYMBOL(GPT_GetCounterH32);
EXPORT_SYMBOL(GPT_SetCompareL32);
EXPORT_SYMBOL(GPT_SetCompareH32);

EXPORT_SYMBOL(GPT_GetCompareL32);
EXPORT_SYMBOL(GPT_GetCompareH32);
EXPORT_SYMBOL(GPT_Reset);
EXPORT_SYMBOL(GPT_Config);
EXPORT_SYMBOL(GPT_Check_IRQSTA);
// GPT_LISR will be called in core.c
EXPORT_SYMBOL(GPT_LISR);
EXPORT_SYMBOL(GPT_Init);

