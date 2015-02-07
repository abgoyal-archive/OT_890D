

/* system header files */
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>

#include <asm/irq.h>
#include <asm/io.h>
#include <asm/mach-types.h>

#include <asm/mach/arch.h>
#include <asm/mach/irq.h>
#include <asm/mach/map.h>
#include <asm/mach/time.h>
#include <mach/board.h>
#include <mach/hardware.h>
#include <mach/mt3351_typedefs.h>
#include <mach/mt3351_reg_base.h>
#include <mach/mt3351_gpio.h>
#include <mach/mt3351_devs.h>

/*=======================================================================*/
/* Board Specific Devices                                                */
/*=======================================================================*/

/* MT3328 GPS */
struct platform_device mt3328_device_gps = {
	.name	       = "mt3328-gps",
	.id            = -1,
};

/* MT6611 Bluetooth */
struct platform_device mt6611_device_bt = {
	.name	       = "mt6611-bt",
	.id            = -1,
};

/*=======================================================================*/
/* Board Specific Devices Power Management                               */
/*=======================================================================*/
static void mt3351_sd3_ext_power_on(void)
{
    mt_set_gpio_dir(72, GPIO_DIR_OUT);
    mt_set_gpio_out(72, GPIO_OUT_ONE); /* enable card power source */
}

static void mt3351_sd3_ext_power_off(void)
{
    mt_set_gpio_out(72, GPIO_OUT_ZERO); /* disable card power source */
    mt_set_gpio_dir(72, GPIO_DIR_IN);
}

/*=======================================================================*/
/* Board Specific Devices Init                                           */
/*=======================================================================*/
static __init int board_init(void)
{
    /* MT3351 board device init */
    mt3351_board_init();
    
    /* MT3328 GPS */
    platform_device_register(&mt3328_device_gps);

    /* MT6611 Bluetooth */
    platform_device_register(&mt6611_device_bt);

    return 0;
}

late_initcall(board_init);

/*=======================================================================*/
/* Board Devices Capability                                              */
/*=======================================================================*/
#if defined(CFG_DEV_MSDC1)
struct mt3351_sd_host_hw mt3351_sd1_hw = {
    .cmd_odc        = MSDC_ODC_16MA,
    .data_odc       = MSDC_ODC_16MA,
    .cmd_slew_rate  = MSDC_ODC_SLEW_FAST,
    .data_slew_rate = MSDC_ODC_SLEW_FAST,
    .data_pins      = CFG_MSDC1_DATA_PINS,
    .data_offset    = CFG_MSDC1_DATA_OFFSET,
    .flags          = MSDC_CD_PIN_EN,
};
#endif
#if defined(CFG_DEV_MSDC2)
struct mt3351_sd_host_hw mt3351_sd2_hw = {
    .cmd_odc         = MSDC_ODC_4MA,
    .data_odc        = MSDC_ODC_4MA,
    .cmd_slew_rate   = MSDC_ODC_SLEW_FAST,
    .data_slew_rate  = MSDC_ODC_SLEW_FAST,
    .data_pins       = CFG_MSDC2_DATA_PINS,
    .data_offset     = CFG_MSDC2_DATA_OFFSET,
    .flags           = 0,
};
#endif
#if defined(CFG_DEV_MSDC3)
struct mt3351_sd_host_hw mt3351_sd3_hw = {
    .cmd_odc         = MSDC_ODC_16MA,
    .data_odc        = MSDC_ODC_16MA,
    .cmd_slew_rate   = MSDC_ODC_SLEW_FAST,
    .data_slew_rate  = MSDC_ODC_SLEW_FAST,
    .data_pins       = CFG_MSDC3_DATA_PINS,
    .data_offset     = CFG_MSDC3_DATA_OFFSET,
    .ext_power_on    = mt3351_sd3_ext_power_on,
    .ext_power_off   = mt3351_sd3_ext_power_off,
    .flags           = MSDC_CD_PIN_EN | MSDC_REMOVABLE,
};
#endif

/*=======================================================================*/
/* Board Memory Mapping                                                  */
/*=======================================================================*/
static struct map_desc mt3351_io_desc[] __initdata =
{
    {
        .virtual	= IO_VIRT,
        .pfn		= __phys_to_pfn(IO_PHYS),
        .length		= IO_SIZE,
        .type		= MT_DEVICE
    },
	
    {
        .virtual	= (IO_VIRT+0x100000),
        .pfn		= __phys_to_pfn(IO_PHYS+0x100000),
        .length		= IO_SIZE,
        .type		= MT_DEVICE
    },

#if 0
    /* FIXME: shall replace with io agents */
    {IO_VIRT, IO_PHYS, IO_SIZE, MT_DEVICE},
    {IO_VIRT+0x100000, IO_PHYS+0x100000, IO_SIZE, MT_DEVICE},
    /*{PBIA_VIRT, PBIA_PHYS, PBIA_SIZE, MT_DEVICE},*/
#endif
};

static void __init mt3351_map_io(void)
{
    iotable_init(mt3351_io_desc, ARRAY_SIZE(mt3351_io_desc));
}

/*=======================================================================*/
/* Board Low-Level Init                                                  */
/*=======================================================================*/
static void mt3351_power_off(void)
{
    /* Power down v-core1, but it is not work as charger-in */
	DRV_WriteReg32((RTC_base+0x07C), 0x00);
}

static void __init mt3351_init_pinmux(void)
{
    mt_set_gpio_PinMux(PIN_MUX_JTAG_CTRL_JTAG, JTAG_CTL);
    mt_set_gpio_PinMux(PIN_MUX_SPI_CTRL_GPIO, SPI_CTL);
    
    mt_set_gpio_PinMux(PIN_MUX_DAI_CTRL_DAI, DAI_CTL);
    mt_set_gpio_PinMux(PIN_MUX_PWM_CTRL_GPIO, PWM_CTL);
    
    mt_set_gpio_PinMux(PIN_MUX_MC0_CTRL_MC4, MC0_CTL);
    mt_set_gpio_PinMux(PIN_MUX_MC1_CTRL_GPIO, MC1_CTL);
    mt_set_gpio_PinMux(PIN_MUX_NLD_CTRLL_MC, NLD_CTLL);
    
    mt_set_gpio_PinMux(PIN_MUX_CAM0_CTRL_GPIO, CAM0_CTL);
    mt_set_gpio_PinMux(PIN_MUX_CAM1_CTRL_CAMERA, CAM1_CTL);

    mt_set_gpio_PinMux(PIN_MUX_UT0_CTRL_UART, UT0_CTL);
    mt_set_gpio_PinMux(PIN_MUX_UT1_CTRL_GPIO, UT1_CTL);
    mt_set_gpio_PinMux(PIN_MUX_UT2_CTRL_UART, UT2_CTL);
    mt_set_gpio_PinMux(PIN_MUX_UT3_CTRL_UART, UT3_CTL);
    mt_set_gpio_PinMux(PIN_MUX_UT4_CTRL_GPIO, UT4_CTL);

    mt_set_gpio_PinMux(PIN_MUX_DPI_CTRL_DPI666, DPI_CTL);
    mt_set_gpio_PinMux(PIN_MUX_NLD_CTRLH_GPIO, NLD_CTLH);
}

static void __init mt3351_init_gpio(void)
{
    /* For MT3328 GPS clock */
    mt_set_gpio_OCFG(GPIO22_OCTL_ECLK, GPIO22_OCTL); /* External Clock */ 

    /* For MT3328 GPS clock on MNxx board */
    mt_set_gpio_dir(23, GPIO_DIR_OUT);
    mt_set_gpio_debug(23, GPIO_DEBUG_ENABLE);
    DRV_WriteReg32((CONFIG_BASE+0x070C), 9);

    /* PWKEY SRC */
    mt_set_gpio_ICFG(EINT7_GPIO19_INPUT, EINT7_SRC);

    /* BT CTS/RTS */
    mt_set_gpio_dir(9, GPIO_DIR_IN);
    mt_set_gpio_dir(10, GPIO_DIR_OUT);
    
    mt_set_gpio_ICFG(UCTS2_ZERO_INPUT, UCTS2_SRC);    
    mt_set_gpio_OCFG(GPIO10_OCTL_UART2RTS, GPIO10_OCTL); 

    /* USB EINT */
    mt_set_gpio_dir(7, GPIO_DIR_IN);
    mt_set_gpio_ICFG(EINT1_GPIO7_INPUT, EINT1_SRC);

    /* PWM PWM5 GPIO */
    mt_set_gpio_dir(13, GPIO_DIR_OUT);
    mt_set_gpio_OCFG(GPIO13_OCTL_PWM5, GPIO13_OCTL);    

    /* LCM GPIO */
    mt_set_gpio_dir(84, GPIO_DIR_OUT);          /* POWER_ON */
    mt_set_gpio_pullup(84, GPIO_PU_DISABLE);
    mt_set_gpio_pulldown(84, GPIO_PD_DISABLE);
    mt_set_gpio_dir(85, GPIO_DIR_OUT);          /* DISP_ON */
    mt_set_gpio_pullup(85, GPIO_PU_DISABLE);
    mt_set_gpio_pulldown(85, GPIO_PD_DISABLE);
}

static void __init mt3351_init_machine(void)
{    
    /* Setting the Machine power off function */
    pm_power_off = mt3351_power_off;

    /* Init pinmux */
    mt3351_init_pinmux();

    /* Init gpio */
    mt3351_init_gpio();    
}

extern void __init mt3351_init_irq(void);
extern struct sys_timer mt3351_timer;

MACHINE_START(MT3351, "MT3351 43EVK")
    //.phys_ram       = MT3351_SDRAM_PA,
    .phys_io        = MT3351_PA_UART,
    .io_pg_offst    = (MT3351_VA_UART >> 18) & 0xfffc,
    .boot_params    = MT3351_SDRAM_PA + 0x100,
    .map_io         = mt3351_map_io,
    .init_irq       = mt3351_init_irq,
    .init_machine   = mt3351_init_machine,
    .timer          = &mt3351_timer,
MACHINE_END

