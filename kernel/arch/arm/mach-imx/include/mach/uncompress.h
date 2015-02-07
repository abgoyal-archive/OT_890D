

#define UART(x) (*(volatile unsigned long *)(serial_port + (x)))

#define UART1_BASE 0x206000
#define UART2_BASE 0x207000
#define USR2 0x98
#define USR2_TXFE (1<<14)
#define TXR  0x40
#define UCR1 0x80
#define UCR1_UARTEN 1

static void putc(int c)
{
	unsigned long serial_port;

	do {
		serial_port = UART1_BASE;
		if ( UART(UCR1) & UCR1_UARTEN )
			break;
		serial_port = UART2_BASE;
		if ( UART(UCR1) & UCR1_UARTEN )
			break;
		return;
	} while(0);

	while (!(UART(USR2) & USR2_TXFE))
		barrier();

	UART(TXR) = c;
}

static inline void flush(void)
{
}

#define arch_decomp_setup()

#define arch_decomp_wdog()
