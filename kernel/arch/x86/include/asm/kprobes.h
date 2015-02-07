
#ifndef _ASM_X86_KPROBES_H
#define _ASM_X86_KPROBES_H
#include <linux/types.h>
#include <linux/ptrace.h>
#include <linux/percpu.h>

#define  __ARCH_WANT_KPROBES_INSN_SLOT

struct pt_regs;
struct kprobe;

typedef u8 kprobe_opcode_t;
#define BREAKPOINT_INSTRUCTION	0xcc
#define RELATIVEJUMP_INSTRUCTION 0xe9
#define MAX_INSN_SIZE 16
#define MAX_STACK_SIZE 64
#define MIN_STACK_SIZE(ADDR)					       \
	(((MAX_STACK_SIZE) < (((unsigned long)current_thread_info()) + \
			      THREAD_SIZE - (unsigned long)(ADDR)))    \
	 ? (MAX_STACK_SIZE)					       \
	 : (((unsigned long)current_thread_info()) +		       \
	    THREAD_SIZE - (unsigned long)(ADDR)))

#define flush_insn_slot(p)	do { } while (0)

extern const int kretprobe_blacklist_size;

void arch_remove_kprobe(struct kprobe *p);
void kretprobe_trampoline(void);

/* Architecture specific copy of original instruction*/
struct arch_specific_insn {
	/* copy of the original instruction */
	kprobe_opcode_t *insn;
	/*
	 * boostable = -1: This instruction type is not boostable.
	 * boostable = 0: This instruction type is boostable.
	 * boostable = 1: This instruction has been boosted: we have
	 * added a relative jump after the instruction copy in insn,
	 * so no single-step and fixup are needed (unless there's
	 * a post_handler or break_handler).
	 */
	int boostable;
};

struct prev_kprobe {
	struct kprobe *kp;
	unsigned long status;
	unsigned long old_flags;
	unsigned long saved_flags;
};

/* per-cpu kprobe control block */
struct kprobe_ctlblk {
	unsigned long kprobe_status;
	unsigned long kprobe_old_flags;
	unsigned long kprobe_saved_flags;
	unsigned long *jprobe_saved_sp;
	struct pt_regs jprobe_saved_regs;
	kprobe_opcode_t jprobes_stack[MAX_STACK_SIZE];
	struct prev_kprobe prev_kprobe;
};

extern int kprobe_fault_handler(struct pt_regs *regs, int trapnr);
extern int kprobe_exceptions_notify(struct notifier_block *self,
				    unsigned long val, void *data);
#endif /* _ASM_X86_KPROBES_H */
