

#ifndef __ASM_ARCH_HARDWARE_H__
#error "Do not include this directly, instead #include <mach/hardware.h>"
#endif

/* PCI controller GPIO to IRQ pin mappings */
#define	COYOTE_PCI_SLOT0_PIN	6
#define	COYOTE_PCI_SLOT1_PIN	11

#define	COYOTE_PCI_SLOT0_DEVID	14
#define	COYOTE_PCI_SLOT1_DEVID	15

#define	COYOTE_IDE_BASE_PHYS	IXP4XX_EXP_BUS_BASE(3)
#define	COYOTE_IDE_BASE_VIRT	0xFFFE1000
#define	COYOTE_IDE_REGION_SIZE	0x1000

#define	COYOTE_IDE_DATA_PORT	0xFFFE10E0
#define	COYOTE_IDE_CTRL_PORT	0xFFFE10FC
#define	COYOTE_IDE_ERROR_PORT	0xFFFE10E2

