
#ifndef _ASM_BUG_H
#define _ASM_BUG_H

#define _debug_bug_trap()					\
do {								\
	asm volatile(						\
		"	syscall 15			\n"	\
		"0:					\n"	\
		"	.section __bug_table,\"a\"	\n"	\
		"	.long 0b,%0,%1			\n"	\
		"	.previous			\n"	\
		:						\
		: "i"(__FILE__), "i"(__LINE__)			\
		);						\
} while (0)

#define BUG() _debug_bug_trap()

#define HAVE_ARCH_BUG
#include <asm-generic/bug.h>

#endif /* _ASM_BUG_H */
