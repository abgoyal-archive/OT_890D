

#ifndef __ARCH_ARM_MACH_OMAP_COMMON_H
#define __ARCH_ARM_MACH_OMAP_COMMON_H

#include <linux/i2c.h>

struct sys_timer;

extern void omap_map_common_io(void);
extern struct sys_timer omap_timer;
extern void omap_serial_init(void);
extern void omap_serial_enable_clocks(int enable);
#if defined(CONFIG_I2C_OMAP) || defined(CONFIG_I2C_OMAP_MODULE)
extern int omap_register_i2c_bus(int bus_id, u32 clkrate,
				 struct i2c_board_info const *info,
				 unsigned len);
#else
static inline int omap_register_i2c_bus(int bus_id, u32 clkrate,
				 struct i2c_board_info const *info,
				 unsigned len)
{
	return 0;
}
#endif

/* IO bases for various OMAP processors */
struct omap_globals {
	u32		class;		/* OMAP class to detect */
	void __iomem	*tap;		/* Control module ID code */
	void __iomem	*sdrc;		/* SDRAM Controller */
	void __iomem	*sms;		/* SDRAM Memory Scheduler */
	void __iomem	*ctrl;		/* System Control Module */
	void __iomem	*prm;		/* Power and Reset Management */
	void __iomem	*cm;		/* Clock Management */
};

void omap2_set_globals_242x(void);
void omap2_set_globals_243x(void);
void omap2_set_globals_343x(void);

/* These get called from omap2_set_globals_xxxx(), do not call these */
void omap2_set_globals_tap(struct omap_globals *);
void omap2_set_globals_memory(struct omap_globals *);
void omap2_set_globals_control(struct omap_globals *);
void omap2_set_globals_prcm(struct omap_globals *);

#endif /* __ARCH_ARM_MACH_OMAP_COMMON_H */
