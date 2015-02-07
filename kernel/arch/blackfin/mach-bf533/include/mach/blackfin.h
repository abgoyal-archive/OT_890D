

#ifndef _MACH_BLACKFIN_H_
#define _MACH_BLACKFIN_H_

#define BF533_FAMILY

#include "bf533.h"
#include "mem_map.h"
#include "defBF532.h"
#include "anomaly.h"

#if !defined(__ASSEMBLY__)
#include "cdefBF532.h"
#endif

#define BFIN_UART_NR_PORTS      1

#define CH_UART_RX CH_UART0_RX
#define CH_UART_TX CH_UART0_TX

#define IRQ_UART_ERROR IRQ_UART0_ERROR
#define IRQ_UART_RX    IRQ_UART0_RX
#define IRQ_UART_TX    IRQ_UART0_TX

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

#endif				/* _MACH_BLACKFIN_H_ */
