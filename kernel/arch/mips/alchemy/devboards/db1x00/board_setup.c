

#include <linux/init.h>

#include <asm/mach-au1x00/au1000.h>
#include <asm/mach-db1x00/db1x00.h>

#include <prom.h>


static BCSR * const bcsr = (BCSR *)BCSR_KSEG1_ADDR;

const char *get_system_type(void)
{
#ifdef CONFIG_MIPS_BOSPORUS
	return "Alchemy Bosporus Gateway Reference";
#else
	return "Alchemy Db1x00";
#endif
}

void board_reset(void)
{
	/* Hit BCSR.SW_RESET[RESET] */
	bcsr->swreset = 0x0000;
}

void __init board_setup(void)
{
	u32 pin_func = 0;
	char *argptr;

	argptr = prom_getcmdline();
#ifdef CONFIG_SERIAL_8250_CONSOLE
	argptr = strstr(argptr, "console=");
	if (argptr == NULL) {
		argptr = prom_getcmdline();
		strcat(argptr, " console=ttyS0,115200");
	}
#endif

#ifdef CONFIG_FB_AU1100
	argptr = strstr(argptr, "video=");
	if (argptr == NULL) {
		argptr = prom_getcmdline();
		/* default panel */
		/*strcat(argptr, " video=au1100fb:panel:Sharp_320x240_16");*/
	}
#endif

#if defined(CONFIG_SOUND_AU1X00) && !defined(CONFIG_SOC_AU1000)
	/* au1000 does not support vra, au1500 and au1100 do */
	strcat(argptr, " au1000_audio=vra");
	argptr = prom_getcmdline();
#endif

	/* Not valid for Au1550 */
#if defined(CONFIG_IRDA) && \
   (defined(CONFIG_SOC_AU1000) || defined(CONFIG_SOC_AU1100))
	/* Set IRFIRSEL instead of GPIO15 */
	pin_func = au_readl(SYS_PINFUNC) | SYS_PF_IRF;
	au_writel(pin_func, SYS_PINFUNC);
	/* Power off until the driver is in use */
	bcsr->resets &= ~BCSR_RESETS_IRDA_MODE_MASK;
	bcsr->resets |=  BCSR_RESETS_IRDA_MODE_OFF;
	au_sync();
#endif
	bcsr->pcmcia = 0x0000; /* turn off PCMCIA power */

#ifdef CONFIG_MIPS_MIRAGE
	/* Enable GPIO[31:0] inputs */
	au_writel(0, SYS_PININPUTEN);

	/* GPIO[20] is output, tristate the other input primary GPIOs */
	au_writel(~(1 << 20), SYS_TRIOUTCLR);

	/* Set GPIO[210:208] instead of SSI_0 */
	pin_func = au_readl(SYS_PINFUNC) | SYS_PF_S0;

	/* Set GPIO[215:211] for LEDs */
	pin_func |= 5 << 2;

	/* Set GPIO[214:213] for more LEDs */
	pin_func |= 5 << 12;

	/* Set GPIO[207:200] instead of PCMCIA/LCD */
	pin_func |= SYS_PF_LCD | SYS_PF_PC;
	au_writel(pin_func, SYS_PINFUNC);

	/*
	 * Enable speaker amplifier.  This should
	 * be part of the audio driver.
	 */
	au_writel(au_readl(GPIO2_DIR) | 0x200, GPIO2_DIR);
	au_writel(0x02000200, GPIO2_OUTPUT);
#endif

	au_sync();

#ifdef CONFIG_MIPS_DB1000
	printk(KERN_INFO "AMD Alchemy Au1000/Db1000 Board\n");
#endif
#ifdef CONFIG_MIPS_DB1500
	printk(KERN_INFO "AMD Alchemy Au1500/Db1500 Board\n");
#endif
#ifdef CONFIG_MIPS_DB1100
	printk(KERN_INFO "AMD Alchemy Au1100/Db1100 Board\n");
#endif
#ifdef CONFIG_MIPS_BOSPORUS
	printk(KERN_INFO "AMD Alchemy Bosporus Board\n");
#endif
#ifdef CONFIG_MIPS_MIRAGE
	printk(KERN_INFO "AMD Alchemy Mirage Board\n");
#endif
#ifdef CONFIG_MIPS_DB1550
	printk(KERN_INFO "AMD Alchemy Au1550/Db1550 Board\n");
#endif
}
