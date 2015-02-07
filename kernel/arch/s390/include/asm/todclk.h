

#ifndef __ASM_TODCLK_H
#define __ASM_TODCLK_H

#ifdef __KERNEL__

#define TOD_uSEC (0x1000ULL)
#define TOD_mSEC (1000 * TOD_uSEC)
#define TOD_SEC (1000 * TOD_mSEC)
#define TOD_MIN (60 * TOD_SEC)
#define TOD_HOUR (60 * TOD_MIN)

#endif

#endif
