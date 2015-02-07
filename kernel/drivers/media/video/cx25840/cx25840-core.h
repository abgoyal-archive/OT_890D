

#ifndef _CX25840_CORE_H_
#define _CX25840_CORE_H_


#include <linux/videodev2.h>
#include <media/v4l2-device.h>
#include <linux/i2c.h>

#define CX25840_CID_ENABLE_PVR150_WORKAROUND (V4L2_CID_PRIVATE_BASE+0)

struct cx25840_state {
	struct i2c_client *c;
	struct v4l2_subdev sd;
	int pvr150_workaround;
	int radio;
	v4l2_std_id std;
	enum cx25840_video_input vid_input;
	enum cx25840_audio_input aud_input;
	u32 audclk_freq;
	int audmode;
	int unmute_volume; /* -1 if not muted */
	int default_volume;
	int vbi_line_offset;
	u32 id;
	u32 rev;
	int is_cx25836;
	int is_cx23885;
	int is_initialized;
	wait_queue_head_t fw_wait;    /* wake up when the fw load is finished */
	struct work_struct fw_work;   /* work entry for fw load */
};

static inline struct cx25840_state *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct cx25840_state, sd);
}

/* ----------------------------------------------------------------------- */
/* cx25850-core.c 							   */
int cx25840_write(struct i2c_client *client, u16 addr, u8 value);
int cx25840_write4(struct i2c_client *client, u16 addr, u32 value);
u8 cx25840_read(struct i2c_client *client, u16 addr);
u32 cx25840_read4(struct i2c_client *client, u16 addr);
int cx25840_and_or(struct i2c_client *client, u16 addr, unsigned mask, u8 value);
void cx25840_std_setup(struct i2c_client *client);

/* ----------------------------------------------------------------------- */
/* cx25850-firmware.c                                                      */
int cx25840_loadfw(struct i2c_client *client);

/* ----------------------------------------------------------------------- */
/* cx25850-audio.c                                                         */
int cx25840_audio(struct i2c_client *client, unsigned int cmd, void *arg);
void cx25840_audio_set_path(struct i2c_client *client);

/* ----------------------------------------------------------------------- */
/* cx25850-vbi.c                                                           */
int cx25840_vbi(struct i2c_client *client, unsigned int cmd, void *arg);

#endif
