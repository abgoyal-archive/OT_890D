


#ifndef _ME1400_EXT_IRQ_REG_H_
# define _ME1400_EXT_IRQ_REG_H_

# ifdef __KERNEL__

#  define PLX_INTCSR_REG				0x4C	/**< The PLX interrupt control and status register offset. */
#  define PLX_ICR_REG					0x50	/**< The PLX initialization control register offset. */

#  define PLX_LOCAL_INT1_EN				0x01	/**< If set the local interrupt 1 is enabled. */
#  define PLX_LOCAL_INT1_POL			0x02	/**< If set the local interrupt 1 polarity is high active. */
#  define PLX_LOCAL_INT1_STATE			0x04	/**< If set the local interrupt 1 is activ. */
#  define PLX_LOCAL_INT2_EN				0x08	/**< If set the local interrupt 2 is enabled. */
#  define PLX_LOCAL_INT2_POL			0x10	/**< If set the local interrupt 2 polarity is high active. */
#  define PLX_LOCAL_INT2_STATE			0x20	/**< If set the local interrupt 2 is activ. */
#  define PLX_PCI_INT_EN				0x40	/**< If set the PCI interrupt is enabled. */
#  define PLX_SOFT_INT					0x80	/**< If set an interrupt is generated. */

#  define ME1400AB_EXT_IRQ_CTRL_REG		0x11	/**< The external interrupt control register offset. */

#  define ME1400AB_EXT_IRQ_CLK_EN		0x01	/**< If this bit is set, the clock output is enabled. */
#  define ME1400AB_EXT_IRQ_IRQ_EN		0x02	/**< If set the external interrupt is enabled. Clearing this bit clears a pending interrupt. */

#  define ME1400CD_EXT_IRQ_CTRL_REG		0x11	/**< The external interrupt control register offset. */

#  define ME1400CD_EXT_IRQ_CLK_EN		0x10	/**< If set the external interrupt is enabled. Clearing this bit clears a pending interrupt.*/

# endif	//__KERNEL__

#endif //_ME1400_EXT_IRQ_REG_H_
