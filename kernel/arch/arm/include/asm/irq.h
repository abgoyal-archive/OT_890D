
#ifndef __ASM_ARM_IRQ_H
#define __ASM_ARM_IRQ_H

#include <mach/irqs.h>
#include <asm/tcm.h>

#ifndef irq_canonicalize
#define irq_canonicalize(i)	(i)
#endif

#ifndef NO_IRQ
#define NO_IRQ	((unsigned int)(-1))
#endif

#ifndef __ASSEMBLY__
struct irqaction;
extern void migrate_irqs(void);

extern void __tcmfunc asm_do_IRQ(unsigned int, struct pt_regs *);
void init_IRQ(void);

#endif

#endif

