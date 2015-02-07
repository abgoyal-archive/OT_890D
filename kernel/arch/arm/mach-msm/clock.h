

#ifndef __ARCH_ARM_MACH_MSM_CLOCK_H
#define __ARCH_ARM_MACH_MSM_CLOCK_H

#include <linux/list.h>

#define CLKFLAG_INVERT			0x00000001
#define CLKFLAG_NOINVERT		0x00000002
#define CLKFLAG_NONEST			0x00000004
#define CLKFLAG_NORESET			0x00000008

#define CLK_FIRST_AVAILABLE_FLAG	0x00000100
#define CLKFLAG_USE_MIN_MAX_TO_SET	0x00000200
#define CLKFLAG_AUTO_OFF		0x00000400

struct clk {
	uint32_t id;
	uint32_t count;
	uint32_t flags;
	const char *name;
	struct list_head list;
	struct device *dev;
};

#define A11S_CLK_CNTL_ADDR		(MSM_CSR_BASE + 0x100)
#define A11S_CLK_SEL_ADDR		(MSM_CSR_BASE + 0x104)
#define A11S_VDD_SVS_PLEVEL_ADDR	(MSM_CSR_BASE + 0x124)

extern struct clk msm_clocks[];
extern unsigned msm_num_clocks;

#endif

