

#include <linux/serial_reg.h>
#include <mach/regs-uart.h>
#include <asm/mach-types.h>

#define __REG(x)       ((volatile unsigned long *)x)

static volatile unsigned long *UART = FFUART;

static inline void putc(char c)
{
	if (!(UART[UART_IER] & IER_UUE))
		return;
	while (!(UART[UART_LSR] & LSR_TDRQ))
		barrier();
	UART[UART_TX] = c;
}

static inline void flush(void)
{
}

static inline void arch_decomp_setup(void)
{
	if (machine_is_littleton() || machine_is_intelmote2())
		UART = STUART;
}

#define arch_decomp_wdog()
