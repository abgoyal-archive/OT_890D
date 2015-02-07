
#include <linux/sched.h>
#include <linux/percpu.h>
#include <linux/spinlock.h>
#include <linux/ktime.h>
#include <linux/module.h>

unsigned long long __attribute__((weak)) sched_clock(void)
{
	return (unsigned long long)jiffies * (NSEC_PER_SEC / HZ);
}

static __read_mostly int sched_clock_running;

#ifdef CONFIG_HAVE_UNSTABLE_SCHED_CLOCK

struct sched_clock_data {
	/*
	 * Raw spinlock - this is a special case: this might be called
	 * from within instrumentation code so we dont want to do any
	 * instrumentation ourselves.
	 */
	raw_spinlock_t		lock;

	u64			tick_raw;
	u64			tick_gtod;
	u64			clock;
};

static DEFINE_PER_CPU_SHARED_ALIGNED(struct sched_clock_data, sched_clock_data);

static inline struct sched_clock_data *this_scd(void)
{
	return &__get_cpu_var(sched_clock_data);
}

static inline struct sched_clock_data *cpu_sdc(int cpu)
{
	return &per_cpu(sched_clock_data, cpu);
}

void sched_clock_init(void)
{
	u64 ktime_now = ktime_to_ns(ktime_get());
	int cpu;

	for_each_possible_cpu(cpu) {
		struct sched_clock_data *scd = cpu_sdc(cpu);

		scd->lock = (raw_spinlock_t)__RAW_SPIN_LOCK_UNLOCKED;
		scd->tick_raw = 0;
		scd->tick_gtod = ktime_now;
		scd->clock = ktime_now;
	}

	sched_clock_running = 1;
}


static inline u64 wrap_min(u64 x, u64 y)
{
	return (s64)(x - y) < 0 ? x : y;
}

static inline u64 wrap_max(u64 x, u64 y)
{
	return (s64)(x - y) > 0 ? x : y;
}

static u64 __update_sched_clock(struct sched_clock_data *scd, u64 now)
{
	s64 delta = now - scd->tick_raw;
	u64 clock, min_clock, max_clock;

	WARN_ON_ONCE(!irqs_disabled());

	if (unlikely(delta < 0))
		delta = 0;

	/*
	 * scd->clock = clamp(scd->tick_gtod + delta,
	 * 		      max(scd->tick_gtod, scd->clock),
	 * 		      scd->tick_gtod + TICK_NSEC);
	 */

	clock = scd->tick_gtod + delta;
	min_clock = wrap_max(scd->tick_gtod, scd->clock);
	max_clock = wrap_max(scd->clock, scd->tick_gtod + TICK_NSEC);

	clock = wrap_max(clock, min_clock);
	clock = wrap_min(clock, max_clock);

	scd->clock = clock;

	return scd->clock;
}

static void lock_double_clock(struct sched_clock_data *data1,
				struct sched_clock_data *data2)
{
	if (data1 < data2) {
		__raw_spin_lock(&data1->lock);
		__raw_spin_lock(&data2->lock);
	} else {
		__raw_spin_lock(&data2->lock);
		__raw_spin_lock(&data1->lock);
	}
}

u64 sched_clock_cpu(int cpu)
{
	struct sched_clock_data *scd = cpu_sdc(cpu);
	u64 now, clock, this_clock, remote_clock;

	if (unlikely(!sched_clock_running))
		return 0ull;

	WARN_ON_ONCE(!irqs_disabled());
	now = sched_clock();

	if (cpu != raw_smp_processor_id()) {
		struct sched_clock_data *my_scd = this_scd();

		lock_double_clock(scd, my_scd);

		this_clock = __update_sched_clock(my_scd, now);
		remote_clock = scd->clock;

		/*
		 * Use the opportunity that we have both locks
		 * taken to couple the two clocks: we take the
		 * larger time as the latest time for both
		 * runqueues. (this creates monotonic movement)
		 */
		if (likely((s64)(remote_clock - this_clock) < 0)) {
			clock = this_clock;
			scd->clock = clock;
		} else {
			/*
			 * Should be rare, but possible:
			 */
			clock = remote_clock;
			my_scd->clock = remote_clock;
		}

		__raw_spin_unlock(&my_scd->lock);
	} else {
		__raw_spin_lock(&scd->lock);
		clock = __update_sched_clock(scd, now);
	}

	__raw_spin_unlock(&scd->lock);

	return clock;
}

void sched_clock_tick(void)
{
	struct sched_clock_data *scd = this_scd();
	u64 now, now_gtod;

	if (unlikely(!sched_clock_running))
		return;

	WARN_ON_ONCE(!irqs_disabled());

	now_gtod = ktime_to_ns(ktime_get());
	now = sched_clock();

	__raw_spin_lock(&scd->lock);
	scd->tick_raw = now;
	scd->tick_gtod = now_gtod;
	__update_sched_clock(scd, now);
	__raw_spin_unlock(&scd->lock);
}

void sched_clock_idle_sleep_event(void)
{
	sched_clock_cpu(smp_processor_id());
}
EXPORT_SYMBOL_GPL(sched_clock_idle_sleep_event);

void sched_clock_idle_wakeup_event(u64 delta_ns)
{
	if (timekeeping_suspended)
		return;

	sched_clock_tick();
	touch_softlockup_watchdog();
}
EXPORT_SYMBOL_GPL(sched_clock_idle_wakeup_event);

#else /* CONFIG_HAVE_UNSTABLE_SCHED_CLOCK */

void sched_clock_init(void)
{
	sched_clock_running = 1;
}

u64 sched_clock_cpu(int cpu)
{
	if (unlikely(!sched_clock_running))
		return 0;

	return sched_clock();
}

#endif

unsigned long long cpu_clock(int cpu)
{
	unsigned long long clock;
	unsigned long flags;

	local_irq_save(flags);
	clock = sched_clock_cpu(cpu);
	local_irq_restore(flags);

	return clock;
}
EXPORT_SYMBOL_GPL(cpu_clock);
