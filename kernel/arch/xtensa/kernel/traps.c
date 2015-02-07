

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/stringify.h>
#include <linux/kallsyms.h>
#include <linux/delay.h>
#include <linux/hardirq.h>

#include <asm/ptrace.h>
#include <asm/timex.h>
#include <asm/uaccess.h>
#include <asm/pgtable.h>
#include <asm/processor.h>

#ifdef CONFIG_KGDB
extern int gdb_enter;
extern int return_from_debug_flag;
#endif


extern void kernel_exception(void);
extern void user_exception(void);

extern void fast_syscall_kernel(void);
extern void fast_syscall_user(void);
extern void fast_alloca(void);
extern void fast_unaligned(void);
extern void fast_second_level_miss(void);
extern void fast_store_prohibited(void);
extern void fast_coprocessor(void);

extern void do_illegal_instruction (struct pt_regs*);
extern void do_interrupt (struct pt_regs*);
extern void do_unaligned_user (struct pt_regs*);
extern void do_multihit (struct pt_regs*, unsigned long);
extern void do_page_fault (struct pt_regs*, unsigned long);
extern void do_debug (struct pt_regs*);
extern void system_call (struct pt_regs*);


#define KRNL		0x01
#define USER		0x02

#define COPROCESSOR(x)							\
{ EXCCAUSE_COPROCESSOR ## x ## _DISABLED, USER, fast_coprocessor }

typedef struct {
	int cause;
	int fast;
	void* handler;
} dispatch_init_table_t;

static dispatch_init_table_t __initdata dispatch_init_table[] = {

{ EXCCAUSE_ILLEGAL_INSTRUCTION,	0,	   do_illegal_instruction},
{ EXCCAUSE_SYSTEM_CALL,		KRNL,	   fast_syscall_kernel },
{ EXCCAUSE_SYSTEM_CALL,		USER,	   fast_syscall_user },
{ EXCCAUSE_SYSTEM_CALL,		0,	   system_call },
/* EXCCAUSE_INSTRUCTION_FETCH unhandled */
/* EXCCAUSE_LOAD_STORE_ERROR unhandled*/
{ EXCCAUSE_LEVEL1_INTERRUPT,	0,	   do_interrupt },
{ EXCCAUSE_ALLOCA,		USER|KRNL, fast_alloca },
/* EXCCAUSE_INTEGER_DIVIDE_BY_ZERO unhandled */
/* EXCCAUSE_PRIVILEGED unhandled */
#if XCHAL_UNALIGNED_LOAD_EXCEPTION || XCHAL_UNALIGNED_STORE_EXCEPTION
#ifdef CONFIG_UNALIGNED_USER
{ EXCCAUSE_UNALIGNED,		USER,	   fast_unaligned },
#else
{ EXCCAUSE_UNALIGNED,		0,	   do_unaligned_user },
#endif
{ EXCCAUSE_UNALIGNED,		KRNL,	   fast_unaligned },
#endif
{ EXCCAUSE_ITLB_MISS,		0,	   do_page_fault },
{ EXCCAUSE_ITLB_MISS,		USER|KRNL, fast_second_level_miss},
{ EXCCAUSE_ITLB_MULTIHIT,		0,	   do_multihit },
{ EXCCAUSE_ITLB_PRIVILEGE,	0,	   do_page_fault },
/* EXCCAUSE_SIZE_RESTRICTION unhandled */
{ EXCCAUSE_FETCH_CACHE_ATTRIBUTE,	0,	   do_page_fault },
{ EXCCAUSE_DTLB_MISS,		USER|KRNL, fast_second_level_miss},
{ EXCCAUSE_DTLB_MISS,		0,	   do_page_fault },
{ EXCCAUSE_DTLB_MULTIHIT,		0,	   do_multihit },
{ EXCCAUSE_DTLB_PRIVILEGE,	0,	   do_page_fault },
/* EXCCAUSE_DTLB_SIZE_RESTRICTION unhandled */
{ EXCCAUSE_STORE_CACHE_ATTRIBUTE,	USER|KRNL, fast_store_prohibited },
{ EXCCAUSE_STORE_CACHE_ATTRIBUTE,	0,	   do_page_fault },
{ EXCCAUSE_LOAD_CACHE_ATTRIBUTE,	0,	   do_page_fault },
/* XCCHAL_EXCCAUSE_FLOATING_POINT unhandled */
#if XTENSA_HAVE_COPROCESSOR(0)
COPROCESSOR(0),
#endif
#if XTENSA_HAVE_COPROCESSOR(1)
COPROCESSOR(1),
#endif
#if XTENSA_HAVE_COPROCESSOR(2)
COPROCESSOR(2),
#endif
#if XTENSA_HAVE_COPROCESSOR(3)
COPROCESSOR(3),
#endif
#if XTENSA_HAVE_COPROCESSOR(4)
COPROCESSOR(4),
#endif
#if XTENSA_HAVE_COPROCESSOR(5)
COPROCESSOR(5),
#endif
#if XTENSA_HAVE_COPROCESSOR(6)
COPROCESSOR(6),
#endif
#if XTENSA_HAVE_COPROCESSOR(7)
COPROCESSOR(7),
#endif
{ EXCCAUSE_MAPPED_DEBUG,		0,		do_debug },
{ -1, -1, 0 }

};


unsigned long exc_table[EXC_TABLE_SIZE/4];

void die(const char*, struct pt_regs*, long);

static inline void
__die_if_kernel(const char *str, struct pt_regs *regs, long err)
{
	if (!user_mode(regs))
		die(str, regs, err);
}


void do_unhandled(struct pt_regs *regs, unsigned long exccause)
{
	__die_if_kernel("Caught unhandled exception - should not happen",
	    		regs, SIGKILL);

	/* If in user mode, send SIGILL signal to current process */
	printk("Caught unhandled exception in '%s' "
	       "(pid = %d, pc = %#010lx) - should not happen\n"
	       "\tEXCCAUSE is %ld\n",
	       current->comm, task_pid_nr(current), regs->pc, exccause);
	force_sig(SIGILL, current);
}


void do_multihit(struct pt_regs *regs, unsigned long exccause)
{
	die("Caught multihit exception", regs, SIGKILL);
}


unsigned long ignored_level1_interrupts;
extern void do_IRQ(int, struct pt_regs *);

void do_interrupt (struct pt_regs *regs)
{
	unsigned long intread = get_sr (INTREAD);
	unsigned long intenable = get_sr (INTENABLE);
	int i, mask;

	/* Handle all interrupts (no priorities).
	 * (Clear the interrupt before processing, in case it's
	 *  edge-triggered or software-generated)
	 */

	for (i=0, mask = 1; i < XCHAL_NUM_INTERRUPTS; i++, mask <<= 1) {
		if (mask & (intread & intenable)) {
			set_sr (mask, INTCLEAR);
			do_IRQ (i,regs);
		}
	}
}


void
do_illegal_instruction(struct pt_regs *regs)
{
	__die_if_kernel("Illegal instruction in kernel", regs, SIGKILL);

	/* If in user mode, send SIGILL signal to current process. */

	printk("Illegal Instruction in '%s' (pid = %d, pc = %#010lx)\n",
	    current->comm, task_pid_nr(current), regs->pc);
	force_sig(SIGILL, current);
}



#if XCHAL_UNALIGNED_LOAD_EXCEPTION || XCHAL_UNALIGNED_STORE_EXCEPTION
#ifndef CONFIG_UNALIGNED_USER
void
do_unaligned_user (struct pt_regs *regs)
{
	siginfo_t info;

	__die_if_kernel("Unhandled unaligned exception in kernel",
	    		regs, SIGKILL);

	current->thread.bad_vaddr = regs->excvaddr;
	current->thread.error_code = -3;
	printk("Unaligned memory access to %08lx in '%s' "
	       "(pid = %d, pc = %#010lx)\n",
	       regs->excvaddr, current->comm, task_pid_nr(current), regs->pc);
	info.si_signo = SIGBUS;
	info.si_errno = 0;
	info.si_code = BUS_ADRALN;
	info.si_addr = (void *) regs->excvaddr;
	force_sig_info(SIGSEGV, &info, current);

}
#endif
#endif

void
do_debug(struct pt_regs *regs)
{
#ifdef CONFIG_KGDB
	/* If remote debugging is configured AND enabled, we give control to
	 * kgdb.  Otherwise, we fall through, perhaps giving control to the
	 * native debugger.
	 */

	if (gdb_enter) {
		extern void gdb_handle_exception(struct pt_regs *);
		gdb_handle_exception(regs);
		return_from_debug_flag = 1;
		return;
	}
#endif

	__die_if_kernel("Breakpoint in kernel", regs, SIGKILL);

	/* If in user mode, send SIGTRAP signal to current process */

	force_sig(SIGTRAP, current);
}



#define set_handler(idx,handler) (exc_table[idx] = (unsigned long) (handler))

void __init trap_init(void)
{
	int i;

	/* Setup default vectors. */

	for(i = 0; i < 64; i++) {
		set_handler(EXC_TABLE_FAST_USER/4   + i, user_exception);
		set_handler(EXC_TABLE_FAST_KERNEL/4 + i, kernel_exception);
		set_handler(EXC_TABLE_DEFAULT/4 + i, do_unhandled);
	}

	/* Setup specific handlers. */

	for(i = 0; dispatch_init_table[i].cause >= 0; i++) {

		int fast = dispatch_init_table[i].fast;
		int cause = dispatch_init_table[i].cause;
		void *handler = dispatch_init_table[i].handler;

		if (fast == 0)
			set_handler (EXC_TABLE_DEFAULT/4 + cause, handler);
		if (fast && fast & USER)
			set_handler (EXC_TABLE_FAST_USER/4 + cause, handler);
		if (fast && fast & KRNL)
			set_handler (EXC_TABLE_FAST_KERNEL/4 + cause, handler);
	}

	/* Initialize EXCSAVE_1 to hold the address of the exception table. */

	i = (unsigned long)exc_table;
	__asm__ __volatile__("wsr  %0, "__stringify(EXCSAVE_1)"\n" : : "a" (i));
}


void show_regs(struct pt_regs * regs)
{
	int i, wmask;

	wmask = regs->wmask & ~1;

	for (i = 0; i < 16; i++) {
		if ((i % 8) == 0)
			printk ("\n" KERN_INFO "a%02d: ", i);
		printk("%08lx ", regs->areg[i]);
	}
	printk("\n");

	printk("pc: %08lx, ps: %08lx, depc: %08lx, excvaddr: %08lx\n",
	       regs->pc, regs->ps, regs->depc, regs->excvaddr);
	printk("lbeg: %08lx, lend: %08lx lcount: %08lx, sar: %08lx\n",
	       regs->lbeg, regs->lend, regs->lcount, regs->sar);
	if (user_mode(regs))
		printk("wb: %08lx, ws: %08lx, wmask: %08lx, syscall: %ld\n",
		       regs->windowbase, regs->windowstart, regs->wmask,
		       regs->syscall);
}

void show_trace(struct task_struct *task, unsigned long *sp)
{
	unsigned long a0, a1, pc;
	unsigned long sp_start, sp_end;

	a1 = (unsigned long)sp;

	if (a1 == 0)
		__asm__ __volatile__ ("mov %0, a1\n" : "=a"(a1));


	sp_start = a1 & ~(THREAD_SIZE-1);
	sp_end = sp_start + THREAD_SIZE;

	printk("Call Trace:");
#ifdef CONFIG_KALLSYMS
	printk("\n");
#endif
	spill_registers();

	while (a1 > sp_start && a1 < sp_end) {
		sp = (unsigned long*)a1;

		a0 = *(sp - 4);
		a1 = *(sp - 3);

		if (a1 <= (unsigned long) sp)
			break;

		pc = MAKE_PC_FROM_RA(a0, a1);

		if (kernel_text_address(pc)) {
			printk(" [<%08lx>] ", pc);
			print_symbol("%s\n", pc);
		}
	}
	printk("\n");
}


static int kstack_depth_to_print = 24;

void show_stack(struct task_struct *task, unsigned long *sp)
{
	int i = 0;
	unsigned long *stack;

	if (sp == 0)
		__asm__ __volatile__ ("mov %0, a1\n" : "=a"(sp));

 	stack = sp;

	printk("\nStack: ");

	for (i = 0; i < kstack_depth_to_print; i++) {
		if (kstack_end(sp))
			break;
		if (i && ((i % 8) == 0))
			printk("\n       ");
		printk("%08lx ", *sp++);
	}
	printk("\n");
	show_trace(task, stack);
}

void dump_stack(void)
{
	show_stack(current, NULL);
}

EXPORT_SYMBOL(dump_stack);


void show_code(unsigned int *pc)
{
	long i;

	printk("\nCode:");

	for(i = -3 ; i < 6 ; i++) {
		unsigned long insn;
		if (__get_user(insn, pc + i)) {
			printk(" (Bad address in pc)\n");
			break;
		}
		printk("%c%08lx%c",(i?' ':'<'),insn,(i?' ':'>'));
	}
}

DEFINE_SPINLOCK(die_lock);

void die(const char * str, struct pt_regs * regs, long err)
{
	static int die_counter;
	int nl = 0;

	console_verbose();
	spin_lock_irq(&die_lock);

	printk("%s: sig: %ld [#%d]\n", str, err, ++die_counter);
#ifdef CONFIG_PREEMPT
	printk("PREEMPT ");
	nl = 1;
#endif
	if (nl)
		printk("\n");
	show_regs(regs);
	if (!user_mode(regs))
		show_stack(NULL, (unsigned long*)regs->areg[1]);

	add_taint(TAINT_DIE);
	spin_unlock_irq(&die_lock);

	if (in_interrupt())
		panic("Fatal exception in interrupt");

	if (panic_on_oops)
		panic("Fatal exception");

	do_exit(err);
}


