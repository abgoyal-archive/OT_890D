
#include <asm/ftrace.h>

#define MCOUNT_ENTER()		\
	mov.l	r4, @-r15;	\
	mov.l	r5, @-r15;	\
	mov.l	r6, @-r15;	\
	mov.l	r7, @-r15;	\
	sts.l	pr, @-r15;	\
				\
	mov.l	@(20,r15),r4;	\
	sts	pr, r5

#define MCOUNT_LEAVE()		\
	lds.l	@r15+, pr;	\
	mov.l	@r15+, r7;	\
	mov.l	@r15+, r6;	\
	mov.l	@r15+, r5;	\
	rts;			\
	 mov.l	@r15+, r4

	.align 2
	.globl	_mcount
	.type	_mcount,@function
	.globl	mcount
	.type	mcount,@function
_mcount:
mcount:
	MCOUNT_ENTER()

#ifdef CONFIG_DYNAMIC_FTRACE
	.globl	mcount_call
mcount_call:
	mov.l	.Lftrace_stub, r6
#else
	mov.l	.Lftrace_trace_function, r6
	mov.l	ftrace_stub, r7
	cmp/eq	r6, r7
	bt	skip_trace
	mov.l	@r6, r6
#endif

	jsr	@r6
	 nop

skip_trace:
	MCOUNT_LEAVE()

	.align 2
.Lftrace_trace_function:
	.long	ftrace_trace_function

#ifdef CONFIG_DYNAMIC_FTRACE
	.globl ftrace_caller
ftrace_caller:
	MCOUNT_ENTER()

	.globl ftrace_call
ftrace_call:
	mov.l	.Lftrace_stub, r6
	jsr	@r6
	 nop

	MCOUNT_LEAVE()
#endif /* CONFIG_DYNAMIC_FTRACE */

	.align 2
.Lftrace_stub:
	.long	ftrace_stub

	.globl	ftrace_stub
ftrace_stub:
	rts
	 nop
