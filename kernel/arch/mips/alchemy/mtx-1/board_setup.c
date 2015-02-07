

#include <linux/init.h>

#include <asm/mach-au1x00/au1000.h>

#include <prom.h>

extern int (*board_pci_idsel)(unsigned int devsel, int assert);
int mtx1_pci_idsel(unsigned int devsel, int assert);

void board_reset(void)
{
	/* Hit BCSR.SYSTEM_CONTROL[SW_RST] */
	au_writel(0x00000000, 0xAE00001C);
}

void __init board_setup(void)
{
#ifdef CONFIG_SERIAL_8250_CONSOLE
	char *argptr;
	argptr = prom_getcmdline();
	argptr = strstr(argptr, "console=");
	if (argptr == NULL) {
		argptr = prom_getcmdline();
		strcat(argptr, " console=ttyS0,115200");
	}
#endif

#if defined(CONFIG_USB_OHCI_HCD) || defined(CONFIG_USB_OHCI_HCD_MODULE)
	/* Enable USB power switch */
	au_writel(au_readl(GPIO2_DIR) | 0x10, GPIO2_DIR);
	au_writel(0x100000, GPIO2_OUTPUT);
#endif /* defined(CONFIG_USB_OHCI_HCD) || defined(CONFIG_USB_OHCI_HCD_MODULE) */

#ifdef CONFIG_PCI
#if defined(__MIPSEB__)
	au_writel(0xf | (2 << 6) | (1 << 4), Au1500_PCI_CFG);
#else
	au_writel(0xf, Au1500_PCI_CFG);
#endif
#endif

	/* Initialize sys_pinfunc */
	au_writel(SYS_PF_NI2, SYS_PINFUNC);

	/* Initialize GPIO */
	au_writel(0xFFFFFFFF, SYS_TRIOUTCLR);
	au_writel(0x00000001, SYS_OUTPUTCLR); /* set M66EN (PCI 66MHz) to OFF */
	au_writel(0x00000008, SYS_OUTPUTSET); /* set PCI CLKRUN# to OFF */
	au_writel(0x00000002, SYS_OUTPUTSET); /* set EXT_IO3 ON */
	au_writel(0x00000020, SYS_OUTPUTCLR); /* set eth PHY TX_ER to OFF */

	/* Enable LED and set it to green */
	au_writel(au_readl(GPIO2_DIR) | 0x1800, GPIO2_DIR);
	au_writel(0x18000800, GPIO2_OUTPUT);

	board_pci_idsel = mtx1_pci_idsel;

	printk(KERN_INFO "4G Systems MTX-1 Board\n");
}

int
mtx1_pci_idsel(unsigned int devsel, int assert)
{
#define MTX_IDSEL_ONLY_0_AND_3 0
#if MTX_IDSEL_ONLY_0_AND_3
	if (devsel != 0 && devsel != 3) {
		printk(KERN_ERR "*** not 0 or 3\n");
		return 0;
	}
#endif

	if (assert && devsel != 0)
		/* Suppress signal to Cardbus */
		au_writel(0x00000002, SYS_OUTPUTCLR); /* set EXT_IO3 OFF */
	else
		au_writel(0x00000002, SYS_OUTPUTSET); /* set EXT_IO3 ON */
	au_sync_udelay(1);
	return 1;
}

