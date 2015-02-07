
 /* linux/arch/arm/mach-mt3351/mt3351_devs.c
  *
  * Devices supported on MT3351 machine
  *
  * Copyright (C) 2008,2009 MediaTek <www.mediatek.com>
  * Authors: Infinity Chen <infinity.chen@mediatek.com>  
  *
  * This program is free software; you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation; either version 2 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program; if not, write to the Free Software
  * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
  */

#include <linux/autoconf.h>
#include <linux/device.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <mach/irqs.h>
#include <mach/mt3351.h>
#include <mach/mt3351_reg_base.h>
#include <mach/mt3351_devs.h>

/*=======================================================================*/
/* MT3351 SD Hosts                                                       */
/*=======================================================================*/
#if defined(CFG_DEV_MSDC1)
static struct resource mt3351_resource_sd1[] = {
	{
		.start		= MSDC1_BASE,
		.end		= MSDC1_BASE + 0x74,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= MT3351_IRQ_MSDC_CODE,
		.flags		= IORESOURCE_IRQ,
	},
	{
		.start		= MT3351_IRQ_MSDC_EVENT_CODE,
		.flags		= IORESOURCE_IRQ,
	},	
};
#endif

#if defined(CFG_DEV_MSDC2)
static struct resource mt3351_resource_sd2[] = {
	{
		.start		= MSDC2_BASE,
		.end		= MSDC2_BASE + 0x74,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= MT3351_IRQ_MSDC2_CODE,
		.flags		= IORESOURCE_IRQ,
	},
	{
		.start		= MT3351_IRQ_MSDC2_EVENT_CODE,
		.flags		= IORESOURCE_IRQ,
	},	
};
#endif

#if defined(CFG_DEV_MSDC3)
static struct resource mt3351_resource_sd3[] = {
	{
		.start		= MSDC3_BASE,
		.end		= MSDC3_BASE + 0x74,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= MT3351_IRQ_MSDC3_CODE,
		.flags		= IORESOURCE_IRQ,
	},
	{
		.start		= MT3351_IRQ_MSDC3_EVENT_CODE,
		.flags		= IORESOURCE_IRQ,
	},
};
#endif

static struct platform_device mt3351_device_sd[] = 
{
    #if defined(CFG_DEV_MSDC1)
    {
    	.name			= "mt3351-sd",
    	.id				= 0,
    	.num_resources	= ARRAY_SIZE(mt3351_resource_sd1),
    	.resource		= mt3351_resource_sd1,
    	.dev = {
    	    .platform_data = &mt3351_sd1_hw,
    	},
    },
    #endif
    #if defined(CFG_DEV_MSDC2)
    {
    	.name			= "mt3351-sd",
    	.id				= 1,
    	.num_resources	= ARRAY_SIZE(mt3351_resource_sd2),
    	.resource		= mt3351_resource_sd2,
        .dev = {
            .platform_data = &mt3351_sd2_hw,
        },
    },
    #endif
    #if defined(CFG_DEV_MSDC3)
    {
    	.name			= "mt3351-sd",
    	.id				= 2,
    	.num_resources	= ARRAY_SIZE(mt3351_resource_sd3),
    	.resource		= mt3351_resource_sd3,
        .dev = {
            .platform_data = &mt3351_sd3_hw,
        },
    },
    #endif
};

/*=======================================================================*/
/* MT3351 UART Ports                                                     */
/*=======================================================================*/
#if defined(CFG_DEV_UART1)
static struct resource mt3351_resource_uart1[] = {
	{
		.start		= UART1_BASE,
		.end		= UART1_BASE + MT3351_UART_SIZE - 1,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= MT3351_IRQ_UART1_CODE,
		.flags		= IORESOURCE_IRQ,
	},
};
#endif

#if defined(CFG_DEV_UART2)
static struct resource mt3351_resource_uart2[] = {
	{
		.start		= UART2_BASE,
		.end		= UART2_BASE + MT3351_UART_SIZE - 1,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= MT3351_IRQ_UART2_CODE,
		.flags		= IORESOURCE_IRQ,
	},
};
#endif

#if defined(CFG_DEV_UART3)
static struct resource mt3351_resource_uart3[] = {
	{
		.start		= UART3_BASE,
		.end		= UART3_BASE + MT3351_UART_SIZE - 1,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= MT3351_IRQ_UART3_CODE,
		.flags		= IORESOURCE_IRQ,
	},
};
#endif

#if defined(CFG_DEV_UART4)
static struct resource mt3351_resource_uart4[] = {
	{
		.start		= UART4_BASE,
		.end		= UART4_BASE + MT3351_UART_SIZE - 1,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= MT3351_IRQ_UART4_CODE,
		.flags		= IORESOURCE_IRQ,
	},
};
#endif

#if defined(CFG_DEV_UART5)
static struct resource mt3351_resource_uart5[] = {
	{
		.start		= UART5_BASE,
		.end		= UART5_BASE + MT3351_UART_SIZE - 1,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= MT3351_IRQ_UART5_CODE,
		.flags		= IORESOURCE_IRQ,
	},
};
#endif

static struct platform_device mt3351_device_uart[] = {

    #if defined(CFG_DEV_UART1)
    {
    	.name			= "mt3351-uart",
    	.id				= 0,
    	.num_resources	= ARRAY_SIZE(mt3351_resource_uart1),
    	.resource		= mt3351_resource_uart1,
    },
    #endif
    #if defined(CFG_DEV_UART2)
    {
    	.name			= "mt3351-uart",
    	.id				= 1,
    	.num_resources	= ARRAY_SIZE(mt3351_resource_uart2),
    	.resource		= mt3351_resource_uart2,
    },
    #endif
    #if defined(CFG_DEV_UART3)
    {
    	.name			= "mt3351-uart",
    	.id				= 2,
    	.num_resources	= ARRAY_SIZE(mt3351_resource_uart3),
    	.resource		= mt3351_resource_uart3,
    },
    #endif   
    #if defined(CFG_DEV_UART4)
    {
    	.name			= "mt3351-uart",
    	.id				= 3,
    	.num_resources	= ARRAY_SIZE(mt3351_resource_uart4),
    	.resource		= mt3351_resource_uart4,
    },
    #endif
    #if defined(CFG_DEV_UART5)
    {
    	.name			= "mt3351-uart",
    	.id				= 4,
    	.num_resources	= ARRAY_SIZE(mt3351_resource_uart5),
    	.resource		= mt3351_resource_uart5,
    },
    #endif
};

/*=======================================================================*/
/* MT3351 I2C                                                            */
/*=======================================================================*/
#if defined(CFG_DEV_I2C)
static struct resource mt3351_resource_i2c[] = {
	{
		.start		= MT3351_IRQ_I2C_CODE,
		.flags		= IORESOURCE_IRQ,
	},
};

static struct platform_device mt3351_device_i2c = {
	.name	       = "mt3351-i2c",
	.id            = -1,
	.num_resources = ARRAY_SIZE(mt3351_resource_i2c),
	.resource      = mt3351_resource_i2c,
};
#endif


/*=======================================================================*/
/* MT3351 PWM                                                             */
/*=======================================================================*/

static struct platform_device mt3351_device_pwm = 
{
    .name = "mt3351-pwm",
    .id   = -1,
};


/*=======================================================================*/
/* MT3351 FB                                                             */
/*=======================================================================*/

static u64 mtkfb_dmamask = ~(u32)0;

static struct platform_device mt3351_device_fb = {
    .name = "mt3351-fb",
    .id   = 0,
    .dev = {
        .dma_mask = &mtkfb_dmamask,
        .coherent_dma_mask = 0xffffffff,
    },
};

/*=======================================================================*/
/* MT3351 Battery                                                        */
/*=======================================================================*/

#define MODULE_NAME		"mt3351-battery"

static struct platform_device mt3351_bat_device = {
	.name			= MODULE_NAME,
	.id                     = 0,
	.dev			= {
	}
};


/*=======================================================================*/
/* MT3351 Camera                                                        */
/*=======================================================================*/
#define IRQ_CAM                       0
#define MT3351_CAMIF_BOOTMEM_IDX   2
#define MT3351_CAMIF_BOOTMEM_SIZE  0

static struct resource mt3351_cam_dev_resource[] = {
	[0] = {
		.start = 0, //S3C2413_PA_CAMIF,
		.end   = 0,//S3C2413_PA_CAMIF + S3C24XX_SZ_CAMIF - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_CAM,
		.end   = IRQ_CAM,
		.flags = IORESOURCE_IRQ,
	},
	[MT3351_CAMIF_BOOTMEM_IDX] =
	{
		.start = 0,					/* Start with NULL. Memory will be allocated later. */
		.end   = MT3351_CAMIF_BOOTMEM_SIZE,		/* Start with length. Will be added to allocated memory later. */
		.flags = IORESOURCE_MEM | IORESOURCE_DMA,	/* DMA Memory address. */
	},
};

static u64 mt3351_cam_dev_dmamask = 0xffffffffUL;

static struct platform_device mt3351_cam_dev = {
	.name		  = "mt3351-camif",
	.id		  = -1,
	.num_resources	  = 0,//ARRAY_SIZE(s3c_cam_dev_resource),
	.resource	  = mt3351_cam_dev_resource,
	.dev              = {
		.dma_mask = &mt3351_cam_dev_dmamask,
		.coherent_dma_mask = 0xffffffffUL
	}
};

/*=======================================================================*/
/* MT3351 USB GADGET                                                     */
/*=======================================================================*/

struct platform_device mt_device_usbgadget = {
	.name		  = "mt_udc",
	.id		  = -1,
};

/*=======================================================================*/
/* MT3351 Touch Screen                                                   */
/*=======================================================================*/

struct platform_device mt_ts_dev = {
    .name = "mt_ts",
    .id   = -1,
};

/*=======================================================================*/
/* MT3351 Board Device Initialization                                    */
/*=======================================================================*/
__init int mt3351_board_init(void)
{
    int i;

    /* NOTICE: For device power management purpose, devices used by other 
     *         devices should be registered before other devices are registered. 
     *         For example, DMA should be registered before UART, I2C, MSDC 
     *         since these devices could use DMA. By such way, DMA will be 
     *         suspended after UART, I2C, MSDC were suspended.
     */
#if defined(CONFIG_SERIAL_MT3351)
    for (i = 0; i < ARRAY_SIZE(mt3351_device_uart); i++)
        platform_device_register(&mt3351_device_uart[i]);
#endif

#if defined(CONFIG_I2C_MT3351)
    platform_device_register(&mt3351_device_i2c);
#endif

#if defined(CONFIG_MMC_MT3351)
    for (i = 0; i < ARRAY_SIZE(mt3351_device_sd); i++)
        platform_device_register(&mt3351_device_sd[i]);
#endif

    platform_device_register(&mt3351_device_pwm);

#if defined(CONFIG_FB_MT3351)
    platform_device_register(&mt3351_device_fb);
#endif

#if defined(CONFIG_USB_GADGET_MT3351)
    platform_device_register(&mt_device_usbgadget);
#endif

#if defined(CONFIG_BATTERY_MT3351)
	platform_device_register(&mt3351_bat_device);
#endif

#if defined(CONFIG_VIDEO_MT3351_CAMIF)
	platform_device_register(&mt3351_cam_dev);
#endif

#if defined(CONFIG_TOUCHSCREEN_MT3351)
    platform_device_register(&mt_ts_dev);
#endif

    return 0;
}

