

#ifndef __ASM_ARCH_OMAP_LDP_H
#define __ASM_ARCH_OMAP_LDP_H

extern void twl4030_bci_battery_init(void);

#define TWL4030_IRQNUM		INT_34XX_SYS_NIRQ
#define LDP_SMC911X_CS         1
#define LDP_SMC911X_GPIO       152
#define DEBUG_BASE             0x08000000
#define OMAP34XX_ETHR_START    DEBUG_BASE
#endif /* __ASM_ARCH_OMAP_LDP_H */
