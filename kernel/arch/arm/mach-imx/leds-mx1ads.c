

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <mach/hardware.h>
#include <asm/system.h>
#include <asm/leds.h>
#include "leds.h"

void
mx1ads_leds_event(led_event_t ledevt)
{
	unsigned long flags;

	local_irq_save(flags);

	switch (ledevt) {
#ifdef CONFIG_LEDS_CPU
	case led_idle_start:
		DR(0) &= ~(1<<2);
		break;

	case led_idle_end:
		DR(0) |= 1<<2;
		break;
#endif

#ifdef CONFIG_LEDS_TIMER
	case led_timer:
		DR(0) ^= 1<<2;
#endif
	default:
		break;
	}
	local_irq_restore(flags);
}
