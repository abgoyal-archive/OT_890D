

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>

#include <plat/cpu.h>
#include <plat/devs.h>
#include <plat/s3c6400.h>
#include <plat/s3c6410.h>

/* uart registration process */

void __init s3c6400_common_init_uarts(struct s3c2410_uartcfg *cfg, int no)
{
	s3c24xx_init_uartdevs("s3c6400-uart", s3c64xx_uart_resources, cfg, no);
}
