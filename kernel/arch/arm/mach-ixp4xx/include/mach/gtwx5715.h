

#ifndef __ASM_ARCH_HARDWARE_H__
#error "Do not include this directly, instead #include <mach/hardware.h>"
#endif
#include "irqs.h"

#define GTWX5715_GPIO0	0
#define GTWX5715_GPIO1	1
#define GTWX5715_GPIO2	2
#define GTWX5715_GPIO3	3
#define GTWX5715_GPIO4	4
#define GTWX5715_GPIO5	5
#define GTWX5715_GPIO6	6
#define GTWX5715_GPIO7	7
#define GTWX5715_GPIO8	8
#define GTWX5715_GPIO9	9
#define GTWX5715_GPIO10	10
#define GTWX5715_GPIO11	11
#define GTWX5715_GPIO12	12
#define GTWX5715_GPIO13	13
#define GTWX5715_GPIO14	14

#define GTWX5715_GPIO0_IRQ			IRQ_IXP4XX_GPIO0
#define GTWX5715_GPIO1_IRQ			IRQ_IXP4XX_GPIO1
#define GTWX5715_GPIO2_IRQ			IRQ_IXP4XX_GPIO2
#define GTWX5715_GPIO3_IRQ			IRQ_IXP4XX_GPIO3
#define GTWX5715_GPIO4_IRQ			IRQ_IXP4XX_GPIO4
#define GTWX5715_GPIO5_IRQ			IRQ_IXP4XX_GPIO5
#define GTWX5715_GPIO6_IRQ			IRQ_IXP4XX_GPIO6
#define GTWX5715_GPIO7_IRQ			IRQ_IXP4XX_GPIO7
#define GTWX5715_GPIO8_IRQ			IRQ_IXP4XX_GPIO8
#define GTWX5715_GPIO9_IRQ			IRQ_IXP4XX_GPIO9
#define GTWX5715_GPIO10_IRQ		IRQ_IXP4XX_GPIO10
#define GTWX5715_GPIO11_IRQ		IRQ_IXP4XX_GPIO11
#define GTWX5715_GPIO12_IRQ		IRQ_IXP4XX_GPIO12
#define GTWX5715_GPIO13_IRQ		IRQ_IXP4XX_SW_INT1
#define GTWX5715_GPIO14_IRQ		IRQ_IXP4XX_SW_INT2


#define	GTWX5715_PCI_SLOT0_DEVID	0
#define	GTWX5715_PCI_SLOT0_INTA_GPIO	GTWX5715_GPIO10
#define	GTWX5715_PCI_SLOT0_INTB_GPIO	GTWX5715_GPIO11
#define	GTWX5715_PCI_SLOT0_INTA_IRQ	GTWX5715_GPIO10_IRQ
#define	GTWX5715_PCI_SLOT0_INTB_IRQ	GTWX5715_GPIO11_IRQ

#define	GTWX5715_PCI_SLOT1_DEVID	1
#define	GTWX5715_PCI_SLOT1_INTA_GPIO	GTWX5715_GPIO11
#define	GTWX5715_PCI_SLOT1_INTB_GPIO	GTWX5715_GPIO10
#define	GTWX5715_PCI_SLOT1_INTA_IRQ	GTWX5715_GPIO11_IRQ
#define	GTWX5715_PCI_SLOT1_INTB_IRQ	GTWX5715_GPIO10_IRQ

#define GTWX5715_PCI_SLOT_COUNT			2
#define GTWX5715_PCI_INT_PIN_COUNT		2


#define GTWX5715_KSSPI_SELECT	GTWX5715_GPIO5
#define GTWX5715_KSSPI_TXD		GTWX5715_GPIO6
#define GTWX5715_KSSPI_CLOCK	GTWX5715_GPIO7
#define GTWX5715_KSSPI_RXD		GTWX5715_GPIO12


#define GTWX5715_BUTTON_GPIO	GTWX5715_GPIO3
#define GTWX5715_BUTTON_IRQ	GTWX5715_GPIO3_IRQ


#define GTWX5715_LED1_GPIO		GTWX5715_GPIO2
#define GTWX5715_LED2_GPIO		GTWX5715_GPIO9
#define GTWX5715_LED3_GPIO		GTWX5715_GPIO8
#define GTWX5715_LED4_GPIO		GTWX5715_GPIO1
#define GTWX5715_LED9_GPIO		GTWX5715_GPIO4
