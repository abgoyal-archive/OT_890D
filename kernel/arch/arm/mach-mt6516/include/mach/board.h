
 /* linux/include/asm-arm/arch-mt6516/board.h
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

#ifndef __ARCH_ARM_MACH_BOARD_H
#define __ARCH_ARM_MACH_BOARD_H

#include <linux/autoconf.h>
#include <linux/pm.h>
#include <mach/mt6516.h>
#include <board-custom.h>

typedef void (*sdio_irq_handler_t)(void*);  /* external irq handler */
typedef void (*pm_callback_t)(pm_message_t state, void *data);

#define MSDC_CD_PIN_EN      (1 << 0)  /* card detection pin is wired   */
#define MSDC_WP_PIN_EN      (1 << 1)  /* write protection pin is wired */
#define MSDC_SDIO_IRQ       (1 << 2)  /* use internal sdio irq (bus)   */
#define MSDC_EXT_SDIO_IRQ   (1 << 3)  /* use external sdio irq         */
#define MSDC_REMOVABLE      (1 << 4)  /* removable slot                */
#define MSDC_SYS_SUSPEND    (1 << 5)  /* suspended by system           */
#define MSDC_HIGHSPEED      (1 << 6)  /* high-speed mode support       */

/* configure the output driving capacity and slew rate */
#define MSDC_ODC_4MA        (0x0)
#define MSDC_ODC_8MA        (0x2)
#define MSDC_ODC_12MA       (0x4)
#define MSDC_ODC_16MA       (0x6)
#define MSDC_ODC_SLEW_FAST  (0)
#define MSDC_ODC_SLEW_SLOW  (1)

#define MSDC_CMD_PIN        (0)
#define MSDC_DAT_PIN        (1)
#define MSDC_CD_PIN         (2)
#define MSDC_WP_PIN         (3)

enum {
    MSDC_CLKSRC_MCU,
    MSDC_CLKSRC_MCPLL,
};

enum {
    EDGE_RISING  = 0,
    EDGE_FALLING = 1
};

struct mt6516_sd_host_hw {
    unsigned char  clk_src;          /* host clock source */
    unsigned char  cmd_edge;         /* command latch edge */
    unsigned char  data_edge;        /* data latch edge */
    unsigned char  cmd_odc;          /* command driving capability */
    unsigned char  data_odc;         /* data driving capability */
    unsigned char  cmd_slew_rate;    /* command slew rate */
    unsigned char  data_slew_rate;   /* data slew rate */
    unsigned char  padding;          /* unused padding */
    unsigned long  flags;            /* hardware capability flags */
    unsigned long  data_pins;        /* data pins */
    unsigned long  data_offset;      /* data address offset */

    /* config gpio pull mode */
    void (*config_gpio_pin)(int type, int pull);

    /* external power control for card */
    void (*ext_power_on)(void);
    void (*ext_power_off)(void);

    /* external sdio irq operations */
    void (*request_sdio_eirq)(sdio_irq_handler_t sdio_irq_handler, void *data);
    void (*enable_sdio_eirq)(void);
    void (*disable_sdio_eirq)(void);

    /* external cd irq operations */
    void (*request_cd_eirq)(sdio_irq_handler_t cd_irq_handler, void *data);
    void (*enable_cd_eirq)(void);
    void (*disable_cd_eirq)(void);
    int  (*get_cd_status)(void);
    
    /* power management callback for external module */
    void (*register_pm)(pm_callback_t pm_cb, void *data);
};

extern struct mt6516_sd_host_hw mt6516_sd1_hw;
extern struct mt6516_sd_host_hw mt6516_sd2_hw;
extern struct mt6516_sd_host_hw mt6516_sd3_hw;

/*GPS driver*/
#define GPS_FLAG_FORCE_OFF  0x0001
struct mt3326_gps_hardware {
    int (*ext_power_on)(int);
    int (*ext_power_off)(void);
};
extern struct mt3326_gps_hardware mt3326_gps_hw;

/* NAND driver */
struct mt6516_nand_host_hw {
    unsigned int nfi_bus_width;		    /* NFI_BUS_WIDTH */ 
	unsigned int nfi_access_timing;		/* NFI_ACCESS_TIMING */  
	unsigned int nfi_cs_num;			/* NFI_CS_NUM */
	unsigned int nand_sec_size;			/* NAND_SECTOR_SIZE */
	unsigned int nand_sec_shift;		/* NAND_SECTOR_SHIFT */
	unsigned int nand_ecc_size;
	unsigned int nand_ecc_bytes;
	unsigned int nand_ecc_mode;
};
extern struct mt6516_nand_host_hw mt6516_nand_hw;

/*gsensor driver*/
#define GSENSOR_DATA_NUM (3)
struct gsensor_hardware {
    int i2c_num;
    int direction;
    s16 offset[GSENSOR_DATA_NUM+1]; /*+1: for alignment*/
    // TODO: add external interrupt
};


/*msensor driver*/
struct msensor_hardware {
    int i2c_num;
    int direction;
};

#endif /* __ARCH_ARM_MACH_BOARD_H */

