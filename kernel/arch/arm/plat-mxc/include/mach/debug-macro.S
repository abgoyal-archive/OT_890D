

#include <mach/hardware.h>

#ifdef CONFIG_MACH_MX31ADS
#include <mach/board-mx31ads.h>
#endif
#ifdef CONFIG_MACH_PCM037
#include <mach/board-pcm037.h>
#endif
#ifdef CONFIG_MACH_MX31LITE
#include <mach/board-mx31lite.h>
#endif
#ifdef CONFIG_MACH_MX27ADS
#include <mach/board-mx27ads.h>
#endif
#ifdef CONFIG_MACH_PCM038
#include <mach/board-pcm038.h>
#endif
#ifdef CONFIG_MACH_MX31_3DS
#include <mach/board-mx31pdk.h>
#endif
		.macro	addruart,rx
		mrc	p15, 0, \rx, c1, c0
		tst	\rx, #1			@ MMU enabled?
		ldreq	\rx, =MXC_LL_UART_PADDR	@ physical
		ldrne	\rx, =MXC_LL_UART_VADDR	@ virtual
		.endm

		.macro	senduart,rd,rx
		str	\rd, [\rx, #0x40]	@ TXDATA
		.endm

		.macro	waituart,rd,rx
		.endm

		.macro	busyuart,rd,rx
1002:		ldr	\rd, [\rx, #0x98]	@ SR2
		tst	\rd, #1 << 3		@ TXDC
		beq	1002b			@ wait until transmit done
		.endm
