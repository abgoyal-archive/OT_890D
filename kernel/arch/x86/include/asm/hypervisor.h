
#ifndef ASM_X86__HYPERVISOR_H
#define ASM_X86__HYPERVISOR_H

extern unsigned long get_hypervisor_tsc_freq(void);
extern void init_hypervisor(struct cpuinfo_x86 *c);

#endif
