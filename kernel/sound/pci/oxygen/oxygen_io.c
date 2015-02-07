

#include <linux/delay.h>
#include <linux/sched.h>
#include <sound/core.h>
#include <sound/mpu401.h>
#include <asm/io.h>
#include "oxygen.h"

u8 oxygen_read8(struct oxygen *chip, unsigned int reg)
{
	return inb(chip->addr + reg);
}
EXPORT_SYMBOL(oxygen_read8);

u16 oxygen_read16(struct oxygen *chip, unsigned int reg)
{
	return inw(chip->addr + reg);
}
EXPORT_SYMBOL(oxygen_read16);

u32 oxygen_read32(struct oxygen *chip, unsigned int reg)
{
	return inl(chip->addr + reg);
}
EXPORT_SYMBOL(oxygen_read32);

void oxygen_write8(struct oxygen *chip, unsigned int reg, u8 value)
{
	outb(value, chip->addr + reg);
	chip->saved_registers._8[reg] = value;
}
EXPORT_SYMBOL(oxygen_write8);

void oxygen_write16(struct oxygen *chip, unsigned int reg, u16 value)
{
	outw(value, chip->addr + reg);
	chip->saved_registers._16[reg / 2] = cpu_to_le16(value);
}
EXPORT_SYMBOL(oxygen_write16);

void oxygen_write32(struct oxygen *chip, unsigned int reg, u32 value)
{
	outl(value, chip->addr + reg);
	chip->saved_registers._32[reg / 4] = cpu_to_le32(value);
}
EXPORT_SYMBOL(oxygen_write32);

void oxygen_write8_masked(struct oxygen *chip, unsigned int reg,
			  u8 value, u8 mask)
{
	u8 tmp = inb(chip->addr + reg);
	tmp &= ~mask;
	tmp |= value & mask;
	outb(tmp, chip->addr + reg);
	chip->saved_registers._8[reg] = tmp;
}
EXPORT_SYMBOL(oxygen_write8_masked);

void oxygen_write16_masked(struct oxygen *chip, unsigned int reg,
			   u16 value, u16 mask)
{
	u16 tmp = inw(chip->addr + reg);
	tmp &= ~mask;
	tmp |= value & mask;
	outw(tmp, chip->addr + reg);
	chip->saved_registers._16[reg / 2] = cpu_to_le16(tmp);
}
EXPORT_SYMBOL(oxygen_write16_masked);

void oxygen_write32_masked(struct oxygen *chip, unsigned int reg,
			   u32 value, u32 mask)
{
	u32 tmp = inl(chip->addr + reg);
	tmp &= ~mask;
	tmp |= value & mask;
	outl(tmp, chip->addr + reg);
	chip->saved_registers._32[reg / 4] = cpu_to_le32(tmp);
}
EXPORT_SYMBOL(oxygen_write32_masked);

static int oxygen_ac97_wait(struct oxygen *chip, unsigned int mask)
{
	u8 status = 0;

	/*
	 * Reading the status register also clears the bits, so we have to save
	 * the read bits in status.
	 */
	wait_event_timeout(chip->ac97_waitqueue,
			   ({ status |= oxygen_read8(chip, OXYGEN_AC97_INTERRUPT_STATUS);
			      status & mask; }),
			   msecs_to_jiffies(1) + 1);
	/*
	 * Check even after a timeout because this function should not require
	 * the AC'97 interrupt to be enabled.
	 */
	status |= oxygen_read8(chip, OXYGEN_AC97_INTERRUPT_STATUS);
	return status & mask ? 0 : -EIO;
}


void oxygen_write_ac97(struct oxygen *chip, unsigned int codec,
		       unsigned int index, u16 data)
{
	unsigned int count, succeeded;
	u32 reg;

	reg = data;
	reg |= index << OXYGEN_AC97_REG_ADDR_SHIFT;
	reg |= OXYGEN_AC97_REG_DIR_WRITE;
	reg |= codec << OXYGEN_AC97_REG_CODEC_SHIFT;
	succeeded = 0;
	for (count = 5; count > 0; --count) {
		udelay(5);
		oxygen_write32(chip, OXYGEN_AC97_REGS, reg);
		/* require two "completed" writes, just to be sure */
		if (oxygen_ac97_wait(chip, OXYGEN_AC97_INT_WRITE_DONE) >= 0 &&
		    ++succeeded >= 2) {
			chip->saved_ac97_registers[codec][index / 2] = data;
			return;
		}
	}
	snd_printk(KERN_ERR "AC'97 write timeout\n");
}
EXPORT_SYMBOL(oxygen_write_ac97);

u16 oxygen_read_ac97(struct oxygen *chip, unsigned int codec,
		     unsigned int index)
{
	unsigned int count;
	unsigned int last_read = UINT_MAX;
	u32 reg;

	reg = index << OXYGEN_AC97_REG_ADDR_SHIFT;
	reg |= OXYGEN_AC97_REG_DIR_READ;
	reg |= codec << OXYGEN_AC97_REG_CODEC_SHIFT;
	for (count = 5; count > 0; --count) {
		udelay(5);
		oxygen_write32(chip, OXYGEN_AC97_REGS, reg);
		udelay(10);
		if (oxygen_ac97_wait(chip, OXYGEN_AC97_INT_READ_DONE) >= 0) {
			u16 value = oxygen_read16(chip, OXYGEN_AC97_REGS);
			/* we require two consecutive reads of the same value */
			if (value == last_read)
				return value;
			last_read = value;
			/*
			 * Invert the register value bits to make sure that two
			 * consecutive unsuccessful reads do not return the same
			 * value.
			 */
			reg ^= 0xffff;
		}
	}
	snd_printk(KERN_ERR "AC'97 read timeout on codec %u\n", codec);
	return 0;
}
EXPORT_SYMBOL(oxygen_read_ac97);

void oxygen_write_ac97_masked(struct oxygen *chip, unsigned int codec,
			      unsigned int index, u16 data, u16 mask)
{
	u16 value = oxygen_read_ac97(chip, codec, index);
	value &= ~mask;
	value |= data & mask;
	oxygen_write_ac97(chip, codec, index, value);
}
EXPORT_SYMBOL(oxygen_write_ac97_masked);

void oxygen_write_spi(struct oxygen *chip, u8 control, unsigned int data)
{
	unsigned int count;

	/* should not need more than 7.68 us (24 * 320 ns) */
	count = 10;
	while ((oxygen_read8(chip, OXYGEN_SPI_CONTROL) & OXYGEN_SPI_BUSY)
	       && count > 0) {
		udelay(1);
		--count;
	}

	oxygen_write8(chip, OXYGEN_SPI_DATA1, data);
	oxygen_write8(chip, OXYGEN_SPI_DATA2, data >> 8);
	if (control & OXYGEN_SPI_DATA_LENGTH_3)
		oxygen_write8(chip, OXYGEN_SPI_DATA3, data >> 16);
	oxygen_write8(chip, OXYGEN_SPI_CONTROL, control);
}
EXPORT_SYMBOL(oxygen_write_spi);

void oxygen_write_i2c(struct oxygen *chip, u8 device, u8 map, u8 data)
{
	unsigned long timeout;

	/* should not need more than about 300 us */
	timeout = jiffies + msecs_to_jiffies(1);
	do {
		if (!(oxygen_read16(chip, OXYGEN_2WIRE_BUS_STATUS)
		      & OXYGEN_2WIRE_BUSY))
			break;
		udelay(1);
		cond_resched();
	} while (time_after_eq(timeout, jiffies));

	oxygen_write8(chip, OXYGEN_2WIRE_MAP, map);
	oxygen_write8(chip, OXYGEN_2WIRE_DATA, data);
	oxygen_write8(chip, OXYGEN_2WIRE_CONTROL,
		      device | OXYGEN_2WIRE_DIR_WRITE);
}
EXPORT_SYMBOL(oxygen_write_i2c);

static void _write_uart(struct oxygen *chip, unsigned int port, u8 data)
{
	if (oxygen_read8(chip, OXYGEN_MPU401 + 1) & MPU401_TX_FULL)
		msleep(1);
	oxygen_write8(chip, OXYGEN_MPU401 + port, data);
}

void oxygen_reset_uart(struct oxygen *chip)
{
	_write_uart(chip, 1, MPU401_RESET);
	msleep(1); /* wait for ACK */
	_write_uart(chip, 1, MPU401_ENTER_UART);
}
EXPORT_SYMBOL(oxygen_reset_uart);

void oxygen_write_uart(struct oxygen *chip, u8 data)
{
	_write_uart(chip, 0, data);
}
EXPORT_SYMBOL(oxygen_write_uart);
