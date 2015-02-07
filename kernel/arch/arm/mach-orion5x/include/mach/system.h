

#ifndef __ASM_ARCH_SYSTEM_H
#define __ASM_ARCH_SYSTEM_H

#include <mach/hardware.h>
#include <mach/orion5x.h>

static inline void arch_idle(void)
{
	cpu_do_idle();
}

static inline void arch_reset(char mode)
{
	/*
	 * Enable and issue soft reset
	 */
	orion5x_setbits(CPU_RESET_MASK, (1 << 2));
	orion5x_setbits(CPU_SOFT_RESET, 1);
}


#endif
