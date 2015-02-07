

#ifndef _LINUX_I2C_OCORES_H
#define _LINUX_I2C_OCORES_H

struct ocores_i2c_platform_data {
	u32 regstep;   /* distance between registers */
	u32 clock_khz; /* input clock in kHz */
};

#endif /* _LINUX_I2C_OCORES_H */
