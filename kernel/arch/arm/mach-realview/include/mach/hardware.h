
#ifndef __ASM_ARCH_HARDWARE_H
#define __ASM_ARCH_HARDWARE_H

#include <asm/sizes.h>

/* macro to get at IO space when running virtually */
#define IO_ADDRESS(x)		(((x) & 0x03ffffff) + 0xfb000000)
#define __io_address(n)		__io(IO_ADDRESS(n))

#endif
