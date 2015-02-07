

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <asm/delay.h>

#include "../w1.h"
#include "../w1_int.h"

static const unsigned short normal_i2c[] = { I2C_CLIENT_END };

I2C_CLIENT_INSMOD_1(ds2482);

#define DS2482_CMD_RESET		0xF0	/* No param */
#define DS2482_CMD_SET_READ_PTR		0xE1	/* Param: DS2482_PTR_CODE_xxx */
#define DS2482_CMD_CHANNEL_SELECT	0xC3	/* Param: Channel byte - DS2482-800 only */
#define DS2482_CMD_WRITE_CONFIG		0xD2	/* Param: Config byte */
#define DS2482_CMD_1WIRE_RESET		0xB4	/* Param: None */
#define DS2482_CMD_1WIRE_SINGLE_BIT	0x87	/* Param: Bit byte (bit7) */
#define DS2482_CMD_1WIRE_WRITE_BYTE	0xA5	/* Param: Data byte */
#define DS2482_CMD_1WIRE_READ_BYTE	0x96	/* Param: None */
/* Note to read the byte, Set the ReadPtr to Data then read (any addr) */
#define DS2482_CMD_1WIRE_TRIPLET	0x78	/* Param: Dir byte (bit7) */

/* Values for DS2482_CMD_SET_READ_PTR */
#define DS2482_PTR_CODE_STATUS		0xF0
#define DS2482_PTR_CODE_DATA		0xE1
#define DS2482_PTR_CODE_CHANNEL		0xD2	/* DS2482-800 only */
#define DS2482_PTR_CODE_CONFIG		0xC3

#define DS2482_REG_CFG_1WS		0x08
#define DS2482_REG_CFG_SPU		0x04
#define DS2482_REG_CFG_PPM		0x02
#define DS2482_REG_CFG_APU		0x01


static const u8 ds2482_chan_wr[8] =
	{ 0xF0, 0xE1, 0xD2, 0xC3, 0xB4, 0xA5, 0x96, 0x87 };
static const u8 ds2482_chan_rd[8] =
	{ 0xB8, 0xB1, 0xAA, 0xA3, 0x9C, 0x95, 0x8E, 0x87 };


#define DS2482_REG_STS_DIR		0x80
#define DS2482_REG_STS_TSB		0x40
#define DS2482_REG_STS_SBR		0x20
#define DS2482_REG_STS_RST		0x10
#define DS2482_REG_STS_LL		0x08
#define DS2482_REG_STS_SD		0x04
#define DS2482_REG_STS_PPD		0x02
#define DS2482_REG_STS_1WB		0x01


static int ds2482_probe(struct i2c_client *client,
			const struct i2c_device_id *id);
static int ds2482_detect(struct i2c_client *client, int kind,
			 struct i2c_board_info *info);
static int ds2482_remove(struct i2c_client *client);


static const struct i2c_device_id ds2482_id[] = {
	{ "ds2482", 0 },
	{ }
};

static struct i2c_driver ds2482_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "ds2482",
	},
	.probe		= ds2482_probe,
	.remove		= ds2482_remove,
	.id_table	= ds2482_id,
	.detect		= ds2482_detect,
	.address_data	= &addr_data,
};


struct ds2482_data;

struct ds2482_w1_chan {
	struct ds2482_data	*pdev;
	u8			channel;
	struct w1_bus_master	w1_bm;
};

struct ds2482_data {
	struct i2c_client	*client;
	struct mutex		access_lock;

	/* 1-wire interface(s) */
	int			w1_count;	/* 1 or 8 */
	struct ds2482_w1_chan	w1_ch[8];

	/* per-device values */
	u8			channel;
	u8			read_prt;	/* see DS2482_PTR_CODE_xxx */
	u8			reg_config;
};


static inline int ds2482_select_register(struct ds2482_data *pdev, u8 read_ptr)
{
	if (pdev->read_prt != read_ptr) {
		if (i2c_smbus_write_byte_data(pdev->client,
					      DS2482_CMD_SET_READ_PTR,
					      read_ptr) < 0)
			return -1;

		pdev->read_prt = read_ptr;
	}
	return 0;
}

static inline int ds2482_send_cmd(struct ds2482_data *pdev, u8 cmd)
{
	if (i2c_smbus_write_byte(pdev->client, cmd) < 0)
		return -1;

	pdev->read_prt = DS2482_PTR_CODE_STATUS;
	return 0;
}

static inline int ds2482_send_cmd_data(struct ds2482_data *pdev,
				       u8 cmd, u8 byte)
{
	if (i2c_smbus_write_byte_data(pdev->client, cmd, byte) < 0)
		return -1;

	/* all cmds leave in STATUS, except CONFIG */
	pdev->read_prt = (cmd != DS2482_CMD_WRITE_CONFIG) ?
			 DS2482_PTR_CODE_STATUS : DS2482_PTR_CODE_CONFIG;
	return 0;
}



#define DS2482_WAIT_IDLE_TIMEOUT	100

static int ds2482_wait_1wire_idle(struct ds2482_data *pdev)
{
	int temp = -1;
	int retries = 0;

	if (!ds2482_select_register(pdev, DS2482_PTR_CODE_STATUS)) {
		do {
			temp = i2c_smbus_read_byte(pdev->client);
		} while ((temp >= 0) && (temp & DS2482_REG_STS_1WB) &&
			 (++retries < DS2482_WAIT_IDLE_TIMEOUT));
	}

	if (retries > DS2482_WAIT_IDLE_TIMEOUT)
		printk(KERN_ERR "%s: timeout on channel %d\n",
		       __func__, pdev->channel);

	return temp;
}

static int ds2482_set_channel(struct ds2482_data *pdev, u8 channel)
{
	if (i2c_smbus_write_byte_data(pdev->client, DS2482_CMD_CHANNEL_SELECT,
				      ds2482_chan_wr[channel]) < 0)
		return -1;

	pdev->read_prt = DS2482_PTR_CODE_CHANNEL;
	pdev->channel = -1;
	if (i2c_smbus_read_byte(pdev->client) == ds2482_chan_rd[channel]) {
		pdev->channel = channel;
		return 0;
	}
	return -1;
}


static u8 ds2482_w1_touch_bit(void *data, u8 bit)
{
	struct ds2482_w1_chan *pchan = data;
	struct ds2482_data    *pdev = pchan->pdev;
	int status = -1;

	mutex_lock(&pdev->access_lock);

	/* Select the channel */
	ds2482_wait_1wire_idle(pdev);
	if (pdev->w1_count > 1)
		ds2482_set_channel(pdev, pchan->channel);

	/* Send the touch command, wait until 1WB == 0, return the status */
	if (!ds2482_send_cmd_data(pdev, DS2482_CMD_1WIRE_SINGLE_BIT,
				  bit ? 0xFF : 0))
		status = ds2482_wait_1wire_idle(pdev);

	mutex_unlock(&pdev->access_lock);

	return (status & DS2482_REG_STS_SBR) ? 1 : 0;
}

static u8 ds2482_w1_triplet(void *data, u8 dbit)
{
	struct ds2482_w1_chan *pchan = data;
	struct ds2482_data    *pdev = pchan->pdev;
	int status = (3 << 5);

	mutex_lock(&pdev->access_lock);

	/* Select the channel */
	ds2482_wait_1wire_idle(pdev);
	if (pdev->w1_count > 1)
		ds2482_set_channel(pdev, pchan->channel);

	/* Send the triplet command, wait until 1WB == 0, return the status */
	if (!ds2482_send_cmd_data(pdev, DS2482_CMD_1WIRE_TRIPLET,
				  dbit ? 0xFF : 0))
		status = ds2482_wait_1wire_idle(pdev);

	mutex_unlock(&pdev->access_lock);

	/* Decode the status */
	return (status >> 5);
}

static void ds2482_w1_write_byte(void *data, u8 byte)
{
	struct ds2482_w1_chan *pchan = data;
	struct ds2482_data    *pdev = pchan->pdev;

	mutex_lock(&pdev->access_lock);

	/* Select the channel */
	ds2482_wait_1wire_idle(pdev);
	if (pdev->w1_count > 1)
		ds2482_set_channel(pdev, pchan->channel);

	/* Send the write byte command */
	ds2482_send_cmd_data(pdev, DS2482_CMD_1WIRE_WRITE_BYTE, byte);

	mutex_unlock(&pdev->access_lock);
}

static u8 ds2482_w1_read_byte(void *data)
{
	struct ds2482_w1_chan *pchan = data;
	struct ds2482_data    *pdev = pchan->pdev;
	int result;

	mutex_lock(&pdev->access_lock);

	/* Select the channel */
	ds2482_wait_1wire_idle(pdev);
	if (pdev->w1_count > 1)
		ds2482_set_channel(pdev, pchan->channel);

	/* Send the read byte command */
	ds2482_send_cmd(pdev, DS2482_CMD_1WIRE_READ_BYTE);

	/* Wait until 1WB == 0 */
	ds2482_wait_1wire_idle(pdev);

	/* Select the data register */
	ds2482_select_register(pdev, DS2482_PTR_CODE_DATA);

	/* Read the data byte */
	result = i2c_smbus_read_byte(pdev->client);

	mutex_unlock(&pdev->access_lock);

	return result;
}


static u8 ds2482_w1_reset_bus(void *data)
{
	struct ds2482_w1_chan *pchan = data;
	struct ds2482_data    *pdev = pchan->pdev;
	int err;
	u8 retval = 1;

	mutex_lock(&pdev->access_lock);

	/* Select the channel */
	ds2482_wait_1wire_idle(pdev);
	if (pdev->w1_count > 1)
		ds2482_set_channel(pdev, pchan->channel);

	/* Send the reset command */
	err = ds2482_send_cmd(pdev, DS2482_CMD_1WIRE_RESET);
	if (err >= 0) {
		/* Wait until the reset is complete */
		err = ds2482_wait_1wire_idle(pdev);
		retval = !(err & DS2482_REG_STS_PPD);

		/* If the chip did reset since detect, re-config it */
		if (err & DS2482_REG_STS_RST)
			ds2482_send_cmd_data(pdev, DS2482_CMD_WRITE_CONFIG,
					     0xF0);
	}

	mutex_unlock(&pdev->access_lock);

	return retval;
}


static int ds2482_detect(struct i2c_client *client, int kind,
			 struct i2c_board_info *info)
{
	if (!i2c_check_functionality(client->adapter,
				     I2C_FUNC_SMBUS_WRITE_BYTE_DATA |
				     I2C_FUNC_SMBUS_BYTE))
		return -ENODEV;

	strlcpy(info->type, "ds2482", I2C_NAME_SIZE);

	return 0;
}

static int ds2482_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct ds2482_data *data;
	int err = -ENODEV;
	int temp1;
	int idx;

	if (!(data = kzalloc(sizeof(struct ds2482_data), GFP_KERNEL))) {
		err = -ENOMEM;
		goto exit;
	}

	data->client = client;
	i2c_set_clientdata(client, data);

	/* Reset the device (sets the read_ptr to status) */
	if (ds2482_send_cmd(data, DS2482_CMD_RESET) < 0) {
		dev_warn(&client->dev, "DS2482 reset failed.\n");
		goto exit_free;
	}

	/* Sleep at least 525ns to allow the reset to complete */
	ndelay(525);

	/* Read the status byte - only reset bit and line should be set */
	temp1 = i2c_smbus_read_byte(client);
	if (temp1 != (DS2482_REG_STS_LL | DS2482_REG_STS_RST)) {
		dev_warn(&client->dev, "DS2482 reset status "
			 "0x%02X - not a DS2482\n", temp1);
		goto exit_free;
	}

	/* Detect the 8-port version */
	data->w1_count = 1;
	if (ds2482_set_channel(data, 7) == 0)
		data->w1_count = 8;

	/* Set all config items to 0 (off) */
	ds2482_send_cmd_data(data, DS2482_CMD_WRITE_CONFIG, 0xF0);

	mutex_init(&data->access_lock);

	/* Register 1-wire interface(s) */
	for (idx = 0; idx < data->w1_count; idx++) {
		data->w1_ch[idx].pdev = data;
		data->w1_ch[idx].channel = idx;

		/* Populate all the w1 bus master stuff */
		data->w1_ch[idx].w1_bm.data       = &data->w1_ch[idx];
		data->w1_ch[idx].w1_bm.read_byte  = ds2482_w1_read_byte;
		data->w1_ch[idx].w1_bm.write_byte = ds2482_w1_write_byte;
		data->w1_ch[idx].w1_bm.touch_bit  = ds2482_w1_touch_bit;
		data->w1_ch[idx].w1_bm.triplet    = ds2482_w1_triplet;
		data->w1_ch[idx].w1_bm.reset_bus  = ds2482_w1_reset_bus;

		err = w1_add_master_device(&data->w1_ch[idx].w1_bm);
		if (err) {
			data->w1_ch[idx].pdev = NULL;
			goto exit_w1_remove;
		}
	}

	return 0;

exit_w1_remove:
	for (idx = 0; idx < data->w1_count; idx++) {
		if (data->w1_ch[idx].pdev != NULL)
			w1_remove_master_device(&data->w1_ch[idx].w1_bm);
	}
exit_free:
	kfree(data);
exit:
	return err;
}

static int ds2482_remove(struct i2c_client *client)
{
	struct ds2482_data   *data = i2c_get_clientdata(client);
	int idx;

	/* Unregister the 1-wire bridge(s) */
	for (idx = 0; idx < data->w1_count; idx++) {
		if (data->w1_ch[idx].pdev != NULL)
			w1_remove_master_device(&data->w1_ch[idx].w1_bm);
	}

	/* Free the memory */
	kfree(data);
	return 0;
}

static int __init sensors_ds2482_init(void)
{
	return i2c_add_driver(&ds2482_driver);
}

static void __exit sensors_ds2482_exit(void)
{
	i2c_del_driver(&ds2482_driver);
}

MODULE_AUTHOR("Ben Gardner <bgardner@wabtec.com>");
MODULE_DESCRIPTION("DS2482 driver");
MODULE_LICENSE("GPL");

module_init(sensors_ds2482_init);
module_exit(sensors_ds2482_exit);
