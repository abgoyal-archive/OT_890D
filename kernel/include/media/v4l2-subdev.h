

#ifndef _V4L2_SUBDEV_H
#define _V4L2_SUBDEV_H

#include <media/v4l2-common.h>

struct v4l2_device;
struct v4l2_subdev;
struct tuner_setup;


struct v4l2_subdev_core_ops {
	int (*g_chip_ident)(struct v4l2_subdev *sd, struct v4l2_dbg_chip_ident *chip);
	int (*log_status)(struct v4l2_subdev *sd);
	int (*init)(struct v4l2_subdev *sd, u32 val);
	int (*s_standby)(struct v4l2_subdev *sd, u32 standby);
	int (*reset)(struct v4l2_subdev *sd, u32 val);
	int (*s_gpio)(struct v4l2_subdev *sd, u32 val);
	int (*queryctrl)(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc);
	int (*g_ctrl)(struct v4l2_subdev *sd, struct v4l2_control *ctrl);
	int (*s_ctrl)(struct v4l2_subdev *sd, struct v4l2_control *ctrl);
	int (*querymenu)(struct v4l2_subdev *sd, struct v4l2_querymenu *qm);
	long (*ioctl)(struct v4l2_subdev *sd, unsigned int cmd, void *arg);
#ifdef CONFIG_VIDEO_ADV_DEBUG
	int (*g_register)(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg);
	int (*s_register)(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg);
#endif
};

struct v4l2_subdev_tuner_ops {
	int (*s_mode)(struct v4l2_subdev *sd, enum v4l2_tuner_type);
	int (*s_radio)(struct v4l2_subdev *sd);
	int (*s_frequency)(struct v4l2_subdev *sd, struct v4l2_frequency *freq);
	int (*g_frequency)(struct v4l2_subdev *sd, struct v4l2_frequency *freq);
	int (*g_tuner)(struct v4l2_subdev *sd, struct v4l2_tuner *vt);
	int (*s_tuner)(struct v4l2_subdev *sd, struct v4l2_tuner *vt);
	int (*s_std)(struct v4l2_subdev *sd, v4l2_std_id norm);
	int (*s_type_addr)(struct v4l2_subdev *sd, struct tuner_setup *type);
	int (*s_config)(struct v4l2_subdev *sd, const struct v4l2_priv_tun_config *config);
};

struct v4l2_subdev_audio_ops {
	int (*s_clock_freq)(struct v4l2_subdev *sd, u32 freq);
	int (*s_i2s_clock_freq)(struct v4l2_subdev *sd, u32 freq);
	int (*s_routing)(struct v4l2_subdev *sd, const struct v4l2_routing *route);
};

struct v4l2_subdev_video_ops {
	int (*s_routing)(struct v4l2_subdev *sd, const struct v4l2_routing *route);
	int (*s_crystal_freq)(struct v4l2_subdev *sd, struct v4l2_crystal_freq *freq);
	int (*decode_vbi_line)(struct v4l2_subdev *sd, struct v4l2_decode_vbi_line *vbi_line);
	int (*s_vbi_data)(struct v4l2_subdev *sd, const struct v4l2_sliced_vbi_data *vbi_data);
	int (*g_vbi_data)(struct v4l2_subdev *sd, struct v4l2_sliced_vbi_data *vbi_data);
	int (*g_sliced_vbi_cap)(struct v4l2_subdev *sd, struct v4l2_sliced_vbi_cap *cap);
	int (*s_std_output)(struct v4l2_subdev *sd, v4l2_std_id std);
	int (*s_stream)(struct v4l2_subdev *sd, int enable);
	int (*s_fmt)(struct v4l2_subdev *sd, struct v4l2_format *fmt);
	int (*g_fmt)(struct v4l2_subdev *sd, struct v4l2_format *fmt);
};

struct v4l2_subdev_ops {
	const struct v4l2_subdev_core_ops  *core;
	const struct v4l2_subdev_tuner_ops *tuner;
	const struct v4l2_subdev_audio_ops *audio;
	const struct v4l2_subdev_video_ops *video;
};

#define V4L2_SUBDEV_NAME_SIZE 32

struct v4l2_subdev {
	struct list_head list;
	struct module *owner;
	struct v4l2_device *dev;
	const struct v4l2_subdev_ops *ops;
	/* name must be unique */
	char name[V4L2_SUBDEV_NAME_SIZE];
	/* can be used to group similar subdevs, value is driver-specific */
	u32 grp_id;
	/* pointer to private data */
	void *priv;
};

static inline void v4l2_set_subdevdata(struct v4l2_subdev *sd, void *p)
{
	sd->priv = p;
}

static inline void *v4l2_get_subdevdata(const struct v4l2_subdev *sd)
{
	return sd->priv;
}

int v4l2_subdev_command(struct v4l2_subdev *sd, unsigned cmd, void *arg);

static inline void v4l2_subdev_init(struct v4l2_subdev *sd,
					const struct v4l2_subdev_ops *ops)
{
	INIT_LIST_HEAD(&sd->list);
	/* ops->core MUST be set */
	BUG_ON(!ops || !ops->core);
	sd->ops = ops;
	sd->dev = NULL;
	sd->name[0] = '\0';
	sd->grp_id = 0;
	sd->priv = NULL;
}

#define v4l2_subdev_call(sd, o, f, args...)				\
	(!(sd) ? -ENODEV : (((sd) && (sd)->ops->o && (sd)->ops->o->f) ?	\
		(sd)->ops->o->f((sd) , ##args) : -ENOIOCTLCMD))

#endif
