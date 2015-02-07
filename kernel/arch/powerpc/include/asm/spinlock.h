
#ifndef __ASM_SPINLOCK_H
#define __ASM_SPINLOCK_H
#ifdef __KERNEL__

#include <linux/irqflags.h>
#ifdef CONFIG_PPC64
#include <asm/paca.h>
#include <asm/hvcall.h>
#include <asm/iseries/hv_call.h>
#endif
#include <asm/asm-compat.h>
#include <asm/synch.h>

#define __raw_spin_is_locked(x)		((x)->slock != 0)

#ifdef CONFIG_PPC64
/* use 0x800000yy when locked, where yy == CPU number */
#define LOCK_TOKEN	(*(u32 *)(&get_paca()->lock_token))
#else
#define LOCK_TOKEN	1
#endif

#if defined(CONFIG_PPC64) && defined(CONFIG_SMP)
#define CLEAR_IO_SYNC	(get_paca()->io_sync = 0)
#define SYNC_IO		do {						\
				if (unlikely(get_paca()->io_sync)) {	\
					mb();				\
					get_paca()->io_sync = 0;	\
				}					\
			} while (0)
#else
#define CLEAR_IO_SYNC
#define SYNC_IO
#endif

static inline unsigned long __spin_trylock(raw_spinlock_t *lock)
{
	unsigned long tmp, token;

	token = LOCK_TOKEN;
	__asm__ __volatile__(
"1:	lwarx		%0,0,%2\n\
	cmpwi		0,%0,0\n\
	bne-		2f\n\
	stwcx.		%1,0,%2\n\
	bne-		1b\n\
	isync\n\
2:"	: "=&r" (tmp)
	: "r" (token), "r" (&lock->slock)
	: "cr0", "memory");

	return tmp;
}

static inline int __raw_spin_trylock(raw_spinlock_t *lock)
{
	CLEAR_IO_SYNC;
	return __spin_trylock(lock) == 0;
}


#if defined(CONFIG_PPC_SPLPAR) || defined(CONFIG_PPC_ISERIES)
/* We only yield to the hypervisor if we are in shared processor mode */
#define SHARED_PROCESSOR (get_lppaca()->shared_proc)
extern void __spin_yield(raw_spinlock_t *lock);
extern void __rw_yield(raw_rwlock_t *lock);
#else /* SPLPAR || ISERIES */
#define __spin_yield(x)	barrier()
#define __rw_yield(x)	barrier()
#define SHARED_PROCESSOR	0
#endif

static inline void __raw_spin_lock(raw_spinlock_t *lock)
{
	CLEAR_IO_SYNC;
	while (1) {
		if (likely(__spin_trylock(lock) == 0))
			break;
		do {
			HMT_low();
			if (SHARED_PROCESSOR)
				__spin_yield(lock);
		} while (unlikely(lock->slock != 0));
		HMT_medium();
	}
}

static inline
void __raw_spin_lock_flags(raw_spinlock_t *lock, unsigned long flags)
{
	unsigned long flags_dis;

	CLEAR_IO_SYNC;
	while (1) {
		if (likely(__spin_trylock(lock) == 0))
			break;
		local_save_flags(flags_dis);
		local_irq_restore(flags);
		do {
			HMT_low();
			if (SHARED_PROCESSOR)
				__spin_yield(lock);
		} while (unlikely(lock->slock != 0));
		HMT_medium();
		local_irq_restore(flags_dis);
	}
}

static inline void __raw_spin_unlock(raw_spinlock_t *lock)
{
	SYNC_IO;
	__asm__ __volatile__("# __raw_spin_unlock\n\t"
				LWSYNC_ON_SMP: : :"memory");
	lock->slock = 0;
}

#ifdef CONFIG_PPC64
extern void __raw_spin_unlock_wait(raw_spinlock_t *lock);
#else
#define __raw_spin_unlock_wait(lock) \
	do { while (__raw_spin_is_locked(lock)) cpu_relax(); } while (0)
#endif


#define __raw_read_can_lock(rw)		((rw)->lock >= 0)
#define __raw_write_can_lock(rw)	(!(rw)->lock)

#ifdef CONFIG_PPC64
#define __DO_SIGN_EXTEND	"extsw	%0,%0\n"
#define WRLOCK_TOKEN		LOCK_TOKEN	/* it's negative */
#else
#define __DO_SIGN_EXTEND
#define WRLOCK_TOKEN		(-1)
#endif

static inline long __read_trylock(raw_rwlock_t *rw)
{
	long tmp;

	__asm__ __volatile__(
"1:	lwarx		%0,0,%1\n"
	__DO_SIGN_EXTEND
"	addic.		%0,%0,1\n\
	ble-		2f\n"
	PPC405_ERR77(0,%1)
"	stwcx.		%0,0,%1\n\
	bne-		1b\n\
	isync\n\
2:"	: "=&r" (tmp)
	: "r" (&rw->lock)
	: "cr0", "xer", "memory");

	return tmp;
}

static inline long __write_trylock(raw_rwlock_t *rw)
{
	long tmp, token;

	token = WRLOCK_TOKEN;
	__asm__ __volatile__(
"1:	lwarx		%0,0,%2\n\
	cmpwi		0,%0,0\n\
	bne-		2f\n"
	PPC405_ERR77(0,%1)
"	stwcx.		%1,0,%2\n\
	bne-		1b\n\
	isync\n\
2:"	: "=&r" (tmp)
	: "r" (token), "r" (&rw->lock)
	: "cr0", "memory");

	return tmp;
}

static inline void __raw_read_lock(raw_rwlock_t *rw)
{
	while (1) {
		if (likely(__read_trylock(rw) > 0))
			break;
		do {
			HMT_low();
			if (SHARED_PROCESSOR)
				__rw_yield(rw);
		} while (unlikely(rw->lock < 0));
		HMT_medium();
	}
}

static inline void __raw_write_lock(raw_rwlock_t *rw)
{
	while (1) {
		if (likely(__write_trylock(rw) == 0))
			break;
		do {
			HMT_low();
			if (SHARED_PROCESSOR)
				__rw_yield(rw);
		} while (unlikely(rw->lock != 0));
		HMT_medium();
	}
}

static inline int __raw_read_trylock(raw_rwlock_t *rw)
{
	return __read_trylock(rw) > 0;
}

static inline int __raw_write_trylock(raw_rwlock_t *rw)
{
	return __write_trylock(rw) == 0;
}

static inline void __raw_read_unlock(raw_rwlock_t *rw)
{
	long tmp;

	__asm__ __volatile__(
	"# read_unlock\n\t"
	LWSYNC_ON_SMP
"1:	lwarx		%0,0,%1\n\
	addic		%0,%0,-1\n"
	PPC405_ERR77(0,%1)
"	stwcx.		%0,0,%1\n\
	bne-		1b"
	: "=&r"(tmp)
	: "r"(&rw->lock)
	: "cr0", "xer", "memory");
}

static inline void __raw_write_unlock(raw_rwlock_t *rw)
{
	__asm__ __volatile__("# write_unlock\n\t"
				LWSYNC_ON_SMP: : :"memory");
	rw->lock = 0;
}

#define _raw_spin_relax(lock)	__spin_yield(lock)
#define _raw_read_relax(lock)	__rw_yield(lock)
#define _raw_write_relax(lock)	__rw_yield(lock)

#endif /* __KERNEL__ */
#endif /* __ASM_SPINLOCK_H */
