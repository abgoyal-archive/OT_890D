

#ifdef __KERNEL__
#ifndef __MACH_ADS8260_DEFS
#define __MACH_ADS8260_DEFS

#include <linux/seq_file.h>

/* Backword-compatibility stuff for the drivers */
#define CPM_MAP_ADDR		((uint)0xf0000000)
#define CPM_IRQ_OFFSET 0


#define BCSR0_LED0		((uint)0x02000000)      /* 0 == on */
#define BCSR0_LED1		((uint)0x01000000)      /* 0 == on */
#define BCSR1_FETHIEN		((uint)0x08000000)      /* 0 == enable*/
#define BCSR1_FETH_RST		((uint)0x04000000)      /* 0 == reset */
#define BCSR1_RS232_EN1		((uint)0x02000000)      /* 0 ==enable */
#define BCSR1_RS232_EN2		((uint)0x01000000)      /* 0 ==enable */
#define BCSR3_FETHIEN2		((uint)0x10000000)      /* 0 == enable*/
#define BCSR3_FETH2_RST		((uint)0x80000000)      /* 0 == reset */

/* cpm serial driver works with constants below */

#define SIU_INT_SMC1		((uint)0x04+CPM_IRQ_OFFSET)
#define SIU_INT_SMC2		((uint)0x05+CPM_IRQ_OFFSET)
#define SIU_INT_SCC1		((uint)0x28+CPM_IRQ_OFFSET)
#define SIU_INT_SCC2		((uint)0x29+CPM_IRQ_OFFSET)
#define SIU_INT_SCC3		((uint)0x2a+CPM_IRQ_OFFSET)
#define SIU_INT_SCC4		((uint)0x2b+CPM_IRQ_OFFSET)

#endif /* __MACH_ADS8260_DEFS */
#endif /* __KERNEL__ */
