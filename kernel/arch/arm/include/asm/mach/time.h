
#ifndef __ASM_ARM_MACH_TIME_H
#define __ASM_ARM_MACH_TIME_H

#include <linux/sysdev.h>

struct sys_timer {
	struct sys_device	dev;
	void			(*init)(void);
	void			(*suspend)(void);
	void			(*resume)(void);
#ifndef CONFIG_GENERIC_TIME
	unsigned long		(*offset)(void);
#endif
};

extern struct sys_timer *system_timer;
extern void timer_tick(void);

struct timespec;
extern int (*set_rtc)(void);
extern void save_time_delta(struct timespec *delta, struct timespec *rtc);
extern void restore_time_delta(struct timespec *delta, struct timespec *rtc);

#endif
