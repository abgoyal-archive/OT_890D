
#ifndef __HWMSEN_DEV_H__ 
#define __HWMSEN_DEV_H__

#include <linux/types.h> 
#include <linux/hwmsensor.h>
struct hwmsen_prop {
    /*the order follows hwmsensor.h*/
    int   evt_max[C_MAX_HWMSEN_EVENT_NUM];  /*the maximum of reported event value*/
    int   evt_min[C_MAX_HWMSEN_EVENT_NUM];  /*the minimum of reported event value*/
    int   num;
};
/*----------------------------------------------------------------------------*/
struct hwmsen_object {
    void *self;
    int (*activate)(void* self, u8 enable); 
    int (*get_data)(void* self, struct hwmsen_data *data);  /*get sensor output value*/
    int (*set_conf)(void* self, struct hwmsen_conf *conf);  /*set configuration*/
    int (*get_prop)(void* self, struct hwmsen_prop *prop);  /*get property*/
    int (*get_cali)(void* self, struct hwmsen_data *data);
    int (*set_cali)(void* self, struct hwmsen_data *data);
};
/*----------------------------------------------------------------------------*/
extern int hwmsen_attach(HWM_SENSOR sensor, struct hwmsen_object *obj);
extern int hwmsen_detach(HWM_SENSOR sensor);
extern int hwmsen_get_conf(HWM_SENSOR sensor, struct hwmsen_conf *conf);
/*----------------------------------------------------------------------------*/
#endif 
