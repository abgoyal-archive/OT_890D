
#ifndef __PVRUSB2_I2C_CORE_H
#define __PVRUSB2_I2C_CORE_H

#include <linux/list.h>
#include <linux/i2c.h>

struct pvr2_hdw;
struct pvr2_i2c_client;
struct pvr2_i2c_handler;
struct pvr2_i2c_handler_functions;
struct pvr2_i2c_op;
struct pvr2_i2c_op_functions;

struct pvr2_i2c_client {
	struct i2c_client *client;
	struct pvr2_i2c_handler *handler;
	struct list_head list;
	struct pvr2_hdw *hdw;
	int detected_flag;
	int recv_enable;
	unsigned long pend_mask;
	unsigned long ctl_mask;
	void (*status_poll)(struct pvr2_i2c_client *);
};

struct pvr2_i2c_handler {
	void *func_data;
	const struct pvr2_i2c_handler_functions *func_table;
};

struct pvr2_i2c_handler_functions {
	void (*detach)(void *);
	int (*check)(void *);
	void (*update)(void *);
	unsigned int (*describe)(void *,char *,unsigned int);
};

struct pvr2_i2c_op {
	int (*check)(struct pvr2_hdw *);
	void (*update)(struct pvr2_hdw *);
	const char *name;
};

void pvr2_i2c_core_init(struct pvr2_hdw *);
void pvr2_i2c_core_done(struct pvr2_hdw *);

int pvr2_i2c_client_cmd(struct pvr2_i2c_client *,unsigned int cmd,void *arg);
int pvr2_i2c_core_cmd(struct pvr2_hdw *,unsigned int cmd,void *arg);

int pvr2_i2c_core_check_stale(struct pvr2_hdw *);
void pvr2_i2c_core_sync(struct pvr2_hdw *);
void pvr2_i2c_core_status_poll(struct pvr2_hdw *);
unsigned int pvr2_i2c_report(struct pvr2_hdw *,char *buf,unsigned int maxlen);
#define PVR2_I2C_DETAIL_DEBUG   0x0001
#define PVR2_I2C_DETAIL_HANDLER 0x0002
#define PVR2_I2C_DETAIL_CTLMASK 0x0004
#define PVR2_I2C_DETAIL_ALL (\
	PVR2_I2C_DETAIL_DEBUG |\
	PVR2_I2C_DETAIL_HANDLER |\
	PVR2_I2C_DETAIL_CTLMASK)

void pvr2_i2c_probe(struct pvr2_hdw *,struct pvr2_i2c_client *);
const struct pvr2_i2c_op *pvr2_i2c_get_op(unsigned int idx);

#endif /* __PVRUSB2_I2C_CORE_H */


