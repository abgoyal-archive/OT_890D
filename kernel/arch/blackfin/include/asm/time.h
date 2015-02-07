

#ifndef _ASM_BLACKFIN_TIME_H
#define _ASM_BLACKFIN_TIME_H


#ifndef CONFIG_CPU_FREQ
#define TIME_SCALE 1
#define __bfin_cycles_off (0)
#define __bfin_cycles_mod (0)
#else
#define TIME_SCALE 4
extern unsigned long long __bfin_cycles_off;
extern unsigned int __bfin_cycles_mod;
#endif

#endif
