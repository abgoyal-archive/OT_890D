


.text

#include <linux/linkage.h>
#include <asm/segment.h>
#include <asm/page.h>
#include <asm/boot.h>
#include <asm/asm-offsets.h>

.section ".text.head","ax",@progbits
	.globl startup_32

startup_32:
	cld
	/* test KEEP_SEGMENTS flag to see if the bootloader is asking
	 * us to not reload segments */
	testb $(1<<6), BP_loadflags(%esi)
	jnz 1f

	cli
	movl $(__BOOT_DS),%eax
	movl %eax,%ds
	movl %eax,%es
	movl %eax,%fs
	movl %eax,%gs
	movl %eax,%ss
1:

	leal (0x1e4+4)(%esi), %esp
	call 1f
1:	popl %ebp
	subl $1b, %ebp


#ifdef CONFIG_RELOCATABLE
	movl 	%ebp, %ebx
	addl    $(CONFIG_PHYSICAL_ALIGN - 1), %ebx
	andl    $(~(CONFIG_PHYSICAL_ALIGN - 1)), %ebx
#else
	movl $LOAD_PHYSICAL_ADDR, %ebx
#endif

	/* Replace the compressed data size with the uncompressed size */
	subl input_len(%ebp), %ebx
	movl output_len(%ebp), %eax
	addl %eax, %ebx
	/* Add 8 bytes for every 32K input block */
	shrl $12, %eax
	addl %eax, %ebx
	/* Add 32K + 18 bytes of extra slack */
	addl $(32768 + 18), %ebx
	/* Align on a 4K boundary */
	addl $4095, %ebx
	andl $~4095, %ebx

	pushl %esi
	leal _end(%ebp), %esi
	leal _end(%ebx), %edi
	movl $(_end - startup_32), %ecx
	std
	rep
	movsb
	cld
	popl %esi

#ifdef CONFIG_RELOCATABLE
	addl    $(CONFIG_PHYSICAL_ALIGN - 1), %ebp
	andl    $(~(CONFIG_PHYSICAL_ALIGN - 1)), %ebp
#else
	movl	$LOAD_PHYSICAL_ADDR, %ebp
#endif

	leal relocated(%ebx), %eax
	jmp *%eax
.section ".text"
relocated:

	xorl %eax,%eax
	leal _edata(%ebx),%edi
	leal _end(%ebx), %ecx
	subl %edi,%ecx
	cld
	rep
	stosb

	leal boot_stack_end(%ebx), %esp

	movl output_len(%ebx), %eax
	pushl %eax
			# push arguments for decompress_kernel:
	pushl %ebp	# output address
	movl input_len(%ebx), %eax
	pushl %eax	# input_len
	leal input_data(%ebx), %eax
	pushl %eax	# input_data
	leal boot_heap(%ebx), %eax
	pushl %eax	# heap area
	pushl %esi	# real mode pointer
	call decompress_kernel
	addl $20, %esp
	popl %ecx

#if CONFIG_RELOCATABLE
	movl %ebp, %edi
	addl %ecx, %edi

	movl %ebp, %ebx
	subl $LOAD_PHYSICAL_ADDR, %ebx
	jz   2f		/* Nothing to be done if loaded at compiled addr. */

1:	subl $4, %edi
	movl 0(%edi), %ecx
	testl %ecx, %ecx
	jz 2f
	addl %ebx, -__PAGE_OFFSET(%ebx, %ecx)
	jmp 1b
2:
#endif

	xorl %ebx,%ebx
	jmp *%ebp

.bss
/* Stack and heap for uncompression */
.balign 4
boot_heap:
	.fill BOOT_HEAP_SIZE, 1, 0
boot_stack:
	.fill BOOT_STACK_SIZE, 1, 0
boot_stack_end:
