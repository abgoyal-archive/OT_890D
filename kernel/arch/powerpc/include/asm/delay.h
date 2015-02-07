
#ifndef _ASM_POWERPC_DELAY_H
#define _ASM_POWERPC_DELAY_H
#ifdef __KERNEL__


extern void __delay(unsigned long loops);
extern void udelay(unsigned long usecs);

#ifdef CONFIG_PPC64
#define mdelay(n)	udelay((n) * 1000)
#endif

#endif /* __KERNEL__ */
#endif /* _ASM_POWERPC_DELAY_H */
