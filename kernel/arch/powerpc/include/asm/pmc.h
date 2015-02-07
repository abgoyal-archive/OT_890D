
#ifndef _POWERPC_PMC_H
#define _POWERPC_PMC_H
#ifdef __KERNEL__

#include <asm/ptrace.h>

typedef void (*perf_irq_t)(struct pt_regs *);
extern perf_irq_t perf_irq;

int reserve_pmc_hardware(perf_irq_t new_perf_irq);
void release_pmc_hardware(void);

#ifdef CONFIG_PPC64
void power4_enable_pmcs(void);
void pasemi_enable_pmcs(void);
#endif

#endif /* __KERNEL__ */
#endif /* _POWERPC_PMC_H */
