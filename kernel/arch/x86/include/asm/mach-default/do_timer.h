
/* defines for inline arch setup functions */
#include <linux/clockchips.h>

#include <asm/i8259.h>
#include <asm/i8253.h>


static inline void do_timer_interrupt_hook(void)
{
	global_clock_event->event_handler(global_clock_event);
}
