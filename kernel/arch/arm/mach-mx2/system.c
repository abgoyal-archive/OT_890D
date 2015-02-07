

#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/io.h>

#include <mach/hardware.h>
#include <asm/proc-fns.h>
#include <asm/system.h>

void arch_idle(void)
{
	/*
	 * This should do all the clock switching
	 * and wait for interrupt tricks.
	 */
	cpu_do_idle();
}

#define WDOG_WCR_REG                    IO_ADDRESS(WDOG_BASE_ADDR)
#define WDOG_WCR_SRS                    (1 << 4)

void arch_reset(char mode)
{
	struct clk *clk;

	clk = clk_get(NULL, "wdog_clk");
	if (!clk) {
		printk(KERN_ERR"Cannot activate the watchdog. Giving up\n");
		return;
	}

	clk_enable(clk);

	/* Assert SRS signal */
	__raw_writew(__raw_readw(WDOG_WCR_REG) & ~WDOG_WCR_SRS, WDOG_WCR_REG);
}
