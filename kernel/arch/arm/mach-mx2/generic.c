

#include <linux/mm.h>
#include <linux/init.h>
#include <mach/hardware.h>
#include <asm/pgtable.h>
#include <asm/mach/map.h>

/* MX27 memory map definition */
static struct map_desc mxc_io_desc[] __initdata = {
	/*
	 * this fixed mapping covers:
	 * - AIPI1
	 * - AIPI2
	 * - AITC
	 * - ROM Patch
	 * - and some reserved space
	 */
	{
		.virtual = AIPI_BASE_ADDR_VIRT,
		.pfn = __phys_to_pfn(AIPI_BASE_ADDR),
		.length = AIPI_SIZE,
		.type = MT_DEVICE
	},
	/*
	 * this fixed mapping covers:
	 * - CSI
	 * - ATA
	 */
	{
		.virtual = SAHB1_BASE_ADDR_VIRT,
		.pfn = __phys_to_pfn(SAHB1_BASE_ADDR),
		.length = SAHB1_SIZE,
		.type = MT_DEVICE
	},
	/*
	 * this fixed mapping covers:
	 * - EMI
	 */
	{
		.virtual = X_MEMC_BASE_ADDR_VIRT,
		.pfn = __phys_to_pfn(X_MEMC_BASE_ADDR),
		.length = X_MEMC_SIZE,
		.type = MT_DEVICE
	}
};

void __init mxc_map_io(void)
{
	iotable_init(mxc_io_desc, ARRAY_SIZE(mxc_io_desc));
}
