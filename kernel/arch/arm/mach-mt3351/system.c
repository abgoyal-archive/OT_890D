

#ifndef __ASM_ARCH_SYSTEM_H
#define __ASM_ARCH_SYSTEM_H

#include <mach/mt3351.h>
#include <mach/mt3351_typedefs.h>
#include <mach/mt3351_wdt.h>

void MT3351_idle(void)
{
	//Kelvin, 20090203 Inline assembly for ARM11 idle and DCM
	//asm("mcr p15, 0, r0, c7, c0, 4");
}    

void arch_idle(void)
{
	//switch off the system and wait for interrupt
	MT3351_idle();
}

void arch_reset(char mode)
{
	/* William, 20090217 restart by watchdog */
	mt3351_wdt_SetTimeOutValue(1);
	mt3351_wdt_ModeSelection(KAL_TRUE, KAL_TRUE, KAL_TRUE, KAL_FALSE);
	mt3351_wdt_Restart();

	/* enter loop waiting for restart */
	while(1);
}

#endif

