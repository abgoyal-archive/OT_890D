

#include <linux/linkage.h>

#include <asm/asm-offsets.h>
#include <asm/processor-flags.h>
#include <asm/errno.h>
#include <asm/segment.h>

#include <xen/interface/xen.h>

#define RELOC(x, v)	.globl x##_reloc; x##_reloc=v
#define ENDPATCH(x)	.globl x##_end; x##_end=.

/* Pseudo-flag used for virtual NMI, which we don't implement yet */
#define XEN_EFLAGS_NMI	0x80000000

#if 1
#define BUG			ud2a
#define PER_CPU_VAR(var, off)	0xdeadbeef
#endif

ENTRY(xen_irq_enable_direct)
	BUG

	/* Unmask events */
	movb $0, PER_CPU_VAR(xen_vcpu_info, XEN_vcpu_info_mask)

	/* Preempt here doesn't matter because that will deal with
	   any pending interrupts.  The pending check may end up being
	   run on the wrong CPU, but that doesn't hurt. */

	/* Test for pending */
	testb $0xff, PER_CPU_VAR(xen_vcpu_info, XEN_vcpu_info_pending)
	jz 1f

2:	call check_events
1:
ENDPATCH(xen_irq_enable_direct)
	ret
	ENDPROC(xen_irq_enable_direct)
	RELOC(xen_irq_enable_direct, 2b+1)

ENTRY(xen_irq_disable_direct)
	BUG

	movb $1, PER_CPU_VAR(xen_vcpu_info, XEN_vcpu_info_mask)
ENDPATCH(xen_irq_disable_direct)
	ret
	ENDPROC(xen_irq_disable_direct)
	RELOC(xen_irq_disable_direct, 0)

ENTRY(xen_save_fl_direct)
	BUG

	testb $0xff, PER_CPU_VAR(xen_vcpu_info, XEN_vcpu_info_mask)
	setz %ah
	addb %ah,%ah
ENDPATCH(xen_save_fl_direct)
	ret
	ENDPROC(xen_save_fl_direct)
	RELOC(xen_save_fl_direct, 0)

ENTRY(xen_restore_fl_direct)
	BUG

	testb $X86_EFLAGS_IF>>8, %ah
	setz PER_CPU_VAR(xen_vcpu_info, XEN_vcpu_info_mask)
	/* Preempt here doesn't matter because that will deal with
	   any pending interrupts.  The pending check may end up being
	   run on the wrong CPU, but that doesn't hurt. */

	/* check for unmasked and pending */
	cmpw $0x0001, PER_CPU_VAR(xen_vcpu_info, XEN_vcpu_info_pending)
	jz 1f
2:	call check_events
1:
ENDPATCH(xen_restore_fl_direct)
	ret
	ENDPROC(xen_restore_fl_direct)
	RELOC(xen_restore_fl_direct, 2b+1)


check_events:
	push %rax
	push %rcx
	push %rdx
	push %rsi
	push %rdi
	push %r8
	push %r9
	push %r10
	push %r11
	call xen_force_evtchn_callback
	pop %r11
	pop %r10
	pop %r9
	pop %r8
	pop %rdi
	pop %rsi
	pop %rdx
	pop %rcx
	pop %rax
	ret

ENTRY(xen_adjust_exception_frame)
	mov 8+0(%rsp),%rcx
	mov 8+8(%rsp),%r11
	ret $16

hypercall_iret = hypercall_page + __HYPERVISOR_iret * 32
ENTRY(xen_iret)
	pushq $0
1:	jmp hypercall_iret
ENDPATCH(xen_iret)
RELOC(xen_iret, 1b+1)

ENTRY(xen_sysexit)
	pushq $__USER32_DS
	pushq %rcx
	pushq $X86_EFLAGS_IF
	pushq $__USER32_CS
	pushq %rdx

	pushq $0
1:	jmp hypercall_iret
ENDPATCH(xen_sysexit)
RELOC(xen_sysexit, 1b+1)

ENTRY(xen_sysret64)
	/* We're already on the usermode stack at this point, but still
	   with the kernel gs, so we can easily switch back */
	movq %rsp, %gs:pda_oldrsp
	movq %gs:pda_kernelstack,%rsp

	pushq $__USER_DS
	pushq %gs:pda_oldrsp
	pushq %r11
	pushq $__USER_CS
	pushq %rcx

	pushq $VGCF_in_syscall
1:	jmp hypercall_iret
ENDPATCH(xen_sysret64)
RELOC(xen_sysret64, 1b+1)

ENTRY(xen_sysret32)
	/* We're already on the usermode stack at this point, but still
	   with the kernel gs, so we can easily switch back */
	movq %rsp, %gs:pda_oldrsp
	movq %gs:pda_kernelstack, %rsp

	pushq $__USER32_DS
	pushq %gs:pda_oldrsp
	pushq %r11
	pushq $__USER32_CS
	pushq %rcx

	pushq $VGCF_in_syscall
1:	jmp hypercall_iret
ENDPATCH(xen_sysret32)
RELOC(xen_sysret32, 1b+1)


.macro undo_xen_syscall
	mov 0*8(%rsp),%rcx
	mov 1*8(%rsp),%r11
	mov 5*8(%rsp),%rsp
.endm

/* Normal 64-bit system call target */
ENTRY(xen_syscall_target)
	undo_xen_syscall
	jmp system_call_after_swapgs
ENDPROC(xen_syscall_target)

#ifdef CONFIG_IA32_EMULATION

/* 32-bit compat syscall target */
ENTRY(xen_syscall32_target)
	undo_xen_syscall
	jmp ia32_cstar_target
ENDPROC(xen_syscall32_target)

/* 32-bit compat sysenter target */
ENTRY(xen_sysenter_target)
	undo_xen_syscall
	jmp ia32_sysenter_target
ENDPROC(xen_sysenter_target)

#else /* !CONFIG_IA32_EMULATION */

ENTRY(xen_syscall32_target)
ENTRY(xen_sysenter_target)
	lea 16(%rsp), %rsp	/* strip %rcx,%r11 */
	mov $-ENOSYS, %rax
	pushq $VGCF_in_syscall
	jmp hypercall_iret
ENDPROC(xen_syscall32_target)
ENDPROC(xen_sysenter_target)

#endif	/* CONFIG_IA32_EMULATION */
