


#ifndef __ASM_ARCH_MXC_COMMON_H__
#define __ASM_ARCH_MXC_COMMON_H__

struct platform_device;

extern void mxc_map_io(void);
extern void mxc_init_irq(void);
extern void mxc_timer_init(const char *clk_timer);
extern int mxc_clocks_init(unsigned long fref);
extern int mxc_register_gpios(void);
extern int mxc_register_device(struct platform_device *pdev, void *data);

#endif
