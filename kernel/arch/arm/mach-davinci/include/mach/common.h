

#ifndef __ARCH_ARM_MACH_DAVINCI_COMMON_H
#define __ARCH_ARM_MACH_DAVINCI_COMMON_H

struct sys_timer;

extern struct sys_timer davinci_timer;

/* parameters describe VBUS sourcing for host mode */
extern void setup_usb(unsigned mA, unsigned potpgt_msec);

#endif /* __ARCH_ARM_MACH_DAVINCI_COMMON_H */
