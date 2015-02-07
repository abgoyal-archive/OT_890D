

#ifndef __ASM_ARCH_HARDWARE_H__
#error "Do not include this directly, instead #include <mach/hardware.h>"
#endif

#define	PRPMC1100_FLASH_BASE	IXP4XX_EXP_BUS_CS0_BASE_PHYS
#define	PRPMC1100_FLASH_SIZE	IXP4XX_EXP_BUS_CSX_REGION_SIZE

#define	PRPMC1100_PCI_MIN_DEVID	10
#define	PRPMC1100_PCI_MAX_DEVID	16
#define	PRPMC1100_PCI_IRQ_LINES	4


/* PCI controller GPIO to IRQ pin mappings */
#define PRPMC1100_PCI_INTA_PIN	11
#define PRPMC1100_PCI_INTB_PIN	10
#define	PRPMC1100_PCI_INTC_PIN	9
#define	PRPMC1100_PCI_INTD_PIN	8


