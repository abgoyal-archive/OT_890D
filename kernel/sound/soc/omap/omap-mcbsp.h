

#ifndef __OMAP_I2S_H__
#define __OMAP_I2S_H__

/* Source clocks for McBSP sample rate generator */
enum omap_mcbsp_clksrg_clk {
	OMAP_MCBSP_SYSCLK_CLKS_FCLK,	/* Internal FCLK */
	OMAP_MCBSP_SYSCLK_CLKS_EXT,	/* External CLKS pin */
	OMAP_MCBSP_SYSCLK_CLK,		/* Internal ICLK */
	OMAP_MCBSP_SYSCLK_CLKX_EXT,	/* External CLKX pin */
	OMAP_MCBSP_SYSCLK_CLKR_EXT,	/* External CLKR pin */
};

/* McBSP dividers */
enum omap_mcbsp_div {
	OMAP_MCBSP_CLKGDV,		/* Sample rate generator divider */
};

#if defined(CONFIG_ARCH_OMAP2420)
#define NUM_LINKS	2
#endif
#if defined(CONFIG_ARCH_OMAP15XX) || defined(CONFIG_ARCH_OMAP16XX)
#undef  NUM_LINKS
#define NUM_LINKS	3
#endif
#if defined(CONFIG_ARCH_OMAP2430) || defined(CONFIG_ARCH_OMAP34XX)
#undef  NUM_LINKS
#define NUM_LINKS	5
#endif

extern struct snd_soc_dai omap_mcbsp_dai[NUM_LINKS];

#endif
