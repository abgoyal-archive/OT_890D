
#include <linux/clocksource.h>
#include <linux/init.h>

#include <asm/time.h>

#include <asm/octeon/octeon.h>
#include <asm/octeon/cvmx-ipd-defs.h>

void octeon_init_cvmcount(void)
{
	unsigned long flags;
	unsigned loops = 2;

	/* Clobber loops so GCC will not unroll the following while loop. */
	asm("" : "+r" (loops));

	local_irq_save(flags);
	/*
	 * Loop several times so we are executing from the cache,
	 * which should give more deterministic timing.
	 */
	while (loops--)
		write_c0_cvmcount(cvmx_read_csr(CVMX_IPD_CLK_COUNT));
	local_irq_restore(flags);
}

static cycle_t octeon_cvmcount_read(void)
{
	return read_c0_cvmcount();
}

static struct clocksource clocksource_mips = {
	.name		= "OCTEON_CVMCOUNT",
	.read		= octeon_cvmcount_read,
	.mask		= CLOCKSOURCE_MASK(64),
	.flags		= CLOCK_SOURCE_IS_CONTINUOUS,
};

void __init plat_time_init(void)
{
	clocksource_mips.rating = 300;
	clocksource_set_clock(&clocksource_mips, mips_hpt_frequency);
	clocksource_register(&clocksource_mips);
}
