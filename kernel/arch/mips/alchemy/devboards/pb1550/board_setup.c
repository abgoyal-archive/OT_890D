

#include <linux/init.h>
#include <linux/interrupt.h>

#include <asm/mach-au1x00/au1000.h>
#include <asm/mach-pb1x00/pb1550.h>

#include <prom.h>


char irq_tab_alchemy[][5] __initdata = {
	[12] = { -1, INTB, INTC, INTD, INTA }, /* IDSEL 12 - PCI slot 2 (left)  */
	[13] = { -1, INTA, INTB, INTC, INTD }, /* IDSEL 13 - PCI slot 1 (right) */
};

struct au1xxx_irqmap __initdata au1xxx_irq_map[] = {
	{ AU1000_GPIO_0, IRQF_TRIGGER_LOW, 0 },
	{ AU1000_GPIO_1, IRQF_TRIGGER_LOW, 0 },
};

const char *get_system_type(void)
{
	return "Alchemy Pb1550";
}

void board_reset(void)
{
	/* Hit BCSR.SYSTEM[RESET] */
	au_writew(au_readw(0xAF00001C) & ~BCSR_SYSTEM_RESET, 0xAF00001C);
}

void __init board_init_irq(void)
{
	au1xxx_setup_irqmap(au1xxx_irq_map, ARRAY_SIZE(au1xxx_irq_map));
}

void __init board_setup(void)
{
	u32 pin_func;

#ifdef CONFIG_SERIAL_8250_CONSOLE
	char *argptr;
	argptr = prom_getcmdline();
	argptr = strstr(argptr, "console=");
	if (argptr == NULL) {
		argptr = prom_getcmdline();
		strcat(argptr, " console=ttyS0,115200");
	}
#endif

	/*
	 * Enable PSC1 SYNC for AC'97.  Normaly done in audio driver,
	 * but it is board specific code, so put it here.
	 */
	pin_func = au_readl(SYS_PINFUNC);
	au_sync();
	pin_func |= SYS_PF_MUST_BE_SET | SYS_PF_PSC1_S1;
	au_writel(pin_func, SYS_PINFUNC);

	au_writel(0, (u32)bcsr | 0x10); /* turn off PCMCIA power */
	au_sync();

	printk(KERN_INFO "AMD Alchemy Pb1550 Board\n");
}
