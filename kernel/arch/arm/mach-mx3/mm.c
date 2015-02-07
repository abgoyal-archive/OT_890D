

#include <linux/mm.h>
#include <linux/init.h>
#include <mach/hardware.h>
#include <asm/pgtable.h>
#include <asm/mach/map.h>
#include <mach/common.h>


static struct map_desc mxc_io_desc[] __initdata = {
	{
		.virtual	= X_MEMC_BASE_ADDR_VIRT,
		.pfn		= __phys_to_pfn(X_MEMC_BASE_ADDR),
		.length		= X_MEMC_SIZE,
		.type		= MT_DEVICE
	}, {
		.virtual	= AVIC_BASE_ADDR_VIRT,
		.pfn		= __phys_to_pfn(AVIC_BASE_ADDR),
		.length		= AVIC_SIZE,
		.type		= MT_DEVICE_NONSHARED
	},
};

void __init mxc_map_io(void)
{
	iotable_init(mxc_io_desc, ARRAY_SIZE(mxc_io_desc));
}
