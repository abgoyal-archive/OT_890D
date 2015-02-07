

#ifndef _ASM_IDE_H
#define _ASM_IDE_H

#ifdef __KERNEL__

#include <asm/intctl-regs.h>

#undef SUPPORT_SLOW_DATA_PORTS
#define SUPPORT_SLOW_DATA_PORTS 0

#undef SUPPORT_VLB_SYNC
#define SUPPORT_VLB_SYNC 0

#define __ide_mm_insw(port, addr, n) \
	insw((unsigned long) (port), (addr), (n))
#define __ide_mm_insl(port, addr, n) \
	insl((unsigned long) (port), (addr), (n))
#define __ide_mm_outsw(port, addr, n) \
	outsw((unsigned long) (port), (addr), (n))
#define __ide_mm_outsl(port, addr, n) \
	outsl((unsigned long) (port), (addr), (n))

#endif /* __KERNEL__ */
#endif /* _ASM_IDE_H */
