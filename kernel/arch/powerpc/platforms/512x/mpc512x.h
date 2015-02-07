

#ifndef __MPC512X_H__
#define __MPC512X_H__
extern unsigned long mpc512x_find_ips_freq(struct device_node *node);
extern void __init mpc512x_init_IRQ(void);
void __init mpc512x_declare_of_platform_devices(void);
#endif				/* __MPC512X_H__ */
