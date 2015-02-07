

#ifndef __PVRUSB2_CMD_V4L2_H
#define __PVRUSB2_CMD_V4L2_H

#include "pvrusb2-i2c-core.h"

extern const struct pvr2_i2c_op pvr2_i2c_op_v4l2_standard;
extern const struct pvr2_i2c_op pvr2_i2c_op_v4l2_radio;
extern const struct pvr2_i2c_op pvr2_i2c_op_v4l2_bcsh;
extern const struct pvr2_i2c_op pvr2_i2c_op_v4l2_volume;
extern const struct pvr2_i2c_op pvr2_i2c_op_v4l2_frequency;
extern const struct pvr2_i2c_op pvr2_i2c_op_v4l2_crop;
extern const struct pvr2_i2c_op pvr2_i2c_op_v4l2_size;
extern const struct pvr2_i2c_op pvr2_i2c_op_v4l2_audiomode;
extern const struct pvr2_i2c_op pvr2_i2c_op_v4l2_log;

void pvr2_v4l2_cmd_stream(struct pvr2_i2c_client *,int);
void pvr2_v4l2_cmd_status_poll(struct pvr2_i2c_client *);

#endif /* __PVRUSB2_CMD_V4L2_H */

