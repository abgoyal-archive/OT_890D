

#ifndef _SLIC_OS_SPECIFIC_H_
#define _SLIC_OS_SPECIFIC_H_

#define FALSE               (0)
#define TRUE                (1)

#define SLIC_SECS_TO_JIFFS(x)  ((x) * HZ)
#define SLIC_MS_TO_JIFFIES(x)  (SLIC_SECS_TO_JIFFS((x)) / 1000)

#ifdef DEBUG_REGISTER_TRACE
#define WRITE_REG(reg, value, flush)                                      \
	{                                                           \
		adapter->card->reg_type[adapter->card->debug_ix] = 0;   \
		adapter->card->reg_offset[adapter->card->debug_ix] = \
		((unsigned char *)(&reg)) - \
		((unsigned char *)adapter->slic_regs); \
		adapter->card->reg_value[adapter->card->debug_ix++] = value;  \
		if (adapter->card->debug_ix == 32) \
			adapter->card->debug_ix = 0;                      \
		slic_reg32_write((&reg), (value), (flush));            \
	}
#define WRITE_REG64(a, reg, value, regh, valh, flush)                        \
	{                                                           \
		adapter->card->reg_type[adapter->card->debug_ix] = 1;        \
		adapter->card->reg_offset[adapter->card->debug_ix] = \
		((unsigned char *)(&reg)) - \
		((unsigned char *)adapter->slic_regs); \
		adapter->card->reg_value[adapter->card->debug_ix] = value;   \
		adapter->card->reg_valueh[adapter->card->debug_ix++] = valh;  \
		if (adapter->card->debug_ix == 32) \
			adapter->card->debug_ix = 0;                      \
		slic_reg64_write((a), (&reg), (value), (&regh), (valh), \
				(flush));\
	}
#else
#define WRITE_REG(reg, value, flush) \
	slic_reg32_write((&reg), (value), (flush))
#define WRITE_REG64(a, reg, value, regh, valh, flush) \
	slic_reg64_write((a), (&reg), (value), (&regh), (valh), (flush))
#endif

#endif  /* _SLIC_OS_SPECIFIC_H_  */

