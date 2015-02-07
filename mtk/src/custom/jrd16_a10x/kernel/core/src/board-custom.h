

#ifndef __ARCH_ARM_MACH_MT6516_BOARD_E1K_H
#define __ARCH_ARM_MACH_MT6516_BOARD_E1K_H

#include <linux/autoconf.h>

/*=======================================================================*/
/* MT6516 SD                                                             */
/*=======================================================================*/
#define CFG_DEV_MSDC1
#define CFG_DEV_MSDC2

#define CFG_MSDC1_DATA_OFFSET   (0)
#define CFG_MSDC2_DATA_OFFSET   (0)

#define CFG_MSDC1_DATA_PINS     (4)
#define CFG_MSDC2_DATA_PINS     (4)

/*=======================================================================*/
/* MT6516 UART                                                           */
/*=======================================================================*/
#define CFG_DEV_UART1
#define CFG_DEV_UART2
#define CFG_DEV_UART3
#define CFG_DEV_UART4

#define CFG_UART_PORTS          (4)

/*=======================================================================*/
/* MT6516 I2C                                                            */
/*=======================================================================*/
#define CFG_DEV_I2C
//#define CFG_I2C_HIGH_SPEED_MODE
//#define CFG_I2C_DMA_MODE

/*=======================================================================*/
/* MT6516 ADB                                                            */
/*=======================================================================*/
#define ADB_SERIAL "0123456789ABCDEF"

/*=======================================================================*/
/* MT6516 NAND FLASH                                                     */
/*=======================================================================*/
#define USE_AHB_MODE 1

#define NFI_ACCESS_TIMING           0x01123
//0x20022222//0x10123222//0x20022222//(0x10123222) 
// flashtool use 0x700fffff
#define NFI_BUS_WIDTH               (16)

#define NFI_CS_NUM                  (2)
#define NAND_ID_NUM                 (8)
#define NAND_SECTOR_SIZE            (512)
#define NAND_SECTOR_SHIFT           (9)

#define ECC_SIZE					(2048)
#define ECC_BYTES					(32)

#define NAND_ECC_MODE               NAND_ECC_HW

#endif /* __ARCH_ARM_MACH_MT6516_BOARD_E1K_H */

