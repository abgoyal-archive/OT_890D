

#include <linux/kernel.h>
#include <linux/init.h>

#include <asm/leds.h>
#include <asm/mach-types.h>

#include "leds.h"

static int __init
leds_init(void)
{
	if (machine_is_mx1ads()) {
		leds_event = mx1ads_leds_event;
	}

	return 0;
}

__initcall(leds_init);
