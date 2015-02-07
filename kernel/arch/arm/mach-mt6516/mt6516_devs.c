
 /* linux/arch/arm/mach-mt6516/mt6516_devs.c
  *
  * Devices supported on MT6516 machine
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
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/android_pmem.h>
#include <asm/setup.h>
#include <asm/mach/arch.h>
#include <mach/memory.h>
#include <mach/irqs.h>
#include <mach/mt6516.h>
#include <mach/mt6516_reg_base.h>
#include <mach/mt6516_devs.h>
#include <mach/mt6516_ap_config.h> /*for CHIP_VER_ECO_1/CHIP_VER_ECO_2 */
#include <mach/mt6516_boot.h> /*for BOOTMODE */
#include <linux/sysfs.h>
#include <linux/i2c.h>

#include <linux/usb/android.h>
#include <asm/io.h>
#include <mtk_usb_custom.h>

extern unsigned long max_pfn;
char serial_number[64];

#define RESERVED_MEM_MODEM   		PHYS_OFFSET
#ifndef CONFIG_RESERVED_MEM_SIZE_FOR_PMEM
#define CONFIG_RESERVED_MEM_SIZE_FOR_PMEM 	0	       	 	
#endif
#ifndef CONFIG_RESERVED_MEM_SIZE_FOR_CEVA
#define CONFIG_RESERVED_MEM_SIZE_FOR_CEVA 	0	       	 	
#endif

#define RESERVED_MEM_SIZE_FOR_FB (DISP_GetVRamSize())

#define TOTAL_RESERVED_MEM_SIZE (CONFIG_RESERVED_MEM_SIZE_FOR_PMEM + \
                                 CONFIG_RESERVED_MEM_SIZE_FOR_CEVA + \
                                 RESERVED_MEM_SIZE_FOR_FB)


#define MAX_PFN        ((max_pfn << PAGE_SHIFT) + PHYS_OFFSET)

#define PMEM_MM_START  (MAX_PFN)
#define PMEM_MM_SIZE   (CONFIG_RESERVED_MEM_SIZE_FOR_PMEM)

#define CEVA_START     (PMEM_MM_START + PMEM_MM_SIZE)
#define CEVA_SIZE      (CONFIG_RESERVED_MEM_SIZE_FOR_CEVA)

#define FB_START       (CEVA_START + CEVA_SIZE)
#define FB_SIZE        (RESERVED_MEM_SIZE_FOR_FB)
  
extern UINT32 DISP_GetVRamSize(void);
extern void   mtkfb_set_lcm_inited(BOOL isLcmInited);
extern const unsigned int get_disp_id(void);

struct {
	u32 base;
	u32 size;
} bl_fb = {0, 0};

static int use_bl_fb = 0;

/*=======================================================================*/
/* MT6516 SD Hosts                                                       */
/*=======================================================================*/
#if defined(CFG_DEV_MSDC1)
static struct resource mt6516_resource_sd1[] = {
	{
		.start		= MSDC1_BASE,
		.end		= MSDC1_BASE + 0x70,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= MT6516_MSDC1_IRQ_LINE,
		.flags		= IORESOURCE_IRQ,
	},
	{
		.start		= MT6516_MSDC1EVENT_IRQ_LINE,
		.flags		= IORESOURCE_IRQ,
	},	
};
#endif

#if defined(CFG_DEV_MSDC2)
static struct resource mt6516_resource_sd2[] = {
	{
		.start		= MSDC2_BASE,
		.end		= MSDC2_BASE + 0x70,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= MT6516_MSDC2_IRQ_LINE,
		.flags		= IORESOURCE_IRQ,
	},
	{
		.start		= MT6516_MSDC2EVENT_IRQ_LINE,
		.flags		= IORESOURCE_IRQ,
	},	
};
#endif

#if defined(CFG_DEV_MSDC3)
static struct resource mt6516_resource_sd3[] = {
	{
		.start		= MSDC3_BASE,
		.end		= MSDC3_BASE + 0x70,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= MT6516_MSDC3_IRQ_LINE,
		.flags		= IORESOURCE_IRQ,
	},
	{
		.start		= MT6516_MSDC3EVENT_IRQ_LINE,
		.flags		= IORESOURCE_IRQ,
	},
};
#endif

static struct platform_device mt6516_device_sd[] = 
{
    #if defined(CFG_DEV_MSDC1)
    {
    	.name			= "mt6516-sd",
    	.id				= 0,
    	.num_resources	= ARRAY_SIZE(mt6516_resource_sd1),
    	.resource		= mt6516_resource_sd1,
    	.dev = {
    	    .platform_data = &mt6516_sd1_hw,
    	},
    },
    #endif
    #if defined(CFG_DEV_MSDC2)
    {
    	.name			= "mt6516-sd",
    	.id				= 1,
    	.num_resources	= ARRAY_SIZE(mt6516_resource_sd2),
    	.resource		= mt6516_resource_sd2,
        .dev = {
            .platform_data = &mt6516_sd2_hw,
        },
    },
    #endif
    #if defined(CFG_DEV_MSDC3)
    {
    	.name			= "mt6516-sd",
    	.id				= 2,
    	.num_resources	= ARRAY_SIZE(mt6516_resource_sd3),
    	.resource		= mt6516_resource_sd3,
        .dev = {
            .platform_data = &mt6516_sd3_hw,
        },
    },
    #endif
};

/*=======================================================================*/
/* MT6516 UART Ports                                                     */
/*=======================================================================*/
#if defined(CFG_DEV_UART1)
static struct resource mt6516_resource_uart1[] = {
	{
		.start		= UART1_BASE,
		.end		= UART1_BASE + MT6516_UART_SIZE - 1,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= MT6516_UART1_IRQ_LINE,
		.flags		= IORESOURCE_IRQ,
	},
};
#endif

#if defined(CFG_DEV_UART2)
static struct resource mt6516_resource_uart2[] = {
	{
		.start		= UART2_BASE,
		.end		= UART2_BASE + MT6516_UART_SIZE - 1,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= MT6516_UART2_IRQ_LINE,
		.flags		= IORESOURCE_IRQ,
	},
};
#endif

#if defined(CFG_DEV_UART3)
static struct resource mt6516_resource_uart3[] = {
	{
		.start		= UART3_BASE,
		.end		= UART3_BASE + MT6516_UART_SIZE - 1,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= MT6516_UART3_IRQ_LINE,
		.flags		= IORESOURCE_IRQ,
	},
};
#endif

#if defined(CFG_DEV_UART4)
static struct resource mt6516_resource_uart4[] = {
	{
		.start		= UART4_BASE,
		.end		= UART4_BASE + MT6516_UART_SIZE - 1,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= MT6516_UART4_IRQ_LINE,
		.flags		= IORESOURCE_IRQ,
	},
};
#endif
 
static struct platform_device mt6516_device_uart[] = {

    #if defined(CFG_DEV_UART1)
    {
    	.name			= "mt6516-uart",
    	.id				= 0,
    	.num_resources	= ARRAY_SIZE(mt6516_resource_uart1),
    	.resource		= mt6516_resource_uart1,
    },
    #endif
    #if defined(CFG_DEV_UART2)
    {
    	.name			= "mt6516-uart",
    	.id				= 1,
    	.num_resources	= ARRAY_SIZE(mt6516_resource_uart2),
    	.resource		= mt6516_resource_uart2,
    },
    #endif
    #if defined(CFG_DEV_UART3)
    {
    	.name			= "mt6516-uart",
    	.id				= 2,
    	.num_resources	= ARRAY_SIZE(mt6516_resource_uart3),
    	.resource		= mt6516_resource_uart3,
    },
    #endif   

    #if defined(CFG_DEV_UART4)
    {
    	.name			= "mt6516-uart",
    	.id				= 3,
    	.num_resources	= ARRAY_SIZE(mt6516_resource_uart4),
    	.resource		= mt6516_resource_uart4,
    },
    #endif
};

/*=======================================================================*/
/* MT6516 I2C                                                            */
/*=======================================================================*/
#if defined(CFG_DEV_I2C)
static struct resource mt6516_resource_i2c1[] = {
	{
		.start		= I2C_BASE,
		.end		= I2C_BASE + 0x70,
		.flags		= IORESOURCE_MEM,
	},
    {
        .start		= MT6516_I2C_IRQ_LINE,
		.flags		= IORESOURCE_IRQ,
	},
};

static struct resource mt6516_resource_i2c2[] = {
	{
		.start		= I2C2_BASE,
		.end		= I2C2_BASE + 0x70,
		.flags		= IORESOURCE_MEM,
	},
    {
        .start		= MT6516_I2C2_IRQ_LINE,
		.flags		= IORESOURCE_IRQ,
	}, 
};

static struct resource mt6516_resource_i2c3[] = {
	{
		.start		= I2C3_BASE,
		.end		= I2C3_BASE + 0x70,
		.flags		= IORESOURCE_MEM,
	},
    {
        .start		= MT6516_I2C3_IRQ_LINE,
		.flags		= IORESOURCE_IRQ,
	},
};


static struct platform_device mt6516_device_i2c[] = {
    {
    	.name	       = "mt6516-i2c",
        .id            = 0,
    	.num_resources = ARRAY_SIZE(mt6516_resource_i2c1),
    	.resource      = mt6516_resource_i2c1,
    },
    {
    	.name	       = "mt6516-i2c",
        .id            = 1,
    	.num_resources = ARRAY_SIZE(mt6516_resource_i2c2),
    	.resource      = mt6516_resource_i2c2,
    },
    {
        .name          = "mt6516-i2c",
        .id            = 2,
        .num_resources = ARRAY_SIZE(mt6516_resource_i2c3),
        .resource      = mt6516_resource_i2c3,
    },    
};


#endif

struct android_usb_platform_data android_usb_pdata = {
#if !defined(CONFIG_USB_DISABLE_SERIAL)
    .serial_number = "MT123456789",
#endif    
};

#if 0
/*=======================================================================*/
/* MT6516 PWM                                                             */
/*=======================================================================*/

static struct platform_device mt6516_device_pwm = 
{
    .name = "mt6516-pwm",
    .id   = -1,
};
#endif

/*=======================================================================*/
/* MT6516 FB                                                             */
/*=======================================================================*/

static u64 mtkfb_dmamask = ~(u32)0;

static struct resource resource_fb[] = {
	{
		.start		= 0, /* Will be redefined later */
		.end		= 0, 
		.flags		= IORESOURCE_MEM
	}
};	

static struct platform_device mt6516_device_fb = {
    .name = "mt6516-fb",
    .id   = 0,
    .num_resources = ARRAY_SIZE(resource_fb),
    .resource      = resource_fb,
    .dev = {
        .dma_mask = &mtkfb_dmamask,
        .coherent_dma_mask = 0xffffffff,
    },
};

#if 0
/*=======================================================================*/
/* MT6516 Battery                                                        */
/*=======================================================================*/

#define MODULE_NAME		"mt6516-battery"

static struct platform_device mt6516_bat_device = {
	.name	 = MODULE_NAME,
	.id      = -1,
};


/*=======================================================================*/
/* MT6516 Camera                                                        */
/*=======================================================================*/
#define IRQ_CAM                       0
#define MT6516_CAMIF_BOOTMEM_IDX   2
#define MT6516_CAMIF_BOOTMEM_SIZE  0

static struct resource mt6516_cam_dev_resource[] = {
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
	[MT6516_CAMIF_BOOTMEM_IDX] =
	{
		.start = 0,					/* Start with NULL. Memory will be allocated later. */
		.end   = MT6516_CAMIF_BOOTMEM_SIZE,		/* Start with length. Will be added to allocated memory later. */
		.flags = IORESOURCE_MEM | IORESOURCE_DMA,	/* DMA Memory address. */
	},
};

static u64 mt6516_cam_dev_dmamask = 0xffffffffUL;

static struct platform_device mt6516_cam_dev = {
	.name		  = "mt6516-camif",
	.id		  = -1,
	.num_resources	  = 0,//ARRAY_SIZE(s3c_cam_dev_resource),
	.resource	  = mt6516_cam_dev_resource,
	.dev              = {
		.dma_mask = &mt6516_cam_dev_dmamask,
		.coherent_dma_mask = 0xffffffffUL
	}
};
#endif

/*=======================================================================*/
/* MT6516 IDP                                                        */
/*=======================================================================*/
static struct resource mt6516_IDP_resource[] = {
    [0] = {//Image DMA 0
        .start = IMGDMA_BASE,
        .end   = IMGDMA_BASE + 0xF64,
        .flags = IORESOURCE_MEM,
    },
    [1] = {//Capture resizer
        .start = CRZ_BASE,
        .end   = CRZ_BASE + 0x40,
        .flags = IORESOURCE_MEM,
    },
    [2] = {//Drop resizer
        .start = DRZ_BASE,
		.end   = DRZ_BASE + 0x24,
		.flags = IORESOURCE_MEM,
    },
    [3] = {//Image processor
        .start = IMG_BASE,
        .end   = IMG_BASE + 0x330,
        .flags = IORESOURCE_MEM,
    },
    [4] = {//post process resizer
        .start = PRZ_BASE,
        .end   = PRZ_BASE + 0x5C,
        .flags = IORESOURCE_MEM,
    },
    [5] = {//image DMA1
        .start = IMGDMA1_BASE,
        .end   = IMGDMA1_BASE + 0xC88,
        .flags = IORESOURCE_MEM,
    },	
    [6] = {//Image DMA 0 IRQ
        .start = MT6516_IMGDMA_IRQ_LINE,
        .end   = MT6516_IMGDMA_IRQ_LINE,
        .flags = IORESOURCE_IRQ,
    },
    [7] = {//Capture resizer IRQ
        .start = MT6516_CRZ_IRQ_LINE,
        .end   = MT6516_CRZ_IRQ_LINE,
        .flags = IORESOURCE_IRQ,
    },
    [8] = {//Drop resizer IRQ
        .start = MT6516_DRZ_IRQ_LINE,
        .end   = MT6516_DRZ_IRQ_LINE,
        .flags = IORESOURCE_IRQ,
    },
    [9] = {//Image processor IRQ
        .start = MT6516_IMGPROC_IRQ_LINE,
        .end   = MT6516_IMGPROC_IRQ_LINE,
        .flags = IORESOURCE_IRQ,
    },
    [10] = {//post processor resizer IRQ
        .start = MT6516_PRZ_IRQ_LINE,
        .end   = MT6516_PRZ_IRQ_LINE,
        .flags = IORESOURCE_IRQ,
    },
    [11] = {//Image DMA 1 IRQ
        .start = MT6516_IMGDMA2_IRQ_LINE,
        .end = MT6516_IMGDMA2_IRQ_LINE,
        .flags = IORESOURCE_IRQ,
    }
};

static u64 mt6516_IDP_dmamask = ~(u32)0;

static struct platform_device mt6516_IDP_dev = {
	.name		  = "mt6516-IDP",
	.id		  = 0,
	.num_resources	  = ARRAY_SIZE(mt6516_IDP_resource),
	.resource	  = mt6516_IDP_resource,
	.dev              = {
		.dma_mask = &mt6516_IDP_dmamask,
		.coherent_dma_mask = 0xffffffffUL
	}
};

/*=======================================================================*/
/* MT6516 ISP                                                        */
/*=======================================================================*/
static struct resource mt6516_ISP_resource[] = {
    [0] = {// ISP configuration
        .start = CAM_BASE,
        .end   = CAM_BASE + 0x618,
        .flags = IORESOURCE_MEM,
    },
    [1] = {// statistic result
        .start = CAM_ISP_BASE,
        .end   = CAM_ISP_BASE + 0x324,
        .flags = IORESOURCE_MEM,
    },
    [2] = {//ISP IRQ
        .start = MT6516_CAMERA_IRQ_LINE,
        .end   = MT6516_CAMERA_IRQ_LINE,
        .flags = IORESOURCE_IRQ,
    }
};

static u64 mt6516_ISP_dmamask = ~(u32)0;

static struct platform_device mt6516_ISP_dev = {
	.name		  = "mt6516-ISP",
	.id		  = 0,
	.num_resources	  = ARRAY_SIZE(mt6516_ISP_resource),
	.resource	  = mt6516_ISP_resource,
	.dev              = {
		.dma_mask = &mt6516_ISP_dmamask,
		.coherent_dma_mask = 0xffffffffUL
	}
};

/*=======================================================================*/
/* Image sensor                                                        */
/*=======================================================================*/
static struct platform_device sensor_dev_1 = {
	.name		  = "ov2655_sensor",
	.id		  = -1,
};

static struct platform_device sensor_dev_2 = {
	.name		  = "hi253_sensor",
	.id		  = -1,
};

/*=======================================================================*/
/* Lens actuator                                                        */
/*=======================================================================*/
static struct platform_device actuator_dev = {
	.name		  = "lens_actuator",
	.id		  = -1,
};

/*=======================================================================*/
/* MT6516 USB GADGET                                                     */
/*=======================================================================*/

struct platform_device mt_device_usbgadget = {
	.name		  = "mt_udc",
	.id		  = -1,
};

struct platform_device android_usb_device = {
    .name = "android_usb",
    .id   =            -1,
    .dev = {
        .platform_data = &android_usb_pdata,
    },	
};

/*=======================================================================*/
/* MT6516 Touch Screen                                                   */
/*=======================================================================*/

struct platform_device mt_ts_dev = {
    .name = "mt_ts",
    .id   = -1,
};

/*=======================================================================*/
/* MT6516 Keypad                                                   */
/*=======================================================================*/

static struct platform_device mt_kpd_dev = {
    .name = "mt6516-kpd",
    .id   = -1,
};

/*=======================================================================*/
/* MT6516 Vibrator                                                   */
/*=======================================================================*/

static struct platform_device mt_vib_dev = {
    .name = "mt6516_vibrator",
    .id   = -1,
};


/*=======================================================================*/
/* MT6516 GPS module                                                    */
/*=======================================================================*/
/* MT3326 GPS */
struct platform_device mt3326_device_gps = {
	.name	       = "mt3326-gps",
	.id            = -1,
	.dev = {
        .platform_data = &mt3326_gps_hw,
    },	
};

/*=======================================================================*/
/* MT6516 sensor module                                                  */
/*=======================================================================*/
#if defined(CONFIG_SENSORS_MT6516)

#if defined(CONFIG_SENSORS_AK8975) || defined(CONFIG_SENSORS_AK8975_MODULE)
#define CAD0 (0)
#define CAD1 (0)
#define AK8975_BASE_ADDR (0x03)
#define AK8975_ADDR  ((AK8975_BASE_ADDR) << 3 |(CAD0) << 2|(CAD1) << 1)
#define GPIO_AK8975C_INT MT6516_EINT2_NUM_IRQ_LINE
#endif

static struct i2c_board_info i2c_devices_2[] = {
#if defined(CONFIG_SENSORS_AK8975) || defined(CONFIG_SENSORS_AK8975_MODULE)
	{
		I2C_BOARD_INFO("akm8975c", AK8975_ADDR),
		.platform_data = NULL,
		.irq = GPIO_AK8975C_INT,
	},
#endif
#if defined(CONFIG_BMA150_I2C) || defined(CONFIG_BMA150_I2C_MODULE)
	  {
		  I2C_BOARD_INFO("bma150_i2c", 0x70),
		  .platform_data = NULL,
	  },
  #endif
};

struct platform_device sensor_adxl345 = {
	.name	       = "adxl345",
	.id            = -1,
};

struct platform_device sensor_kxte91026 = {
	.name	       = "kxte91026",
	.id            = -1,
};

struct platform_device sensor_ami304 = {
	.name	       = "ami304",
	.id            = -1,
};

struct platform_device sensor_cm3623 = {
	.name	       = "cm3623",
	.id            = -1,
};

/* hwmon sensor */
struct platform_device hwmon_sensor = {
	.name	       = "hwmsensor",
	.id            = -1,
};
#endif

/*=======================================================================*/
/* MT6516 RFKill module (BT)                                             */
/*=======================================================================*/
/* MT66xx RFKill BT */
struct platform_device mt6516_device_rfkill = {
	.name	       = "mt-rfkill",
	.id            = -1,
};


struct platform_device gpio_dev = 
{
    .name = "mt6516-gpio",
    .id   = -1,    
};

/*=======================================================================*/
/* MT6516 PMEM                                                           */
/*=======================================================================*/
static struct android_pmem_platform_data  pdata_multimedia = {
	.name = "pmem_multimedia",
	.no_allocator = 0,
	.cached = 1,
	.buffered = 1
};

static struct platform_device pmem_multimedia_device = {
	.name = "android_pmem",
	.id = 1,
	.dev = { .platform_data = &pdata_multimedia }
};

struct platform_device mt6516_jogball = 
{
    .name = "mt6516-jb",
    .id   = -1,    
};

struct platform_device mt6516_ofn = 
{
    .name = "mtofn",
    .id   = -1,    
};

struct platform_device mt6516_fm = 
{
    .name = "fm",
    .id   = -1,    
};


#if defined(CONFIG_CEVA_MT6516)
/*=======================================================================*/
/* CEVA		                                                         */
/*=======================================================================*/
static struct resource resource_ceva[] = {
	{
		.start		= 0, /* Will be redefined later */
		.end		= 0, 
		.flags		= IORESOURCE_MEM
	}
};	
static struct platform_device ceva_device = {
    	.name	       = "ccci_ceva",
        .id            = 0,
    	.num_resources = ARRAY_SIZE(resource_ceva),
    	.resource      = resource_ceva
};
#endif

/* data structure and macro for system info */
#define to_sysinfo_attribute(x) container_of(x, struct sysinfo_attribute, attr)

struct sysinfo_attribute{
    struct attribute attr;
    ssize_t (*show)(char *buf);
    ssize_t (*store)(const char *buf, size_t count);
};

static struct kobject sn_kobj;

/* ========================================= */
/* implementation of serial number attribute */
/* ========================================= */
static ssize_t sn_show(char *buf){
    return snprintf(buf, 4096, "%s\n", serial_number);
}

static ssize_t sn_store(const char *buf, size_t count){
    return 0;
}

struct sysinfo_attribute sn_attr = {
    .attr = {"SerialNumber", THIS_MODULE, 0644},
    .show = sn_show,
    .store = sn_store
};
/* ========================================= */

/* ============================= */
/* implementation of system info */
/* ============================= */
static ssize_t sysinfo_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
    struct sysinfo_attribute *sysinfo_attr = to_sysinfo_attribute(attr);
    ssize_t ret = -EIO;

    if(sysinfo_attr->show)
        ret = sysinfo_attr->show(buf);

    return ret;
}

static ssize_t sysinfo_store(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
    struct sysinfo_attribute *sysinfo_attr = to_sysinfo_attribute(attr);
    ssize_t ret = -EIO;

    if(sysinfo_attr->store)
        ret = sysinfo_attr->store(buf, count);
    
    return ret;
}

static struct sysfs_ops sn_sysfs_ops = {
    .show = sysinfo_show,
    .store = sysinfo_store
};

static struct attribute *sn_attrs[] = {
    &sn_attr.attr,
    NULL
};

static struct kobj_type sn_ktype = {
    .sysfs_ops = &sn_sysfs_ops,
    .default_attrs = sn_attrs
};
/* ============================= */

/* serial number mapping utility */
/* ================= */
/* Mapping Table     */
/* ================= */

char ser_mapping(unsigned int val){
    char c = 65;

    if(val < 26){
        c = val + 65;
    }
    else if(26 <= val && val < 52){
        c = (val - 26) + 97;
    }
    else if(52 <= val && val < 62){
        c = (val - 52) + 48;
    }
    else if(val == 62){
        c = 45;
    }
    else if(val == 63){
        c = 95;
    }

    return c;
}


/*=======================================================================*/
/* Commandline fitlter to choose the supported commands                  */
/*=======================================================================*/
static void cmdline_filter(struct tag *cmdline_tag, char *default_cmdline)
{
	const char *desired_cmds[] = {
	                                 "rdinit=",
                                     "nand_manf_id=",
                                     "nand_dev_id=",
                                     "uboot_ver=",
                                     "uboot_build_ver=",
			             "jrd_pl_ver=",
        				"jrd_uboot_ver="
					 };

	int i;
	char *cs,*ce;

	cs = cmdline_tag->u.cmdline.cmdline;
	ce = cs;
	while((__u32)ce < (__u32)tag_next(cmdline_tag)) {
	
	    while(*cs == ' ' || *cs == '\0') {
	    	cs++;
	    	ce = cs;
	    }

	    if (*ce == ' ' || *ce == '\0') {	       
	    	for (i = 0; i < sizeof(desired_cmds)/sizeof(char *); i++){	    	
	    	    if (memcmp(cs, desired_cmds[i], strlen(desired_cmds[i])) == 0) {
	    	        *ce = '\0';
         	        //Append to the default command line
        	        strcat(default_cmdline, " ");	
	    	        strcat(default_cmdline, cs);		    	        
	    	    }
	    	}
	    	cs = ce + 1;
	    }
	    ce++;	    
	}	
	if (strlen(default_cmdline) >= COMMAND_LINE_SIZE)
	{
		panic("Command line length is too long.\n\r");
	}
}


/*=======================================================================*/
/* Parse the framebuffer info						 */
/*=======================================================================*/
static int __init parse_tag_videofb_fixup(const struct tag *tags)
{
	bl_fb.base = tags->u.videolfb.lfb_base;
	bl_fb.size = tags->u.videolfb.lfb_size;
        use_bl_fb++;
	return 0;
}

/*=======================================================================*/
/* MT6516 Fixeup function                                                */
/*=======================================================================*/
void mt6516_fixup(struct machine_desc *desc, struct tag *tags, char **cmdline, struct meminfo *mi)
{	
    struct tag *cmdline_tag = NULL;
    struct tag *reserved_mem_bank_tag = NULL;
    struct tag *none_tag = NULL;  
    
    int32_t max_limit_size = CONFIG_MAX_DRAM_SIZE_SUPPORT - 
                             RESERVED_MEM_MODEM;
    int32_t bl_mem_sz = 0;
    
    for (; tags->hdr.size; tags = tag_next(tags)) {
        if (tags->hdr.tag == ATAG_MEM) {
	    bl_mem_sz += tags->u.mem.size;
            if (max_limit_size > 0) {
                if (max_limit_size >= tags->u.mem.size) {
                    max_limit_size -= tags->u.mem.size;              
                } 
		else {
                    tags->u.mem.size = max_limit_size;
                    max_limit_size = 0;
                }

                if (tags->u.mem.size >= (TOTAL_RESERVED_MEM_SIZE)) {
                    reserved_mem_bank_tag = tags;
                }
            }
            else {
                tags->u.mem.size = 0;
            }
        }
        else if (tags->hdr.tag == ATAG_CMDLINE) {
            cmdline_tag = tags;
        }
        else if (tags->hdr.tag == ATAG_BOOT) {
            g_boot_mode = tags->u.boot.bootmode;
        }
        else if (tags->hdr.tag == ATAG_VIDEOLFB) {
            parse_tag_videofb_fixup(tags);
        }
    }
    /* 
    * If the maximum memory size configured in kernel  
    * is smaller than the actual size (passed from BL)
    * Still limit the maximum memory size but use the FB 
    * initialized by BL  
    */
    if (bl_mem_sz >= (CONFIG_MAX_DRAM_SIZE_SUPPORT - RESERVED_MEM_MODEM)) {
	use_bl_fb++;
    } 

    /* Reserve memory in the last bank */
    if (reserved_mem_bank_tag)
        reserved_mem_bank_tag->u.mem.size -= ((__u32)TOTAL_RESERVED_MEM_SIZE);

    if(tags->hdr.tag == ATAG_NONE)
	none_tag = tags;

    if (cmdline_tag != NULL) {

        cmdline_filter(cmdline_tag, *cmdline);
        /* Use the default cmdline */
        memcpy((void*)cmdline_tag,
               (void*)tag_next(cmdline_tag), 
               /* ATAG_NONE actual size */
               (uint32_t)(none_tag) - (uint32_t)(tag_next(cmdline_tag)) + 8);
    }
}

/*=======================================================================*/
/* MT6516 Nand                                                   		 */
/*=======================================================================*/
#if defined(CONFIG_MTD_NAND_MT6516)
#define NFI_base    NFI_BASE//0x80032000
#define NFIECC_base NFIECC_BASE//0x80038000
static struct resource mt6516_resource_nand[] = {
	{
		.start		= NFI_base,
		.end		= NFI_base + 0x1A0,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= NFIECC_base,
		.end		= NFIECC_base + 0x150,
		.flags		= IORESOURCE_MEM,
	},	
	{
		.start		= MT6516_NFI_IRQ_LINE,
		.flags		= IORESOURCE_IRQ,
	},
	{
		.start		= MT6516_NFIECC_IRQ_LINE,
		.flags		= IORESOURCE_IRQ,
	},
};

/*=======================================================================*/
/* MT6516 NAND module                                                    */
/*=======================================================================*/
static struct platform_device mt6516_nand_dev = {
    .name = "mt6516-nand",
    .id   = 0,
   	.num_resources	= ARRAY_SIZE(mt6516_resource_nand),
   	.resource		= mt6516_resource_nand,    
    .dev            = {
        .platform_data = &mt6516_nand_hw,
    },
};
#endif

/*=======================================================================*/
/* MT6516 Board Device Initialization                                    */
/*=======================================================================*/
__init int mt6516_board_init(void)
{
	int i,retval;
	BOOTMODE bootmode = get_boot_mode();
#if !defined(CONFIG_USB_DISABLE_SERIAL)    
	unsigned int ser[4];
	unsigned int j, k, sn_index = 0;

	ser[0] = (unsigned int)__raw_readl(0xf0000010);
	ser[1] = (unsigned int)__raw_readl(0xf0000014);
	ser[2] = (unsigned int)__raw_readl(0xf0000018);
	ser[3] = (unsigned int)__raw_readl(0xf000001c);
	
	memset(serial_number, 0, sizeof(serial_number));
	snprintf(serial_number, 4, "MT-");
	sn_index = 3;

    for(j = 0; j < 4; j++){
       for(k = 0; k < 6; k++){
           serial_number[sn_index] = ser_mapping(ser[j] % 64);
           ser[j] >>= 6;
           sn_index++;
       }
    }
#else
    strncpy(serial_number, ADB_SERIAL, sizeof(serial_number)-1);
#endif
    retval = kobject_init_and_add(&sn_kobj, &sn_ktype, NULL, "SystemInfo");
    if (retval < 0) {
        printk("[%s] fail to add kobject\n", "SystemInfo");
        return retval;
    }
    

	pdata_multimedia.start = PMEM_MM_START;;
	pdata_multimedia.size = PMEM_MM_SIZE;
	printk("PMEM start: 0x%lx size: 0x%lx\n", pdata_multimedia.start, pdata_multimedia.size);

	retval = platform_device_register(&pmem_multimedia_device);
	if (retval != 0){
	       return retval;
	}	

#if defined(CONFIG_CEVA_MT6516)
	resource_ceva[0].start = CEVA_START;
	resource_ceva[0].end = CEVA_START + CEVA_SIZE - 1;
	printk("CEVA start: 0x%x end: 0x%x\n", resource_ceva[0].start, resource_ceva[0].end);
    	if (resource_ceva[0].start & 0xFFFF) {
		BUG();
	}
	retval = platform_device_register(&ceva_device);
	if (retval != 0){
	       return retval;
	}	
#endif


#if defined(CONFIG_SERIAL_MT6516)
	for (i = 0; i < ARRAY_SIZE(mt6516_device_uart); i++){
		retval = platform_device_register(&mt6516_device_uart[i]);
		if (retval != 0){
			return retval;
		}
	}
#endif

#if defined(CONFIG_SENSORS_MT6516)
#if defined(CONFIG_I2C_MT6516)
i2c_register_board_info(2, i2c_devices_2, ARRAY_SIZE(i2c_devices_2));
#endif
#endif

#if defined(CONFIG_I2C_MT6516)
	for (i = 0; i < ARRAY_SIZE(mt6516_device_i2c); i++){
		retval = platform_device_register(&mt6516_device_i2c[i]);
		if (retval != 0){
			return retval;
		}	
	}
	
#endif

#if defined(CONFIG_MMC_MT6516)
	for (i = 0; i < ARRAY_SIZE(mt6516_device_sd); i++){
		retval = platform_device_register(&mt6516_device_sd[i]);
		if (retval != 0){
			 return retval;
		}	
	}
#endif

#if defined(CONFIG_MTD_NAND_MT6516)
	retval = platform_device_register(&mt6516_nand_dev);
	if (retval != 0){
		return retval;
	}	
#endif

#if defined(CONFIG_RHINE_PWM)
	retval = platform_device_register(&mt6516_device_pwm);
	if (retval != 0){
        	return retval;
	}
#endif

#if defined(CONFIG_FB_MT6516)
    /* 
     * Bypass matching the frame buffer info. between boot loader and kernel 
     * if the limited memory size of the kernel is smaller than the 
     * memory size from bootloader
     */
     get_disp_id();
    if (((bl_fb.base == FB_START) && (bl_fb.size == FB_SIZE)) || 
         (use_bl_fb == 2)) {
        printk("FB is initialized by BL(%d)\n", use_bl_fb);
        mtkfb_set_lcm_inited(TRUE);
    } else if ((bl_fb.base == 0) && (bl_fb.size == 0)) {
        printk("FB is not initialized(%d)\n", use_bl_fb);
        mtkfb_set_lcm_inited(FALSE);
    } else {
        printk(
"******************************************************************************\n"
"   WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING\n"
"******************************************************************************\n"
"\n"
"  The default base & size values are not matched between BL and kernel\n"
"    - BOOTLD: start 0x%08x, size %d\n"
"    - KERNEL: start 0x%08lx, size %d\n"
"\n"
"  If you see this warning message, please update your uboot.\n"
"\n"
"  If it still don't work after uboot update, please contact with\n"
"  Dominic and Jett, thanks.\n"
"\n"
"******************************************************************************\n"
"   WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING\n"
"******************************************************************************\n"
"\n",   bl_fb.base, bl_fb.size, FB_START, FB_SIZE);

        {
            int delay_sec = 20;
            
            while (delay_sec >= 0) {
                printk("\rcontinue after %d seconds ...", delay_sec--);
                mdelay(1000);
            }
            printk("\n");
        }
#if 0
        panic("The default base & size values are not matched "
              "between BL and kernel\n");
#endif
    }

	resource_fb[0].start = FB_START;
	resource_fb[0].end = FB_START + FB_SIZE - 1;

	printk("FB start: 0x%x end: 0x%x\n", resource_fb[0].start,
                                         resource_fb[0].end);

	retval = platform_device_register(&mt6516_device_fb);
	if (retval != 0) {
		 return retval;
	}   
#endif

	retval = platform_device_register(&gpio_dev);
	if (retval != 0){
		 return retval;
	}
	

#if defined(CONFIG_USB_GADGET_MT6516)
#if !defined(CONFIG_USB_DISABLE_SERIAL)
    android_usb_pdata.serial_number = serial_number;
#endif
    if((bootmode != 1) && (bootmode != 4)){
        retval = platform_device_register(&android_usb_device);
        if(retval != 0){
            return retval;
        }
    }

	retval = platform_device_register(&mt_device_usbgadget);
	if (retval != 0){
        return retval;
	}	
#endif

#if defined(CONFIG_RHINE_BAT)
	retval = platform_device_register(&mt6516_bat_device);
        if (retval != 0){
           return retval;
	}	
#endif

#if 1 //defined(CONFIG_IDP_MT6516)
	retval = platform_device_register(&mt6516_IDP_dev);
	if (retval != 0){
		return retval;
	}	
#endif

#if 1 //defined(CONFIG_ISP_MT6516)
	retval = platform_device_register(&mt6516_ISP_dev);
        if (retval != 0){
		return retval;
	}	
#endif

#if defined(CONFIG_VIDEO_CAPTURE_DRIVERS)
	retval = platform_device_register(&sensor_dev_2);
        if (retval != 0){
		return retval;
	}

	retval = platform_device_register(&sensor_dev_1);
        if (retval != 0){
		return retval;
	}	
#endif

#if defined(CONFIG_ACTUATOR)
	retval = platform_device_register(&actuator_dev);
        if (retval != 0){
		return retval;
	}	
#endif

#if defined(CONFIG_TOUCHSCREEN_MT6516)
	retval = platform_device_register(&mt_ts_dev);
	if (retval != 0){
		return retval;
	}	
#endif

#if defined(CONFIG_KEYBOARD_MT6516)
	retval = platform_device_register(&mt_kpd_dev);
	if (retval != 0){
		return retval;
	}	
#endif

#if defined(CONFIG_MT6516_VIBRATOR)
	retval = platform_device_register(&mt_vib_dev);
	if (retval != 0){
		return retval;
	}	
#endif


#if defined(CONFIG_MT6516_GPS)
	retval = platform_device_register(&mt3326_device_gps);
	if (retval != 0){
		return retval;
	}	
#endif 


#if defined(CONFIG_SENSORS_MT6516)  

#if defined(CONFIG_SENSORS_ADXL345)
	retval = platform_device_register(&sensor_adxl345);
	if (retval != 0)
		return retval;
#endif 

#if defined(CONFIG_SENSORS_KXTE91026)
#if defined(CONFIG_MT6516_PHONE_BOARD)
    {   /*
          V3 supports KXTE9-1026 motion sensor => implemented
          V2 supports ADXL345 motion sensor => not implemented
        */
        extern UINT32 g_ChipVer;
        if (g_ChipVer == CHIP_VER_ECO_2) {
            retval = platform_device_register(&sensor_kxte91026);
            if (retval != 0)
                return retval;
        }
    }
#else
	retval = platform_device_register(&sensor_kxte91026);
	if (retval != 0)
		return retval;
#endif
#endif 

#if defined(CONFIG_SENSORS_AMI304)
	retval = platform_device_register(&sensor_ami304);
	if (retval != 0)
		return retval;
#endif 

#if defined(CONFIG_SENSORS_CM3623)
	retval = platform_device_register(&sensor_cm3623);
	if (retval != 0)
		return retval;
#endif

	retval = platform_device_register(&hwmon_sensor);
	if (retval != 0)
		return retval;
#endif 


#if 1 //defined(CONFIG_RFKILL)
    retval = platform_device_register(&mt6516_device_rfkill);
    if (retval != 0){
		return retval;
	}
#endif

#if defined(CONFIG_INPUT_MT6516_JOGBALL)
    retval = platform_device_register(&mt6516_jogball);
    if (retval != 0)
        return retval;
#endif 

#if defined(CONFIG_MT6516_ADBS_A320)
    retval = platform_device_register(&mt6516_ofn);
    if (retval != 0)
        return retval;
#endif 

#if 1//defined(CONFIG_FM_RADIO_MT6516)
    retval = platform_device_register(&mt6516_fm);
    if (retval != 0)
        return retval;
#endif

	return 0;
}

int is_pmem_range(unsigned long *base, unsigned long size)
{
	unsigned long start = (unsigned long)base;
	unsigned long end = start + size;

	//printk("[PMEM] start=0x%p,end=0x%p,size=%d\n", start, end, size);
	//printk("[PMEM] PMEM_MM_START=0x%p,PMEM_MM_SIZE=%d\n", PMEM_MM_START, PMEM_MM_SIZE);

	if (start < PMEM_MM_START)
		return 0;
	if (end >= PMEM_MM_START + PMEM_MM_SIZE)
		return 0;

	return 1;
}
EXPORT_SYMBOL(is_pmem_range);

