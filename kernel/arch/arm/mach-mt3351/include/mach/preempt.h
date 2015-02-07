

#ifndef _ASM_ARCH_PREEMT_H
#define _ASM_ARCH_PREEMT_H

static inline unsigned long clock_diff(unsigned long start, unsigned long stop)
{
	return (stop - start);
}

extern unsigned long mt3351_timer_read(int nr);


#define TICKS_PER_USEC                  100
#define ARCH_PREDEFINES_TICKS_PER_USEC
#define readclock()                     (~mt5351_timer_read(1))
#define clock_to_usecs(x)               ((x) / TICKS_PER_USEC)
#define INTERRUPTS_ENABLED(x)           (!(x & PSR_I_BIT))

#endif