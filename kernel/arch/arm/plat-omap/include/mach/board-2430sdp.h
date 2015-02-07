

#ifndef __ASM_ARCH_OMAP_2430SDP_H
#define __ASM_ARCH_OMAP_2430SDP_H

/* Placeholder for 2430SDP specific defines */
#define OMAP24XX_ETHR_START		0x08000300
#define OMAP24XX_ETHR_GPIO_IRQ		149
#define SDP2430_CS0_BASE		0x04000000

/* Function prototypes */
extern void sdp2430_flash_init(void);
extern void sdp2430_usb_init(void);

#endif /* __ASM_ARCH_OMAP_2430SDP_H */
