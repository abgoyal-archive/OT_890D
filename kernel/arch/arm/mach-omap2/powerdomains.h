

#ifndef ARCH_ARM_MACH_OMAP2_POWERDOMAINS
#define ARCH_ARM_MACH_OMAP2_POWERDOMAINS




#include <mach/powerdomain.h>

#include "prcm-common.h"
#include "prm.h"
#include "cm.h"

/* OMAP2/3-common powerdomains and wakeup dependencies */

static struct pwrdm_dep gfx_sgx_wkdeps[] = {
	{
		.pwrdm_name = "core_pwrdm",
		.omap_chip = OMAP_CHIP_INIT(CHIP_IS_OMAP24XX)
	},
	{
		.pwrdm_name = "iva2_pwrdm",
		.omap_chip = OMAP_CHIP_INIT(CHIP_IS_OMAP3430)
	},
	{
		.pwrdm_name = "mpu_pwrdm",
		.omap_chip = OMAP_CHIP_INIT(CHIP_IS_OMAP24XX |
					    CHIP_IS_OMAP3430)
	},
	{
		.pwrdm_name = "wkup_pwrdm",
		.omap_chip = OMAP_CHIP_INIT(CHIP_IS_OMAP24XX |
					    CHIP_IS_OMAP3430)
	},
	{ NULL },
};

static struct pwrdm_dep cam_gfx_sleepdeps[] = {
	{
		.pwrdm_name = "mpu_pwrdm",
		.omap_chip = OMAP_CHIP_INIT(CHIP_IS_OMAP3430)
	},
	{ NULL },
};


#include "powerdomains24xx.h"
#include "powerdomains34xx.h"



static struct powerdomain gfx_pwrdm = {
	.name		  = "gfx_pwrdm",
	.prcm_offs	  = GFX_MOD,
	.omap_chip	  = OMAP_CHIP_INIT(CHIP_IS_OMAP24XX |
					   CHIP_IS_OMAP3430ES1),
	.wkdep_srcs	  = gfx_sgx_wkdeps,
	.sleepdep_srcs	  = cam_gfx_sleepdeps,
	.pwrsts		  = PWRSTS_OFF_RET_ON,
	.pwrsts_logic_ret = PWRDM_POWER_RET,
	.banks		  = 1,
	.pwrsts_mem_ret	  = {
		[0] = PWRDM_POWER_RET, /* MEMRETSTATE */
	},
	.pwrsts_mem_on	  = {
		[0] = PWRDM_POWER_ON,  /* MEMONSTATE */
	},
};

static struct powerdomain wkup_pwrdm = {
	.name		= "wkup_pwrdm",
	.prcm_offs	= WKUP_MOD,
	.omap_chip	= OMAP_CHIP_INIT(CHIP_IS_OMAP24XX | CHIP_IS_OMAP3430),
	.dep_bit	= OMAP_EN_WKUP_SHIFT,
};



/* As powerdomains are added or removed above, this list must also be changed */
static struct powerdomain *powerdomains_omap[] __initdata = {

	&gfx_pwrdm,
	&wkup_pwrdm,

#ifdef CONFIG_ARCH_OMAP24XX
	&dsp_pwrdm,
	&mpu_24xx_pwrdm,
	&core_24xx_pwrdm,
#endif

#ifdef CONFIG_ARCH_OMAP2430
	&mdm_pwrdm,
#endif

#ifdef CONFIG_ARCH_OMAP34XX
	&iva2_pwrdm,
	&mpu_34xx_pwrdm,
	&neon_pwrdm,
	&core_34xx_pwrdm,
	&cam_pwrdm,
	&dss_pwrdm,
	&per_pwrdm,
	&emu_pwrdm,
	&sgx_pwrdm,
	&usbhost_pwrdm,
#endif

	NULL
};


#endif
