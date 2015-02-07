
#ifndef ASM_X86__VMWARE_H
#define ASM_X86__VMWARE_H

extern unsigned long vmware_get_tsc_khz(void);
extern int vmware_platform(void);
extern void vmware_set_feature_bits(struct cpuinfo_x86 *c);

#endif
