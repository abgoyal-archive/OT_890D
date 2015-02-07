

#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/percpu.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/sysdev.h>
#include <linux/clocksource.h>
#include <linux/jiffies.h>
#include <linux/time.h>
#include <linux/tick.h>


__cacheline_aligned_in_smp DEFINE_SEQLOCK(xtime_lock);


struct timespec xtime __attribute__ ((aligned (16)));
struct timespec wall_to_monotonic __attribute__ ((aligned (16)));
static unsigned long total_sleep_time;		/* seconds */

/* flag for if timekeeping is suspended */
int __read_mostly timekeeping_suspended;

static struct timespec xtime_cache __attribute__ ((aligned (16)));
void update_xtime_cache(u64 nsec)
{
	xtime_cache = xtime;
	timespec_add_ns(&xtime_cache, nsec);
}

struct clocksource *clock;


#ifdef CONFIG_GENERIC_TIME
static void clocksource_forward_now(void)
{
	cycle_t cycle_now, cycle_delta;
	s64 nsec;

	cycle_now = clocksource_read(clock);
	cycle_delta = (cycle_now - clock->cycle_last) & clock->mask;
	clock->cycle_last = cycle_now;

	nsec = cyc2ns(clock, cycle_delta);
	timespec_add_ns(&xtime, nsec);

	nsec = ((s64)cycle_delta * clock->mult_orig) >> clock->shift;
	clock->raw_time.tv_nsec += nsec;
}

void getnstimeofday(struct timespec *ts)
{
	cycle_t cycle_now, cycle_delta;
	unsigned long seq;
	s64 nsecs;

	WARN_ON(timekeeping_suspended);

	do {
		seq = read_seqbegin(&xtime_lock);

		*ts = xtime;

		/* read clocksource: */
		cycle_now = clocksource_read(clock);

		/* calculate the delta since the last update_wall_time: */
		cycle_delta = (cycle_now - clock->cycle_last) & clock->mask;

		/* convert to nanoseconds: */
		nsecs = cyc2ns(clock, cycle_delta);

	} while (read_seqretry(&xtime_lock, seq));

	timespec_add_ns(ts, nsecs);
}

EXPORT_SYMBOL(getnstimeofday);

void do_gettimeofday(struct timeval *tv)
{
	struct timespec now;

	getnstimeofday(&now);
	tv->tv_sec = now.tv_sec;
	tv->tv_usec = now.tv_nsec/1000;
}

EXPORT_SYMBOL(do_gettimeofday);
int do_settimeofday(struct timespec *tv)
{
	struct timespec ts_delta;
	unsigned long flags;

	if ((unsigned long)tv->tv_nsec >= NSEC_PER_SEC)
		return -EINVAL;

	write_seqlock_irqsave(&xtime_lock, flags);

	clocksource_forward_now();

	ts_delta.tv_sec = tv->tv_sec - xtime.tv_sec;
	ts_delta.tv_nsec = tv->tv_nsec - xtime.tv_nsec;
	wall_to_monotonic = timespec_sub(wall_to_monotonic, ts_delta);

	xtime = *tv;

	update_xtime_cache(0);

	clock->error = 0;
	ntp_clear();

	update_vsyscall(&xtime, clock);

	write_sequnlock_irqrestore(&xtime_lock, flags);

	/* signal hrtimers about time change */
	clock_was_set();

	return 0;
}

EXPORT_SYMBOL(do_settimeofday);

static void change_clocksource(void)
{
	struct clocksource *new;

	new = clocksource_get_next();

	if (clock == new)
		return;

	clocksource_forward_now();

	new->raw_time = clock->raw_time;

	clock = new;
	clock->cycle_last = 0;
	clock->cycle_last = clocksource_read(new);
	clock->error = 0;
	clock->xtime_nsec = 0;
	clocksource_calculate_interval(clock, NTP_INTERVAL_LENGTH);

	tick_clock_notify();

	/*
	 * We're holding xtime lock and waking up klogd would deadlock
	 * us on enqueue.  So no printing!
	printk(KERN_INFO "Time: %s clocksource has been installed.\n",
	       clock->name);
	 */
}
#else
static inline void clocksource_forward_now(void) { }
static inline void change_clocksource(void) { }
#endif

void getrawmonotonic(struct timespec *ts)
{
	unsigned long seq;
	s64 nsecs;
	cycle_t cycle_now, cycle_delta;

	do {
		seq = read_seqbegin(&xtime_lock);

		/* read clocksource: */
		cycle_now = clocksource_read(clock);

		/* calculate the delta since the last update_wall_time: */
		cycle_delta = (cycle_now - clock->cycle_last) & clock->mask;

		/* convert to nanoseconds: */
		nsecs = ((s64)cycle_delta * clock->mult_orig) >> clock->shift;

		*ts = clock->raw_time;

	} while (read_seqretry(&xtime_lock, seq));

	timespec_add_ns(ts, nsecs);
}
EXPORT_SYMBOL(getrawmonotonic);


int timekeeping_valid_for_hres(void)
{
	unsigned long seq;
	int ret;

	do {
		seq = read_seqbegin(&xtime_lock);

		ret = clock->flags & CLOCK_SOURCE_VALID_FOR_HRES;

	} while (read_seqretry(&xtime_lock, seq));

	return ret;
}

unsigned long __attribute__((weak)) read_persistent_clock(void)
{
	return 0;
}

void __init timekeeping_init(void)
{
	unsigned long flags;
	unsigned long sec = read_persistent_clock();

	write_seqlock_irqsave(&xtime_lock, flags);

	ntp_init();

	clock = clocksource_get_next();
	clocksource_calculate_interval(clock, NTP_INTERVAL_LENGTH);
	clock->cycle_last = clocksource_read(clock);

	xtime.tv_sec = sec;
	xtime.tv_nsec = 0;
	set_normalized_timespec(&wall_to_monotonic,
		-xtime.tv_sec, -xtime.tv_nsec);
	update_xtime_cache(0);
	total_sleep_time = 0;
	write_sequnlock_irqrestore(&xtime_lock, flags);
}

/* time in seconds when suspend began */
static unsigned long timekeeping_suspend_time;

static int timekeeping_resume(struct sys_device *dev)
{
	unsigned long flags;
	unsigned long now = read_persistent_clock();

	clocksource_resume();

	write_seqlock_irqsave(&xtime_lock, flags);

	if (now && (now > timekeeping_suspend_time)) {
		unsigned long sleep_length = now - timekeeping_suspend_time;

		xtime.tv_sec += sleep_length;
		wall_to_monotonic.tv_sec -= sleep_length;
		total_sleep_time += sleep_length;
	}
	update_xtime_cache(0);
	/* re-base the last cycle value */
	clock->cycle_last = 0;
	clock->cycle_last = clocksource_read(clock);
	clock->error = 0;
	timekeeping_suspended = 0;
	write_sequnlock_irqrestore(&xtime_lock, flags);

	touch_softlockup_watchdog();

	clockevents_notify(CLOCK_EVT_NOTIFY_RESUME, NULL);

	/* Resume hrtimers */
	hres_timers_resume();

	return 0;
}

static int timekeeping_suspend(struct sys_device *dev, pm_message_t state)
{
	unsigned long flags;

	timekeeping_suspend_time = read_persistent_clock();

	write_seqlock_irqsave(&xtime_lock, flags);
	clocksource_forward_now();
	timekeeping_suspended = 1;
	write_sequnlock_irqrestore(&xtime_lock, flags);

	clockevents_notify(CLOCK_EVT_NOTIFY_SUSPEND, NULL);

	return 0;
}

/* sysfs resume/suspend bits for timekeeping */
static struct sysdev_class timekeeping_sysclass = {
	.name		= "timekeeping",
	.resume		= timekeeping_resume,
	.suspend	= timekeeping_suspend,
};

static struct sys_device device_timer = {
	.id		= 0,
	.cls		= &timekeeping_sysclass,
};

static int __init timekeeping_init_device(void)
{
	int error = sysdev_class_register(&timekeeping_sysclass);
	if (!error)
		error = sysdev_register(&device_timer);
	return error;
}

device_initcall(timekeeping_init_device);

static __always_inline int clocksource_bigadjust(s64 error, s64 *interval,
						 s64 *offset)
{
	s64 tick_error, i;
	u32 look_ahead, adj;
	s32 error2, mult;

	/*
	 * Use the current error value to determine how much to look ahead.
	 * The larger the error the slower we adjust for it to avoid problems
	 * with losing too many ticks, otherwise we would overadjust and
	 * produce an even larger error.  The smaller the adjustment the
	 * faster we try to adjust for it, as lost ticks can do less harm
	 * here.  This is tuned so that an error of about 1 msec is adjusted
	 * within about 1 sec (or 2^20 nsec in 2^SHIFT_HZ ticks).
	 */
	error2 = clock->error >> (NTP_SCALE_SHIFT + 22 - 2 * SHIFT_HZ);
	error2 = abs(error2);
	for (look_ahead = 0; error2 > 0; look_ahead++)
		error2 >>= 2;

	/*
	 * Now calculate the error in (1 << look_ahead) ticks, but first
	 * remove the single look ahead already included in the error.
	 */
	tick_error = tick_length >> (NTP_SCALE_SHIFT - clock->shift + 1);
	tick_error -= clock->xtime_interval >> 1;
	error = ((error - tick_error) >> look_ahead) + tick_error;

	/* Finally calculate the adjustment shift value.  */
	i = *interval;
	mult = 1;
	if (error < 0) {
		error = -error;
		*interval = -*interval;
		*offset = -*offset;
		mult = -1;
	}
	for (adj = 0; error > i; adj++)
		error >>= 1;

	*interval <<= adj;
	*offset <<= adj;
	return mult << adj;
}

static void clocksource_adjust(s64 offset)
{
	s64 error, interval = clock->cycle_interval;
	int adj;

	error = clock->error >> (NTP_SCALE_SHIFT - clock->shift - 1);
	if (error > interval) {
		error >>= 2;
		if (likely(error <= interval))
			adj = 1;
		else
			adj = clocksource_bigadjust(error, &interval, &offset);
	} else if (error < -interval) {
		error >>= 2;
		if (likely(error >= -interval)) {
			adj = -1;
			interval = -interval;
			offset = -offset;
		} else
			adj = clocksource_bigadjust(error, &interval, &offset);
	} else
		return;

	clock->mult += adj;
	clock->xtime_interval += interval;
	clock->xtime_nsec -= offset;
	clock->error -= (interval - offset) <<
			(NTP_SCALE_SHIFT - clock->shift);
}

void update_wall_time(void)
{
	cycle_t offset;

	/* Make sure we're fully resumed: */
	if (unlikely(timekeeping_suspended))
		return;

#ifdef CONFIG_GENERIC_TIME
	offset = (clocksource_read(clock) - clock->cycle_last) & clock->mask;
#else
	offset = clock->cycle_interval;
#endif
	clock->xtime_nsec = (s64)xtime.tv_nsec << clock->shift;

	/* normally this loop will run just once, however in the
	 * case of lost or late ticks, it will accumulate correctly.
	 */
	while (offset >= clock->cycle_interval) {
		/* accumulate one interval */
		offset -= clock->cycle_interval;
		clock->cycle_last += clock->cycle_interval;

		clock->xtime_nsec += clock->xtime_interval;
		if (clock->xtime_nsec >= (u64)NSEC_PER_SEC << clock->shift) {
			clock->xtime_nsec -= (u64)NSEC_PER_SEC << clock->shift;
			xtime.tv_sec++;
			second_overflow();
		}

		clock->raw_time.tv_nsec += clock->raw_interval;
		if (clock->raw_time.tv_nsec >= NSEC_PER_SEC) {
			clock->raw_time.tv_nsec -= NSEC_PER_SEC;
			clock->raw_time.tv_sec++;
		}

		/* accumulate error between NTP and clock interval */
		clock->error += tick_length;
		clock->error -= clock->xtime_interval << (NTP_SCALE_SHIFT - clock->shift);
	}

	/* correct the clock when NTP error is too big */
	clocksource_adjust(offset);

	/*
	 * Since in the loop above, we accumulate any amount of time
	 * in xtime_nsec over a second into xtime.tv_sec, its possible for
	 * xtime_nsec to be fairly small after the loop. Further, if we're
	 * slightly speeding the clocksource up in clocksource_adjust(),
	 * its possible the required corrective factor to xtime_nsec could
	 * cause it to underflow.
	 *
	 * Now, we cannot simply roll the accumulated second back, since
	 * the NTP subsystem has been notified via second_overflow. So
	 * instead we push xtime_nsec forward by the amount we underflowed,
	 * and add that amount into the error.
	 *
	 * We'll correct this error next time through this function, when
	 * xtime_nsec is not as small.
	 */
	if (unlikely((s64)clock->xtime_nsec < 0)) {
		s64 neg = -(s64)clock->xtime_nsec;
		clock->xtime_nsec = 0;
		clock->error += neg << (NTP_SCALE_SHIFT - clock->shift);
	}

	/* store full nanoseconds into xtime after rounding it up and
	 * add the remainder to the error difference.
	 */
	xtime.tv_nsec = ((s64)clock->xtime_nsec >> clock->shift) + 1;
	clock->xtime_nsec -= (s64)xtime.tv_nsec << clock->shift;
	clock->error += clock->xtime_nsec << (NTP_SCALE_SHIFT - clock->shift);

	update_xtime_cache(cyc2ns(clock, offset));

	/* check to see if there is a new clocksource to use */
	change_clocksource();
	update_vsyscall(&xtime, clock);
}

void getboottime(struct timespec *ts)
{
	set_normalized_timespec(ts,
		- (wall_to_monotonic.tv_sec + total_sleep_time),
		- wall_to_monotonic.tv_nsec);
}

void monotonic_to_bootbased(struct timespec *ts)
{
	ts->tv_sec += total_sleep_time;
}

unsigned long get_seconds(void)
{
	return xtime_cache.tv_sec;
}
EXPORT_SYMBOL(get_seconds);


struct timespec current_kernel_time(void)
{
	struct timespec now;
	unsigned long seq;

	do {
		seq = read_seqbegin(&xtime_lock);

		now = xtime_cache;
	} while (read_seqretry(&xtime_lock, seq));

	return now;
}
EXPORT_SYMBOL(current_kernel_time);
