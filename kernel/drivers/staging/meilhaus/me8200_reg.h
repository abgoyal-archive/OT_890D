


#ifndef _ME8200_REG_H_
#define _ME8200_REG_H_

#ifdef __KERNEL__

#define ME8200_IRQ_MODE_REG				0xD	// R/W

#define ME8200_IRQ_MODE_MASK     			0x3

#define ME8200_IRQ_MODE_MASK_MASK			0x0
#define ME8200_IRQ_MODE_MASK_COMPARE			0x1

#define ME8200_IRQ_MODE_BIT_ENABLE_POWER		0x10
#define ME8200_IRQ_MODE_BIT_CLEAR_POWER			0x40

#define ME8200_IRQ_MODE_DI_SHIFT			2
#define ME8200_IRQ_MODE_POWER_SHIFT			1

#endif
#endif
