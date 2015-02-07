
#ifndef __ASM_ARCH_MXC_UNCOMPRESS_H__
#define __ASM_ARCH_MXC_UNCOMPRESS_H__

#define __MXC_BOOT_UNCOMPRESS

#include <mach/hardware.h>

#define UART(x) (*(volatile unsigned long *)(serial_port + (x)))

#define USR2 0x98
#define USR2_TXFE (1<<14)
#define TXR  0x40
#define UCR1 0x80
#define UCR1_UARTEN 1


static void putc(int ch)
{
	static unsigned long serial_port = 0;

	if (unlikely(serial_port == 0)) {
		do {
			serial_port = UART1_BASE_ADDR;
			if (UART(UCR1) & UCR1_UARTEN)
				break;
			serial_port = UART2_BASE_ADDR;
			if (UART(UCR1) & UCR1_UARTEN)
				break;
			return;
		} while (0);
	}

	while (!(UART(USR2) & USR2_TXFE))
		barrier();

	UART(TXR) = ch;
}

#define flush() do { } while (0)

#define arch_decomp_setup()

#define arch_decomp_wdog()

#endif				/* __ASM_ARCH_MXC_UNCOMPRESS_H__ */
