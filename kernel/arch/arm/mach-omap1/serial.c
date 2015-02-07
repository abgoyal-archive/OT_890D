

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/serial.h>
#include <linux/tty.h>
#include <linux/serial_8250.h>
#include <linux/serial_reg.h>
#include <linux/clk.h>
#include <linux/io.h>

#include <asm/mach-types.h>

#include <mach/board.h>
#include <mach/mux.h>
#include <mach/gpio.h>
#include <mach/fpga.h>
#ifdef CONFIG_PM
#include <mach/pm.h>
#endif

static struct clk * uart1_ck;
static struct clk * uart2_ck;
static struct clk * uart3_ck;

static inline unsigned int omap_serial_in(struct plat_serial8250_port *up,
					  int offset)
{
	offset <<= up->regshift;
	return (unsigned int)__raw_readb(up->membase + offset);
}

static inline void omap_serial_outp(struct plat_serial8250_port *p, int offset,
				    int value)
{
	offset <<= p->regshift;
	__raw_writeb(value, p->membase + offset);
}

static void __init omap_serial_reset(struct plat_serial8250_port *p)
{
	omap_serial_outp(p, UART_OMAP_MDR1, 0x07);	/* disable UART */
	omap_serial_outp(p, UART_OMAP_SCR, 0x08);	/* TX watermark */
	omap_serial_outp(p, UART_OMAP_MDR1, 0x00);	/* enable UART */

	if (!cpu_is_omap15xx()) {
		omap_serial_outp(p, UART_OMAP_SYSC, 0x01);
		while (!(omap_serial_in(p, UART_OMAP_SYSC) & 0x01));
	}
}

static struct plat_serial8250_port serial_platform_data[] = {
	{
		.membase	= IO_ADDRESS(OMAP_UART1_BASE),
		.mapbase	= OMAP_UART1_BASE,
		.irq		= INT_UART1,
		.flags		= UPF_BOOT_AUTOCONF,
		.iotype		= UPIO_MEM,
		.regshift	= 2,
		.uartclk	= OMAP16XX_BASE_BAUD * 16,
	},
	{
		.membase	= IO_ADDRESS(OMAP_UART2_BASE),
		.mapbase	= OMAP_UART2_BASE,
		.irq		= INT_UART2,
		.flags		= UPF_BOOT_AUTOCONF,
		.iotype		= UPIO_MEM,
		.regshift	= 2,
		.uartclk	= OMAP16XX_BASE_BAUD * 16,
	},
	{
		.membase	= IO_ADDRESS(OMAP_UART3_BASE),
		.mapbase	= OMAP_UART3_BASE,
		.irq		= INT_UART3,
		.flags		= UPF_BOOT_AUTOCONF,
		.iotype		= UPIO_MEM,
		.regshift	= 2,
		.uartclk	= OMAP16XX_BASE_BAUD * 16,
	},
	{ },
};

static struct platform_device serial_device = {
	.name			= "serial8250",
	.id			= PLAT8250_DEV_PLATFORM,
	.dev			= {
		.platform_data	= serial_platform_data,
	},
};

void __init omap_serial_init(void)
{
	int i;
	const struct omap_uart_config *info;

	if (cpu_is_omap730()) {
		serial_platform_data[0].regshift = 0;
		serial_platform_data[1].regshift = 0;
		serial_platform_data[0].irq = INT_730_UART_MODEM_1;
		serial_platform_data[1].irq = INT_730_UART_MODEM_IRDA_2;
	}

	if (cpu_is_omap15xx()) {
		serial_platform_data[0].uartclk = OMAP1510_BASE_BAUD * 16;
		serial_platform_data[1].uartclk = OMAP1510_BASE_BAUD * 16;
		serial_platform_data[2].uartclk = OMAP1510_BASE_BAUD * 16;
	}

	info = omap_get_config(OMAP_TAG_UART, struct omap_uart_config);
	if (info == NULL)
		return;

	for (i = 0; i < OMAP_MAX_NR_PORTS; i++) {
		unsigned char reg;

		if (!((1 << i) & info->enabled_uarts)) {
			serial_platform_data[i].membase = NULL;
			serial_platform_data[i].mapbase = 0;
			continue;
		}

		switch (i) {
		case 0:
			uart1_ck = clk_get(NULL, "uart1_ck");
			if (IS_ERR(uart1_ck))
				printk("Could not get uart1_ck\n");
			else {
				clk_enable(uart1_ck);
				if (cpu_is_omap15xx())
					clk_set_rate(uart1_ck, 12000000);
			}
			if (cpu_is_omap15xx()) {
				omap_cfg_reg(UART1_TX);
				omap_cfg_reg(UART1_RTS);
				if (machine_is_omap_innovator()) {
					reg = fpga_read(OMAP1510_FPGA_POWER);
					reg |= OMAP1510_FPGA_PCR_COM1_EN;
					fpga_write(reg, OMAP1510_FPGA_POWER);
					udelay(10);
				}
			}
			break;
		case 1:
			uart2_ck = clk_get(NULL, "uart2_ck");
			if (IS_ERR(uart2_ck))
				printk("Could not get uart2_ck\n");
			else {
				clk_enable(uart2_ck);
				if (cpu_is_omap15xx())
					clk_set_rate(uart2_ck, 12000000);
				else
					clk_set_rate(uart2_ck, 48000000);
			}
			if (cpu_is_omap15xx()) {
				omap_cfg_reg(UART2_TX);
				omap_cfg_reg(UART2_RTS);
				if (machine_is_omap_innovator()) {
					reg = fpga_read(OMAP1510_FPGA_POWER);
					reg |= OMAP1510_FPGA_PCR_COM2_EN;
					fpga_write(reg, OMAP1510_FPGA_POWER);
					udelay(10);
				}
			}
			break;
		case 2:
			uart3_ck = clk_get(NULL, "uart3_ck");
			if (IS_ERR(uart3_ck))
				printk("Could not get uart3_ck\n");
			else {
				clk_enable(uart3_ck);
				if (cpu_is_omap15xx())
					clk_set_rate(uart3_ck, 12000000);
			}
			if (cpu_is_omap15xx()) {
				omap_cfg_reg(UART3_TX);
				omap_cfg_reg(UART3_RX);
			}
			break;
		}
		omap_serial_reset(&serial_platform_data[i]);
	}
}

#ifdef CONFIG_OMAP_SERIAL_WAKE

static irqreturn_t omap_serial_wake_interrupt(int irq, void *dev_id)
{
	/* Need to do something with serial port right after wake-up? */
	return IRQ_HANDLED;
}

void omap_serial_wake_trigger(int enable)
{
	if (!cpu_is_omap16xx())
		return;

	if (uart1_ck != NULL) {
		if (enable)
			omap_cfg_reg(V14_16XX_GPIO37);
		else
			omap_cfg_reg(V14_16XX_UART1_RX);
	}
	if (uart2_ck != NULL) {
		if (enable)
			omap_cfg_reg(R9_16XX_GPIO18);
		else
			omap_cfg_reg(R9_16XX_UART2_RX);
	}
	if (uart3_ck != NULL) {
		if (enable)
			omap_cfg_reg(L14_16XX_GPIO49);
		else
			omap_cfg_reg(L14_16XX_UART3_RX);
	}
}

static void __init omap_serial_set_port_wakeup(int gpio_nr)
{
	int ret;

	ret = gpio_request(gpio_nr, "UART wake");
	if (ret < 0) {
		printk(KERN_ERR "Could not request UART wake GPIO: %i\n",
		       gpio_nr);
		return;
	}
	gpio_direction_input(gpio_nr);
	ret = request_irq(gpio_to_irq(gpio_nr), &omap_serial_wake_interrupt,
			  IRQF_TRIGGER_RISING, "serial wakeup", NULL);
	if (ret) {
		gpio_free(gpio_nr);
		printk(KERN_ERR "No interrupt for UART wake GPIO: %i\n",
		       gpio_nr);
		return;
	}
	enable_irq_wake(gpio_to_irq(gpio_nr));
}

static int __init omap_serial_wakeup_init(void)
{
	if (!cpu_is_omap16xx())
		return 0;

	if (uart1_ck != NULL)
		omap_serial_set_port_wakeup(37);
	if (uart2_ck != NULL)
		omap_serial_set_port_wakeup(18);
	if (uart3_ck != NULL)
		omap_serial_set_port_wakeup(49);

	return 0;
}
late_initcall(omap_serial_wakeup_init);

#endif	/* CONFIG_OMAP_SERIAL_WAKE */

static int __init omap_init(void)
{
	return platform_device_register(&serial_device);
}
arch_initcall(omap_init);
