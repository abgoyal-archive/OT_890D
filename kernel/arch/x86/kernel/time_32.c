

#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/mca.h>

#include <asm/arch_hooks.h>
#include <asm/hpet.h>
#include <asm/time.h>
#include <asm/timer.h>

#include "do_timer.h"

int timer_ack;

unsigned long profile_pc(struct pt_regs *regs)
{
	unsigned long pc = instruction_pointer(regs);

#ifdef CONFIG_SMP
	if (!user_mode_vm(regs) && in_lock_functions(pc)) {
#ifdef CONFIG_FRAME_POINTER
		return *(unsigned long *)(regs->bp + sizeof(long));
#else
		unsigned long *sp = (unsigned long *)&regs->sp;

		/* Return address is either directly at stack pointer
		   or above a saved flags. Eflags has bits 22-31 zero,
		   kernel addresses don't. */
		if (sp[0] >> 22)
			return sp[0];
		if (sp[1] >> 22)
			return sp[1];
#endif
	}
#endif
	return pc;
}
EXPORT_SYMBOL(profile_pc);

irqreturn_t timer_interrupt(int irq, void *dev_id)
{
	/* Keep nmi watchdog up to date */
	inc_irq_stat(irq0_irqs);

#ifdef CONFIG_X86_IO_APIC
	if (timer_ack) {
		/*
		 * Subtle, when I/O APICs are used we have to ack timer IRQ
		 * manually to deassert NMI lines for the watchdog if run
		 * on an 82489DX-based system.
		 */
		spin_lock(&i8259A_lock);
		outb(0x0c, PIC_MASTER_OCW3);
		/* Ack the IRQ; AEOI will end it automatically. */
		inb(PIC_MASTER_POLL);
		spin_unlock(&i8259A_lock);
	}
#endif

	do_timer_interrupt_hook();

#ifdef CONFIG_MCA
	if (MCA_bus) {
		/* The PS/2 uses level-triggered interrupts.  You can't
		turn them off, nor would you want to (any attempt to
		enable edge-triggered interrupts usually gets intercepted by a
		special hardware circuit).  Hence we have to acknowledge
		the timer interrupt.  Through some incredibly stupid
		design idea, the reset for IRQ 0 is done by setting the
		high bit of the PPI port B (0x61).  Note that some PS/2s,
		notably the 55SX, work fine if this is removed.  */

		u8 irq_v = inb_p(0x61);		/* read the current state */
		outb_p(irq_v | 0x80, 0x61);	/* reset the IRQ */
	}
#endif

	return IRQ_HANDLED;
}

/* Duplicate of time_init() below, with hpet_enable part added */
void __init hpet_time_init(void)
{
	if (!hpet_enable())
		setup_pit_timer();
	time_init_hook();
}

void __init time_init(void)
{
	pre_time_init_hook();
	tsc_init();
	late_time_init = choose_time_init();
}
