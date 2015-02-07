
#ifndef _ASM_IA64_XEN_EVENTS_H
#define _ASM_IA64_XEN_EVENTS_H

enum ipi_vector {
	XEN_RESCHEDULE_VECTOR,
	XEN_IPI_VECTOR,
	XEN_CMCP_VECTOR,
	XEN_CPEP_VECTOR,

	XEN_NR_IPIS,
};

static inline int xen_irqs_disabled(struct pt_regs *regs)
{
	return !(ia64_psr(regs)->i);
}

static inline void xen_do_IRQ(int irq, struct pt_regs *regs)
{
	struct pt_regs *old_regs;
	old_regs = set_irq_regs(regs);
	irq_enter();
	__do_IRQ(irq);
	irq_exit();
	set_irq_regs(old_regs);
}
#define irq_ctx_init(cpu)	do { } while (0)

#endif /* _ASM_IA64_XEN_EVENTS_H */
