

#include <mach/hardware.h>
#include <mach/msm_iomap.h>

	.macro	addruart,rx
	@ see if the MMU is enabled and select appropriate base address
	mrc	p15, 0, \rx, c1, c0
	tst	\rx, #1
	ldreq	\rx, =MSM_UART1_PHYS
	movne	\rx, #0
	.endm

	.macro	senduart,rd,rx
	teq	\rx, #0
	strne	\rd, [\rx, #0x0C]
	.endm

	.macro	waituart,rd,rx
	@ wait for TX_READY
	teq	\rx, #0
	bne	2f
1:	ldr	\rd, [\rx, #0x08]
	tst	\rd, #0x04
	beq	1b
2:
	.endm

	.macro	busyuart,rd,rx
	.endm
