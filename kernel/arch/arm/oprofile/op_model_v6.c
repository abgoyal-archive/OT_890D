

/* #define DEBUG */
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/oprofile.h>
#include <linux/interrupt.h>
#include <asm/irq.h>
#include <asm/system.h>

#include "op_counter.h"
#include "op_arm_model.h"
#include "op_model_arm11_core.h"

static int irqs[] = {
#ifdef CONFIG_ARCH_OMAP2
	3,
#endif
};

static void armv6_pmu_stop(void)
{
	arm11_stop_pmu();
	arm11_release_interrupts(irqs, ARRAY_SIZE(irqs));
}

static int armv6_pmu_start(void)
{
	int ret;

	ret = arm11_request_interrupts(irqs, ARRAY_SIZE(irqs));
	if (ret >= 0)
		ret = arm11_start_pmu();

	return ret;
}

static int armv6_detect_pmu(void)
{
	return 0;
}

struct op_arm_model_spec op_armv6_spec = {
	.init		= armv6_detect_pmu,
	.num_counters	= 3,
	.setup_ctrs	= arm11_setup_pmu,
	.start		= armv6_pmu_start,
	.stop		= armv6_pmu_stop,
	.name		= "arm/armv6",
};
