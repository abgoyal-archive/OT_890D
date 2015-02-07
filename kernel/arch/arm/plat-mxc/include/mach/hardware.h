

#ifndef __ASM_ARCH_MXC_HARDWARE_H__
#define __ASM_ARCH_MXC_HARDWARE_H__

#include <asm/sizes.h>

#ifdef CONFIG_ARCH_MX3
# include <mach/mx31.h>
#endif

#ifdef CONFIG_ARCH_MX2
# ifdef CONFIG_MACH_MX27
#  include <mach/mx27.h>
# endif
#endif

#ifdef CONFIG_ARCH_MX1
# include <mach/mx1.h>
#endif

#include <mach/mxc.h>

#endif /* __ASM_ARCH_MXC_HARDWARE_H__ */
