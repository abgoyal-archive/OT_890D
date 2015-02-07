

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/sysdev.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>
#include <linux/io.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>

#include <mach/hardware.h>
#include <asm/irq.h>

#include <plat/cpu-freq.h>

#include <mach/regs-clock.h>
#include <plat/regs-serial.h>

#include <plat/s3c2410.h>
#include <plat/cpu.h>
#include <plat/devs.h>
#include <plat/clock.h>
#include <plat/pll.h>

/* Initial IO mappings */

static struct map_desc s3c2410_iodesc[] __initdata = {
	IODESC_ENT(CLKPWR),
	IODESC_ENT(TIMER),
	IODESC_ENT(WATCHDOG),
};

/* our uart devices */

/* uart registration process */

void __init s3c2410_init_uarts(struct s3c2410_uartcfg *cfg, int no)
{
	s3c24xx_init_uartdevs("s3c2410-uart", s3c2410_uart_resources, cfg, no);
}


void __init s3c2410_map_io(void)
{
	iotable_init(s3c2410_iodesc, ARRAY_SIZE(s3c2410_iodesc));
}

void __init_or_cpufreq s3c2410_setup_clocks(void)
{
	struct clk *xtal_clk;
	unsigned long tmp;
	unsigned long xtal;
	unsigned long fclk;
	unsigned long hclk;
	unsigned long pclk;

	xtal_clk = clk_get(NULL, "xtal");
	xtal = clk_get_rate(xtal_clk);
	clk_put(xtal_clk);

	/* now we've got our machine bits initialised, work out what
	 * clocks we've got */

	fclk = s3c24xx_get_pll(__raw_readl(S3C2410_MPLLCON), xtal);

	tmp = __raw_readl(S3C2410_CLKDIVN);

	/* work out clock scalings */

	hclk = fclk / ((tmp & S3C2410_CLKDIVN_HDIVN) ? 2 : 1);
	pclk = hclk / ((tmp & S3C2410_CLKDIVN_PDIVN) ? 2 : 1);

	/* print brieft summary of clocks, etc */

	printk("S3C2410: core %ld.%03ld MHz, memory %ld.%03ld MHz, peripheral %ld.%03ld MHz\n",
	       print_mhz(fclk), print_mhz(hclk), print_mhz(pclk));

	/* initialise the clocks here, to allow other things like the
	 * console to use them
	 */

	s3c24xx_setup_clocks(fclk, hclk, pclk);
}

void __init s3c2410_init_clocks(int xtal)
{
	s3c24xx_register_baseclocks(xtal);
	s3c2410_setup_clocks();
	s3c2410_baseclk_add();
}

struct sysdev_class s3c2410_sysclass = {
	.name = "s3c2410-core",
};

static struct sys_device s3c2410_sysdev = {
	.cls		= &s3c2410_sysclass,
};


static int __init s3c2410_core_init(void)
{
	return sysdev_class_register(&s3c2410_sysclass);
}

core_initcall(s3c2410_core_init);

int __init s3c2410_init(void)
{
	printk("S3C2410: Initialising architecture\n");

	return sysdev_register(&s3c2410_sysdev);
}
