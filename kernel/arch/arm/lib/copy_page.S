
#include <linux/linkage.h>
#include <asm/assembler.h>
#include <asm/asm-offsets.h>

#define COPY_COUNT (PAGE_SZ/64 PLD( -1 ))

		.text
		.align	5
ENTRY(copy_page)
		stmfd	sp!, {r4, lr}			@	2
	PLD(	pld	[r1, #0]		)
	PLD(	pld	[r1, #32]		)
		mov	r2, #COPY_COUNT			@	1
		ldmia	r1!, {r3, r4, ip, lr}		@	4+1
1:	PLD(	pld	[r1, #64]		)
	PLD(	pld	[r1, #96]		)
2:		stmia	r0!, {r3, r4, ip, lr}		@	4
		ldmia	r1!, {r3, r4, ip, lr}		@	4+1
		stmia	r0!, {r3, r4, ip, lr}		@	4
		ldmia	r1!, {r3, r4, ip, lr}		@	4+1
		stmia	r0!, {r3, r4, ip, lr}		@	4
		ldmia	r1!, {r3, r4, ip, lr}		@	4
		subs	r2, r2, #1			@	1
		stmia	r0!, {r3, r4, ip, lr}		@	4
		ldmgtia	r1!, {r3, r4, ip, lr}		@	4
		bgt	1b				@	1
	PLD(	ldmeqia r1!, {r3, r4, ip, lr}	)
	PLD(	beq	2b			)
		ldmfd	sp!, {r4, pc}			@	3
ENDPROC(copy_page)
