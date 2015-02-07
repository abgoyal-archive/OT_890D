
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>

#include <asm/mach/map.h>

#include <mach/hardware.h>

static struct map_desc imx_io_desc[] __initdata = {
	{
		.virtual	= IMX_IO_BASE,
		.pfn		= __phys_to_pfn(IMX_IO_PHYS),
		.length		= IMX_IO_SIZE,
		.type		= MT_DEVICE
	}
};

void __init mxc_map_io(void)
{
	iotable_init(imx_io_desc, ARRAY_SIZE(imx_io_desc));
}
