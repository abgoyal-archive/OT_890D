
#ifndef _ASM_ATOMIC_H
#define _ASM_ATOMIC_H

#ifdef CONFIG_SMP
#error not SMP safe
#endif


#define ATOMIC_INIT(i)	{ (i) }

#ifdef __KERNEL__

#define atomic_read(v)	((v)->counter)

#define atomic_set(v, i) (((v)->counter) = (i))

#include <asm/system.h>

static inline int atomic_add_return(int i, atomic_t *v)
{
	unsigned long flags;
	int temp;

	local_irq_save(flags);
	temp = v->counter;
	temp += i;
	v->counter = temp;
	local_irq_restore(flags);

	return temp;
}

static inline int atomic_sub_return(int i, atomic_t *v)
{
	unsigned long flags;
	int temp;

	local_irq_save(flags);
	temp = v->counter;
	temp -= i;
	v->counter = temp;
	local_irq_restore(flags);

	return temp;
}

static inline int atomic_add_negative(int i, atomic_t *v)
{
	return atomic_add_return(i, v) < 0;
}

static inline void atomic_add(int i, atomic_t *v)
{
	atomic_add_return(i, v);
}

static inline void atomic_sub(int i, atomic_t *v)
{
	atomic_sub_return(i, v);
}

static inline void atomic_inc(atomic_t *v)
{
	atomic_add_return(1, v);
}

static inline void atomic_dec(atomic_t *v)
{
	atomic_sub_return(1, v);
}

#define atomic_dec_return(v)		atomic_sub_return(1, (v))
#define atomic_inc_return(v)		atomic_add_return(1, (v))

#define atomic_sub_and_test(i, v)	(atomic_sub_return((i), (v)) == 0)
#define atomic_dec_and_test(v)		(atomic_sub_return(1, (v)) == 0)
#define atomic_inc_and_test(v)		(atomic_add_return(1, (v)) == 0)

#define atomic_add_unless(v, a, u)				\
({								\
	int c, old;						\
	c = atomic_read(v);					\
	while (c != (u) && (old = atomic_cmpxchg((v), c, c + (a))) != c) \
		c = old;					\
	c != (u);						\
})

#define atomic_inc_not_zero(v) atomic_add_unless((v), 1, 0)

static inline void atomic_clear_mask(unsigned long mask, unsigned long *addr)
{
	unsigned long flags;

	mask = ~mask;
	local_irq_save(flags);
	*addr &= mask;
	local_irq_restore(flags);
}

#define atomic_xchg(ptr, v)		(xchg(&(ptr)->counter, (v)))
#define atomic_cmpxchg(v, old, new)	(cmpxchg(&((v)->counter), (old), (new)))

/* Atomic operations are already serializing on MN10300??? */
#define smp_mb__before_atomic_dec()	barrier()
#define smp_mb__after_atomic_dec()	barrier()
#define smp_mb__before_atomic_inc()	barrier()
#define smp_mb__after_atomic_inc()	barrier()

#include <asm-generic/atomic.h>

#endif /* __KERNEL__ */
#endif /* _ASM_ATOMIC_H */
