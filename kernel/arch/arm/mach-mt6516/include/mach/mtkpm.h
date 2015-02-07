
#ifndef _MTKPM_H
#define _MTKPM_H


#include <linux/suspend.h>

int _Chip_pm_begin(void);
int _Chip_pm_prepare(void);
int _Chip_pm_enter(suspend_state_t state);
void _Chip_pm_end(void);
void _Chip_pm_finish(void);
void _Chip_PM_init(void);

#endif

