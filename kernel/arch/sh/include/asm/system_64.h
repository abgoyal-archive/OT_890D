
#ifndef __ASM_SH_SYSTEM_64_H
#define __ASM_SH_SYSTEM_64_H

#include <asm/processor.h>

struct task_struct *sh64_switch_to(struct task_struct *prev,
				   struct thread_struct *prev_thread,
				   struct task_struct *next,
				   struct thread_struct *next_thread);

#define switch_to(prev,next,last)				\
do {								\
	if (last_task_used_math != next) {			\
		struct pt_regs *regs = next->thread.uregs;	\
		if (regs) regs->sr |= SR_FD;			\
	}							\
	last = sh64_switch_to(prev, &prev->thread, next,	\
			      &next->thread);			\
} while (0)

#define __uses_jump_to_uncached

#define jump_to_uncached()	do { } while (0)
#define back_to_cached()	do { } while (0)

#endif /* __ASM_SH_SYSTEM_64_H */
