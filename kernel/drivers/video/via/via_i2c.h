
#ifndef __VIA_I2C_H__
#define __VIA_I2C_H__

#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>

struct via_i2c_stuff {
	u16 i2c_port;			/* GPIO or I2C port */
	struct i2c_adapter adapter;
	struct i2c_algo_bit_data algo;
};

#define I2CPORT           0x3c4
#define I2CPORTINDEX      0x31
#define GPIOPORT          0x3C4
#define GPIOPORTINDEX     0x2C
#define I2C_BUS             1
#define GPIO_BUS            2
#define DELAYPORT           0x3C3

int viafb_i2c_readbyte(u8 slave_addr, u8 index, u8 *pdata);
int viafb_i2c_writebyte(u8 slave_addr, u8 index, u8 data);
int viafb_i2c_readbytes(u8 slave_addr, u8 index, u8 *buff, int buff_len);
int viafb_create_i2c_bus(void *par);
void viafb_delete_i2c_buss(void *par);
#endif /* __VIA_I2C_H__ */
