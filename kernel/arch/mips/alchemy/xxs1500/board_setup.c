

#include <linux/init.h>
#include <linux/delay.h>

#include <asm/mach-au1x00/au1000.h>

#include <prom.h>

void board_reset(void)
{
	/* Hit BCSR.SYSTEM_CONTROL[SW_RST] */
	au_writel(0x00000000, 0xAE00001C);
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

	/* Set multiple use pins (UART3/GPIO) to UART (it's used as UART too) */
	pin_func  = au_readl(SYS_PINFUNC) & ~SYS_PF_UR3;
	pin_func |= SYS_PF_UR3;
	au_writel(pin_func, SYS_PINFUNC);

	/* Enable UART */
	au_writel(0x01, UART3_ADDR + UART_MOD_CNTRL); /* clock enable (CE) */
	mdelay(10);
	au_writel(0x03, UART3_ADDR + UART_MOD_CNTRL); /* CE and "enable" */
	mdelay(10);

	/* Enable DTR = USB power up */
	au_writel(0x01, UART3_ADDR + UART_MCR); /* UART_MCR_DTR is 0x01??? */

#ifdef CONFIG_PCMCIA_XXS1500
	/* Setup PCMCIA signals */
	au_writel(0, SYS_PININPUTEN);

	/* GPIO 0, 1, and 4 are inputs */
	au_writel(1 | (1 << 1) | (1 << 4), SYS_TRIOUTCLR);

	/* Enable GPIO2 if not already enabled */
	au_writel(1, GPIO2_ENABLE);
	/* GPIO2 208/9/10/11 are inputs */
	au_writel((1 << 8) | (1 << 9) | (1 << 10) | (1 << 11), GPIO2_DIR);

	/* Turn off power */
	au_writel((au_readl(GPIO2_PINSTATE) & ~(1 << 14)) | (1 << 30),
		  GPIO2_OUTPUT);
#endif

#ifdef CONFIG_PCI
#if defined(__MIPSEB__)
	au_writel(0xf | (2 << 6) | (1 << 4), Au1500_PCI_CFG);
#else
	au_writel(0xf, Au1500_PCI_CFG);
#endif
#endif
}
