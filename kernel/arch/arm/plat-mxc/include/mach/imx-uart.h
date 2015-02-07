

#ifndef ASMARM_ARCH_UART_H
#define ASMARM_ARCH_UART_H

#define IMXUART_HAVE_RTSCTS (1<<0)

struct imxuart_platform_data {
	int (*init)(struct platform_device *pdev);
	int (*exit)(struct platform_device *pdev);
	unsigned int flags;
};

int __init imx_init_uart(int uart_no, struct imxuart_platform_data *pdata);

#endif
