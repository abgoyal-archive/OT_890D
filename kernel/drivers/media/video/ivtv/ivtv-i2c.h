

#ifndef IVTV_I2C_H
#define IVTV_I2C_H

int ivtv_i2c_register(struct ivtv *itv, unsigned idx);
struct v4l2_subdev *ivtv_find_hw(struct ivtv *itv, u32 hw);

/* init + register i2c algo-bit adapter */
int init_ivtv_i2c(struct ivtv *itv);
void exit_ivtv_i2c(struct ivtv *itv);

#endif
