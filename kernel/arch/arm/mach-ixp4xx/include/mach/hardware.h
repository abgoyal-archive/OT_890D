


#ifndef __ASM_ARCH_HARDWARE_H__
#define __ASM_ARCH_HARDWARE_H__

#define PCIBIOS_MIN_IO		0x00001000
#define PCIBIOS_MIN_MEM		(cpu_is_ixp43x() ? 0x40000000 : 0x48000000)

#define	HAVE_ARCH_PCI_SET_DMA_MASK

#define pcibios_assign_all_busses()	1

/* Register locations and bits */
#include "ixp4xx-regs.h"

#ifndef __ASSEMBLER__
#include <mach/cpu.h>
#endif

/* Platform helper functions and definitions */
#include "platform.h"

/* Platform specific details */
#include "ixdp425.h"
#include "avila.h"
#include "coyote.h"
#include "prpmc1100.h"
#include "nslu2.h"
#include "nas100d.h"
#include "dsmg600.h"
#include "fsg.h"

#endif  /* _ASM_ARCH_HARDWARE_H */
