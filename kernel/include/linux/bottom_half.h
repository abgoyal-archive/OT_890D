
#ifndef _LINUX_BH_H
#define _LINUX_BH_H

#include <asm/tcm.h>

extern void local_bh_disable(void);
extern void __tcmfunc _local_bh_enable(void);
extern void local_bh_enable(void);
extern void local_bh_enable_ip(unsigned long ip);

#endif /* _LINUX_BH_H */
