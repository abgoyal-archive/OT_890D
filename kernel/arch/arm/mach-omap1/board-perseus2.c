

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/input.h>

#include <mach/hardware.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/flash.h>
#include <asm/mach/map.h>

#include <mach/tc.h>
#include <mach/gpio.h>
#include <mach/mux.h>
#include <mach/fpga.h>
#include <mach/nand.h>
#include <mach/keypad.h>
#include <mach/common.h>
#include <mach/board.h>

static int p2_keymap[] = {
	KEY(0,0,KEY_UP),
	KEY(0,1,KEY_RIGHT),
	KEY(0,2,KEY_LEFT),
	KEY(0,3,KEY_DOWN),
	KEY(0,4,KEY_ENTER),
	KEY(1,0,KEY_F10),
	KEY(1,1,KEY_SEND),
	KEY(1,2,KEY_END),
	KEY(1,3,KEY_VOLUMEDOWN),
	KEY(1,4,KEY_VOLUMEUP),
	KEY(1,5,KEY_RECORD),
	KEY(2,0,KEY_F9),
	KEY(2,1,KEY_3),
	KEY(2,2,KEY_6),
	KEY(2,3,KEY_9),
	KEY(2,4,KEY_KPDOT),
	KEY(3,0,KEY_BACK),
	KEY(3,1,KEY_2),
	KEY(3,2,KEY_5),
	KEY(3,3,KEY_8),
	KEY(3,4,KEY_0),
	KEY(3,5,KEY_KPSLASH),
	KEY(4,0,KEY_HOME),
	KEY(4,1,KEY_1),
	KEY(4,2,KEY_4),
	KEY(4,3,KEY_7),
	KEY(4,4,KEY_KPASTERISK),
	KEY(4,5,KEY_POWER),
	0
};

static struct resource smc91x_resources[] = {
	[0] = {
		.start	= H2P2_DBG_FPGA_ETHR_START,	/* Physical */
		.end	= H2P2_DBG_FPGA_ETHR_START + 0xf,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= INT_730_MPU_EXT_NIRQ,
		.end	= 0,
		.flags	= IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHEDGE,
	},
};

static struct mtd_partition nor_partitions[] = {
	/* bootloader (U-Boot, etc) in first sector */
	{
	      .name		= "bootloader",
	      .offset		= 0,
	      .size		= SZ_128K,
	      .mask_flags	= MTD_WRITEABLE, /* force read-only */
	},
	/* bootloader params in the next sector */
	{
	      .name		= "params",
	      .offset		= MTDPART_OFS_APPEND,
	      .size		= SZ_128K,
	      .mask_flags	= 0,
	},
	/* kernel */
	{
	      .name		= "kernel",
	      .offset		= MTDPART_OFS_APPEND,
	      .size		= SZ_2M,
	      .mask_flags	= 0
	},
	/* rest of flash is a file system */
	{
	      .name		= "rootfs",
	      .offset		= MTDPART_OFS_APPEND,
	      .size		= MTDPART_SIZ_FULL,
	      .mask_flags	= 0
	},
};

static struct flash_platform_data nor_data = {
	.map_name	= "cfi_probe",
	.width		= 2,
	.parts		= nor_partitions,
	.nr_parts	= ARRAY_SIZE(nor_partitions),
};

static struct resource nor_resource = {
	.start		= OMAP_CS0_PHYS,
	.end		= OMAP_CS0_PHYS + SZ_32M - 1,
	.flags		= IORESOURCE_MEM,
};

static struct platform_device nor_device = {
	.name		= "omapflash",
	.id		= 0,
	.dev		= {
		.platform_data	= &nor_data,
	},
	.num_resources	= 1,
	.resource	= &nor_resource,
};

static struct omap_nand_platform_data nand_data = {
	.options	= NAND_SAMSUNG_LP_OPTIONS,
};

static struct resource nand_resource = {
	.start		= OMAP_CS3_PHYS,
	.end		= OMAP_CS3_PHYS + SZ_4K - 1,
	.flags		= IORESOURCE_MEM,
};

static struct platform_device nand_device = {
	.name		= "omapnand",
	.id		= 0,
	.dev		= {
		.platform_data	= &nand_data,
	},
	.num_resources	= 1,
	.resource	= &nand_resource,
};

static struct platform_device smc91x_device = {
	.name		= "smc91x",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(smc91x_resources),
	.resource	= smc91x_resources,
};

static struct resource kp_resources[] = {
	[0] = {
		.start	= INT_730_MPUIO_KEYPAD,
		.end	= INT_730_MPUIO_KEYPAD,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct omap_kp_platform_data kp_data = {
	.rows		= 8,
	.cols		= 8,
	.keymap		= p2_keymap,
	.keymapsize	= ARRAY_SIZE(p2_keymap),
	.delay		= 4,
	.dbounce	= 1,
};

static struct platform_device kp_device = {
	.name		= "omap-keypad",
	.id		= -1,
	.dev		= {
		.platform_data = &kp_data,
	},
	.num_resources	= ARRAY_SIZE(kp_resources),
	.resource	= kp_resources,
};

static struct platform_device lcd_device = {
	.name		= "lcd_p2",
	.id		= -1,
};

static struct platform_device *devices[] __initdata = {
	&nor_device,
	&nand_device,
	&smc91x_device,
	&kp_device,
	&lcd_device,
};

#define P2_NAND_RB_GPIO_PIN	62

static int nand_dev_ready(struct omap_nand_platform_data *data)
{
	return gpio_get_value(P2_NAND_RB_GPIO_PIN);
}

static struct omap_uart_config perseus2_uart_config __initdata = {
	.enabled_uarts = ((1 << 0) | (1 << 1)),
};

static struct omap_lcd_config perseus2_lcd_config __initdata = {
	.ctrl_name	= "internal",
};

static struct omap_board_config_kernel perseus2_config[] __initdata = {
	{ OMAP_TAG_UART,	&perseus2_uart_config },
	{ OMAP_TAG_LCD,		&perseus2_lcd_config },
};

static void __init omap_perseus2_init(void)
{
	if (gpio_request(P2_NAND_RB_GPIO_PIN, "NAND ready") < 0)
		BUG();
	nand_data.dev_ready = nand_dev_ready;

	omap_cfg_reg(L3_1610_FLASH_CS2B_OE);
	omap_cfg_reg(M8_1610_FLASH_CS2B_WE);

	platform_add_devices(devices, ARRAY_SIZE(devices));

	omap_board_config = perseus2_config;
	omap_board_config_size = ARRAY_SIZE(perseus2_config);
	omap_serial_init();
	omap_register_i2c_bus(1, 100, NULL, 0);
}

static void __init perseus2_init_smc91x(void)
{
	fpga_write(1, H2P2_DBG_FPGA_LAN_RESET);
	mdelay(50);
	fpga_write(fpga_read(H2P2_DBG_FPGA_LAN_RESET) & ~1,
		   H2P2_DBG_FPGA_LAN_RESET);
	mdelay(50);
}

static void __init omap_perseus2_init_irq(void)
{
	omap1_init_common_hw();
	omap_init_irq();
	omap_gpio_init();
	perseus2_init_smc91x();
}
/* Only FPGA needs to be mapped here. All others are done with ioremap */
static struct map_desc omap_perseus2_io_desc[] __initdata = {
	{
		.virtual	= H2P2_DBG_FPGA_BASE,
		.pfn		= __phys_to_pfn(H2P2_DBG_FPGA_START),
		.length		= H2P2_DBG_FPGA_SIZE,
		.type		= MT_DEVICE
	}
};

static void __init omap_perseus2_map_io(void)
{
	omap1_map_common_io();
	iotable_init(omap_perseus2_io_desc,
		     ARRAY_SIZE(omap_perseus2_io_desc));

	/* Early, board-dependent init */

	/*
	 * Hold GSM Reset until needed
	 */
	omap_writew(omap_readw(OMAP730_DSP_M_CTL) & ~1, OMAP730_DSP_M_CTL);

	/*
	 * UARTs -> done automagically by 8250 driver
	 */

	/*
	 * CSx timings, GPIO Mux ... setup
	 */

	/* Flash: CS0 timings setup */
	omap_writel(0x0000fff3, OMAP730_FLASH_CFG_0);
	omap_writel(0x00000088, OMAP730_FLASH_ACFG_0);

	/*
	 * Ethernet support through the debug board
	 * CS1 timings setup
	 */
	omap_writel(0x0000fff3, OMAP730_FLASH_CFG_1);
	omap_writel(0x00000000, OMAP730_FLASH_ACFG_1);

	/*
	 * Configure MPU_EXT_NIRQ IO in IO_CONF9 register,
	 * It is used as the Ethernet controller interrupt
	 */
	omap_writel(omap_readl(OMAP730_IO_CONF_9) & 0x1FFFFFFF, OMAP730_IO_CONF_9);
}

MACHINE_START(OMAP_PERSEUS2, "OMAP730 Perseus2")
	/* Maintainer: Kevin Hilman <kjh@hilman.org> */
	.phys_io	= 0xfff00000,
	.io_pg_offst	= ((0xfef00000) >> 18) & 0xfffc,
	.boot_params	= 0x10000100,
	.map_io		= omap_perseus2_map_io,
	.init_irq	= omap_perseus2_init_irq,
	.init_machine	= omap_perseus2_init,
	.timer		= &omap_timer,
MACHINE_END
