
#include <linux/linkage.h>
#include <asm/errno.h>

ENTRY(__put_user_1)
1:	strbt	r2, [r0]
	mov	r0, #0
	mov	pc, lr
ENDPROC(__put_user_1)

ENTRY(__put_user_2)
	mov	ip, r2, lsr #8
#ifndef __ARMEB__
2:	strbt	r2, [r0], #1
3:	strbt	ip, [r0]
#else
2:	strbt	ip, [r0], #1
3:	strbt	r2, [r0]
#endif
	mov	r0, #0
	mov	pc, lr
ENDPROC(__put_user_2)

ENTRY(__put_user_4)
4:	strt	r2, [r0]
	mov	r0, #0
	mov	pc, lr
ENDPROC(__put_user_4)

ENTRY(__put_user_8)
5:	strt	r2, [r0], #4
6:	strt	r3, [r0]
	mov	r0, #0
	mov	pc, lr
ENDPROC(__put_user_8)

__put_user_bad:
	mov	r0, #-EFAULT
	mov	pc, lr
ENDPROC(__put_user_bad)

.section __ex_table, "a"
	.long	1b, __put_user_bad
	.long	2b, __put_user_bad
	.long	3b, __put_user_bad
	.long	4b, __put_user_bad
	.long	5b, __put_user_bad
	.long	6b, __put_user_bad
.previous
