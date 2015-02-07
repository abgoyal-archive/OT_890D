

#ifndef __ASM_ARCH_HARDWARE_H__
#error "Do not include this directly, instead #include <mach/hardware.h>"
#endif

#define DSMG600_SDA_PIN		5
#define DSMG600_SCL_PIN		4

#define DSMG600_PCI_MAX_DEV	4
#define DSMG600_PCI_IRQ_LINES	3


/* PCI controller GPIO to IRQ pin mappings */
#define DSMG600_PCI_INTA_PIN	11
#define DSMG600_PCI_INTB_PIN	10
#define DSMG600_PCI_INTC_PIN	9
#define DSMG600_PCI_INTD_PIN	8
#define DSMG600_PCI_INTE_PIN	7
#define DSMG600_PCI_INTF_PIN	6

/* DSM-G600 Timer Setting */
#define DSMG600_FREQ 66000000

/* Buttons */

#define DSMG600_PB_GPIO		15	/* power button */
#define DSMG600_RB_GPIO		3	/* reset button */

/* Power control */

#define DSMG600_PO_GPIO		2	/* power off */

/* LEDs */

#define DSMG600_LED_PWR_GPIO	0
#define DSMG600_LED_WLAN_GPIO	14
