

#include "stv06xx_vv6410.h"

static int vv6410_probe(struct sd *sd)
{
	u16 data;
	int err;

	err = stv06xx_read_sensor(sd, VV6410_DEVICEH, &data);

	if (err < 0)
		return -ENODEV;

	if (data == 0x19) {
		info("vv6410 sensor detected");

		sd->gspca_dev.cam.cam_mode = stv06xx_sensor_vv6410.modes;
		sd->gspca_dev.cam.nmodes = stv06xx_sensor_vv6410.nmodes;
		sd->desc.ctrls = stv06xx_sensor_vv6410.ctrls;
		sd->desc.nctrls = stv06xx_sensor_vv6410.nctrls;
		return 0;
	}

	return -ENODEV;
}

static int vv6410_init(struct sd *sd)
{
	int err = 0, i;

	for (i = 0; i < ARRAY_SIZE(stv_bridge_init); i++) {
		/* if NULL then len contains single value */
		if (stv_bridge_init[i].data == NULL) {
			err = stv06xx_write_bridge(sd,
				stv_bridge_init[i].start,
				stv_bridge_init[i].len);
		} else {
			int j;
			for (j = 0; j < stv_bridge_init[i].len; j++)
				err = stv06xx_write_bridge(sd,
					stv_bridge_init[i].start + j,
					stv_bridge_init[i].data[j]);
		}
	}

	if (err < 0)
		return err;

	err = stv06xx_write_sensor_bytes(sd, (u8 *) vv6410_sensor_init,
					 ARRAY_SIZE(vv6410_sensor_init));

	return (err < 0) ? err : 0;
}

static int vv6410_start(struct sd *sd)
{
	int err;
	struct cam *cam = &sd->gspca_dev.cam;
	u32 priv = cam->cam_mode[sd->gspca_dev.curr_mode].priv;

	if (priv & VV6410_CROP_TO_QVGA) {
		PDEBUG(D_CONF, "Cropping to QVGA");
		stv06xx_write_sensor(sd, VV6410_XENDH, 320 - 1);
		stv06xx_write_sensor(sd, VV6410_YENDH, 240 - 1);
	} else {
		stv06xx_write_sensor(sd, VV6410_XENDH, 360 - 1);
		stv06xx_write_sensor(sd, VV6410_YENDH, 294 - 1);
	}

	if (priv & VV6410_SUBSAMPLE) {
		PDEBUG(D_CONF, "Enabling subsampling");
		stv06xx_write_bridge(sd, STV_Y_CTRL, 0x02);
		stv06xx_write_bridge(sd, STV_X_CTRL, 0x06);

		stv06xx_write_bridge(sd, STV_SCAN_RATE, 0x10);
	} else {
		stv06xx_write_bridge(sd, STV_Y_CTRL, 0x01);
		stv06xx_write_bridge(sd, STV_X_CTRL, 0x0a);

		stv06xx_write_bridge(sd, STV_SCAN_RATE, 0x20);
	}

	/* Turn on LED */
	err = stv06xx_write_bridge(sd, STV_LED_CTRL, LED_ON);
	if (err < 0)
		return err;

	err = stv06xx_write_sensor(sd, VV6410_SETUP0, 0);
	if (err < 0)
		return err;

	PDEBUG(D_STREAM, "Starting stream");

	return 0;
}

static int vv6410_stop(struct sd *sd)
{
	int err;

	/* Turn off LED */
	err = stv06xx_write_bridge(sd, STV_LED_CTRL, LED_OFF);
	if (err < 0)
		return err;

	err = stv06xx_write_sensor(sd, VV6410_SETUP0, VV6410_LOW_POWER_MODE);
	if (err < 0)
		return err;

	PDEBUG(D_STREAM, "Halting stream");

	return (err < 0) ? err : 0;
}

static int vv6410_dump(struct sd *sd)
{
	u8 i;
	int err = 0;

	info("Dumping all vv6410 sensor registers");
	for (i = 0; i < 0xff && !err; i++) {
		u16 data;
		err = stv06xx_read_sensor(sd, i, &data);
		info("Register 0x%x contained 0x%x", i, data);
	}
	return (err < 0) ? err : 0;
}

static int vv6410_get_hflip(struct gspca_dev *gspca_dev, __s32 *val)
{
	int err;
	u16 i2c_data;
	struct sd *sd = (struct sd *) gspca_dev;

	err = stv06xx_read_sensor(sd, VV6410_DATAFORMAT, &i2c_data);

	*val = (i2c_data & VV6410_HFLIP) ? 1 : 0;

	PDEBUG(D_V4L2, "Read horizontal flip %d", *val);

	return (err < 0) ? err : 0;
}

static int vv6410_set_hflip(struct gspca_dev *gspca_dev, __s32 val)
{
	int err;
	u16 i2c_data;
	struct sd *sd = (struct sd *) gspca_dev;
	err = stv06xx_read_sensor(sd, VV6410_DATAFORMAT, &i2c_data);
	if (err < 0)
		return err;

	if (val)
		i2c_data |= VV6410_HFLIP;
	else
		i2c_data &= ~VV6410_HFLIP;

	PDEBUG(D_V4L2, "Set horizontal flip to %d", val);
	err = stv06xx_write_sensor(sd, VV6410_DATAFORMAT, i2c_data);

	return (err < 0) ? err : 0;
}

static int vv6410_get_vflip(struct gspca_dev *gspca_dev, __s32 *val)
{
	int err;
	u16 i2c_data;
	struct sd *sd = (struct sd *) gspca_dev;

	err = stv06xx_read_sensor(sd, VV6410_DATAFORMAT, &i2c_data);

	*val = (i2c_data & VV6410_VFLIP) ? 1 : 0;

	PDEBUG(D_V4L2, "Read vertical flip %d", *val);

	return (err < 0) ? err : 0;
}

static int vv6410_set_vflip(struct gspca_dev *gspca_dev, __s32 val)
{
	int err;
	u16 i2c_data;
	struct sd *sd = (struct sd *) gspca_dev;
	err = stv06xx_read_sensor(sd, VV6410_DATAFORMAT, &i2c_data);
	if (err < 0)
		return err;

	if (val)
		i2c_data |= VV6410_VFLIP;
	else
		i2c_data &= ~VV6410_VFLIP;

	PDEBUG(D_V4L2, "Set vertical flip to %d", val);
	err = stv06xx_write_sensor(sd, VV6410_DATAFORMAT, i2c_data);

	return (err < 0) ? err : 0;
}

static int vv6410_get_analog_gain(struct gspca_dev *gspca_dev, __s32 *val)
{
	int err;
	u16 i2c_data;
	struct sd *sd = (struct sd *) gspca_dev;

	err = stv06xx_read_sensor(sd, VV6410_ANALOGGAIN, &i2c_data);

	*val = i2c_data & 0xf;

	PDEBUG(D_V4L2, "Read analog gain %d", *val);

	return (err < 0) ? err : 0;
}

static int vv6410_set_analog_gain(struct gspca_dev *gspca_dev, __s32 val)
{
	int err;
	struct sd *sd = (struct sd *) gspca_dev;

	PDEBUG(D_V4L2, "Set analog gain to %d", val);
	err = stv06xx_write_sensor(sd, VV6410_ANALOGGAIN, 0xf0 | (val & 0xf));

	return (err < 0) ? err : 0;
}
