

#ifndef __ASM_PLAT_UNCOMPRESS_H
#define __ASM_PLAT_UNCOMPRESS_H

typedef unsigned int upf_t;	/* cannot include linux/serial_core.h */

/* uart setup */

static unsigned int fifo_mask;
static unsigned int fifo_max;

/* forward declerations */

static void arch_detect_cpu(void);

/* defines for UART registers */

#include <plat/regs-serial.h>
#include <plat/regs-watchdog.h>

/* working in physical space... */
#undef S3C2410_WDOGREG
#define S3C2410_WDOGREG(x) ((S3C24XX_PA_WATCHDOG + (x)))

/* how many bytes we allow into the FIFO at a time in FIFO mode */
#define FIFO_MAX	 (14)

#define uart_base S3C_PA_UART + (S3C_UART_OFFSET * CONFIG_S3C_LOWLEVEL_UART_PORT)

static __inline__ void
uart_wr(unsigned int reg, unsigned int val)
{
	volatile unsigned int *ptr;

	ptr = (volatile unsigned int *)(reg + uart_base);
	*ptr = val;
}

static __inline__ unsigned int
uart_rd(unsigned int reg)
{
	volatile unsigned int *ptr;

	ptr = (volatile unsigned int *)(reg + uart_base);
	return *ptr;
}


static void putc(int ch)
{
	if (uart_rd(S3C2410_UFCON) & S3C2410_UFCON_FIFOMODE) {
		int level;

		while (1) {
			level = uart_rd(S3C2410_UFSTAT);
			level &= fifo_mask;

			if (level < fifo_max)
				break;
		}

	} else {
		/* not using fifos */

		while ((uart_rd(S3C2410_UTRSTAT) & S3C2410_UTRSTAT_TXE) != S3C2410_UTRSTAT_TXE)
			barrier();
	}

	/* write byte to transmission register */
	uart_wr(S3C2410_UTXH, ch);
}

static inline void flush(void)
{
}

#define __raw_writel(d,ad) do { *((volatile unsigned int *)(ad)) = (d); } while(0)


#ifdef CONFIG_S3C_BOOT_WATCHDOG

#define WDOG_COUNT (0xff00)

static inline void arch_decomp_wdog(void)
{
	__raw_writel(WDOG_COUNT, S3C2410_WTCNT);
}

static void arch_decomp_wdog_start(void)
{
	__raw_writel(WDOG_COUNT, S3C2410_WTDAT);
	__raw_writel(WDOG_COUNT, S3C2410_WTCNT);
	__raw_writel(S3C2410_WTCON_ENABLE | S3C2410_WTCON_DIV128 | S3C2410_WTCON_RSTEN | S3C2410_WTCON_PRESCALE(0x80), S3C2410_WTCON);
}

#else
#define arch_decomp_wdog_start()
#define arch_decomp_wdog()
#endif

#ifdef CONFIG_S3C_BOOT_ERROR_RESET

static void arch_decomp_error(const char *x)
{
	putstr("\n\n");
	putstr(x);
	putstr("\n\n -- System resetting\n");

	__raw_writel(0x4000, S3C2410_WTDAT);
	__raw_writel(0x4000, S3C2410_WTCNT);
	__raw_writel(S3C2410_WTCON_ENABLE | S3C2410_WTCON_DIV128 | S3C2410_WTCON_RSTEN | S3C2410_WTCON_PRESCALE(0x40), S3C2410_WTCON);

	while(1);
}

#define arch_error arch_decomp_error
#endif

static void error(char *err);

#ifdef CONFIG_S3C_BOOT_UART_FORCE_FIFO
static inline void arch_enable_uart_fifo(void)
{
	u32 fifocon = uart_rd(S3C2410_UFCON);

	if (!(fifocon & S3C2410_UFCON_FIFOMODE)) {
		fifocon |= S3C2410_UFCON_RESETBOTH;
		uart_wr(S3C2410_UFCON, fifocon);

		/* wait for fifo reset to complete */
		while (1) {
			fifocon = uart_rd(S3C2410_UFCON);
			if (!(fifocon & S3C2410_UFCON_RESETBOTH))
				break;
		}
	}
}
#else
#define arch_enable_uart_fifo() do { } while(0)
#endif


static void
arch_decomp_setup(void)
{
	/* we may need to setup the uart(s) here if we are not running
	 * on an BAST... the BAST will have left the uarts configured
	 * after calling linux.
	 */

	arch_detect_cpu();
	arch_decomp_wdog_start();

	/* Enable the UART FIFOs if they where not enabled and our
	 * configuration says we should turn them on.
	 */

	arch_enable_uart_fifo();
}


#endif /* __ASM_PLAT_UNCOMPRESS_H */