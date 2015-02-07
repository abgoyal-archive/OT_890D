


#ifndef _ME8100_DI_REG_H_
#define _ME8100_DI_REG_H_

#ifdef __KERNEL__

#define ME8100_RES_INT_REG_A		0x02	//(r, )
#define ME8100_DI_REG_A			0x04	//(r, )
#define ME8100_PATTERN_REG_A		0x08	//( ,w)
#define ME8100_MASK_REG_A		0x0A	//( ,w)
#define ME8100_INT_DI_REG_A		0x0A	//(r, )

#define ME8100_RES_INT_REG_B		0x0E	//(r, )
#define ME8100_DI_REG_B			0x10	//(r, )
#define ME8100_PATTERN_REG_B		0x14	//( ,w)
#define ME8100_MASK_REG_B		0x16	//( ,w)
#define ME8100_INT_DI_REG_B		0x16	//(r, )

#define ME8100_REG_OFFSET		0x0C

#endif
#endif
