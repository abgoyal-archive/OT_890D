

#ifndef __ASM_M68K_PROCESSOR_H
#define __ASM_M68K_PROCESSOR_H

#define current_text_addr() ({ __label__ _l; _l: &&_l;})

#include <linux/compiler.h>
#include <linux/threads.h>
#include <asm/types.h>
#include <asm/segment.h>
#include <asm/fpu.h>
#include <asm/ptrace.h>
#include <asm/current.h>

static inline unsigned long rdusp(void)
{
#ifdef CONFIG_COLDFIRE
	extern unsigned int sw_usp;
	return(sw_usp);
#else
  	unsigned long usp;
	__asm__ __volatile__("move %/usp,%0" : "=a" (usp));
	return usp;
#endif
}

static inline void wrusp(unsigned long usp)
{
#ifdef CONFIG_COLDFIRE
	extern unsigned int sw_usp;
	sw_usp = usp;
#else
	__asm__ __volatile__("move %0,%/usp" : : "a" (usp));
#endif
}

#define TASK_SIZE	(0xF0000000UL)

#define TASK_UNMAPPED_BASE	0

   
struct thread_struct {
	unsigned long  ksp;		/* kernel stack pointer */
	unsigned long  usp;		/* user stack pointer */
	unsigned short sr;		/* saved status register */
	unsigned short fs;		/* saved fs (sfc, dfc) */
	unsigned long  crp[2];		/* cpu root pointer */
	unsigned long  esp0;		/* points to SR of stack frame */
	unsigned long  fp[8*3];
	unsigned long  fpcntl[3];	/* fp control regs */
	unsigned char  fpstate[FPSTATESIZE];  /* floating point state */
};

#define INIT_THREAD  { \
	sizeof(init_stack) + (unsigned long) init_stack, 0, \
	PS_S, __KERNEL_DS, \
	{0, 0}, 0, {0,}, {0, 0, 0}, {0,}, \
}

#if defined(CONFIG_COLDFIRE)
#define	reformat(_regs)		do { (_regs)->format = 0x4; } while(0)
#else
#define	reformat(_regs)		do { } while (0)
#endif

#define start_thread(_regs, _pc, _usp)			\
do {							\
	set_fs(USER_DS); /* reads from user space */	\
	(_regs)->pc = (_pc);				\
	((struct switch_stack *)(_regs))[-1].a6 = 0;	\
	reformat(_regs);				\
	if (current->mm)				\
		(_regs)->d5 = current->mm->start_data;	\
	(_regs)->sr &= ~0x2000;				\
	wrusp(_usp);					\
} while(0)

/* Forward declaration, a strange C thing */
struct task_struct;

/* Free all resources held by a thread. */
static inline void release_thread(struct task_struct *dead_task)
{
}

/* Prepare to copy thread state - unlazy all lazy status */
#define prepare_to_copy(tsk)	do { } while (0)

extern int kernel_thread(int (*fn)(void *), void * arg, unsigned long flags);

static inline void exit_thread(void)
{
}

unsigned long thread_saved_pc(struct task_struct *tsk);
unsigned long get_wchan(struct task_struct *p);

#define	KSTK_EIP(tsk)	\
    ({			\
	unsigned long eip = 0;	 \
	if ((tsk)->thread.esp0 > PAGE_SIZE && \
	    (virt_addr_valid((tsk)->thread.esp0))) \
	      eip = ((struct pt_regs *) (tsk)->thread.esp0)->pc; \
	eip; })
#define	KSTK_ESP(tsk)	((tsk) == current ? rdusp() : (tsk)->thread.usp)

#define cpu_relax()    barrier()

#endif
