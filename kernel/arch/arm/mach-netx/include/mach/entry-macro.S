
#include <mach/hardware.h>

		.macro  disable_fiq
		.endm

		.macro  get_irqnr_preamble, base, tmp
		.endm

		.macro  arch_ret_to_user, tmp1, tmp2
		.endm

		.macro  get_irqnr_and_base, irqnr, irqstat, base, tmp
		mov	\base, #io_p2v(0x00100000)
		add	\base, \base, #0x000ff000

		ldr	\irqstat, [\base, #0]
		clz	\irqnr, \irqstat
		rsb     \irqnr, \irqnr, #31
		cmp	\irqstat, #0
		.endm

