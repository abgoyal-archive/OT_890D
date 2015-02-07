

#include <asm/asmmacro.h>
#include <asm/asm-offsets.h>
#include "entry.h"

#define DATA8(sym, init_value)			\
	.pushsection .data.read_mostly ;	\
	.align 8 ;				\
	.global sym ;				\
	sym: ;					\
	data8 init_value ;			\
	.popsection

#define BRANCH(targ, reg, breg)		\
	movl reg=targ ;			\
	;;				\
	ld8 reg=[reg] ;			\
	;;				\
	mov breg=reg ;			\
	br.cond.sptk.many breg

#define BRANCH_PROC(sym, reg, breg)				\
	DATA8(paravirt_ ## sym ## _targ, ia64_native_ ## sym) ; \
	GLOBAL_ENTRY(paravirt_ ## sym) ;			\
		BRANCH(paravirt_ ## sym ## _targ, reg, breg) ;	\
	END(paravirt_ ## sym)

#define BRANCH_PROC_UNWINFO(sym, reg, breg)			\
	DATA8(paravirt_ ## sym ## _targ, ia64_native_ ## sym) ; \
	GLOBAL_ENTRY(paravirt_ ## sym) ;			\
		PT_REGS_UNWIND_INFO(0) ;			\
		BRANCH(paravirt_ ## sym ## _targ, reg, breg) ;	\
	END(paravirt_ ## sym)


BRANCH_PROC(switch_to, r22, b7)
BRANCH_PROC_UNWINFO(leave_syscall, r22, b7)
BRANCH_PROC(work_processed_syscall, r2, b7)
BRANCH_PROC_UNWINFO(leave_kernel, r22, b7)
