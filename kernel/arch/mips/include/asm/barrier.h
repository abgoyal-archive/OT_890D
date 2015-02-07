
#ifndef __ASM_BARRIER_H
#define __ASM_BARRIER_H


#define read_barrier_depends()		do { } while(0)
#define smp_read_barrier_depends()	do { } while(0)

#ifdef CONFIG_CPU_HAS_SYNC
#define __sync()				\
	__asm__ __volatile__(			\
		".set	push\n\t"		\
		".set	noreorder\n\t"		\
		".set	mips2\n\t"		\
		"sync\n\t"			\
		".set	pop"			\
		: /* no output */		\
		: /* no input */		\
		: "memory")
#else
#define __sync()	do { } while(0)
#endif

#define __fast_iob()				\
	__asm__ __volatile__(			\
		".set	push\n\t"		\
		".set	noreorder\n\t"		\
		"lw	$0,%0\n\t"		\
		"nop\n\t"			\
		".set	pop"			\
		: /* no output */		\
		: "m" (*(int *)CKSEG1)		\
		: "memory")

#define fast_wmb()	__sync()
#define fast_rmb()	__sync()
#define fast_mb()	__sync()
#ifdef CONFIG_SGI_IP28
#define fast_iob()				\
	__asm__ __volatile__(			\
		".set	push\n\t"		\
		".set	noreorder\n\t"		\
		"lw	$0,%0\n\t"		\
		"sync\n\t"			\
		"lw	$0,%0\n\t"		\
		".set	pop"			\
		: /* no output */		\
		: "m" (*(int *)CKSEG1ADDR(0x1fa00004)) \
		: "memory")
#else
#define fast_iob()				\
	do {					\
		__sync();			\
		__fast_iob();			\
	} while (0)
#endif

#ifdef CONFIG_CPU_HAS_WB

#include <asm/wbflush.h>

#define wmb()		fast_wmb()
#define rmb()		fast_rmb()
#define mb()		wbflush()
#define iob()		wbflush()

#else /* !CONFIG_CPU_HAS_WB */

#define wmb()		fast_wmb()
#define rmb()		fast_rmb()
#define mb()		fast_mb()
#define iob()		fast_iob()

#endif /* !CONFIG_CPU_HAS_WB */

#if defined(CONFIG_WEAK_ORDERING) && defined(CONFIG_SMP)
#define __WEAK_ORDERING_MB	"       sync	\n"
#else
#define __WEAK_ORDERING_MB	"		\n"
#endif
#if defined(CONFIG_WEAK_REORDERING_BEYOND_LLSC) && defined(CONFIG_SMP)
#define __WEAK_LLSC_MB		"       sync	\n"
#else
#define __WEAK_LLSC_MB		"		\n"
#endif

#define smp_mb()	__asm__ __volatile__(__WEAK_ORDERING_MB : : :"memory")
#define smp_rmb()	__asm__ __volatile__(__WEAK_ORDERING_MB : : :"memory")
#define smp_wmb()	__asm__ __volatile__(__WEAK_ORDERING_MB : : :"memory")

#define set_mb(var, value) \
	do { var = value; smp_mb(); } while (0)

#define smp_llsc_mb()	__asm__ __volatile__(__WEAK_LLSC_MB : : :"memory")
#define smp_llsc_rmb()	__asm__ __volatile__(__WEAK_LLSC_MB : : :"memory")
#define smp_llsc_wmb()	__asm__ __volatile__(__WEAK_LLSC_MB : : :"memory")

#endif /* __ASM_BARRIER_H */
