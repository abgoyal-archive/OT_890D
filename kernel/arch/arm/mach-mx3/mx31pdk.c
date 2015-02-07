

#include <linux/types.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/irq.h>

#include <mach/hardware.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/time.h>
#include <asm/memory.h>
#include <asm/mach/map.h>
#include <mach/common.h>
#include <mach/board-mx31pdk.h>
#include <mach/imx-uart.h>
#include <mach/iomux-mx3.h>
#include "devices.h"


static struct imxuart_platform_data uart_pdata = {
	.flags = IMXUART_HAVE_RTSCTS,
};

static inline void mxc_init_imx_uart(void)
{
	mxc_iomux_mode(MX31_PIN_CTS1__CTS1);
	mxc_iomux_mode(MX31_PIN_RTS1__RTS1);
	mxc_iomux_mode(MX31_PIN_TXD1__TXD1);
	mxc_iomux_mode(MX31_PIN_RXD1__RXD1);

	mxc_register_device(&mxc_uart_device0, &uart_pdata);
}

static struct map_desc mx31pdk_io_desc[] __initdata = {
	{
		.virtual	= AIPS1_BASE_ADDR_VIRT,
		.pfn		= __phys_to_pfn(AIPS1_BASE_ADDR),
		.length		= AIPS1_SIZE,
		.type		= MT_DEVICE_NONSHARED
	}, {
		.virtual	= AIPS2_BASE_ADDR_VIRT,
		.pfn		= __phys_to_pfn(AIPS2_BASE_ADDR),
		.length		= AIPS2_SIZE,
		.type		= MT_DEVICE_NONSHARED
	},
};

static void __init mx31pdk_map_io(void)
{
	mxc_map_io();
	iotable_init(mx31pdk_io_desc, ARRAY_SIZE(mx31pdk_io_desc));
}

static void __init mxc_board_init(void)
{
	mxc_init_imx_uart();
}

static void __init mx31pdk_timer_init(void)
{
	mxc_clocks_init(26000000);
	mxc_timer_init("ipg_clk.0");
}

static struct sys_timer mx31pdk_timer = {
	.init	= mx31pdk_timer_init,
};

MACHINE_START(MX31_3DS, "Freescale MX31PDK (3DS)")
	/* Maintainer: Freescale Semiconductor, Inc. */
	.phys_io	= AIPS1_BASE_ADDR,
	.io_pg_offst	= ((AIPS1_BASE_ADDR_VIRT) >> 18) & 0xfffc,
	.boot_params    = PHYS_OFFSET + 0x100,
	.map_io         = mx31pdk_map_io,
	.init_irq       = mxc_init_irq,
	.init_machine   = mxc_board_init,
	.timer          = &mx31pdk_timer,
MACHINE_END
