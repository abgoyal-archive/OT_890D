

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/param.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/device.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/rtc.h>
#include <asm/machvec.h>
#include <mach/sysasic.h>

extern struct irq_chip systemasic_int;
extern void aica_time_init(void);
extern int gapspci_init(void);
extern int systemasic_irq_demux(int);

static void __init dreamcast_setup(char **cmdline_p)
{
	int i;

	/* Mask all hardware events */
	/* XXX */

	/* Acknowledge any previous events */
	/* XXX */

	__set_io_port_base(0xa0000000);

	/* Assign all virtual IRQs to the System ASIC int. handler */
	for (i = HW_EVENT_IRQ_BASE; i < HW_EVENT_IRQ_MAX; i++)
		set_irq_chip_and_handler(i, &systemasic_int,
					 handle_level_irq);

	board_time_init = aica_time_init;

#ifdef CONFIG_PCI
	if (gapspci_init() < 0)
		printk(KERN_WARNING "GAPSPCI was not detected.\n");
#endif
}

static struct sh_machine_vector mv_dreamcast __initmv = {
	.mv_name		= "Sega Dreamcast",
	.mv_setup		= dreamcast_setup,
	.mv_irq_demux		= systemasic_irq_demux,
};
