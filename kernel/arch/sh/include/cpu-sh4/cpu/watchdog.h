
#ifndef __ASM_CPU_SH4_WATCHDOG_H
#define __ASM_CPU_SH4_WATCHDOG_H

/* Register definitions */
#define WTCNT		0xffc00008
#define WTCSR		0xffc0000c

/* Bit definitions */
#define WTCSR_TME	0x80
#define WTCSR_WT	0x40
#define WTCSR_RSTS	0x20
#define WTCSR_WOVF	0x10
#define WTCSR_IOVF	0x08

#endif /* __ASM_CPU_SH4_WATCHDOG_H */

