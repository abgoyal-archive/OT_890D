

#include <mach/hardware.h>

void arch_idle(void);

static inline void arch_reset(char mode)
{
	for (;;) ;  /* depends on IPC w/ other core */
}
