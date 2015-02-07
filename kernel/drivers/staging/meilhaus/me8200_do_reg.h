


#ifndef _ME8200_DO_REG_H_
#define _ME8200_DO_REG_H_

#ifdef __KERNEL__

#define ME8200_DO_IRQ_STATUS_REG			0x0	// R
#define ME8200_DO_PORT_0_REG				0x1	// R/W
#define ME8200_DO_PORT_1_REG				0x2	// R/W

#define ME8200_DO_IRQ_STATUS_BIT_ACTIVE		0x1
#define ME8200_DO_IRQ_STATUS_SHIFT			1

#endif
#endif
