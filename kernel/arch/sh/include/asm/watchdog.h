
#ifndef __ASM_SH_WATCHDOG_H
#define __ASM_SH_WATCHDOG_H
#ifdef __KERNEL__

#include <linux/types.h>
#include <cpu/watchdog.h>
#include <asm/io.h>

#ifndef WTCNT_R
#  define WTCNT_R	WTCNT
#endif

#ifndef WTCSR_R
#  define WTCSR_R	WTCSR
#endif

#define WTCNT_HIGH	0x5a
#define WTCSR_HIGH	0xa5

#define WTCSR_CKS2	0x04
#define WTCSR_CKS1	0x02
#define WTCSR_CKS0	0x01

#define WTCSR_CKS_32	0x00
#define WTCSR_CKS_64	0x01
#define WTCSR_CKS_128	0x02
#define WTCSR_CKS_256	0x03
#define WTCSR_CKS_512	0x04
#define WTCSR_CKS_1024	0x05
#define WTCSR_CKS_2048	0x06
#define WTCSR_CKS_4096	0x07

static inline __u8 sh_wdt_read_cnt(void)
{
	return ctrl_inb(WTCNT_R);
}

static inline void sh_wdt_write_cnt(__u8 val)
{
	ctrl_outw((WTCNT_HIGH << 8) | (__u16)val, WTCNT);
}

static inline __u8 sh_wdt_read_csr(void)
{
	return ctrl_inb(WTCSR_R);
}

static inline void sh_wdt_write_csr(__u8 val)
{
	ctrl_outw((WTCSR_HIGH << 8) | (__u16)val, WTCSR);
}

#endif /* __KERNEL__ */
#endif /* __ASM_SH_WATCHDOG_H */
