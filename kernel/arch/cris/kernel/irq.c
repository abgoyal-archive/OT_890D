


#include <linux/module.h>
#include <linux/ptrace.h>
#include <linux/irq.h>

#include <linux/kernel_stat.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/timex.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/init.h>
#include <linux/seq_file.h>
#include <linux/errno.h>
#include <linux/spinlock.h>

#include <asm/io.h>

void ack_bad_irq(unsigned int irq)
{
	printk("unexpected IRQ trap at vector %02x\n", irq);
}

int show_interrupts(struct seq_file *p, void *v)
{
	int i = *(loff_t *) v, j;
	struct irqaction * action;
	unsigned long flags;

	if (i == 0) {
		seq_printf(p, "           ");
		for_each_online_cpu(j)
			seq_printf(p, "CPU%d       ",j);
		seq_putc(p, '\n');
	}

	if (i < NR_IRQS) {
		spin_lock_irqsave(&irq_desc[i].lock, flags);
		action = irq_desc[i].action;
		if (!action)
			goto skip;
		seq_printf(p, "%3d: ",i);
#ifndef CONFIG_SMP
		seq_printf(p, "%10u ", kstat_irqs(i));
#else
		for_each_online_cpu(j)
			seq_printf(p, "%10u ", kstat_cpu(j).irqs[i]);
#endif
		seq_printf(p, " %14s", irq_desc[i].chip->typename);
		seq_printf(p, "  %s", action->name);

		for (action=action->next; action; action = action->next)
			seq_printf(p, ", %s", action->name);

		seq_putc(p, '\n');
skip:
		spin_unlock_irqrestore(&irq_desc[i].lock, flags);
	}
	return 0;
}



asmlinkage void do_IRQ(int irq, struct pt_regs * regs)
{
	unsigned long sp;
	struct pt_regs *old_regs = set_irq_regs(regs);
	irq_enter();
	sp = rdsp();
	if (unlikely((sp & (PAGE_SIZE - 1)) < (PAGE_SIZE/8))) {
		printk("do_IRQ: stack overflow: %lX\n", sp);
		show_stack(NULL, (unsigned long *)sp);
	}
	__do_IRQ(irq);
        irq_exit();
	set_irq_regs(old_regs);
}

void weird_irq(void)
{
	local_irq_disable();
	printk("weird irq\n");
	while(1);
}

