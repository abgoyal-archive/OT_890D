

#ifndef _MACH_BLACKFIN_H_
#define _MACH_BLACKFIN_H_

#define BF538_FAMILY

#include "bf538.h"
#include "mem_map.h"
#include "defBF539.h"
#include "anomaly.h"


#if !defined(__ASSEMBLY__)
#include "cdefBF538.h"

#if defined(CONFIG_BF539)
#include "cdefBF539.h"
#endif
#endif

/* UART_IIR Register */
#define STATUS(x)	((x << 1) & 0x06)
#define STATUS_P1	0x02
#define STATUS_P0	0x01

#define BFIN_UART_NR_PORTS	3

#define OFFSET_THR              0x00	/* Transmit Holding register            */
#define OFFSET_RBR              0x00	/* Receive Buffer register              */
#define OFFSET_DLL              0x00	/* Divisor Latch (Low-Byte)             */
#define OFFSET_IER              0x04	/* Interrupt Enable Register            */
#define OFFSET_DLH              0x04	/* Divisor Latch (High-Byte)            */
#define OFFSET_IIR              0x08	/* Interrupt Identification Register    */
#define OFFSET_LCR              0x0C	/* Line Control Register                */
#define OFFSET_MCR              0x10	/* Modem Control Register               */
#define OFFSET_LSR              0x14	/* Line Status Register                 */
#define OFFSET_MSR              0x18	/* Modem Status Register                */
#define OFFSET_SCR              0x1C	/* SCR Scratch Register                 */
#define OFFSET_GCTL             0x24	/* Global Control Register              */


#define bfin_write_MDMA_D0_IRQ_STATUS	bfin_write_MDMA0_D0_IRQ_STATUS
#define bfin_write_MDMA_D0_START_ADDR   bfin_write_MDMA0_D0_START_ADDR
#define bfin_write_MDMA_S0_START_ADDR   bfin_write_MDMA0_S0_START_ADDR
#define bfin_write_MDMA_D0_X_COUNT      bfin_write_MDMA0_D0_X_COUNT
#define bfin_write_MDMA_S0_X_COUNT      bfin_write_MDMA0_S0_X_COUNT
#define bfin_write_MDMA_D0_Y_COUNT      bfin_write_MDMA0_D0_Y_COUNT
#define bfin_write_MDMA_S0_Y_COUNT      bfin_write_MDMA0_S0_Y_COUNT
#define bfin_write_MDMA_D0_X_MODIFY     bfin_write_MDMA0_D0_X_MODIFY
#define bfin_write_MDMA_S0_X_MODIFY     bfin_write_MDMA0_S0_X_MODIFY
#define bfin_write_MDMA_D0_Y_MODIFY     bfin_write_MDMA0_D0_Y_MODIFY
#define bfin_write_MDMA_S0_Y_MODIFY     bfin_write_MDMA0_S0_Y_MODIFY
#define bfin_write_MDMA_S0_CONFIG       bfin_write_MDMA0_S0_CONFIG
#define bfin_write_MDMA_D0_CONFIG       bfin_write_MDMA0_D0_CONFIG
#define bfin_read_MDMA_S0_CONFIG        bfin_read_MDMA0_S0_CONFIG
#define bfin_read_MDMA_D0_IRQ_STATUS    bfin_read_MDMA0_D0_IRQ_STATUS
#define bfin_write_MDMA_S0_IRQ_STATUS   bfin_write_MDMA0_S0_IRQ_STATUS


/* DPMC*/
#define bfin_read_STOPCK_OFF() bfin_read_STOPCK()
#define bfin_write_STOPCK_OFF(val) bfin_write_STOPCK(val)
#define STOPCK_OFF STOPCK

/* PLL_DIV Masks													*/
#define CCLK_DIV1 CSEL_DIV1	/*          CCLK = VCO / 1                                  */
#define CCLK_DIV2 CSEL_DIV2	/*          CCLK = VCO / 2                                  */
#define CCLK_DIV4 CSEL_DIV4	/*          CCLK = VCO / 4                                  */
#define CCLK_DIV8 CSEL_DIV8	/*          CCLK = VCO / 8                                  */

#endif
