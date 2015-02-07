
#include <linux/linkage.h>
#include <asm/asm-offsets.h>
#include <asm/page.h>
#include <asm/tlbflush.h>
#include "proc-macros.S"

ENTRY(v7wbi_flush_user_tlb_range)
	vma_vm_mm r3, r2			@ get vma->vm_mm
	mmid	r3, r3				@ get vm_mm->context.id
	dsb
	mov	r0, r0, lsr #PAGE_SHIFT		@ align address
	mov	r1, r1, lsr #PAGE_SHIFT
	asid	r3, r3				@ mask ASID
	orr	r0, r3, r0, lsl #PAGE_SHIFT	@ Create initial MVA
	mov	r1, r1, lsl #PAGE_SHIFT
	vma_vm_flags r2, r2			@ get vma->vm_flags
1:
	mcr	p15, 0, r0, c8, c6, 1		@ TLB invalidate D MVA (was 1)
	tst	r2, #VM_EXEC			@ Executable area ?
	mcrne	p15, 0, r0, c8, c5, 1		@ TLB invalidate I MVA (was 1)
	add	r0, r0, #PAGE_SZ
	cmp	r0, r1
	blo	1b
	mov	ip, #0
	mcr	p15, 0, ip, c7, c5, 6		@ flush BTAC/BTB
	dsb
	mov	pc, lr
ENDPROC(v7wbi_flush_user_tlb_range)

ENTRY(v7wbi_flush_kern_tlb_range)
	dsb
	mov	r0, r0, lsr #PAGE_SHIFT		@ align address
	mov	r1, r1, lsr #PAGE_SHIFT
	mov	r0, r0, lsl #PAGE_SHIFT
	mov	r1, r1, lsl #PAGE_SHIFT
1:
	mcr	p15, 0, r0, c8, c6, 1		@ TLB invalidate D MVA
	mcr	p15, 0, r0, c8, c5, 1		@ TLB invalidate I MVA
	add	r0, r0, #PAGE_SZ
	cmp	r0, r1
	blo	1b
	mov	r2, #0
	mcr	p15, 0, r2, c7, c5, 6		@ flush BTAC/BTB
	dsb
	isb
	mov	pc, lr
ENDPROC(v7wbi_flush_kern_tlb_range)

	.section ".text.init", #alloc, #execinstr

	.type	v7wbi_tlb_fns, #object
ENTRY(v7wbi_tlb_fns)
	.long	v7wbi_flush_user_tlb_range
	.long	v7wbi_flush_kern_tlb_range
	.long	v6wbi_tlb_flags
	.size	v7wbi_tlb_fns, . - v7wbi_tlb_fns
