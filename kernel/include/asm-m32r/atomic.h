
#ifndef _ASM_M32R_ATOMIC_H
#define _ASM_M32R_ATOMIC_H


#include <linux/types.h>
#include <asm/assembler.h>
#include <asm/system.h>


#define ATOMIC_INIT(i)	{ (i) }

#define atomic_read(v)	((v)->counter)

#define atomic_set(v,i)	(((v)->counter) = (i))

static __inline__ int atomic_add_return(int i, atomic_t *v)
{
	unsigned long flags;
	int result;

	local_irq_save(flags);
	__asm__ __volatile__ (
		"# atomic_add_return		\n\t"
		DCACHE_CLEAR("%0", "r4", "%1")
		M32R_LOCK" %0, @%1;		\n\t"
		"add	%0, %2;			\n\t"
		M32R_UNLOCK" %0, @%1;		\n\t"
		: "=&r" (result)
		: "r" (&v->counter), "r" (i)
		: "memory"
#ifdef CONFIG_CHIP_M32700_TS1
		, "r4"
#endif	/* CONFIG_CHIP_M32700_TS1 */
	);
	local_irq_restore(flags);

	return result;
}

static __inline__ int atomic_sub_return(int i, atomic_t *v)
{
	unsigned long flags;
	int result;

	local_irq_save(flags);
	__asm__ __volatile__ (
		"# atomic_sub_return		\n\t"
		DCACHE_CLEAR("%0", "r4", "%1")
		M32R_LOCK" %0, @%1;		\n\t"
		"sub	%0, %2;			\n\t"
		M32R_UNLOCK" %0, @%1;		\n\t"
		: "=&r" (result)
		: "r" (&v->counter), "r" (i)
		: "memory"
#ifdef CONFIG_CHIP_M32700_TS1
		, "r4"
#endif	/* CONFIG_CHIP_M32700_TS1 */
	);
	local_irq_restore(flags);

	return result;
}

#define atomic_add(i,v) ((void) atomic_add_return((i), (v)))

#define atomic_sub(i,v) ((void) atomic_sub_return((i), (v)))

#define atomic_sub_and_test(i,v) (atomic_sub_return((i), (v)) == 0)

static __inline__ int atomic_inc_return(atomic_t *v)
{
	unsigned long flags;
	int result;

	local_irq_save(flags);
	__asm__ __volatile__ (
		"# atomic_inc_return		\n\t"
		DCACHE_CLEAR("%0", "r4", "%1")
		M32R_LOCK" %0, @%1;		\n\t"
		"addi	%0, #1;			\n\t"
		M32R_UNLOCK" %0, @%1;		\n\t"
		: "=&r" (result)
		: "r" (&v->counter)
		: "memory"
#ifdef CONFIG_CHIP_M32700_TS1
		, "r4"
#endif	/* CONFIG_CHIP_M32700_TS1 */
	);
	local_irq_restore(flags);

	return result;
}

static __inline__ int atomic_dec_return(atomic_t *v)
{
	unsigned long flags;
	int result;

	local_irq_save(flags);
	__asm__ __volatile__ (
		"# atomic_dec_return		\n\t"
		DCACHE_CLEAR("%0", "r4", "%1")
		M32R_LOCK" %0, @%1;		\n\t"
		"addi	%0, #-1;		\n\t"
		M32R_UNLOCK" %0, @%1;		\n\t"
		: "=&r" (result)
		: "r" (&v->counter)
		: "memory"
#ifdef CONFIG_CHIP_M32700_TS1
		, "r4"
#endif	/* CONFIG_CHIP_M32700_TS1 */
	);
	local_irq_restore(flags);

	return result;
}

#define atomic_inc(v) ((void)atomic_inc_return(v))

#define atomic_dec(v) ((void)atomic_dec_return(v))

#define atomic_inc_and_test(v) (atomic_inc_return(v) == 0)

#define atomic_dec_and_test(v) (atomic_dec_return(v) == 0)

#define atomic_add_negative(i,v) (atomic_add_return((i), (v)) < 0)

#define atomic_cmpxchg(v, o, n) ((int)cmpxchg(&((v)->counter), (o), (n)))
#define atomic_xchg(v, new) (xchg(&((v)->counter), new))

static __inline__ int atomic_add_unless(atomic_t *v, int a, int u)
{
	int c, old;
	c = atomic_read(v);
	for (;;) {
		if (unlikely(c == (u)))
			break;
		old = atomic_cmpxchg((v), c, c + (a));
		if (likely(old == c))
			break;
		c = old;
	}
	return c != (u);
}

#define atomic_inc_not_zero(v) atomic_add_unless((v), 1, 0)

static __inline__ void atomic_clear_mask(unsigned long  mask, atomic_t *addr)
{
	unsigned long flags;
	unsigned long tmp;

	local_irq_save(flags);
	__asm__ __volatile__ (
		"# atomic_clear_mask		\n\t"
		DCACHE_CLEAR("%0", "r5", "%1")
		M32R_LOCK" %0, @%1;		\n\t"
		"and	%0, %2;			\n\t"
		M32R_UNLOCK" %0, @%1;		\n\t"
		: "=&r" (tmp)
		: "r" (addr), "r" (~mask)
		: "memory"
#ifdef CONFIG_CHIP_M32700_TS1
		, "r5"
#endif	/* CONFIG_CHIP_M32700_TS1 */
	);
	local_irq_restore(flags);
}

static __inline__ void atomic_set_mask(unsigned long  mask, atomic_t *addr)
{
	unsigned long flags;
	unsigned long tmp;

	local_irq_save(flags);
	__asm__ __volatile__ (
		"# atomic_set_mask		\n\t"
		DCACHE_CLEAR("%0", "r5", "%1")
		M32R_LOCK" %0, @%1;		\n\t"
		"or	%0, %2;			\n\t"
		M32R_UNLOCK" %0, @%1;		\n\t"
		: "=&r" (tmp)
		: "r" (addr), "r" (mask)
		: "memory"
#ifdef CONFIG_CHIP_M32700_TS1
		, "r5"
#endif	/* CONFIG_CHIP_M32700_TS1 */
	);
	local_irq_restore(flags);
}

/* Atomic operations are already serializing on m32r */
#define smp_mb__before_atomic_dec()	barrier()
#define smp_mb__after_atomic_dec()	barrier()
#define smp_mb__before_atomic_inc()	barrier()
#define smp_mb__after_atomic_inc()	barrier()

#include <asm-generic/atomic.h>
#endif	/* _ASM_M32R_ATOMIC_H */
