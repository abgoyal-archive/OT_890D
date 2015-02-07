


#ifndef _ME8200_DIO_REG_H_
#define _ME8200_DIO_REG_H_

#ifdef __KERNEL__

#define ME8200_DIO_CTRL_REG					0x7	// R/W
#define ME8200_DIO_PORT_0_REG				0x8	// R/W
#define ME8200_DIO_PORT_1_REG				0x9	// R/W
#define ME8200_DIO_PORT_REG					ME8200_DIO_PORT_0_REG	// R/W

#define ME8200_DIO_CTRL_BIT_MODE_0			0x01
#define ME8200_DIO_CTRL_BIT_MODE_1			0x02
#define ME8200_DIO_CTRL_BIT_MODE_2			0x04
#define ME8200_DIO_CTRL_BIT_MODE_3			0x08

#endif
#endif
