
#ifndef __ASM_ARCH_SYSTEM_H
#define __ASM_ARCH_SYSTEM_H

#include <linux/io.h>
#include <mach/hardware.h>

extern void davinci_watchdog_reset(void);

static void arch_idle(void)
{
	cpu_do_idle();
}

static void arch_reset(char mode)
{
	davinci_watchdog_reset();
}

#endif /* __ASM_ARCH_SYSTEM_H */
