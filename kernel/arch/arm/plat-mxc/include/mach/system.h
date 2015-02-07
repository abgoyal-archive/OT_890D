

#ifndef __ASM_ARCH_MXC_SYSTEM_H__
#define __ASM_ARCH_MXC_SYSTEM_H__

static inline void arch_idle(void)
{
	cpu_do_idle();
}

static inline void arch_reset(char mode)
{
	cpu_reset(0);
}

#endif /* __ASM_ARCH_MXC_SYSTEM_H__ */
