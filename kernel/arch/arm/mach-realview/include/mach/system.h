
#ifndef __ASM_ARCH_SYSTEM_H
#define __ASM_ARCH_SYSTEM_H

#include <linux/io.h>
#include <mach/hardware.h>
#include <mach/platform.h>

static inline void arch_idle(void)
{
	/*
	 * This should do all the clock switching
	 * and wait for interrupt tricks
	 */
	cpu_do_idle();
}

static inline void arch_reset(char mode)
{
	void __iomem *hdr_ctrl = __io_address(REALVIEW_SYS_BASE) + REALVIEW_SYS_RESETCTL_OFFSET;
	unsigned int val;

	/*
	 * To reset, we hit the on-board reset register
	 * in the system FPGA
	 */
	val = __raw_readl(hdr_ctrl);
	val |= REALVIEW_SYS_CTRL_RESET_CONFIGCLR;
	__raw_writel(val, hdr_ctrl);
}

#endif
