

#ifndef __ASM_ARCH_MXC_H__
#define __ASM_ARCH_MXC_H__

#ifndef __ASM_ARCH_MXC_HARDWARE_H__
#error "Do not include directly."
#endif

/* clean up all things that are not used */
#ifndef CONFIG_ARCH_MX3
# define cpu_is_mx31() (0)
#endif

#ifndef CONFIG_MACH_MX27
# define cpu_is_mx27() (0)
#endif

#if defined(CONFIG_ARCH_MX3) || defined(CONFIG_ARCH_MX2)
#define CSCR_U(n) (IO_ADDRESS(WEIM_BASE_ADDR) + n * 0x10)
#define CSCR_L(n) (IO_ADDRESS(WEIM_BASE_ADDR) + n * 0x10 + 0x4)
#define CSCR_A(n) (IO_ADDRESS(WEIM_BASE_ADDR) + n * 0x10 + 0x8)
#endif

#endif /*  __ASM_ARCH_MXC_H__ */
