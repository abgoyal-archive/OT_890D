

#ifndef __ASM_ARCH_SYSTEM_H
#define __ASM_ARCH_SYSTEM_H

#include <mach/mt6516.h>
#include <mach/mt6516_typedefs.h>
#include <mach/mt6516_apmcusys.h>
#include <mach/mt6516_gpt_sw.h>

extern void MT6516_Idle(void);  
extern void rtc_mark_swreset(void);

unsigned long enter_idle_times=0;
unsigned long all_idle_time=0;
unsigned long total_time=0;

void MT6516_idle(void)
{
    MT6516_Idle();
}    

void XGPTConfig_DVFS(XGPT_NUM gpt_num) {
  XGPT_CONFIG config;
  XGPT_CLK_DIV clkDiv = XGPT_CLK_DIV_1;
  XGPT_Reset(XGPT7);
  config.num          = gpt_num;
  config.mode         = XGPT_FREE_RUN;
  config.clkDiv       = clkDiv;
  config.bIrqEnable   = TRUE;
  config.u4Compare    = 0;
  if (XGPT_Config(config) == FALSE) return;                    
}


#if 0
void arch_idle(void)
{
	MT6516_idle();
}
#else
void arch_idle(void)
{
	unsigned long exec_time_idle_1=0,exec_time_idle_2=0;
	
	enter_idle_times++;
	
	XGPTConfig_DVFS(XGPT7);
	exec_time_idle_1 = 0;
    XGPT_Start(XGPT7);
	
	MT6516_idle();
	
	XGPT_Stop(XGPT7);
    exec_time_idle_2 = XGPT_GetCounter(XGPT7)*30;
    XGPT_Restart(XGPT7);

	all_idle_time = exec_time_idle_2 - exec_time_idle_1;
	total_time += all_idle_time;
	
}
#endif

void arch_reset(char mode)
{
	printk("MT6516 SW Reset\n");

	DRV_WriteReg32(WDT_MODE, 0x2221);
	DRV_WriteReg32(WDT_RESTART, 0x1971);
	DRV_WriteReg32(WDT_SWRST, 0x1209);

	/* enter loop waiting for restart */
	while(1);
}

void aee_bug(const char *source, const char *msg)
{
	char final_msg[128];

	struct pt_regs regs;
	/* Grab function backtrace */
	asm volatile(
	    "stmia %0, {r0 - r15}\n\t"
	    :
	    : "r" (&regs)
	    : "memory");
	regs.ARM_cpsr |= SYSTEM_MODE;

	snprintf(final_msg, sizeof(final_msg), "AEE[%s] - %s", source, msg);
	die(final_msg, &regs, 0);
}

EXPORT_SYMBOL(aee_bug);

#endif
