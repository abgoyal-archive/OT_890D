
#ifndef _ASM_TIMEX_H
#define _ASM_TIMEX_H

#ifdef __KERNEL__

#include <asm/mipsregs.h>

#define CLOCK_TICK_RATE 1193182


typedef unsigned int cycles_t;

static inline cycles_t get_cycles(void)
{
	return 0;
}

#endif /* __KERNEL__ */

#endif /*  _ASM_TIMEX_H */
