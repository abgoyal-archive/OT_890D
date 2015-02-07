

#ifndef __ARCH_ARM_MACH_MT3351_BOARD_43EVK_H
#define __ARCH_ARM_MACH_MT3351_BOARD_43EVK_H

#include <linux/autoconf.h>

/*=======================================================================*/
/* MT3351 SD                                                             */
/*=======================================================================*/
#define CFG_DEV_MSDC1
#define CFG_DEV_MSDC3

#define CFG_MSDC1_DATA_OFFSET   (0xb00000)
#define CFG_MSDC3_DATA_OFFSET   (0)

#define CFG_MSDC1_DATA_PINS     (4)
#define CFG_MSDC3_DATA_PINS     (4)

#define CFG_MSDC1_CARD_DETECT   (0)
#define CFG_MSDC3_CARD_DETECT   (1)

/*=======================================================================*/
/* MT3351 UART                                                           */
/*=======================================================================*/
#define CFG_DEV_UART1
#define CFG_DEV_UART2
#define CFG_DEV_UART3

#define CFG_UART_PORTS          (3)

/*=======================================================================*/
/* MT3351 I2C                                                            */
/*=======================================================================*/
#define CFG_DEV_I2C
//#define CFG_I2C_HIGH_SPEED_MODE
//#define CFG_I2C_DMA_MODE

#endif /* __ARCH_ARM_MACH_MT3351_BOARD_43EVK_H */

