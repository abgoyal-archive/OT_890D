

#ifndef _MACH_BLACKFIN_H_
#define _MACH_BLACKFIN_H_

#define BF527_FAMILY

#include "bf527.h"
#include "mem_map.h"
#include "defBF522.h"
#include "anomaly.h"

#if defined(CONFIG_BF527) || defined(CONFIG_BF526)
#include "defBF527.h"
#endif

#if defined(CONFIG_BF525) || defined(CONFIG_BF524)
#include "defBF525.h"
#endif

#if !defined(__ASSEMBLY__)
#include "cdefBF522.h"

#if defined(CONFIG_BF527) || defined(CONFIG_BF526)
#include "cdefBF527.h"
#endif

#if defined(CONFIG_BF525) || defined(CONFIG_BF524)
#include "cdefBF525.h"
#endif
#endif

/* UART_IIR Register */
#define STATUS(x)	((x << 1) & 0x06)
#define STATUS_P1	0x02
#define STATUS_P0	0x01

#define BFIN_UART_NR_PORTS	2

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
