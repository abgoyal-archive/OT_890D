

#ifndef V4L2_COMMON_H_
#define V4L2_COMMON_H_

#include <media/v4l2-dev.h>

#define v4l_printk(level, name, adapter, addr, fmt, arg...) \
	printk(level "%s %d-%04x: " fmt, name, i2c_adapter_id(adapter), addr , ## arg)

#define v4l_client_printk(level, client, fmt, arg...)			    \
	v4l_printk(level, (client)->driver->driver.name, (client)->adapter, \
		   (client)->addr, fmt , ## arg)

#define v4l_err(client, fmt, arg...) \
	v4l_client_printk(KERN_ERR, client, fmt , ## arg)

#define v4l_warn(client, fmt, arg...) \
	v4l_client_printk(KERN_WARNING, client, fmt , ## arg)

#define v4l_info(client, fmt, arg...) \
	v4l_client_printk(KERN_INFO, client, fmt , ## arg)

#define v4l_dbg(level, debug, client, fmt, arg...)			     \
	do { 								     \
		if (debug >= (level))					     \
			v4l_client_printk(KERN_DEBUG, client, fmt , ## arg); \
	} while (0)

/* ------------------------------------------------------------------------- */

/* These printk constructs can be used with v4l2_device and v4l2_subdev */
#define v4l2_printk(level, dev, fmt, arg...) \
	printk(level "%s: " fmt, (dev)->name , ## arg)

#define v4l2_err(dev, fmt, arg...) \
	v4l2_printk(KERN_ERR, dev, fmt , ## arg)

#define v4l2_warn(dev, fmt, arg...) \
	v4l2_printk(KERN_WARNING, dev, fmt , ## arg)

#define v4l2_info(dev, fmt, arg...) \
	v4l2_printk(KERN_INFO, dev, fmt , ## arg)

#define v4l2_dbg(level, debug, dev, fmt, arg...)			\
	do { 								\
		if (debug >= (level))					\
			v4l2_printk(KERN_DEBUG, dev, fmt , ## arg); 	\
	} while (0)

/* ------------------------------------------------------------------------- */

/* Priority helper functions */

struct v4l2_prio_state {
	atomic_t prios[4];
};
int v4l2_prio_init(struct v4l2_prio_state *global);
int v4l2_prio_change(struct v4l2_prio_state *global, enum v4l2_priority *local,
		     enum v4l2_priority new);
int v4l2_prio_open(struct v4l2_prio_state *global, enum v4l2_priority *local);
int v4l2_prio_close(struct v4l2_prio_state *global, enum v4l2_priority *local);
enum v4l2_priority v4l2_prio_max(struct v4l2_prio_state *global);
int v4l2_prio_check(struct v4l2_prio_state *global, enum v4l2_priority *local);

/* ------------------------------------------------------------------------- */

/* Control helper functions */

int v4l2_ctrl_check(struct v4l2_ext_control *ctrl, struct v4l2_queryctrl *qctrl,
		const char **menu_items);
const char *v4l2_ctrl_get_name(u32 id);
const char **v4l2_ctrl_get_menu(u32 id);
int v4l2_ctrl_query_fill(struct v4l2_queryctrl *qctrl, s32 min, s32 max, s32 step, s32 def);
int v4l2_ctrl_query_fill_std(struct v4l2_queryctrl *qctrl);
int v4l2_ctrl_query_menu(struct v4l2_querymenu *qmenu,
		struct v4l2_queryctrl *qctrl, const char **menu_items);
#define V4L2_CTRL_MENU_IDS_END (0xffffffff)
int v4l2_ctrl_query_menu_valid_items(struct v4l2_querymenu *qmenu, const u32 *ids);
u32 v4l2_ctrl_next(const u32 * const *ctrl_classes, u32 id);

/* ------------------------------------------------------------------------- */

/* Register/chip ident helper function */

struct i2c_client; /* forward reference */
int v4l2_chip_match_i2c_client(struct i2c_client *c, const struct v4l2_dbg_match *match);
int v4l2_chip_ident_i2c_client(struct i2c_client *c, struct v4l2_dbg_chip_ident *chip,
		u32 ident, u32 revision);
int v4l2_chip_match_host(const struct v4l2_dbg_match *match);

/* ------------------------------------------------------------------------- */

/* Helper function for I2C legacy drivers */

struct i2c_driver;
struct i2c_adapter;
struct i2c_client;
struct i2c_device_id;
struct v4l2_device;
struct v4l2_subdev;
struct v4l2_subdev_ops;

int v4l2_i2c_attach(struct i2c_adapter *adapter, int address, struct i2c_driver *driver,
		const char *name,
		int (*probe)(struct i2c_client *, const struct i2c_device_id *));

struct v4l2_subdev *v4l2_i2c_new_subdev(struct i2c_adapter *adapter,
		const char *module_name, const char *client_type, u8 addr);
struct v4l2_subdev *v4l2_i2c_new_probed_subdev(struct i2c_adapter *adapter,
		const char *module_name, const char *client_type,
		const unsigned short *addrs);
/* Initialize an v4l2_subdev with data from an i2c_client struct */
void v4l2_i2c_subdev_init(struct v4l2_subdev *sd, struct i2c_client *client,
		const struct v4l2_subdev_ops *ops);

/* ------------------------------------------------------------------------- */

/* Internal ioctls */

/* VIDIOC_INT_DECODE_VBI_LINE */
struct v4l2_decode_vbi_line {
	u32 is_second_field;	/* Set to 0 for the first (odd) field,
				   set to 1 for the second (even) field. */
	u8 *p; 			/* Pointer to the sliced VBI data from the decoder.
				   On exit points to the start of the payload. */
	u32 line;		/* Line number of the sliced VBI data (1-23) */
	u32 type;		/* VBI service type (V4L2_SLICED_*). 0 if no service found */
};

struct v4l2_priv_tun_config {
	int tuner;
	void *priv;
};

/* audio ioctls */

/* v4l device was opened in Radio mode, to be replaced by VIDIOC_INT_S_TUNER_MODE */
#define AUDC_SET_RADIO        _IO('d',88)

/* tuner ioctls */

/* Sets tuner type and its I2C addr */
#define TUNER_SET_TYPE_ADDR          _IOW('d', 90, int)

#define TUNER_SET_STANDBY            _IOW('d', 91, int)

/* Sets tda9887 specific stuff, like port1, port2 and qss */
#define TUNER_SET_CONFIG           _IOW('d', 92, struct v4l2_priv_tun_config)

/* Switch the tuner to a specific tuner mode. Replacement of AUDC_SET_RADIO */
#define VIDIOC_INT_S_TUNER_MODE	     _IOW('d', 93, enum v4l2_tuner_type)

#define VIDIOC_INT_S_STANDBY 	     _IOW('d', 94, u32)

/* 100, 101 used by  VIDIOC_DBG_[SG]_REGISTER */

#define VIDIOC_INT_RESET            	_IOW ('d', 102, u32)

#define VIDIOC_INT_AUDIO_CLOCK_FREQ 	_IOW ('d', 103, u32)

#define VIDIOC_INT_DECODE_VBI_LINE  	_IOWR('d', 104, struct v4l2_decode_vbi_line)

#define VIDIOC_INT_S_VBI_DATA 		_IOW ('d', 105, struct v4l2_sliced_vbi_data)

#define VIDIOC_INT_G_VBI_DATA 		_IOWR('d', 106, struct v4l2_sliced_vbi_data)

#define VIDIOC_INT_I2S_CLOCK_FREQ 	_IOW ('d', 108, u32)

struct v4l2_routing {
	u32 input;
	u32 output;
};

#define	VIDIOC_INT_S_AUDIO_ROUTING	_IOW ('d', 109, struct v4l2_routing)
#define	VIDIOC_INT_G_AUDIO_ROUTING	_IOR ('d', 110, struct v4l2_routing)
#define	VIDIOC_INT_S_VIDEO_ROUTING	_IOW ('d', 111, struct v4l2_routing)
#define	VIDIOC_INT_G_VIDEO_ROUTING	_IOR ('d', 112, struct v4l2_routing)

struct v4l2_crystal_freq {
	u32 freq;	/* frequency in Hz of the crystal */
	u32 flags; 	/* device specific flags */
};

#define VIDIOC_INT_S_CRYSTAL_FREQ 	_IOW('d', 113, struct v4l2_crystal_freq)

#define VIDIOC_INT_INIT			_IOW('d', 114, u32)

#define VIDIOC_INT_S_STD_OUTPUT		_IOW('d', 115, v4l2_std_id)

#define VIDIOC_INT_G_STD_OUTPUT		_IOW('d', 116, v4l2_std_id)

#define VIDIOC_INT_S_GPIO		_IOW('d', 117, u32)

#endif /* V4L2_COMMON_H_ */
