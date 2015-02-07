
#include <linux/clocksource.h>
#include <linux/jiffies.h>
#include <linux/init.h>

#define NSEC_PER_JIFFY	((u32)((((u64)NSEC_PER_SEC)<<8)/ACTHZ))

#define JIFFIES_SHIFT	8

static cycle_t jiffies_read(void)
{
	return (cycle_t) jiffies;
}

struct clocksource clocksource_jiffies = {
	.name		= "jiffies",
	.rating		= 1, /* lowest valid rating*/
	.read		= jiffies_read,
	.mask		= 0xffffffff, /*32bits*/
	.mult		= NSEC_PER_JIFFY << JIFFIES_SHIFT, /* details above */
	.mult_orig	= NSEC_PER_JIFFY << JIFFIES_SHIFT,
	.shift		= JIFFIES_SHIFT,
};

static int __init init_jiffies_clocksource(void)
{
	return clocksource_register(&clocksource_jiffies);
}

core_initcall(init_jiffies_clocksource);
