
 /* linux/include/asm-arm/arch-mt3351/board.h
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
#include <mach/mt3351.h>

#if   defined(CONFIG_MT3351_EVK_BOARD)
#include <mach/board-43evk.h>
#elif defined(CONFIG_MT3351_EVZ_BOARD)
#include <mach/board-evz.h>
#elif defined(CONFIG_MT3351_EVB_BOARD)
#include <mach/board-evb.h>
#else
#error "Unknown board type"
#endif

typedef void (*sdio_irq_handler_t)(void*);  /* external irq handler */
typedef void (*pm_callback_t)(pm_message_t state, void *data);

#define MSDC_CD_PIN_EN      (1 << 0)
#define MSDC_WP_PIN_EN      (1 << 1)
#define MSDC_EXT_SDIO_IRQ   (1 << 2)
#define MSDC_REMOVABLE      (1 << 3)

/* configure the output driving capacity and slew rate */
#define MSDC_ODC_4MA                    (0x0)
#define MSDC_ODC_8MA                    (0x2)
#define MSDC_ODC_12MA                   (0x4)
#define MSDC_ODC_16MA                   (0x6)
#define MSDC_ODC_SLEW_FAST              (0)
#define MSDC_ODC_SLEW_SLOW              (1)

struct mt3351_sd_host_hw {
    unsigned short cmd_odc;          /* output driving capability */
    unsigned short data_odc;         /* output driving capability */
    unsigned short cmd_slew_rate;
    unsigned short data_slew_rate;
    unsigned long  flags;            /* hardware capability flags */
    unsigned long  data_pins;        /* data pins */
    unsigned long  data_offset;      /* data address offset */
    void (*ext_power_on)(void);      /* external power on */
    void (*ext_power_off)(void);     /* external power off */
    void (*request_sdio_eirq)(sdio_irq_handler_t sdio_irq_handler, void *data);
    void (*enable_sdio_eirq)(void);
    void (*disable_sdio_eirq)(void);
    void (*register_pm)(pm_callback_t pm_cb, void *data);
};

extern struct mt3351_sd_host_hw mt3351_sd1_hw;
extern struct mt3351_sd_host_hw mt3351_sd2_hw;
extern struct mt3351_sd_host_hw mt3351_sd3_hw;

#endif /* __ARCH_ARM_MACH_BOARD_H */

