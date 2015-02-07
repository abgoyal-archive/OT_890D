
#ifndef _LINUX_CLOCKSOURCE_H
#define _LINUX_CLOCKSOURCE_H

#include <linux/types.h>
#include <linux/timex.h>
#include <linux/time.h>
#include <linux/list.h>
#include <linux/cache.h>
#include <linux/timer.h>
#include <asm/div64.h>
#include <asm/io.h>

/* clocksource cycle base type */
typedef u64 cycle_t;
struct clocksource;

struct clocksource {
	/*
	 * First part of structure is read mostly
	 */
	char *name;
	struct list_head list;
	int rating;
	cycle_t (*read)(void);
	cycle_t mask;
	u32 mult;
	u32 mult_orig;
	u32 shift;
	unsigned long flags;
	cycle_t (*vread)(void);
	void (*resume)(void);
#ifdef CONFIG_IA64
	void *fsys_mmio;        /* used by fsyscall asm code */
#define CLKSRC_FSYS_MMIO_SET(mmio, addr)      ((mmio) = (addr))
#else
#define CLKSRC_FSYS_MMIO_SET(mmio, addr)      do { } while (0)
#endif

	/* timekeeping specific data, ignore */
	cycle_t cycle_interval;
	u64	xtime_interval;
	u32	raw_interval;
	/*
	 * Second part is written at each timer interrupt
	 * Keep it in a different cache line to dirty no
	 * more than one cache line.
	 */
	cycle_t cycle_last ____cacheline_aligned_in_smp;
	u64 xtime_nsec;
	s64 error;
	struct timespec raw_time;

#ifdef CONFIG_CLOCKSOURCE_WATCHDOG
	/* Watchdog related data, used by the framework */
	struct list_head wd_list;
	cycle_t wd_last;
#endif
};

extern struct clocksource *clock;	/* current clocksource */

#define CLOCK_SOURCE_IS_CONTINUOUS		0x01
#define CLOCK_SOURCE_MUST_VERIFY		0x02

#define CLOCK_SOURCE_WATCHDOG			0x10
#define CLOCK_SOURCE_VALID_FOR_HRES		0x20

/* simplify initialization of mask field */
#define CLOCKSOURCE_MASK(bits) (cycle_t)((bits) < 64 ? ((1ULL<<(bits))-1) : -1)

static inline u32 clocksource_khz2mult(u32 khz, u32 shift_constant)
{
	/*  khz = cyc/(Million ns)
	 *  mult/2^shift  = ns/cyc
	 *  mult = ns/cyc * 2^shift
	 *  mult = 1Million/khz * 2^shift
	 *  mult = 1000000 * 2^shift / khz
	 *  mult = (1000000<<shift) / khz
	 */
	u64 tmp = ((u64)1000000) << shift_constant;

	tmp += khz/2; /* round for do_div */
	do_div(tmp, khz);

	return (u32)tmp;
}

static inline u32 clocksource_hz2mult(u32 hz, u32 shift_constant)
{
	/*  hz = cyc/(Billion ns)
	 *  mult/2^shift  = ns/cyc
	 *  mult = ns/cyc * 2^shift
	 *  mult = 1Billion/hz * 2^shift
	 *  mult = 1000000000 * 2^shift / hz
	 *  mult = (1000000000<<shift) / hz
	 */
	u64 tmp = ((u64)1000000000) << shift_constant;

	tmp += hz/2; /* round for do_div */
	do_div(tmp, hz);

	return (u32)tmp;
}

static inline cycle_t clocksource_read(struct clocksource *cs)
{
	return cs->read();
}

static inline s64 cyc2ns(struct clocksource *cs, cycle_t cycles)
{
	u64 ret = (u64)cycles;
	ret = (ret * cs->mult) >> cs->shift;
	return ret;
}

static inline void clocksource_calculate_interval(struct clocksource *c,
					  	  unsigned long length_nsec)
{
	u64 tmp;

	/* Do the ns -> cycle conversion first, using original mult */
	tmp = length_nsec;
	tmp <<= c->shift;
	tmp += c->mult_orig/2;
	do_div(tmp, c->mult_orig);

	c->cycle_interval = (cycle_t)tmp;
	if (c->cycle_interval == 0)
		c->cycle_interval = 1;

	/* Go back from cycles -> shifted ns, this time use ntp adjused mult */
	c->xtime_interval = (u64)c->cycle_interval * c->mult;
	c->raw_interval = ((u64)c->cycle_interval * c->mult_orig) >> c->shift;
}


/* used to install a new clocksource */
extern int clocksource_register(struct clocksource*);
extern void clocksource_unregister(struct clocksource*);
extern void clocksource_touch_watchdog(void);
extern struct clocksource* clocksource_get_next(void);
extern void clocksource_change_rating(struct clocksource *cs, int rating);
extern void clocksource_resume(void);

#ifdef CONFIG_GENERIC_TIME_VSYSCALL
extern void update_vsyscall(struct timespec *ts, struct clocksource *c);
extern void update_vsyscall_tz(void);
#else
static inline void update_vsyscall(struct timespec *ts, struct clocksource *c)
{
}

static inline void update_vsyscall_tz(void)
{
}
#endif

#endif /* _LINUX_CLOCKSOURCE_H */
