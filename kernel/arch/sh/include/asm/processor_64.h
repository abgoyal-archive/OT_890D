
#ifndef __ASM_SH_PROCESSOR_64_H
#define __ASM_SH_PROCESSOR_64_H

#ifndef __ASSEMBLY__

#include <linux/compiler.h>
#include <asm/page.h>
#include <asm/types.h>
#include <asm/ptrace.h>
#include <cpu/registers.h>

#define current_text_addr() ({ \
void *pc; \
unsigned long long __dummy = 0; \
__asm__("gettr	tr0, %1\n\t" \
	"pta	4, tr0\n\t" \
	"gettr	tr0, %0\n\t" \
	"ptabs	%1, tr0\n\t"	\
	:"=r" (pc), "=r" (__dummy) \
	: "1" (__dummy)); \
pc; })

#endif

#define TASK_SIZE	0x7ffff000UL

#define STACK_TOP	TASK_SIZE
#define STACK_TOP_MAX	STACK_TOP

#define TASK_UNMAPPED_BASE	(TASK_SIZE / 3)

#if defined(CONFIG_SH64_SR_WATCH)
#define SR_MMU   0x84000000
#else
#define SR_MMU   0x80000000
#endif

#define SR_IMASK 0x000000f0
#define SR_FD    0x00008000
#define SR_SSTEP 0x08000000

#ifndef __ASSEMBLY__


struct sh_fpu_hard_struct {
	unsigned long fp_regs[64];
	unsigned int fpscr;
	/* long status; * software status information */
};

#if 0
/* Dummy fpu emulator  */
struct sh_fpu_soft_struct {
	unsigned long long fp_regs[32];
	unsigned int fpscr;
	unsigned char lookahead;
	unsigned long entry_pc;
};
#endif

union sh_fpu_union {
	struct sh_fpu_hard_struct hard;
	/* 'hard' itself only produces 32 bit alignment, yet we need
	   to access it using 64 bit load/store as well. */
	unsigned long long alignment_dummy;
};

struct thread_struct {
	unsigned long sp;
	unsigned long pc;
	/* This stores the address of the pt_regs built during a context
	   switch, or of the register save area built for a kernel mode
	   exception.  It is used for backtracing the stack of a sleeping task
	   or one that traps in kernel mode. */
        struct pt_regs *kregs;
	/* This stores the address of the pt_regs constructed on entry from
	   user mode.  It is a fixed value over the lifetime of a process, or
	   NULL for a kernel thread. */
	struct pt_regs *uregs;

	unsigned long trap_no, error_code;
	unsigned long address;
	/* Hardware debugging registers may come here */

	/* floating point info */
	union sh_fpu_union fpu;
};

#define INIT_MMAP \
{ &init_mm, 0, 0, NULL, PAGE_SHARED, VM_READ | VM_WRITE | VM_EXEC, 1, NULL, NULL }

#define INIT_THREAD  {				\
	.sp		= sizeof(init_stack) +	\
			  (long) &init_stack,	\
	.pc		= 0,			\
        .kregs		= &fake_swapper_regs,	\
	.uregs	        = NULL,			\
	.trap_no	= 0,			\
	.error_code	= 0,			\
	.address	= 0,			\
	.fpu		= { { { 0, } }, }	\
}

#define SR_USER (SR_MMU | SR_FD)

#define start_thread(regs, new_pc, new_sp)			\
	set_fs(USER_DS);					\
	regs->sr = SR_USER;	/* User mode. */		\
	regs->pc = new_pc - 4;	/* Compensate syscall exit */	\
	regs->pc |= 1;		/* Set SHmedia ! */		\
	regs->regs[18] = 0;					\
	regs->regs[15] = new_sp

/* Forward declaration, a strange C thing */
struct task_struct;
struct mm_struct;

/* Free all resources held by a thread. */
extern void release_thread(struct task_struct *);
extern int kernel_thread(int (*fn)(void *), void * arg, unsigned long flags);


/* Copy and release all segment info associated with a VM */
#define copy_segments(p, mm)	do { } while (0)
#define release_segments(mm)	do { } while (0)
#define forget_segments()	do { } while (0)
#define prepare_to_copy(tsk)	do { } while (0)

static inline void disable_fpu(void)
{
	unsigned long long __dummy;

	/* Set FD flag in SR */
	__asm__ __volatile__("getcon	" __SR ", %0\n\t"
			     "or	%0, %1, %0\n\t"
			     "putcon	%0, " __SR "\n\t"
			     : "=&r" (__dummy)
			     : "r" (SR_FD));
}

static inline void enable_fpu(void)
{
	unsigned long long __dummy;

	/* Clear out FD flag in SR */
	__asm__ __volatile__("getcon	" __SR ", %0\n\t"
			     "and	%0, %1, %0\n\t"
			     "putcon	%0, " __SR "\n\t"
			     : "=&r" (__dummy)
			     : "r" (~SR_FD));
}

#if defined(CONFIG_SH64_FPU_DENORM_FLUSH)
#define FPSCR_INIT  0x00040000
#else
#define FPSCR_INIT  0x00000000
#endif

#ifdef CONFIG_SH_FPU
/* Initialise the FP state of a task */
void fpinit(struct sh_fpu_hard_struct *fpregs);
#else
#define fpinit(fpregs)	do { } while (0)
#endif

extern struct task_struct *last_task_used_math;

#define thread_saved_pc(tsk)	(tsk->thread.pc)

extern unsigned long get_wchan(struct task_struct *p);

#define KSTK_EIP(tsk)  ((tsk)->thread.pc)
#define KSTK_ESP(tsk)  ((tsk)->thread.sp)

#define user_stack_pointer(regs)	((regs)->regs[15])

#endif	/* __ASSEMBLY__ */
#endif /* __ASM_SH_PROCESSOR_64_H */