

#include <linux/suspend.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/interrupt.h>
#include <linux/sysfs.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/io.h>

#include <asm/irq.h>
#include <asm/atomic.h>
#include <asm/mach/time.h>
#include <asm/mach/irq.h>

#include <mach/irqs.h>
#include <mach/clock.h>
#include <mach/sram.h>
#include <mach/pm.h>

static struct clk *vclk;
static void (*omap2_sram_idle)(void);
static void (*omap2_sram_suspend)(int dllctrl, int cpu_rev);
static void (*saved_idle)(void);

extern void __init pmdomain_init(void);
extern void pmdomain_set_autoidle(void);

static unsigned int omap24xx_sleep_save[OMAP24XX_SLEEP_SAVE_SIZE];

void omap2_pm_idle(void)
{
	local_irq_disable();
	local_fiq_disable();
	if (need_resched()) {
		local_fiq_enable();
		local_irq_enable();
		return;
	}

	omap2_sram_idle();
	local_fiq_enable();
	local_irq_enable();
}

static int omap2_pm_prepare(void)
{
	/* We cannot sleep in idle until we have resumed */
	saved_idle = pm_idle;
	pm_idle = NULL;
	return 0;
}

static int omap2_pm_suspend(void)
{
	return 0;
}

static int omap2_pm_enter(suspend_state_t state)
{
	int ret = 0;

	switch (state)
	{
	case PM_SUSPEND_STANDBY:
	case PM_SUSPEND_MEM:
		ret = omap2_pm_suspend();
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static void omap2_pm_finish(void)
{
	pm_idle = saved_idle;
}

static struct platform_suspend_ops omap_pm_ops = {
	.prepare	= omap2_pm_prepare,
	.enter		= omap2_pm_enter,
	.finish		= omap2_pm_finish,
	.valid		= suspend_valid_only_mem,
};

int __init omap2_pm_init(void)
{
	return 0;
}

__initcall(omap2_pm_init);
