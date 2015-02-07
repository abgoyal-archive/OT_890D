

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/serial_8250.h>
#include <linux/serial_reg.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/io.h>

#include <asm/irq.h>
#include <mach/hardware.h>
#include <mach/serial.h>
#include <mach/irqs.h>

#define UART_DAVINCI_PWREMU 0x0c

static inline unsigned int davinci_serial_in(struct plat_serial8250_port *up,
					  int offset)
{
	offset <<= up->regshift;
	return (unsigned int)__raw_readb(up->membase + offset);
}

static inline void davinci_serial_outp(struct plat_serial8250_port *p,
				       int offset, int value)
{
	offset <<= p->regshift;
	__raw_writeb(value, p->membase + offset);
}

static struct plat_serial8250_port serial_platform_data[] = {
	{
		.membase	= (char *)IO_ADDRESS(DAVINCI_UART0_BASE),
		.mapbase	= (unsigned long)DAVINCI_UART0_BASE,
		.irq		= IRQ_UARTINT0,
		.flags		= UPF_BOOT_AUTOCONF | UPF_SKIP_TEST,
		.iotype		= UPIO_MEM,
		.regshift	= 2,
		.uartclk	= 27000000,
	},
	{
		.flags		= 0
	},
};

static struct platform_device serial_device = {
	.name			= "serial8250",
	.id			= PLAT8250_DEV_PLATFORM,
	.dev			= {
		.platform_data	= serial_platform_data,
	},
};

static void __init davinci_serial_reset(struct plat_serial8250_port *p)
{
	/* reset both transmitter and receiver: bits 14,13 = UTRST, URRST */
	unsigned int pwremu = 0;

	davinci_serial_outp(p, UART_IER, 0);  /* disable all interrupts */

	davinci_serial_outp(p, UART_DAVINCI_PWREMU, pwremu);
	mdelay(10);

	pwremu |= (0x3 << 13);
	pwremu |= 0x1;
	davinci_serial_outp(p, UART_DAVINCI_PWREMU, pwremu);
}

static int __init davinci_init(void)
{
	davinci_serial_reset(&serial_platform_data[0]);
	return platform_device_register(&serial_device);
}

arch_initcall(davinci_init);
