
#include <linux/linkage.h>
#include <linux/lguest.h>
#include <asm/lguest_hcall.h>
#include <asm/asm-offsets.h>
#include <asm/thread_info.h>
#include <asm/processor-flags.h>

.section .init.text, "ax", @progbits
ENTRY(lguest_entry)
	/* We make the "initialization" hypercall now to tell the Host about
	 * us, and also find out where it put our page tables. */
	movl $LHCALL_LGUEST_INIT, %eax
	movl $lguest_data - __PAGE_OFFSET, %edx
	int $LGUEST_TRAP_ENTRY

	/* Set up the initial stack so we can run C code. */
	movl $(init_thread_union+THREAD_SIZE),%esp

	/* Jumps are relative, and we're running __PAGE_OFFSET too low at the
	 * moment. */
	jmp lguest_init+__PAGE_OFFSET

.text
#define LGUEST_PATCH(name, insns...)			\
	lgstart_##name:	insns; lgend_##name:;		\
	.globl lgstart_##name; .globl lgend_##name

LGUEST_PATCH(cli, movl $0, lguest_data+LGUEST_DATA_irq_enabled)
LGUEST_PATCH(sti, movl $X86_EFLAGS_IF, lguest_data+LGUEST_DATA_irq_enabled)
LGUEST_PATCH(popf, movl %eax, lguest_data+LGUEST_DATA_irq_enabled)
LGUEST_PATCH(pushf, movl lguest_data+LGUEST_DATA_irq_enabled, %eax)
/*:*/

/* These demark the EIP range where host should never deliver interrupts. */
.global lguest_noirq_start
.global lguest_noirq_end


ENTRY(lguest_iret)
	pushl	%eax
	movl	12(%esp), %eax
lguest_noirq_start:
	/* Note the %ss: segment prefix here.  Normal data accesses use the
	 * "ds" segment, but that will have already been restored for whatever
	 * we're returning to (such as userspace): we can't trust it.  The %ss:
	 * prefix makes sure we use the stack segment, which is still valid. */
	movl	%eax,%ss:lguest_data+LGUEST_DATA_irq_enabled
	popl	%eax
	iret
lguest_noirq_end:
