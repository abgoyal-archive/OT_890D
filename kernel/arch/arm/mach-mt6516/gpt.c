
   
#include <mach/irqs.h>
#include <asm/io.h>
#include <mach/dma.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/irq.h>

#include <mach/mt6516_typedefs.h>
#include <mach/mt6516_gpt_sw.h>
#include <mach/mt6516.h>
#include <linux/proc_fs.h>
#include <asm/tcm.h>

//#define MT6516_XGPT_GPT_DEBUG
#ifdef MT6516_XGPT_GPT_DEBUG
#define GPT_DEBUG printk
#define XGPT_DEBUG printk
#else
#define GPT_DEBUG(x,...)
#define XGPT_DEBUG(x,...)
#endif


#define XGPT_CON_ENABLE 0x0001
#define XGPT_CON_CLEAR 0x0002
#define XGPT_CLK_DIVISOR_MASK 0x00000007

const UINT32 g_xgpt_mask[XGPT_TOTAL_COUNT] = { 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040};

const UINT32 g_xgpt_addr_con[XGPT_TOTAL_COUNT] = { MT6516_XGPT1_CON, MT6516_XGPT2_CON, 
MT6516_XGPT3_CON, MT6516_XGPT4_CON, MT6516_XGPT5_CON, MT6516_XGPT6_CON, MT6516_XGPT7_CON};
	  
const UINT32 g_xgpt_addr_clk[XGPT_TOTAL_COUNT] = { MT6516_XGPT1_PRESCALE, MT6516_XGPT2_PRESCALE, 
MT6516_XGPT3_PRESCALE, MT6516_XGPT4_PRESCALE, MT6516_XGPT5_PRESCALE, MT6516_XGPT6_PRESCALE, MT6516_XGPT7_PRESCALE};
	  
const UINT32 g_xgpt_addr_cnt[XGPT_TOTAL_COUNT] = { MT6516_XGPT1_COUNT, MT6516_XGPT2_COUNT, 
MT6516_XGPT3_COUNT, MT6516_XGPT4_COUNT, MT6516_XGPT5_COUNT, MT6516_XGPT6_COUNT, MT6516_XGPT7_COUNT};
	  
const UINT32 g_xgpt_addr_compare[XGPT_TOTAL_COUNT] = { MT6516_XGPT1_COMPARE, MT6516_XGPT2_COMPARE, 
MT6516_XGPT3_COMPARE, MT6516_XGPT4_COMPARE, MT6516_XGPT5_COMPARE, MT6516_XGPT6_COMPARE, MT6516_XGPT7_COMPARE};  

UINT32 g_xgpt_status[XGPT_TOTAL_COUNT] = {USED,USED,NOT_USED,NOT_USED,NOT_USED,NOT_USED,NOT_USED};

static spinlock_t g_xgpt_lock = SPIN_LOCK_UNLOCKED;
unsigned long g_xgpt_flags;


#define GPT_CON_ENABLE 0x8000

const UINT32 g_gpt_addr_con[GPT_TOTAL_COUNT] = { MT6516_GPT1_CON, MT6516_GPT2_CON, 
MT6516_GPT3_CON};
	  
const UINT32 g_gpt_addr_clk[GPT_TOTAL_COUNT] = { MT6516_GPT1_PRESCALE, MT6516_GPT2_PRESCALE, 
MT6516_GPT3_PRESCALE};
	  
const UINT32 g_gpt_addr_dat[GPT_TOTAL_COUNT] = { MT6516_GPT1_DAT, MT6516_GPT2_DAT, MT6516_GPT3_DAT};

UINT32 g_gpt_status[GPT_TOTAL_COUNT] = {NOT_USED,NOT_USED,NOT_USED};

static spinlock_t g_gpt_lock = SPIN_LOCK_UNLOCKED;
unsigned long g_gpt_flags;


extern void MT6516_IRQUnmask(unsigned int line);
extern void MT6516_IRQSensitivity(unsigned char code, unsigned char edge);


// XGPT2~7 Handle Function
XGPT_Func g_xgpt_func;

// GPT1~2 Handle Function
GPT_Func g_gpt_func;

// tasklet
struct tasklet_struct g_xgpt2_tlet;  
struct tasklet_struct g_xgpt3_tlet;  
struct tasklet_struct g_xgpt4_tlet;  
struct tasklet_struct g_xgpt5_tlet;  
struct tasklet_struct g_xgpt6_tlet;  
struct tasklet_struct g_xgpt7_tlet;  
struct tasklet_struct g_gpt1_tlet;  
struct tasklet_struct g_gpt2_tlet;  


void XGPT2_Tasklet(unsigned long arg)
{
    if (g_xgpt_func.xgpt2_func != NULL)
    {    	
        g_xgpt_func.xgpt2_func(XGPT2);
    }	
    else
    {	printk("g_xgpt_func.xgpt2_fun (arg:%lu) = NULL!\n",arg);
    }
}

void XGPT3_Tasklet(unsigned long arg)
{
    if (g_xgpt_func.xgpt3_func != NULL)
    {
        g_xgpt_func.xgpt3_func(XGPT3);
    }	
    else
    {	printk("g_xgpt_func.xgpt3_fun (arg:%lu) = NULL!\n",arg);
    }
}

void XGPT4_Tasklet(unsigned long arg)
{
    if (g_xgpt_func.xgpt4_func != NULL)
    {
        g_xgpt_func.xgpt4_func(XGPT4);
    }	
    else
    {	printk("g_xgpt_func.xgpt4_fun (arg:%lu) = NULL!\n",arg);
    }
}

void XGPT5_Tasklet(unsigned long arg)
{
    if (g_xgpt_func.xgpt5_func != NULL)
    {
        g_xgpt_func.xgpt5_func(XGPT5);
    }  	
    else
    {	printk("g_xgpt_func.xgpt5_fun (arg:%lu) = NULL!\n",arg);
    }
}

void XGPT6_Tasklet(unsigned long arg)
{
    if (g_xgpt_func.xgpt6_func != NULL)
    {
        g_xgpt_func.xgpt6_func(XGPT6);
    }   	
    else
    {	printk("g_xgpt_func.xgpt6_fun (arg:%lu) = NULL!\n",arg);
    }
}

void XGPT7_Tasklet(unsigned long arg)
{
    if (g_xgpt_func.xgpt7_func != NULL)
    {
        g_xgpt_func.xgpt7_func(XGPT7);
    }   	
    else
    {	printk("g_xgpt_func.xgpt7_fun (arg:%lu) = NULL!\n",arg);
    }
}

void GPT1_Tasklet(unsigned long arg)
{
    if (g_gpt_func.gpt1_func != NULL)
    {
        g_gpt_func.gpt1_func(GPT1);
    }   	
    else
    {	printk("g_gpt_func.gpt1_fun (arg:%lu) = NULL!\n",arg);
    }
}

void GPT2_Tasklet(unsigned long arg)
{
    if (g_gpt_func.gpt2_func != NULL)
    {
        g_gpt_func.gpt2_func(GPT2);
    }   	
    else
    {	printk("g_gpt_func.gpt2_fun (arg:%lu) = NULL!\n",arg);
    }
}



 /******************************************************************************
 * DESCRIPTION:
 *   Enable Interrupt of XGPT
 * PARAMETERS: 
 *   XGPT Number (XGPT2~7)
 ******************************************************************************/
void XGPT_EnableIRQ(XGPT_NUM numXGPT)
{
	spin_lock_irqsave(&g_xgpt_lock,g_xgpt_flags);	
	
    if(XGPT1 != numXGPT)
    	DRV_SetReg32(MT6516_XGPT_IRQEN, g_xgpt_mask[numXGPT]);

    spin_unlock_irqrestore(&g_xgpt_lock,g_xgpt_flags);	
}

 /******************************************************************************
 * DESCRIPTION:
 *   Disable Interrupt of XGPT
 * PARAMETERS: 
 *   XGPT Number (XGPT2~7)
 ******************************************************************************/
void XGPT_DisableIRQ(XGPT_NUM numXGPT)
{
	spin_lock_irqsave(&g_xgpt_lock,g_xgpt_flags);	
	
    if(XGPT1 != numXGPT)	
    	DRV_ClrReg32(MT6516_XGPT_IRQEN, g_xgpt_mask[numXGPT]);

    spin_unlock_irqrestore(&g_xgpt_lock,g_xgpt_flags);	
}


 /******************************************************************************
 * DESCRIPTION:
 *   Check XGPT Interrupt is enable or disable
 * PARAMETERS: 
 *   XGPT Number (XGPT2~7)
 * RETURNS: 
 *   TRUE : Interrupt is enabled
 *   FALSE: Interrupt is not enabled
 ******************************************************************************/
BOOL XGPT_IsIRQEnable(XGPT_NUM numXGPT)
{
    return (__raw_readl(MT6516_XGPT_IRQEN) & g_xgpt_mask[numXGPT])? TRUE : FALSE;
}


 /******************************************************************************
 * DESCRIPTION:
 *   This function is to read IRQ status. if none of gpt channel has issued an interrupt
 *   ,then the return value is XGPT_TOTAL_COUNT. 
 *   Otherwise, the return value would be the number of gpt has issued an interrupt
 * RETURNS: 
 *   numXGPT : XGPT Number 
 *           (XGPT2,XGPT3,XGPT4,XGPT5,XGPT6,XGPT7)
 ******************************************************************************/
XGPT_NUM __tcmfunc XGPT_Get_IRQSTA(void)
{
    BOOL sta;
    UINT32 numXGPT;

    sta = __raw_readl(MT6516_XGPT_IRQSTA);
          
    for (numXGPT = XGPT2; numXGPT < XGPT_TOTAL_COUNT; numXGPT++)
    {
        if (sta & g_xgpt_mask[numXGPT])
            break;
    }    
    return numXGPT;
}


 /******************************************************************************
 * DESCRIPTION:
 *   This function is used to check XGPT's IRQ status 
 * PARAMETERS: 
 *   XGPT number (XGPT2~7)
 * RETURNS: 
 *   TRUE : xgpt channel has issued an interrupt. 
 *   FALSE: xgpt channel hasn't issued an interrupt.  
 ******************************************************************************/
BOOL __tcmfunc XGPT_Check_IRQSTA(XGPT_NUM numXGPT)
{
    BOOL sta;

    sta = __raw_readl(MT6516_XGPT_IRQSTA);

    return (sta & g_xgpt_mask[numXGPT]);

}

 /******************************************************************************
 * DESCRIPTION:
 *   Clean IRQ status bit by writing IRQ ack bit
 * PARAMETERS: 
 *   XGPT number
 ******************************************************************************/
void __tcmfunc XGPT_AckIRQ(XGPT_NUM numXGPT)
{
	spin_lock_irqsave(&g_xgpt_lock,g_xgpt_flags);	

    if(XGPT1 != numXGPT)		
    	DRV_SetReg32(MT6516_XGPT_IRQACK, g_xgpt_mask[numXGPT]);

    spin_unlock_irqrestore(&g_xgpt_lock,g_xgpt_flags);	
}

 /******************************************************************************
 * DESCRIPTION:
 *   Check whether XGPT is running or not
 * PARAMETERS: 
 *   XGPT number
 * RETURNS: 
 *   TRUE : running
 *   FALSE: stop
 ******************************************************************************/
BOOL XGPT_IsStart(XGPT_NUM numXGPT)
{
    return (__raw_readl(g_xgpt_addr_con[numXGPT] ) & XGPT_CON_ENABLE)? TRUE : FALSE;
}


 /******************************************************************************
 * DESCRIPTION:
 *   Start XGPT Timer
 * PARAMETERS: 
 *   XGPT number
 ******************************************************************************/
void XGPT_Start(XGPT_NUM numXGPT)
{
	spin_lock_irqsave(&g_xgpt_lock,g_xgpt_flags);	

   	if(XGPT_IsStart(numXGPT)==FALSE)
   	{	g_xgpt_status[numXGPT]=USED;
   		DRV_SetReg32(g_xgpt_addr_con[numXGPT] , XGPT_CON_ENABLE|XGPT_CON_CLEAR);        
   	}

    spin_unlock_irqrestore(&g_xgpt_lock,g_xgpt_flags);	
}

void XGPT_Restart(XGPT_NUM numXGPT)
{
	spin_lock_irqsave(&g_xgpt_lock,g_xgpt_flags);	

   	if(XGPT_IsStart(numXGPT)==FALSE)
   	{	g_xgpt_status[numXGPT]=USED;
   		DRV_SetReg32(g_xgpt_addr_con[numXGPT] , XGPT_CON_ENABLE);         
   	}

    spin_unlock_irqrestore(&g_xgpt_lock,g_xgpt_flags);	
}

 /******************************************************************************
 * DESCRIPTION:
 *   Stop XGPT Timer
 * PARAMETERS: 
 *   XGPT number (XGPT2~7)
 ******************************************************************************/
void XGPT_Stop(XGPT_NUM numXGPT)
{
	spin_lock_irqsave(&g_xgpt_lock,g_xgpt_flags);	
    if(XGPT1 != numXGPT)
    {	DRV_ClrReg32(g_xgpt_addr_con[numXGPT] , XGPT_CON_ENABLE);
    	g_xgpt_status[numXGPT]=NOT_USED;    	
    }
    spin_unlock_irqrestore(&g_xgpt_lock,g_xgpt_flags);	
}


 /******************************************************************************
 * DESCRIPTION:
 *   This function is used to clean XGPT counter value
 *   If it has been started, stop the timer. 
 *   After cleaning XGPT counter value, restart this XGPT counter again
 * PARAMETERS: 
 *   XGPT number
 ******************************************************************************/
void XGPT_ClearCount(XGPT_NUM numXGPT)
{
    BOOL bIsStarted;

    //In order to get the counter as 0 after the clear command
    //Stop the timer if it has been started.
    //after get the counter as 0, restore the timer status
    bIsStarted = XGPT_IsStart(numXGPT);
    if (bIsStarted)
    	XGPT_Stop(numXGPT);

    spin_lock_irqsave(&g_xgpt_lock,g_xgpt_flags);	

    DRV_SetReg32(g_xgpt_addr_con[numXGPT] , XGPT_CON_CLEAR);

    spin_unlock_irqrestore(&g_xgpt_lock,g_xgpt_flags);	

    //After setting the clear command, it needs to wait one cycle time of the clock source to become 0
    while (XGPT_GetCounter(numXGPT) != 0){}
    
    if (bIsStarted)
        XGPT_Start(numXGPT);    	
}

 /******************************************************************************
 * DESCRIPTION:
 *   This function is used to set XGPT mode
 *   (XGPT_ONE_SHOT,XGPT_REPEAT,XGPT_KEEP_GO,XGPT_FREE_RUN)
 * PARAMETERS: 
 *   XGPT number (XGPT2~7) and XGPT Mode
 ******************************************************************************/
void XGPT_SetOpMode(XGPT_NUM numXGPT, XGPT_CON_MODE mode)
{
    UINT32 gptMode;
    
    if(XGPT1 != numXGPT)
    {
		gptMode = __raw_readl(g_xgpt_addr_con[numXGPT]) & (~XGPT_FREE_RUN);
		gptMode |= mode;

		spin_lock_irqsave(&g_xgpt_lock,g_xgpt_flags);			

        __raw_writel(gptMode,g_xgpt_addr_con[numXGPT]);

        spin_unlock_irqrestore(&g_xgpt_lock,g_xgpt_flags);	
    }
}

 /******************************************************************************
 * DESCRIPTION:
 *   This function is used to get XGPT mode
 *   (XGPT_ONE_SHOT,XGPT_REPEAT,XGPT_KEEP_GO,XGPT_FREE_RUN)
 * PARAMETERS: 
 *   XGPT number (XGPT1~7)
 * RETURNS: 
 *   XGPT Mode 
 ******************************************************************************/
XGPT_CON_MODE XGPT_GetOpMode(XGPT_NUM numXGPT)
{
    UINT32 u4Con;
    XGPT_CON_MODE mode = XGPT_ONE_SHOT;
    
    u4Con = __raw_readl(g_xgpt_addr_con[numXGPT]) & XGPT_FREE_RUN;
    if (u4Con == XGPT_FREE_RUN)
        mode = XGPT_FREE_RUN;
    else if (u4Con == XGPT_KEEP_GO)
        mode = XGPT_KEEP_GO;
    else if (u4Con == XGPT_REPEAT)
        mode = XGPT_REPEAT;

    return mode;
}

 /******************************************************************************
 * DESCRIPTION:
 *   This function is used to set XGPT clock divisor 
 *   (XGPT_CLK_DIV_1,XGPT_CLK_DIV_2,XGPT_CLK_DIV_4,XGPT_CLK_DIV_8,
 *    XGPT_CLK_DIV_16,XGPT_CLK_DIV_32,XGPT_CLK_DIV_64,XGPT_CLK_DIV_128)
 * PARAMETERS: 
 *   XGPT number (XGPT2~7) and clock divisor
 ******************************************************************************/
void XGPT_SetClkDivisor(XGPT_NUM numXGPT, XGPT_CLK_DIV clkDiv)
{
    UINT32 div;
    
    if(XGPT1 != numXGPT)
    {
		div = __raw_readl(g_xgpt_addr_clk[numXGPT]) & (~XGPT_CLK_DIVISOR_MASK);
		div |= clkDiv;	

		spin_lock_irqsave(&g_xgpt_lock,g_xgpt_flags);		

		__raw_writel(div,g_xgpt_addr_clk[numXGPT]);

	    spin_unlock_irqrestore(&g_xgpt_lock,g_xgpt_flags);	
    }
}

 /******************************************************************************
 * DESCRIPTION:
 *   This function is used to get XGPT clock divisor 
 *   (XGPT_CLK_DIV_1,XGPT_CLK_DIV_2,XGPT_CLK_DIV_4,XGPT_CLK_DIV_8,
 *    XGPT_CLK_DIV_16,XGPT_CLK_DIV_32,XGPT_CLK_DIV_64,XGPT_CLK_DIV_128)
 * PARAMETERS: 
 *   XGPT number (XGPT1~7)
 * RETURNS: 
 *   XGPT clock divisor 
 ******************************************************************************/
XGPT_CLK_DIV XGPT_GetClkDivisor(XGPT_NUM numXGPT)
{
    return __raw_readl(g_xgpt_addr_clk[numXGPT]) & XGPT_CLK_DIV_128;
}


 /******************************************************************************
 * DESCRIPTION:
 *   This function is used to get XGPT counter
 * PARAMETERS: 
 *   XGPT number (XGPT1~7)
 * RETURNS: 
 *   XGPT counter value 
 ******************************************************************************/
UINT32 XGPT_GetCounter(XGPT_NUM numXGPT)
{  
    return __raw_readl(g_xgpt_addr_cnt[numXGPT]);      
}

 
 /******************************************************************************
 * DESCRIPTION:
 *   This function is used to set XGPT compare
 * PARAMETERS: 
 *   XGPT number (XGPT2~7)
 ******************************************************************************/
void XGPT_SetCompare(XGPT_NUM numXGPT, UINT32 u4Compare)
{
    if(XGPT1 != numXGPT)
    {    	
		spin_lock_irqsave(&g_xgpt_lock,g_xgpt_flags);	
    	__raw_writel(u4Compare,g_xgpt_addr_compare[numXGPT]);    
	    spin_unlock_irqrestore(&g_xgpt_lock,g_xgpt_flags);	    	
    }
}


 /******************************************************************************
 * DESCRIPTION:
 *   This function is used to get XGPT compare
 * PARAMETERS: 
 *   XGPT number (XGPT1~7)
 * RETURNS: 
 *   XGPT counter value
 ******************************************************************************/
UINT32 XGPT_GetCompare(XGPT_NUM numXGPT)
{
    return __raw_readl(g_xgpt_addr_compare[numXGPT]);
}

 /******************************************************************************
 * DESCRIPTION:
 *   This function is used to reset XGPT register
 * PARAMETERS: 
 *   XGPT number (XGPT1~5)
 ******************************************************************************/
void XGPT_Reset(XGPT_NUM numXGPT)
{

    // cannot reset XGPT1
    // Reset XGPT0 will cause sysmten timer fail
    if(XGPT1 == numXGPT)
    {	printk("XGPT_Reset : cannot reset system timer (XGPT1)\n");
    	ASSERT(0);
    }
     
    // (1) stop XGPT
    XGPT_Stop(numXGPT);
    
    // (2) Clean XGPT counter
    XGPT_ClearCount(numXGPT);
    
    // (3) Clean XGPT Comparator
    XGPT_SetCompare(numXGPT,0);

    // (4) Clean XGPT Counter
    XGPT_ClearCount(numXGPT);
    
    // (5) Disbale Interrupt
    XGPT_DisableIRQ(numXGPT);
    
    // (6) Default mode should be one shot
    XGPT_SetOpMode(numXGPT,XGPT_ONE_SHOT);   
    
    // (7) Default Clock Divisor should be XGPT_CLK_DIV_1
    XGPT_SetClkDivisor(numXGPT,XGPT_CLK_DIV_1);    
    
}

 /******************************************************************************
 * DESCRIPTION:
 *   This function is used to config XGPT register
 *   (1) mode
 *   (2) clkDiv
 *   (3) bIrqEnable
 *   (4) u4Compare
 * PARAMETERS: 
 *   XGPT configuration data structure
 * RETURNS: 
 *   TRUE : config XGPT pass
 *   FALSE: config XGPT fail
 ******************************************************************************/
BOOL XGPT_Config(XGPT_CONFIG config)
{
    XGPT_NUM num;
    XGPT_CON_MODE mode;
    XGPT_CLK_DIV clkDiv;
    BOOL bIrqEnable;
    UINT32 u4Compare;
    
    BOOL bRet = TRUE;

    num = config.num;
    mode = config.mode;
    clkDiv = config.clkDiv;
    bIrqEnable = config.bIrqEnable;
    u4Compare = config.u4Compare;

   	if(XGPT_IsStart(num))
   	{	printk("XGPT_Config : mt6516 XGPT number %d is already in use\n",num+1);
   		ASSERT(0);
   	}
   	else
   	{	GPT_DEBUG("XGPT_Config : mt6516 XGPT number %d is free to use\n",num+1);
   	}

    XGPT_SetOpMode(num, mode);    

    if (XGPT_GetOpMode(num) != mode)
    {
        bRet = FALSE;
    }          

    XGPT_SetClkDivisor(num, clkDiv);
    if (XGPT_GetClkDivisor(num) != clkDiv)
    {
        bRet = FALSE;
    }

    if (bIrqEnable)
        XGPT_EnableIRQ(num);
    else
        XGPT_DisableIRQ(num);

    XGPT_SetCompare(num, u4Compare);
    if (XGPT_GetCompare(num) != u4Compare)
    {
        bRet = FALSE;
    }    
    
    return bRet;
}

 /******************************************************************************
 *   XGPT LISR to handle XGPT2~7 interrupt
 *   In this function, it will schedule corresonding XGPT tasklet
 ******************************************************************************/
void __tcmfunc XGPT_LISR()
{
    XGPT_NUM numXGPT;

    numXGPT = XGPT_Get_IRQSTA();

    switch(numXGPT)
    {	case XGPT2:
    	     tasklet_schedule(&g_xgpt2_tlet);	    
    	     break;
    	case XGPT3:
    	     tasklet_schedule(&g_xgpt3_tlet);	    
    	     break;
    	case XGPT4:
    	     tasklet_schedule(&g_xgpt4_tlet);	    
    	     break;
    	case XGPT5:
    	     tasklet_schedule(&g_xgpt5_tlet);	    
    	     break;
    	case XGPT6:
    	     tasklet_schedule(&g_xgpt6_tlet);	    
    	     break; 
    	case XGPT7:
    	     tasklet_schedule(&g_xgpt7_tlet);	    
    	     break;     	     
    	case XGPT1:
    	default:
    	     break;
    }

    XGPT_AckIRQ(numXGPT); 
}


 /******************************************************************************
 * DESCRIPTION:
 *   Register XGPT2~7 interrupt handler (call back function)
 * PARAMETERS: 
 *   XGPT number (XGPT2~7) and handler
 ******************************************************************************/

void XGPT_Init(XGPT_NUM timerNum, void (*XGPT_Callback)(UINT16))
{

    if (timerNum == XGPT2)
    {
    	g_xgpt_func.xgpt2_func = XGPT_Callback;
    }
    if (timerNum == XGPT3)
    {
    	g_xgpt_func.xgpt3_func = XGPT_Callback;
    }
    if (timerNum == XGPT4)
    {
    	g_xgpt_func.xgpt4_func = XGPT_Callback;
    }    
    if (timerNum == XGPT5)
    {
    	g_xgpt_func.xgpt5_func = XGPT_Callback;
    }
    if (timerNum == XGPT6)
    {
    	g_xgpt_func.xgpt6_func = XGPT_Callback;
    }
    if (timerNum == XGPT7)
    {
    	g_xgpt_func.xgpt7_func = XGPT_Callback;
    }
}

 /******************************************************************************
 * DESCRIPTION:
 *   Check whether GPT is running or not
 * PARAMETERS: 
 *   GPT number
 * RETURNS: 
 *   TRUE : running
 *   FALSE: stop
 ******************************************************************************/
BOOL GPT_IsStart(GPT_NUM numGPT)
{
    return (__raw_readl(g_gpt_addr_con[numGPT] ) & GPT_CON_ENABLE)? TRUE : FALSE;
}

 /******************************************************************************
 * DESCRIPTION:
 *   Start GPT Timer
 * PARAMETERS: 
 *   GPT number
 ******************************************************************************/
void GPT_Start(GPT_NUM numGPT)
{
	spin_lock_irqsave(&g_gpt_lock,g_gpt_flags);	

   	if(GPT_IsStart(numGPT))
   	{	printk("GPT_Start : mt6516 GPT number %d is already in use\n",numGPT+1);
 	   	ASSERT(0);
   	}
   	else
   	{	g_gpt_status[numGPT]=USED;
   	}
   	
    DRV_SetReg32(g_gpt_addr_con[numGPT] , GPT_CON_ENABLE);             
    spin_unlock_irqrestore(&g_gpt_lock,g_gpt_flags);	
}

 /******************************************************************************
 * DESCRIPTION:
 *   Stop GPT Timer
 * PARAMETERS: 
 *   GPT number (GPT1~3)
 ******************************************************************************/
void GPT_Stop(GPT_NUM numGPT)
{
	spin_lock_irqsave(&g_gpt_lock,g_gpt_flags);	
   	DRV_ClrReg32(g_gpt_addr_con[numGPT] , GPT_CON_ENABLE);
   	g_gpt_status[numGPT]=NOT_USED;       	
    spin_unlock_irqrestore(&g_gpt_lock,g_gpt_flags);	
}

 /******************************************************************************
 * DESCRIPTION:
 *   This function is used to set GPT mode
 *   (GPT_ONE_SHOT,GPT_REPEAT)
 * PARAMETERS: 
 *   GPT number (XGPT1~2) and GPT Mode
 ******************************************************************************/
void GPT_SetOpMode(GPT_NUM numGPT, GPT_CON_MODE mode)
{
    UINT32 gptMode;
    
	gptMode = (__raw_readl(g_gpt_addr_con[numGPT])) & 0xbFFF; // clear mode bit

	gptMode |= mode;

	spin_lock_irqsave(&g_gpt_lock,g_gpt_flags);			

    __raw_writel(gptMode,g_gpt_addr_con[numGPT]);

    spin_unlock_irqrestore(&g_gpt_lock,g_gpt_flags);	
}

 /******************************************************************************
 * DESCRIPTION:
 *   This function is used to get GPT mode
 *   (GPT_ONE_SHOT,GPT_REPEAT)
 * PARAMETERS: 
 *   GPT number (GPT1~2)
 * RETURNS: 
 *   GPT Mode
 ******************************************************************************/
GPT_CON_MODE GPT_GetOpMode(GPT_NUM numGPT)
{
    UINT32 u4Mode;     
    u4Mode = __raw_readl(g_gpt_addr_con[numGPT]) & 0x4000; //get mode bit
    return u4Mode;
}

 /******************************************************************************
 * DESCRIPTION:
 *   This function is used to set GPT clock divisor 
 *   (GPT_CLK_DIV_1,GPT_CLK_DIV_2,GPT_CLK_DIV_4,GPT_CLK_DIV_8,
 *    GPT_CLK_DIV_16,GPT_CLK_DIV_32,GPT_CLK_DIV_64,GPT_CLK_DIV_128)
 * PARAMETERS: 
 *   GPT number (XGPT1~3) and clock divisor
 ******************************************************************************/
void GPT_SetClkDivisor(GPT_NUM numGPT, GPT_CLK_DIV clkDiv)
{
    UINT32 div;
    
	div = __raw_readl(g_gpt_addr_clk[numGPT]) & 0x0000;
	div |= clkDiv;

	spin_lock_irqsave(&g_gpt_lock,g_gpt_flags);		
	__raw_writel(div,g_gpt_addr_clk[numGPT]);
    spin_unlock_irqrestore(&g_gpt_lock,g_gpt_flags);	
}

 /******************************************************************************
 * DESCRIPTION:
 *   This function is used to get GPT clock divisor 
 *   (GPT_CLK_DIV_1,GPT_CLK_DIV_2,GPT_CLK_DIV_4,GPT_CLK_DIV_8,
 *    GPT_CLK_DIV_16,GPT_CLK_DIV_32,GPT_CLK_DIV_64,GPT_CLK_DIV_128)
 * PARAMETERS: 
 *   GPT number (GPT1~3)
 * RETURNS: 
 *   GPT clock divisor 
 ******************************************************************************/
GPT_CLK_DIV GPT_GetClkDivisor(GPT_NUM numGPT)
{
    return __raw_readl(g_gpt_addr_clk[numGPT]);
}

 /******************************************************************************
 * DESCRIPTION:
 *   This function is used to set GPT timeout value
 * PARAMETERS: 
 *   GPT number (GPT1~2)
 ******************************************************************************/
void GPT_SetTimeout(GPT_NUM numGPT, UINT32 u4Dat)
{
	spin_lock_irqsave(&g_gpt_lock,g_gpt_flags);	
   	__raw_writel(u4Dat,g_gpt_addr_dat[numGPT]);    
    spin_unlock_irqrestore(&g_gpt_lock,g_gpt_flags);	    	
}

 /******************************************************************************
 * DESCRIPTION:
 *   This function is used to get GPT timeout value
 * PARAMETERS: 
 *   GPT number (XGPT1~2)
 * RETURNS: 
 *   GPT counter value
 ******************************************************************************/
UINT32 GPT_GetTimeout(GPT_NUM numGPT)
{
    return __raw_readl(g_gpt_addr_dat[numGPT]);
}

 /*****************************************************************************
 * DESCRIPTION:
 *   This function is used to config GPT register
 *   (1) mode
 *   (2) clkDiv
 *   (3) bIrqEnable
 *   (4) u4Compare
 * PARAMETERS: 
 *   GPT configuration data structure
 * RETURNS: 
 *   TRUE : config GPT pass
 *   FALSE: config GPT fail
 ******************************************************************************/
BOOL GPT_Config(GPT_CONFIG config)
{
    GPT_NUM num;
    GPT_CON_MODE mode;
    GPT_CLK_DIV clkDiv;
    UINT32 u4Timeout;
    
    BOOL bRet = TRUE;

    num = config.num;
    mode = config.mode;
    clkDiv = config.clkDiv;
    u4Timeout = config.u4Timeout;

   	if(GPT_IsStart(num))
   	{	printk("MT6516 GPT number %d is already in use\n",num+1);
   		ASSERT(0);
   	}
   	else
   	{	GPT_DEBUG("MT6516 GPT number %d is free to use\n",num+1);
   	}

   	if(num==GPT3)
   	{	printk("GPT_Config : MT6516 GPT3 shouldn't be used, please use system timer to profile\n");
 	   	ASSERT(0);
   	}

    GPT_SetOpMode(num, mode);    

    if (GPT_GetOpMode(num) != mode)
    {
        bRet = FALSE;
    }          

    GPT_SetClkDivisor(num, clkDiv);
    if (GPT_GetClkDivisor(num) != clkDiv)
    {
        bRet = FALSE;
    }

	//GPT interrupt is enable

    GPT_SetTimeout(num, u4Timeout);
    if (GPT_GetTimeout(num) != u4Timeout)
    {
        bRet = FALSE;
    }    
    
    return bRet;
}

 /******************************************************************************
 * DESCRIPTION:
 *   Register GPT1~2 interrupt handler (call back function)
 * PARAMETERS: 
 *   GPT number (GPT1~2) and handler
 ******************************************************************************/

void GPT_Init(GPT_NUM timerNum, void (*GPT_Callback)(UINT16))
{

    if (timerNum == GPT1)
    {
    	g_gpt_func.gpt1_func = GPT_Callback;
    }
    else if (timerNum == GPT2)
    {
    	g_gpt_func.gpt2_func = GPT_Callback;
    }
    else
    {	printk("GPT_Init : gpt number %d is wrong\n",timerNum+1);
    	ASSERT(0);
    }
}

 /******************************************************************************
 *   GPT LISR to handle GPT1~2 interrupt 
 *   ( it will schedule corresonding GPT tasklet )
 ******************************************************************************/
static __tcmfunc irqreturn_t GPT_LISR(int irq, void *dev_id)
{
    UINT32 u4GPT;

    u4GPT = __raw_readl(MT6516_GPT_STA);

    GPT_DEBUG("GPT_LISR : MT6516_GPT_STA = %x\n",u4GPT);

    if(u4GPT == 1)
    {	tasklet_schedule(&g_gpt1_tlet);	    
    }
    else if(u4GPT == 2) 
    {	tasklet_schedule(&g_gpt2_tlet);	    
    }
    else if(u4GPT == 3)
    {	tasklet_schedule(&g_gpt1_tlet);	    
   		tasklet_schedule(&g_gpt2_tlet);	    
    }
    else
    {	printk("GPT_LISR : gpt number is wrong\n");
    	ASSERT(0);
    }

   	return IRQ_HANDLED;
}


static int XGPT_Proc(char *page, char **start, off_t off,int count, int *eof, void *data)
{
  char *p = page;
  int len = 0; 

  p += sprintf(p, "\n\r(HW Timer) XGPT Status :\n\r" );
  p += sprintf(p, "=========================================\n\r" ); 
  p += sprintf(p, "0 = disable, 1 = enable\n\r" );  
  p += sprintf(p, "XGPT1 : %d\n\r",XGPT_IsStart(XGPT1));   
  p += sprintf(p, "XGPT2 : %d\n\r",XGPT_IsStart(XGPT2));   
  p += sprintf(p, "XGPT3 : %d\n\r",XGPT_IsStart(XGPT3));  
  p += sprintf(p, "XGPT4 : %d\n\r",XGPT_IsStart(XGPT4));  
  p += sprintf(p, "XGPT5 : %d\n\r",XGPT_IsStart(XGPT5));  
  p += sprintf(p, "XGPT6 : %d\n\r",XGPT_IsStart(XGPT6));  
  p += sprintf(p, "XGPT7 : %d\n\n",XGPT_IsStart(XGPT7));    
  
  *start = page + off;

  len = p - page;
  if (len > off)
		len -= off;
  else
		len = 0;

  return len < count ? len  : count;     
}

static int GPT_Proc(char *page, char **start, off_t off,int count, int *eof, void *data)
{
  char *p = page;
  int len = 0; 

  p += sprintf(p, "\n\r(HW Timer) GPT Status :\n\r" );
  p += sprintf(p, "=========================================\n\r" ); 
  p += sprintf(p, "0 = disable, 1 = enable\n\r" );    
  p += sprintf(p, "GPT1 : %d\n\r",XGPT_IsStart(GPT1));   
  p += sprintf(p, "GPT2 : %d\n\r",XGPT_IsStart(GPT2));   
  p += sprintf(p, "GPT3 : %d\n\n",XGPT_IsStart(GPT3));  
  
  *start = page + off;

  len = p - page;
  if (len > off)
		len -= off;
  else
		len = 0;

  return len < count ? len  : count; 
}

static struct irqaction MT6516_GPT_ISR =
{
    .name       = "MT6516 GPT timer",
    .flags      = IRQF_DISABLED | IRQF_TIMER | IRQF_IRQPOLL,
    .handler    = GPT_LISR,
};

 /******************************************************************************
 *   Initialize XGPT/GPT Module
 ******************************************************************************/
static int __init mt_init_xgpt_gpt(void){   

    /* register the tasklet */
    tasklet_init(&g_xgpt2_tlet,XGPT2_Tasklet, 0);
    tasklet_init(&g_xgpt3_tlet,XGPT3_Tasklet, 0);
    tasklet_init(&g_xgpt4_tlet,XGPT4_Tasklet, 0);
    tasklet_init(&g_xgpt5_tlet,XGPT5_Tasklet, 0);           
    tasklet_init(&g_xgpt6_tlet,XGPT6_Tasklet, 0);
    tasklet_init(&g_xgpt7_tlet,XGPT7_Tasklet, 0);    
    printk("MT6516 XGPT is intialized!!\n");

    tasklet_init(&g_gpt1_tlet,GPT1_Tasklet, 0);
    tasklet_init(&g_gpt2_tlet,GPT2_Tasklet, 0);    
    printk("MT6516 GPT is intialized!!\n");

    // initialize gpt 
    DRV_SetReg32(0xF0039340, 0x00000010);   //enable power bit
    #ifndef MT6516_IRQ_APGPT_CODE
    #define MT6516_IRQ_APGPT_CODE 0x6
    #endif
    if(setup_irq(MT6516_IRQ_APGPT_CODE, &MT6516_GPT_ISR))
    {	printk("MT6516 GPT irq line is NOT AVAILABLE!!\n");	
    }
    MT6516_IRQSensitivity(MT6516_IRQ_APGPT_CODE, MT6516_LEVEL_SENSITIVE);
    MT6516_IRQUnmask(MT6516_IRQ_APGPT_CODE);


    // Shu-Hsin: 20090910 Crate proc entry at /proc/xgpt_stat
    create_proc_read_entry("xgpt_stat", S_IRUGO, NULL, XGPT_Proc, NULL);


    // Shu-Hsin: 20090910 Crate proc entry at /proc/gpt_stat
    create_proc_read_entry("gpt_stat", S_IRUGO, NULL, GPT_Proc, NULL);  
    

    return 0;
}


static void __exit mt_exit_xgpt_gpt(void)
{	printk("MT6516 XGPT is unintialized!!\n");
	printk("MT6516 GPT is unintialized!!\n");
}

arch_initcall(mt_init_xgpt_gpt);
module_exit(mt_exit_xgpt_gpt);

EXPORT_SYMBOL(XGPT_EnableIRQ);
EXPORT_SYMBOL(XGPT_DisableIRQ);
EXPORT_SYMBOL(XGPT_IsIRQEnable);
EXPORT_SYMBOL(XGPT_Get_IRQSTA);
EXPORT_SYMBOL(XGPT_AckIRQ);
EXPORT_SYMBOL(XGPT_Start);
EXPORT_SYMBOL(XGPT_Stop);
EXPORT_SYMBOL(XGPT_IsStart);
EXPORT_SYMBOL(XGPT_ClearCount);
EXPORT_SYMBOL(XGPT_SetOpMode);
EXPORT_SYMBOL(XGPT_GetOpMode);
EXPORT_SYMBOL(XGPT_SetClkDivisor);
EXPORT_SYMBOL(XGPT_GetClkDivisor);
EXPORT_SYMBOL(XGPT_GetCounter);
EXPORT_SYMBOL(XGPT_SetCompare);
EXPORT_SYMBOL(XGPT_GetCompare);
EXPORT_SYMBOL(XGPT_Reset);
EXPORT_SYMBOL(XGPT_Config);
EXPORT_SYMBOL(XGPT_Check_IRQSTA);
EXPORT_SYMBOL(XGPT_LISR);
EXPORT_SYMBOL(XGPT_Init);

EXPORT_SYMBOL(GPT_Start);
EXPORT_SYMBOL(GPT_Stop);
EXPORT_SYMBOL(GPT_SetOpMode);
EXPORT_SYMBOL(GPT_GetOpMode);
EXPORT_SYMBOL(GPT_SetClkDivisor);
EXPORT_SYMBOL(GPT_GetClkDivisor);
EXPORT_SYMBOL(GPT_GetTimeout);
EXPORT_SYMBOL(GPT_SetTimeout);
EXPORT_SYMBOL(GPT_Config);
EXPORT_SYMBOL(GPT_LISR);
EXPORT_SYMBOL(GPT_Init);

MODULE_DESCRIPTION("MT6516 general purpose timer");

