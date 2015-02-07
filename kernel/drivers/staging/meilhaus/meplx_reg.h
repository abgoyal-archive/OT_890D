


#ifndef _MEPLX_REG_H_
#define _MEPLX_REG_H_

#ifdef __KERNEL__

#define PLX_INTCSR				0x4C		/**< Interrupt control and status register. */
#define PLX_INTCSR_LOCAL_INT1_EN		0x01		/**< If set, local interrupt 1 is enabled (r/w). */
#define PLX_INTCSR_LOCAL_INT1_POL		0x02		/**< If set, local interrupt 1 polarity is active high (r/w). */
#define PLX_INTCSR_LOCAL_INT1_STATE		0x04		/**< If set, local interrupt 1 is active (r/_). */
#define PLX_INTCSR_LOCAL_INT2_EN		0x08		/**< If set, local interrupt 2 is enabled (r/w). */
#define PLX_INTCSR_LOCAL_INT2_POL		0x10		/**< If set, local interrupt 2 polarity is active high (r/w). */
#define PLX_INTCSR_LOCAL_INT2_STATE		0x20		/**< If set, local interrupt 2 is active  (r/_). */
#define PLX_INTCSR_PCI_INT_EN			0x40		/**< If set, PCI interrupt is enabled (r/w). */
#define PLX_INTCSR_SOFT_INT			0x80		/**< If set, a software interrupt is generated (r/w). */

#define PLX_ICR					0x50		/**< Initialization control register. */
#define PLX_ICR_BIT_EEPROM_CLOCK_SET		0x01000000
#define PLX_ICR_BIT_EEPROM_CHIP_SELECT		0x02000000
#define PLX_ICR_BIT_EEPROM_WRITE		0x04000000
#define PLX_ICR_BIT_EEPROM_READ			0x08000000
#define PLX_ICR_BIT_EEPROM_VALID		0x10000000

#define PLX_ICR_MASK_EEPROM			0x1F000000
#define EEPROM_DELAY				1

#endif
#endif
