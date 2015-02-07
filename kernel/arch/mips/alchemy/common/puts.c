

#include <asm/mach-au1x00/au1000.h>

#define SERIAL_BASE   UART_BASE
#define SER_CMD       0x7
#define SER_DATA      0x1
#define TX_BUSY       0x20

#define TIMEOUT       0xffffff
#define SLOW_DOWN

static volatile unsigned long * const com1 = (unsigned long *)SERIAL_BASE;

#ifdef SLOW_DOWN
static inline void slow_down(void)
{
	int k;

	for (k = 0; k < 10000; k++);
}
#else
#define slow_down()
#endif

void
prom_putchar(const unsigned char c)
{
	unsigned char ch;
	int i = 0;

	do {
		ch = com1[SER_CMD];
		slow_down();
		i++;
		if (i > TIMEOUT)
			break;
	} while (0 == (ch & TX_BUSY));

	com1[SER_DATA] = c;
}
