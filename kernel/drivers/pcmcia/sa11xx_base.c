

#include <linux/module.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>

#include <mach/hardware.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>

#include "soc_common.h"
#include "sa11xx_base.h"


static unsigned int
sa1100_pcmcia_default_mecr_timing(struct soc_pcmcia_socket *skt,
				  unsigned int cpu_speed,
				  unsigned int cmd_time)
{
	return sa1100_pcmcia_mecr_bs(cmd_time, cpu_speed);
}

static int
sa1100_pcmcia_set_mecr(struct soc_pcmcia_socket *skt, unsigned int cpu_clock)
{
	struct soc_pcmcia_timing timing;
	u32 mecr, old_mecr;
	unsigned long flags;
	unsigned int bs_io, bs_mem, bs_attr;

	soc_common_pcmcia_get_timing(skt, &timing);

	bs_io = skt->ops->get_timing(skt, cpu_clock, timing.io);
	bs_mem = skt->ops->get_timing(skt, cpu_clock, timing.mem);
	bs_attr = skt->ops->get_timing(skt, cpu_clock, timing.attr);

	local_irq_save(flags);

	old_mecr = mecr = MECR;
	MECR_FAST_SET(mecr, skt->nr, 0);
	MECR_BSIO_SET(mecr, skt->nr, bs_io);
	MECR_BSA_SET(mecr, skt->nr, bs_attr);
	MECR_BSM_SET(mecr, skt->nr, bs_mem);
	if (old_mecr != mecr)
		MECR = mecr;

	local_irq_restore(flags);

	debug(skt, 2, "FAST %X  BSM %X  BSA %X  BSIO %X\n",
	      MECR_FAST_GET(mecr, skt->nr),
	      MECR_BSM_GET(mecr, skt->nr), MECR_BSA_GET(mecr, skt->nr),
	      MECR_BSIO_GET(mecr, skt->nr));

	return 0;
}

#ifdef CONFIG_CPU_FREQ
static int
sa1100_pcmcia_frequency_change(struct soc_pcmcia_socket *skt,
			       unsigned long val,
			       struct cpufreq_freqs *freqs)
{
	switch (val) {
	case CPUFREQ_PRECHANGE:
		if (freqs->new > freqs->old)
			sa1100_pcmcia_set_mecr(skt, freqs->new);
		break;

	case CPUFREQ_POSTCHANGE:
		if (freqs->new < freqs->old)
			sa1100_pcmcia_set_mecr(skt, freqs->new);
		break;
	case CPUFREQ_RESUMECHANGE:
		sa1100_pcmcia_set_mecr(skt, freqs->new);
		break;
	}

	return 0;
}

#endif

static int
sa1100_pcmcia_set_timing(struct soc_pcmcia_socket *skt)
{
	return sa1100_pcmcia_set_mecr(skt, cpufreq_get(0));
}

static int
sa1100_pcmcia_show_timing(struct soc_pcmcia_socket *skt, char *buf)
{
	struct soc_pcmcia_timing timing;
	unsigned int clock = cpufreq_get(0);
	unsigned long mecr = MECR;
	char *p = buf;

	soc_common_pcmcia_get_timing(skt, &timing);

	p+=sprintf(p, "I/O      : %u (%u)\n", timing.io,
		   sa1100_pcmcia_cmd_time(clock, MECR_BSIO_GET(mecr, skt->nr)));

	p+=sprintf(p, "attribute: %u (%u)\n", timing.attr,
		   sa1100_pcmcia_cmd_time(clock, MECR_BSA_GET(mecr, skt->nr)));

	p+=sprintf(p, "common   : %u (%u)\n", timing.mem,
		   sa1100_pcmcia_cmd_time(clock, MECR_BSM_GET(mecr, skt->nr)));

	return p - buf;
}

int sa11xx_drv_pcmcia_probe(struct device *dev, struct pcmcia_low_level *ops,
			    int first, int nr)
{
	/*
	 * set default MECR calculation if the board specific
	 * code did not specify one...
	 */
	if (!ops->get_timing)
		ops->get_timing = sa1100_pcmcia_default_mecr_timing;

	/* Provide our SA11x0 specific timing routines. */
	ops->set_timing  = sa1100_pcmcia_set_timing;
	ops->show_timing = sa1100_pcmcia_show_timing;
#ifdef CONFIG_CPU_FREQ
	ops->frequency_change = sa1100_pcmcia_frequency_change;
#endif

	return soc_common_drv_pcmcia_probe(dev, ops, first, nr);
}
EXPORT_SYMBOL(sa11xx_drv_pcmcia_probe);

static int __init sa11xx_pcmcia_init(void)
{
	return 0;
}
fs_initcall(sa11xx_pcmcia_init);

static void __exit sa11xx_pcmcia_exit(void) {}

module_exit(sa11xx_pcmcia_exit);

MODULE_AUTHOR("John Dorsey <john+@cs.cmu.edu>");
MODULE_DESCRIPTION("Linux PCMCIA Card Services: SA-11xx core socket driver");
MODULE_LICENSE("Dual MPL/GPL");
