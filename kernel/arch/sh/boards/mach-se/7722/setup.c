
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/ata_platform.h>
#include <linux/input.h>
#include <linux/smc91x.h>
#include <mach-se/mach/se7722.h>
#include <mach-se/mach/mrshpc.h>
#include <asm/machvec.h>
#include <asm/clock.h>
#include <asm/io.h>
#include <asm/heartbeat.h>
#include <asm/sh_keysc.h>

/* Heartbeat */
static struct heartbeat_data heartbeat_data = {
	.regsize = 16,
};

static struct resource heartbeat_resources[] = {
	[0] = {
		.start  = PA_LED,
		.end    = PA_LED,
		.flags  = IORESOURCE_MEM,
	},
};

static struct platform_device heartbeat_device = {
	.name           = "heartbeat",
	.id             = -1,
	.dev = {
		.platform_data = &heartbeat_data,
	},
	.num_resources  = ARRAY_SIZE(heartbeat_resources),
	.resource       = heartbeat_resources,
};

/* SMC91x */
static struct smc91x_platdata smc91x_info = {
	.flags = SMC91X_USE_16BIT,
};

static struct resource smc91x_eth_resources[] = {
	[0] = {
		.name   = "smc91x-regs" ,
		.start  = PA_LAN + 0x300,
		.end    = PA_LAN + 0x300 + 0x10 ,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = SMC_IRQ,
		.end    = SMC_IRQ,
		.flags  = IORESOURCE_IRQ,
	},
};

static struct platform_device smc91x_eth_device = {
	.name           = "smc91x",
	.id             = 0,
	.dev = {
		.dma_mask               = NULL,         /* don't use dma */
		.coherent_dma_mask      = 0xffffffff,
		.platform_data	= &smc91x_info,
	},
	.num_resources  = ARRAY_SIZE(smc91x_eth_resources),
	.resource       = smc91x_eth_resources,
};

static struct resource cf_ide_resources[] = {
	[0] = {
		.start  = PA_MRSHPC_IO + 0x1f0,
		.end    = PA_MRSHPC_IO + 0x1f0 + 8 ,
		.flags  = IORESOURCE_IO,
	},
	[1] = {
		.start  = PA_MRSHPC_IO + 0x1f0 + 0x206,
		.end    = PA_MRSHPC_IO + 0x1f0 +8 + 0x206 + 8,
		.flags  = IORESOURCE_IO,
	},
	[2] = {
		.start  = MRSHPC_IRQ0,
		.end    = MRSHPC_IRQ0,
		.flags  = IORESOURCE_IRQ,
	},
};

static struct platform_device cf_ide_device  = {
	.name           = "pata_platform",
	.id             = -1,
	.num_resources  = ARRAY_SIZE(cf_ide_resources),
	.resource       = cf_ide_resources,
};

static struct sh_keysc_info sh_keysc_info = {
	.mode = SH_KEYSC_MODE_1, /* KEYOUT0->5, KEYIN0->4 */
	.scan_timing = 3,
	.delay = 5,
	.keycodes = { /* SW1 -> SW30 */
		KEY_A, KEY_B, KEY_C, KEY_D, KEY_E,
		KEY_F, KEY_G, KEY_H, KEY_I, KEY_J,
		KEY_K, KEY_L, KEY_M, KEY_N, KEY_O,
		KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T,
		KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y,
		KEY_Z,
		KEY_HOME, KEY_SLEEP, KEY_WAKEUP, KEY_COFFEE, /* life */
	},
};

static struct resource sh_keysc_resources[] = {
	[0] = {
		.start  = 0x044b0000,
		.end    = 0x044b000f,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = 79,
		.flags  = IORESOURCE_IRQ,
	},
};

static struct platform_device sh_keysc_device = {
	.name           = "sh_keysc",
	.id             = 0, /* "keysc0" clock */
	.num_resources  = ARRAY_SIZE(sh_keysc_resources),
	.resource       = sh_keysc_resources,
	.dev	= {
		.platform_data	= &sh_keysc_info,
	},
};

static struct platform_device *se7722_devices[] __initdata = {
	&heartbeat_device,
	&smc91x_eth_device,
	&cf_ide_device,
	&sh_keysc_device,
};

static int __init se7722_devices_setup(void)
{
	mrshpc_setup_windows();
	return platform_add_devices(se7722_devices, ARRAY_SIZE(se7722_devices));
}
device_initcall(se7722_devices_setup);

static void __init se7722_setup(char **cmdline_p)
{
	ctrl_outw(0x010D, FPGA_OUT);    /* FPGA */

	ctrl_outw(0x0000, PORT_PECR);   /* PORT E 1 = IRQ5 ,E 0 = BS */
	ctrl_outw(0x1000, PORT_PJCR);   /* PORT J 1 = IRQ1,J 0 =IRQ0 */

	/* LCDC I/O */
	ctrl_outw(0x0020, PORT_PSELD);

	/* SIOF1*/
	ctrl_outw(0x0003, PORT_PSELB);
	ctrl_outw(0xe000, PORT_PSELC);
	ctrl_outw(0x0000, PORT_PKCR);

	/* LCDC */
	ctrl_outw(0x4020, PORT_PHCR);
	ctrl_outw(0x0000, PORT_PLCR);
	ctrl_outw(0x0000, PORT_PMCR);
	ctrl_outw(0x0002, PORT_PRCR);
	ctrl_outw(0x0000, PORT_PXCR);   /* LCDC,CS6A */

	/* KEYSC */
	ctrl_outw(0x0A10, PORT_PSELA); /* BS,SHHID2 */
	ctrl_outw(0x0000, PORT_PYCR);
	ctrl_outw(0x0000, PORT_PZCR);
	ctrl_outw(ctrl_inw(PORT_HIZCRA) & ~0x4000, PORT_HIZCRA);
	ctrl_outw(ctrl_inw(PORT_HIZCRC) & ~0xc000, PORT_HIZCRC);
}

static struct sh_machine_vector mv_se7722 __initmv = {
	.mv_name                = "Solution Engine 7722" ,
	.mv_setup               = se7722_setup ,
	.mv_nr_irqs		= SE7722_FPGA_IRQ_BASE + SE7722_FPGA_IRQ_NR,
	.mv_init_irq		= init_se7722_IRQ,
};
