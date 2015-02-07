

#ifndef __ASM_ARCH_HARDWARE_H__
#error "Do not include this directly, instead #include <mach/hardware.h>"
#endif

#define FSG_SDA_PIN		12
#define FSG_SCL_PIN		13

#define FSG_PCI_MAX_DEV		3
#define FSG_PCI_IRQ_LINES	3


/* PCI controller GPIO to IRQ pin mappings */
#define FSG_PCI_INTA_PIN	6
#define FSG_PCI_INTB_PIN	7
#define FSG_PCI_INTC_PIN	5

/* Buttons */

#define FSG_SB_GPIO		4	/* sync button */
#define FSG_RB_GPIO		9	/* reset button */
#define FSG_UB_GPIO		10	/* usb button */

/* LEDs */

#define FSG_LED_WLAN_BIT	0
#define FSG_LED_WAN_BIT		1
#define FSG_LED_SATA_BIT	2
#define FSG_LED_USB_BIT		4
#define FSG_LED_RING_BIT	5
#define FSG_LED_SYNC_BIT	7
