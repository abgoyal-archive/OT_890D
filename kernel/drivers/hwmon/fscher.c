


#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>


static const unsigned short normal_i2c[] = { 0x73, I2C_CLIENT_END };


I2C_CLIENT_INSMOD_1(fscher);


/* chip identification */
#define FSCHER_REG_IDENT_0		0x00
#define FSCHER_REG_IDENT_1		0x01
#define FSCHER_REG_IDENT_2		0x02
#define FSCHER_REG_REVISION		0x03

/* global control and status */
#define FSCHER_REG_EVENT_STATE		0x04
#define FSCHER_REG_CONTROL		0x05

/* watchdog */
#define FSCHER_REG_WDOG_PRESET		0x28
#define FSCHER_REG_WDOG_STATE		0x23
#define FSCHER_REG_WDOG_CONTROL		0x21

/* fan 0 */
#define FSCHER_REG_FAN0_MIN		0x55
#define FSCHER_REG_FAN0_ACT		0x0e
#define FSCHER_REG_FAN0_STATE		0x0d
#define FSCHER_REG_FAN0_RIPPLE		0x0f

/* fan 1 */
#define FSCHER_REG_FAN1_MIN		0x65
#define FSCHER_REG_FAN1_ACT		0x6b
#define FSCHER_REG_FAN1_STATE		0x62
#define FSCHER_REG_FAN1_RIPPLE		0x6f

/* fan 2 */
#define FSCHER_REG_FAN2_MIN		0xb5
#define FSCHER_REG_FAN2_ACT		0xbb
#define FSCHER_REG_FAN2_STATE		0xb2
#define FSCHER_REG_FAN2_RIPPLE		0xbf

/* voltage supervision */
#define FSCHER_REG_VOLT_12		0x45
#define FSCHER_REG_VOLT_5		0x42
#define FSCHER_REG_VOLT_BATT		0x48

/* temperature 0 */
#define FSCHER_REG_TEMP0_ACT		0x64
#define FSCHER_REG_TEMP0_STATE		0x71

/* temperature 1 */
#define FSCHER_REG_TEMP1_ACT		0x32
#define FSCHER_REG_TEMP1_STATE		0x81

/* temperature 2 */
#define FSCHER_REG_TEMP2_ACT		0x35
#define FSCHER_REG_TEMP2_STATE		0x91


static int fscher_probe(struct i2c_client *client,
			const struct i2c_device_id *id);
static int fscher_detect(struct i2c_client *client, int kind,
			 struct i2c_board_info *info);
static int fscher_remove(struct i2c_client *client);
static struct fscher_data *fscher_update_device(struct device *dev);
static void fscher_init_client(struct i2c_client *client);

static int fscher_read_value(struct i2c_client *client, u8 reg);
static int fscher_write_value(struct i2c_client *client, u8 reg, u8 value);

 
static const struct i2c_device_id fscher_id[] = {
	{ "fscher", fscher },
	{ }
};

static struct i2c_driver fscher_driver = {
	.class		= I2C_CLASS_HWMON,
	.driver = {
		.name	= "fscher",
	},
	.probe		= fscher_probe,
	.remove		= fscher_remove,
	.id_table	= fscher_id,
	.detect		= fscher_detect,
	.address_data	= &addr_data,
};


struct fscher_data {
	struct device *hwmon_dev;
	struct mutex update_lock;
	char valid; /* zero until following fields are valid */
	unsigned long last_updated; /* in jiffies */

	/* register values */
	u8 revision;		/* revision of chip */
	u8 global_event;	/* global event status */
	u8 global_control;	/* global control register */
	u8 watchdog[3];		/* watchdog */
	u8 volt[3];		/* 12, 5, battery voltage */ 
	u8 temp_act[3];		/* temperature */
	u8 temp_status[3];	/* status of sensor */
	u8 fan_act[3];		/* fans revolutions per second */
	u8 fan_status[3];	/* fan status */
	u8 fan_min[3];		/* fan min value for rps */
	u8 fan_ripple[3];	/* divider for rps */
};


#define sysfs_r(kind, sub, offset, reg) \
static ssize_t show_##kind##sub (struct fscher_data *, char *, int); \
static ssize_t show_##kind##offset##sub (struct device *, struct device_attribute *attr, char *); \
static ssize_t show_##kind##offset##sub (struct device *dev, struct device_attribute *attr, char *buf) \
{ \
	struct fscher_data *data = fscher_update_device(dev); \
	return show_##kind##sub(data, buf, (offset)); \
}

#define sysfs_w(kind, sub, offset, reg) \
static ssize_t set_##kind##sub (struct i2c_client *, struct fscher_data *, const char *, size_t, int, int); \
static ssize_t set_##kind##offset##sub (struct device *, struct device_attribute *attr, const char *, size_t); \
static ssize_t set_##kind##offset##sub (struct device *dev, struct device_attribute *attr, const char *buf, size_t count) \
{ \
	struct i2c_client *client = to_i2c_client(dev); \
	struct fscher_data *data = i2c_get_clientdata(client); \
	return set_##kind##sub(client, data, buf, count, (offset), reg); \
}

#define sysfs_rw_n(kind, sub, offset, reg) \
sysfs_r(kind, sub, offset, reg) \
sysfs_w(kind, sub, offset, reg) \
static DEVICE_ATTR(kind##offset##sub, S_IRUGO | S_IWUSR, show_##kind##offset##sub, set_##kind##offset##sub);

#define sysfs_rw(kind, sub, reg) \
sysfs_r(kind, sub, 0, reg) \
sysfs_w(kind, sub, 0, reg) \
static DEVICE_ATTR(kind##sub, S_IRUGO | S_IWUSR, show_##kind##0##sub, set_##kind##0##sub);

#define sysfs_ro_n(kind, sub, offset, reg) \
sysfs_r(kind, sub, offset, reg) \
static DEVICE_ATTR(kind##offset##sub, S_IRUGO, show_##kind##offset##sub, NULL);

#define sysfs_ro(kind, sub, reg) \
sysfs_r(kind, sub, 0, reg) \
static DEVICE_ATTR(kind, S_IRUGO, show_##kind##0##sub, NULL);

#define sysfs_fan(offset, reg_status, reg_min, reg_ripple, reg_act) \
sysfs_rw_n(pwm,        , offset, reg_min) \
sysfs_rw_n(fan, _status, offset, reg_status) \
sysfs_rw_n(fan, _div   , offset, reg_ripple) \
sysfs_ro_n(fan, _input , offset, reg_act)

#define sysfs_temp(offset, reg_status, reg_act) \
sysfs_rw_n(temp, _status, offset, reg_status) \
sysfs_ro_n(temp, _input , offset, reg_act)
    
#define sysfs_in(offset, reg_act) \
sysfs_ro_n(in, _input, offset, reg_act)

#define sysfs_revision(reg_revision) \
sysfs_ro(revision, , reg_revision)

#define sysfs_alarms(reg_events) \
sysfs_ro(alarms, , reg_events)

#define sysfs_control(reg_control) \
sysfs_rw(control, , reg_control)

#define sysfs_watchdog(reg_control, reg_status, reg_preset) \
sysfs_rw(watchdog, _control, reg_control) \
sysfs_rw(watchdog, _status , reg_status) \
sysfs_rw(watchdog, _preset , reg_preset)

sysfs_fan(1, FSCHER_REG_FAN0_STATE, FSCHER_REG_FAN0_MIN,
	     FSCHER_REG_FAN0_RIPPLE, FSCHER_REG_FAN0_ACT)
sysfs_fan(2, FSCHER_REG_FAN1_STATE, FSCHER_REG_FAN1_MIN,
	     FSCHER_REG_FAN1_RIPPLE, FSCHER_REG_FAN1_ACT)
sysfs_fan(3, FSCHER_REG_FAN2_STATE, FSCHER_REG_FAN2_MIN,
	     FSCHER_REG_FAN2_RIPPLE, FSCHER_REG_FAN2_ACT)

sysfs_temp(1, FSCHER_REG_TEMP0_STATE, FSCHER_REG_TEMP0_ACT)
sysfs_temp(2, FSCHER_REG_TEMP1_STATE, FSCHER_REG_TEMP1_ACT)
sysfs_temp(3, FSCHER_REG_TEMP2_STATE, FSCHER_REG_TEMP2_ACT)

sysfs_in(0, FSCHER_REG_VOLT_12)
sysfs_in(1, FSCHER_REG_VOLT_5)
sysfs_in(2, FSCHER_REG_VOLT_BATT)

sysfs_revision(FSCHER_REG_REVISION)
sysfs_alarms(FSCHER_REG_EVENTS)
sysfs_control(FSCHER_REG_CONTROL)
sysfs_watchdog(FSCHER_REG_WDOG_CONTROL, FSCHER_REG_WDOG_STATE, FSCHER_REG_WDOG_PRESET)
  
static struct attribute *fscher_attributes[] = {
	&dev_attr_revision.attr,
	&dev_attr_alarms.attr,
	&dev_attr_control.attr,

	&dev_attr_watchdog_status.attr,
	&dev_attr_watchdog_control.attr,
	&dev_attr_watchdog_preset.attr,

	&dev_attr_in0_input.attr,
	&dev_attr_in1_input.attr,
	&dev_attr_in2_input.attr,

	&dev_attr_fan1_status.attr,
	&dev_attr_fan1_div.attr,
	&dev_attr_fan1_input.attr,
	&dev_attr_pwm1.attr,
	&dev_attr_fan2_status.attr,
	&dev_attr_fan2_div.attr,
	&dev_attr_fan2_input.attr,
	&dev_attr_pwm2.attr,
	&dev_attr_fan3_status.attr,
	&dev_attr_fan3_div.attr,
	&dev_attr_fan3_input.attr,
	&dev_attr_pwm3.attr,

	&dev_attr_temp1_status.attr,
	&dev_attr_temp1_input.attr,
	&dev_attr_temp2_status.attr,
	&dev_attr_temp2_input.attr,
	&dev_attr_temp3_status.attr,
	&dev_attr_temp3_input.attr,
	NULL
};

static const struct attribute_group fscher_group = {
	.attrs = fscher_attributes,
};


/* Return 0 if detection is successful, -ENODEV otherwise */
static int fscher_detect(struct i2c_client *new_client, int kind,
			 struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = new_client->adapter;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -ENODEV;

	/* Do the remaining detection unless force or force_fscher parameter */
	if (kind < 0) {
		if ((i2c_smbus_read_byte_data(new_client,
		     FSCHER_REG_IDENT_0) != 0x48)	/* 'H' */
		 || (i2c_smbus_read_byte_data(new_client,
		     FSCHER_REG_IDENT_1) != 0x45)	/* 'E' */
		 || (i2c_smbus_read_byte_data(new_client,
		     FSCHER_REG_IDENT_2) != 0x52))	/* 'R' */
			return -ENODEV;
	}

	strlcpy(info->type, "fscher", I2C_NAME_SIZE);

	return 0;
}

static int fscher_probe(struct i2c_client *new_client,
			const struct i2c_device_id *id)
{
	struct fscher_data *data;
	int err;

	data = kzalloc(sizeof(struct fscher_data), GFP_KERNEL);
	if (!data) {
		err = -ENOMEM;
		goto exit;
	}

	i2c_set_clientdata(new_client, data);
	data->valid = 0;
	mutex_init(&data->update_lock);

	fscher_init_client(new_client);

	/* Register sysfs hooks */
	if ((err = sysfs_create_group(&new_client->dev.kobj, &fscher_group)))
		goto exit_free;

	data->hwmon_dev = hwmon_device_register(&new_client->dev);
	if (IS_ERR(data->hwmon_dev)) {
		err = PTR_ERR(data->hwmon_dev);
		goto exit_remove_files;
	}

	return 0;

exit_remove_files:
	sysfs_remove_group(&new_client->dev.kobj, &fscher_group);
exit_free:
	kfree(data);
exit:
	return err;
}

static int fscher_remove(struct i2c_client *client)
{
	struct fscher_data *data = i2c_get_clientdata(client);

	hwmon_device_unregister(data->hwmon_dev);
	sysfs_remove_group(&client->dev.kobj, &fscher_group);

	kfree(data);
	return 0;
}

static int fscher_read_value(struct i2c_client *client, u8 reg)
{
	dev_dbg(&client->dev, "read reg 0x%02x\n", reg);

	return i2c_smbus_read_byte_data(client, reg);
}

static int fscher_write_value(struct i2c_client *client, u8 reg, u8 value)
{
	dev_dbg(&client->dev, "write reg 0x%02x, val 0x%02x\n",
		reg, value);

	return i2c_smbus_write_byte_data(client, reg, value);
}

/* Called when we have found a new FSC Hermes. */
static void fscher_init_client(struct i2c_client *client)
{
	struct fscher_data *data = i2c_get_clientdata(client);

	/* Read revision from chip */
	data->revision =  fscher_read_value(client, FSCHER_REG_REVISION);
}

static struct fscher_data *fscher_update_device(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct fscher_data *data = i2c_get_clientdata(client);

	mutex_lock(&data->update_lock);

	if (time_after(jiffies, data->last_updated + 2 * HZ) || !data->valid) {

		dev_dbg(&client->dev, "Starting fscher update\n");

		data->temp_act[0] = fscher_read_value(client, FSCHER_REG_TEMP0_ACT);
		data->temp_act[1] = fscher_read_value(client, FSCHER_REG_TEMP1_ACT);
		data->temp_act[2] = fscher_read_value(client, FSCHER_REG_TEMP2_ACT);
		data->temp_status[0] = fscher_read_value(client, FSCHER_REG_TEMP0_STATE);
		data->temp_status[1] = fscher_read_value(client, FSCHER_REG_TEMP1_STATE);
		data->temp_status[2] = fscher_read_value(client, FSCHER_REG_TEMP2_STATE);

		data->volt[0] = fscher_read_value(client, FSCHER_REG_VOLT_12);
		data->volt[1] = fscher_read_value(client, FSCHER_REG_VOLT_5);
		data->volt[2] = fscher_read_value(client, FSCHER_REG_VOLT_BATT);

		data->fan_act[0] = fscher_read_value(client, FSCHER_REG_FAN0_ACT);
		data->fan_act[1] = fscher_read_value(client, FSCHER_REG_FAN1_ACT);
		data->fan_act[2] = fscher_read_value(client, FSCHER_REG_FAN2_ACT);
		data->fan_status[0] = fscher_read_value(client, FSCHER_REG_FAN0_STATE);
		data->fan_status[1] = fscher_read_value(client, FSCHER_REG_FAN1_STATE);
		data->fan_status[2] = fscher_read_value(client, FSCHER_REG_FAN2_STATE);
		data->fan_min[0] = fscher_read_value(client, FSCHER_REG_FAN0_MIN);
		data->fan_min[1] = fscher_read_value(client, FSCHER_REG_FAN1_MIN);
		data->fan_min[2] = fscher_read_value(client, FSCHER_REG_FAN2_MIN);
		data->fan_ripple[0] = fscher_read_value(client, FSCHER_REG_FAN0_RIPPLE);
		data->fan_ripple[1] = fscher_read_value(client, FSCHER_REG_FAN1_RIPPLE);
		data->fan_ripple[2] = fscher_read_value(client, FSCHER_REG_FAN2_RIPPLE);

		data->watchdog[0] = fscher_read_value(client, FSCHER_REG_WDOG_PRESET);
		data->watchdog[1] = fscher_read_value(client, FSCHER_REG_WDOG_STATE);
		data->watchdog[2] = fscher_read_value(client, FSCHER_REG_WDOG_CONTROL);

		data->global_event = fscher_read_value(client, FSCHER_REG_EVENT_STATE);
		data->global_control = fscher_read_value(client,
							FSCHER_REG_CONTROL);

		data->last_updated = jiffies;
		data->valid = 1;                 
	}

	mutex_unlock(&data->update_lock);

	return data;
}



#define FAN_INDEX_FROM_NUM(nr)	((nr) - 1)

static ssize_t set_fan_status(struct i2c_client *client, struct fscher_data *data,
			      const char *buf, size_t count, int nr, int reg)
{
	/* bits 0..1, 3..7 reserved => mask with 0x04 */  
	unsigned long v = simple_strtoul(buf, NULL, 10) & 0x04;
	
	mutex_lock(&data->update_lock);
	data->fan_status[FAN_INDEX_FROM_NUM(nr)] &= ~v;
	fscher_write_value(client, reg, v);
	mutex_unlock(&data->update_lock);
	return count;
}

static ssize_t show_fan_status(struct fscher_data *data, char *buf, int nr)
{
	/* bits 0..1, 3..7 reserved => mask with 0x04 */  
	return sprintf(buf, "%u\n", data->fan_status[FAN_INDEX_FROM_NUM(nr)] & 0x04);
}

static ssize_t set_pwm(struct i2c_client *client, struct fscher_data *data,
		       const char *buf, size_t count, int nr, int reg)
{
	unsigned long v = simple_strtoul(buf, NULL, 10);

	mutex_lock(&data->update_lock);
	data->fan_min[FAN_INDEX_FROM_NUM(nr)] = v > 0xff ? 0xff : v;
	fscher_write_value(client, reg, data->fan_min[FAN_INDEX_FROM_NUM(nr)]);
	mutex_unlock(&data->update_lock);
	return count;
}

static ssize_t show_pwm(struct fscher_data *data, char *buf, int nr)
{
	return sprintf(buf, "%u\n", data->fan_min[FAN_INDEX_FROM_NUM(nr)]);
}

static ssize_t set_fan_div(struct i2c_client *client, struct fscher_data *data,
			   const char *buf, size_t count, int nr, int reg)
{
	/* supported values: 2, 4, 8 */
	unsigned long v = simple_strtoul(buf, NULL, 10);

	switch (v) {
	case 2: v = 1; break;
	case 4: v = 2; break;
	case 8: v = 3; break;
	default:
		dev_err(&client->dev, "fan_div value %ld not "
			 "supported. Choose one of 2, 4 or 8!\n", v);
		return -EINVAL;
	}

	mutex_lock(&data->update_lock);

	/* bits 2..7 reserved => mask with 0x03 */
	data->fan_ripple[FAN_INDEX_FROM_NUM(nr)] &= ~0x03;
	data->fan_ripple[FAN_INDEX_FROM_NUM(nr)] |= v;

	fscher_write_value(client, reg, data->fan_ripple[FAN_INDEX_FROM_NUM(nr)]);
	mutex_unlock(&data->update_lock);
	return count;
}

static ssize_t show_fan_div(struct fscher_data *data, char *buf, int nr)
{
	/* bits 2..7 reserved => mask with 0x03 */  
	return sprintf(buf, "%u\n", 1 << (data->fan_ripple[FAN_INDEX_FROM_NUM(nr)] & 0x03));
}

#define RPM_FROM_REG(val)	(val*60)

static ssize_t show_fan_input (struct fscher_data *data, char *buf, int nr)
{
	return sprintf(buf, "%u\n", RPM_FROM_REG(data->fan_act[FAN_INDEX_FROM_NUM(nr)]));
}



#define TEMP_INDEX_FROM_NUM(nr)		((nr) - 1)

static ssize_t set_temp_status(struct i2c_client *client, struct fscher_data *data,
			       const char *buf, size_t count, int nr, int reg)
{
	/* bits 2..7 reserved, 0 read only => mask with 0x02 */  
	unsigned long v = simple_strtoul(buf, NULL, 10) & 0x02;

	mutex_lock(&data->update_lock);
	data->temp_status[TEMP_INDEX_FROM_NUM(nr)] &= ~v;
	fscher_write_value(client, reg, v);
	mutex_unlock(&data->update_lock);
	return count;
}

static ssize_t show_temp_status(struct fscher_data *data, char *buf, int nr)
{
	/* bits 2..7 reserved => mask with 0x03 */
	return sprintf(buf, "%u\n", data->temp_status[TEMP_INDEX_FROM_NUM(nr)] & 0x03);
}

#define TEMP_FROM_REG(val)	(((val) - 128) * 1000)

static ssize_t show_temp_input(struct fscher_data *data, char *buf, int nr)
{
	return sprintf(buf, "%d\n", TEMP_FROM_REG(data->temp_act[TEMP_INDEX_FROM_NUM(nr)]));
}

#define VOLT_FROM_REG(val)	((val) * 10)

static ssize_t show_in_input(struct fscher_data *data, char *buf, int nr)
{
	return sprintf(buf, "%u\n", VOLT_FROM_REG(data->volt[nr]));
}



static ssize_t show_revision(struct fscher_data *data, char *buf, int nr)
{
	return sprintf(buf, "%u\n", data->revision);
}



static ssize_t show_alarms(struct fscher_data *data, char *buf, int nr)
{
	/* bits 2, 5..6 reserved => mask with 0x9b */
	return sprintf(buf, "%u\n", data->global_event & 0x9b);
}



static ssize_t set_control(struct i2c_client *client, struct fscher_data *data,
			   const char *buf, size_t count, int nr, int reg)
{
	/* bits 1..7 reserved => mask with 0x01 */  
	unsigned long v = simple_strtoul(buf, NULL, 10) & 0x01;

	mutex_lock(&data->update_lock);
	data->global_control = v;
	fscher_write_value(client, reg, v);
	mutex_unlock(&data->update_lock);
	return count;
}

static ssize_t show_control(struct fscher_data *data, char *buf, int nr)
{
	/* bits 1..7 reserved => mask with 0x01 */
	return sprintf(buf, "%u\n", data->global_control & 0x01);
}



static ssize_t set_watchdog_control(struct i2c_client *client, struct
				    fscher_data *data, const char *buf, size_t count,
				    int nr, int reg)
{
	/* bits 0..3 reserved => mask with 0xf0 */  
	unsigned long v = simple_strtoul(buf, NULL, 10) & 0xf0;

	mutex_lock(&data->update_lock);
	data->watchdog[2] &= ~0xf0;
	data->watchdog[2] |= v;
	fscher_write_value(client, reg, data->watchdog[2]);
	mutex_unlock(&data->update_lock);
	return count;
}

static ssize_t show_watchdog_control(struct fscher_data *data, char *buf, int nr)
{
	/* bits 0..3 reserved, bit 5 write only => mask with 0xd0 */
	return sprintf(buf, "%u\n", data->watchdog[2] & 0xd0);
}

static ssize_t set_watchdog_status(struct i2c_client *client, struct fscher_data *data,
				   const char *buf, size_t count, int nr, int reg)
{
	/* bits 0, 2..7 reserved => mask with 0x02 */  
	unsigned long v = simple_strtoul(buf, NULL, 10) & 0x02;

	mutex_lock(&data->update_lock);
	data->watchdog[1] &= ~v;
	fscher_write_value(client, reg, v);
	mutex_unlock(&data->update_lock);
	return count;
}

static ssize_t show_watchdog_status(struct fscher_data *data, char *buf, int nr)
{
	/* bits 0, 2..7 reserved => mask with 0x02 */
	return sprintf(buf, "%u\n", data->watchdog[1] & 0x02);
}

static ssize_t set_watchdog_preset(struct i2c_client *client, struct fscher_data *data,
				   const char *buf, size_t count, int nr, int reg)
{
	unsigned long v = simple_strtoul(buf, NULL, 10) & 0xff;
	
	mutex_lock(&data->update_lock);
	data->watchdog[0] = v;
	fscher_write_value(client, reg, data->watchdog[0]);
	mutex_unlock(&data->update_lock);
	return count;
}

static ssize_t show_watchdog_preset(struct fscher_data *data, char *buf, int nr)
{
	return sprintf(buf, "%u\n", data->watchdog[0]);
}

static int __init sensors_fscher_init(void)
{
	return i2c_add_driver(&fscher_driver);
}

static void __exit sensors_fscher_exit(void)
{
	i2c_del_driver(&fscher_driver);
}

MODULE_AUTHOR("Reinhard Nissl <rnissl@gmx.de>");
MODULE_DESCRIPTION("FSC Hermes driver");
MODULE_LICENSE("GPL");

module_init(sensors_fscher_init);
module_exit(sensors_fscher_exit);
