
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/wait.h>

#include <linux/hwmsensor.h>
#include "hwmsen_helper.h"
#include "hwmsen_dev.h"
struct hwmsen_context { /*sensor context*/
    atomic_t                init;
    atomic_t                enable;
    struct hwmsen_info     *info;
    struct hwmsen_object    obj;
};
/*----------------------------------------------------------------------------*/
struct dev_context {
    atomic_t     active_count;
    struct mutex lock;
    struct hwmsen_context cxt[C_MAX_HWMSEN_NUM];
};
/*----------------------------------------------------------------------------*/
typedef enum {
    HWM_TRC_REPORT_NUM = 0x0001,
    HWM_TRC_REPORT_EVT = 0x0002,    
} HWM_TRC;
/*----------------------------------------------------------------------------*/
#define C_MAX_OBJECT_NUM 1
struct hwmdev_object {
    struct input_dev   *idev;
    struct miscdevice   mdev;
    struct dev_context *dc;
    struct work_struct  report;
    atomic_t            delay; /*polling period for reporting input event*/
    atomic_t            wake;  /*user-space request to wake-up, used with stop*/
    struct timer_list   timer;  /* polling timer */
    atomic_t            trace;
};
static struct dev_context dev_cxt = {
    .active_count = ATOMIC_INIT(0),    
    .lock = __MUTEX_INITIALIZER(dev_cxt.lock),        
    .cxt =     
    {
        { ATOMIC_INIT(0), ATOMIC_INIT(0), NULL, {NULL, NULL, NULL, NULL} },
        { ATOMIC_INIT(0), ATOMIC_INIT(0), NULL, {NULL, NULL, NULL, NULL} },
        { ATOMIC_INIT(0), ATOMIC_INIT(0), NULL, {NULL, NULL, NULL, NULL} },
        { ATOMIC_INIT(0), ATOMIC_INIT(0), NULL, {NULL, NULL, NULL, NULL} },
        { ATOMIC_INIT(0), ATOMIC_INIT(0), NULL, {NULL, NULL, NULL, NULL} },
        { ATOMIC_INIT(0), ATOMIC_INIT(0), NULL, {NULL, NULL, NULL, NULL} },
        { ATOMIC_INIT(0), ATOMIC_INIT(0), NULL, {NULL, NULL, NULL, NULL} },
        { ATOMIC_INIT(0), ATOMIC_INIT(0), NULL, {NULL, NULL, NULL, NULL} },
    },
};
/*----------------------------------------------------------------------------*/
/*the max/min will be queried from real sensor device*/
static struct hwmsen_info sen_info[C_MAX_HWMSEN_NUM] = {    
    {   /*HWM_SENSOR_ACCELERATION*/   
        3, {{0x00, 0x00, 0x00, 1024},   /*HWM_EVT_ACC_X*/
            {0x01, 0x00, 0x00, 1024},   /*HWM_EVT_ACC_Y*/
            {0x02, 0x00, 0x00, 1024},   /*HWM_EVT_ACC_Z*/
            {0x03, 0x00, 0x00,    0}}
    },
    {   /*HWM_SENSOR_MAGNETIC_FIELD*/
        3, {{0x04, 0x00, 0x00,  600},   /*HWM_EVT_MAG_X*/
            {0x05, 0x00, 0x00,  600},   /*HWM_EVT_MAG_Y*/
            {0x06, 0x00, 0x00,  600},   /*HWM_EVT_MAG_Z*/
            {0x07, 0x00, 0x00,    0}}
    },
    {   /*HWM_SENSOR_ORIENTATION*/
        3, {{0x08, 0x00, 0x00, 0x00},   /*HWM_EVT_ORI_AZIMUTH*/
            {0x09, 0x00, 0x00, 0x00},   /*HWM_EVT_ORI_PITCH*/
            {0x0A, 0x00, 0x00, 0x00},   /*HWM_EVT_ORI_ROLL*/
            {0x0B, 0x00, 0x00, 0x00}}
    },
    {   /*HWM_SENSOR_GYROSCOPE*/
        0, {{0x0C, 0x00, 0x00, 0x00},
            {0x0D, 0x00, 0x00, 0x00},
            {0x0E, 0x00, 0x00, 0x00},
            {0x0F, 0x00, 0x00, 0x00}}
    },
    {   /*HWM_SENSOR_LIGHT*/
        1, {{0x10, 0x00, 0x00, 0x00},   /*HWM_EVT_LIGHT*/
            {0x11, 0x00, 0x00, 0x00},
            {0x12, 0x00, 0x00, 0x00},
            {0x13, 0x00, 0x00, 0x00}}
    },
    {   /*HWM_SENSOR_PRESSURE*/
        0, {{0x14, 0x00, 0x00, 0x00},
            {0x15, 0x00, 0x00, 0x00},
            {0x16, 0x00, 0x00, 0x00},
            {0x17, 0x00, 0x00, 0x00}}
    },
    {   /*HWM_SENSOR_PROXIMITY*/
        0, {{0x18, 0x00, 0x00, 0x00},
            {0x19, 0x00, 0x00, 0x00},
            {0x1A, 0x00, 0x00, 0x00},
            {0x1B, 0x00, 0x00, 0x00}}
    },
    {   /*HWM_SENSOR_PROXIMITY*/
        1, {{0x1C, 0x00, 0x00, 0x00},   /*HWM_EVT_DISTANCE*/
            {0x1D, 0x00, 0x00, 0x00},
            {0x1E, 0x00, 0x00, 0x00},
            {0x1F, 0x00, 0x00, 0x00}}
    },

};
/*----------------------------------------------------------------------------*/
static struct hwmdev_object *dev_objlist[C_MAX_OBJECT_NUM] = {
    NULL,
};
int hwmsen_get_conf(HWM_SENSOR sensor, struct hwmsen_conf *conf) 
{
    if (sensor >= HWM_SENSOR_MAX || !conf) {
        return -EINVAL;
    } else {
        int idx;
        struct hwmsen_info *info = &sen_info[sensor];
        conf->num = info->num;
        for (idx = 0; idx < info->num; idx++)
            conf->sensitivity[idx] = info->evt[idx].sensitivity;
        return 0;
    }
}
/*----------------------------------------------------------------------------*/
EXPORT_SYMBOL_GPL(hwmsen_get_conf);
static void hwmsen_work_func(struct work_struct *work)
{
    struct hwmdev_object *obj = container_of(work, struct hwmdev_object, report);
    struct hwmsen_context *cxt = NULL;
    struct hwmsen_data dat;
    int err, idx, pos;
    atomic_t reported = ATOMIC_INIT(0);
    atomic_t count = ATOMIC_INIT(0);
    int trc = atomic_read(&obj->trace);

    if (!obj)
        return;

    if (atomic_read(&obj->wake)) {
        input_event(obj->idev, EV_SYN, SYN_CONFIG, 0);
        atomic_set(&obj->wake, 0);
        HWM_VER("report wakeup event!!\n");
        return;
    }
    
    atomic_set(&reported, 0);
    for (idx = 0; idx < C_MAX_HWMSEN_NUM; idx++) {
        cxt = &obj->dc->cxt[idx];
        if (!atomic_read(&cxt->enable) || !cxt->obj.get_data)
            continue;
        if ((err = cxt->obj.get_data(cxt->obj.self, &dat))) {
            HWM_ERR("get data from sensor (%d) fails!!\n", idx);
            continue;
        }
        if (!dat.num)
            continue;

        atomic_set(&reported, 1);
        for (pos = 0; pos < dat.num; pos++) {
            input_report_abs(obj->idev, cxt->info->evt[pos].code, dat.raw[pos]);
            atomic_inc(&count);
            if (trc & HWM_TRC_REPORT_EVT)
                HWM_LOG("[%2d] %2d: %5d\n", idx, cxt->info->evt[pos].code, dat.raw[pos]);
        }
    }
    
    if (atomic_read(&reported)) {
        if (trc & HWM_TRC_REPORT_NUM)
            HWM_LOG("event count: %d\n", atomic_read(&count));
        input_sync(obj->idev);
    } else
        HWM_LOG("no available sensor!!\n");
    mod_timer(&obj->timer, jiffies + atomic_read(&obj->delay)/(1000/HZ));    
}
/*----------------------------------------------------------------------------*/
static void hwmsen_poll(unsigned long data)
{
    struct hwmdev_object *obj = (struct hwmdev_object *)data;

    if (obj)
        schedule_work(&obj->report);
}
/*----------------------------------------------------------------------------*/
static struct hwmdev_object *hwmsen_alloc_object(void)
{
    struct hwmdev_object *obj = kzalloc(sizeof(*obj), GFP_KERNEL);    

    if (!obj)
        return NULL;

    obj->dc = &dev_cxt;
    atomic_set(&obj->delay, 200); /*5Hz*/
    atomic_set(&obj->wake, 0);
    INIT_WORK(&obj->report, hwmsen_work_func);
    init_timer(&obj->timer);
    obj->timer.expires  = jiffies + atomic_read(&obj->delay)/(1000/HZ);
    obj->timer.function = hwmsen_poll;
    obj->timer.data     = (unsigned long)obj;
    return obj;    
}
/*----------------------------------------------------------------------------*/
int hwmsen_attach(HWM_SENSOR sensor, struct hwmsen_object *obj)
{
    struct dev_context *mcxt = &dev_cxt;
    int err = 0;
    if ((sensor >= HWM_SENSOR_MAX) || (mcxt == NULL))
        return -EINVAL;

    mutex_lock(&mcxt->lock);
    if (atomic_read(&mcxt->cxt[sensor].init)) {
        err = -EEXIST;
    } else {
        atomic_set(&mcxt->cxt[sensor].init, 1);
        atomic_set(&mcxt->cxt[sensor].enable, 0);
        memcpy(&mcxt->cxt[sensor].obj, obj, sizeof(*obj));
    }
    mutex_unlock(&mcxt->lock);
    HWM_VER("%d\n", sensor);
    return 0;
}
/*----------------------------------------------------------------------------*/
EXPORT_SYMBOL_GPL(hwmsen_attach);
/*----------------------------------------------------------------------------*/
int hwmsen_detach(HWM_SENSOR sensor) 
{
    struct dev_context *mcxt = &dev_cxt;    
    if ((sensor >= HWM_SENSOR_MAX) || (mcxt == NULL))
        return -EINVAL;

    mutex_lock(&mcxt->lock);
    atomic_set(&mcxt->cxt[sensor].init, 0);
    atomic_set(&mcxt->cxt[sensor].enable, 0);
    memset(&mcxt->cxt[sensor].obj, 0x00, sizeof(mcxt->cxt[sensor].obj));
    mutex_unlock(&mcxt->lock);
    HWM_VER("%d\n", sensor);
    return 0;
}
/*----------------------------------------------------------------------------*/
EXPORT_SYMBOL_GPL(hwmsen_detach);
/*----------------------------------------------------------------------------*/
static int hwmsen_enable(struct hwmdev_object *obj, HWM_SENSOR sensor, u32 enable)
{
    struct hwmsen_context *cxt = NULL;
    int err = 0;

    if (!obj || sensor >= HWM_SENSOR_MAX) {
        HWM_ERR("invalid argument: %p, %d\n", obj, sensor);
        return -EINVAL;
    } else if (!atomic_read(&obj->dc->cxt[sensor].init)) {
        HWM_ERR("the sensor (%d) is not attached!!\n", sensor);
        return -ENODEV;
    } else if (atomic_read(&obj->dc->cxt[sensor].enable) == enable) {
        HWM_ERR("the sensor (%d) state unchange (%d)!!\n", sensor, enable);
        return 0;
    }
    
    mutex_lock(&obj->dc->lock);
    cxt = &obj->dc->cxt[sensor];    
    atomic_set(&cxt->enable, enable);
    if (enable == 1) {
        if ((err = cxt->obj.activate(cxt->obj.self, enable))) {
            HWM_ERR("activate sensor(%d) err = %d\n", sensor, err);
            err = -EINVAL;
        } else {
            if (0 == atomic_read(&obj->dc->active_count)) {                
                obj->timer.expires = jiffies + atomic_read(&obj->delay)/(1000/HZ);
                add_timer(&obj->timer);
            }
            atomic_inc(&obj->dc->active_count);
        }
    } else {
        if ((err = cxt->obj.activate(cxt->obj.self, enable))) {
            HWM_ERR("deactiva sensor(%d) err = %d\n", sensor, err);
            err = -EINVAL;
        } else {       
            atomic_dec(&obj->dc->active_count);
            if (0 == atomic_read(&obj->dc->active_count)) {
                del_timer_sync(&obj->timer);
                cancel_work_sync(&obj->report);                            
            }
        }
    }
    mutex_unlock(&obj->dc->lock);    
    HWM_VER("sensor(%d), flag(%d), act-cnt(%d)\n", sensor, enable, 
            atomic_read(&obj->dc->active_count));
    return err;
}
/*----------------------------------------------------------------------------*/
static int hwmsen_wakeup(struct hwmdev_object *obj)
{
    if (!obj) {
        HWM_ERR("null pointer!!\n");
        return -EINVAL;
    }
    
    input_event(obj->idev, EV_SYN, SYN_CONFIG, 0);
    HWM_VER("report wakeup event!!\n");
    return 0;
}
/*----------------------------------------------------------------------------*/
static ssize_t hwmsen_show_hwmdev(struct device* dev, 
                                 struct device_attribute *attr, char *buf) 
{
    struct hwmdev_object *devobj = (struct hwmdev_object*)dev_get_drvdata(dev);
    int idx, len = 0;

    if (!devobj || !devobj->dc) {
        HWM_ERR("null pointer: %p, %p", devobj, (!devobj) ? (NULL) : (devobj->dc));
        return 0;
    }
    for (idx = 0; idx < C_MAX_HWMSEN_NUM; idx++) 
        len += snprintf(buf+len, PAGE_SIZE-len, "    %d", idx);
    len += snprintf(buf+len, PAGE_SIZE-len, "\n");
    for (idx = 0; idx < C_MAX_HWMSEN_NUM; idx++)
        len += snprintf(buf+len, PAGE_SIZE-len, "    %d", atomic_read(&devobj->dc->cxt[idx].enable));
    len += snprintf(buf+len, PAGE_SIZE-len, "\n");
    return len;
}
/*----------------------------------------------------------------------------*/
static ssize_t hwmsen_store_active(struct device* dev, struct device_attribute *attr,
                                  const char *buf, size_t count)
{
    struct hwmdev_object *devobj = (struct hwmdev_object*)dev_get_drvdata(dev);
    int sensor, enable, err, idx;

    if (!devobj || !devobj->dc) {
        HWM_ERR("null pointer!!\n");
        return count;
    }

    if (!strncmp(buf, "all-start", 9)) {
        for (idx = 0; idx < C_MAX_HWMSEN_NUM; idx++)
            hwmsen_enable(devobj, idx, 1);
    } else if (!strncmp(buf, "all-stop", 8)) {
        for (idx = 0; idx < C_MAX_HWMSEN_NUM; idx++)
            hwmsen_enable(devobj, idx, 0);    
    } else if (2 == sscanf(buf, "%d %d", &sensor, &enable)) {
        if ((err = hwmsen_enable(devobj, sensor, enable)))
            HWM_ERR("sensor enable failed: %d\n", err);
    } 

    return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t hwmsen_show_delay(struct device* dev, 
                                 struct device_attribute *attr, char *buf) 
{
    struct hwmdev_object *devobj = (struct hwmdev_object*)dev_get_drvdata(dev);

    if (!devobj || !devobj->dc) {
        HWM_ERR("null pointer!!\n");
        return 0;
    }
    return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&devobj->delay));    
}
/*----------------------------------------------------------------------------*/
static ssize_t hwmsen_store_delay(struct device* dev, struct device_attribute *attr,
                                  const char *buf, size_t count)
{
    struct hwmdev_object *devobj = (struct hwmdev_object*)dev_get_drvdata(dev);
    int delay;

    if (!devobj || !devobj->dc) {
        HWM_ERR("null pointer!!\n");
        return count;
    }

    if (1 != sscanf(buf, "%d", &delay)) {
        HWM_ERR("invalid format!!\n");
        return count;
    }

    atomic_set(&devobj->delay, delay);
    return count;
}                                 
/*----------------------------------------------------------------------------*/
static ssize_t hwmsen_show_wake(struct device* dev, 
                                 struct device_attribute *attr, char *buf) 
{
    struct hwmdev_object *devobj = (struct hwmdev_object*)dev_get_drvdata(dev);

    if (!devobj || !devobj->dc) {
        HWM_ERR("null pointer!!\n");
        return 0;
    }
    return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&devobj->wake));    
}
/*----------------------------------------------------------------------------*/
static ssize_t hwmsen_store_wake(struct device* dev, struct device_attribute *attr,
                                  const char *buf, size_t count)
{
    struct hwmdev_object *devobj = (struct hwmdev_object*)dev_get_drvdata(dev);
    int wake, err;

    if (!devobj || !devobj->dc) {
        HWM_ERR("null pointer!!\n");
        return count;
    }

    if (1 != sscanf(buf, "%d", &wake)) {
        HWM_ERR("invalid format!!\n");
        return count;
    }

    if ((err = hwmsen_wakeup(devobj))) {
        HWM_ERR("wakeup sensor fail, %d\n", err);
        return count;
    }
    
    return count;
}  
/*----------------------------------------------------------------------------*/
static ssize_t hwmsen_show_caliacc(struct device *dev, 
								   struct device_attribute *attr, char* buf) 
{
    struct hwmdev_object *obj = (struct hwmdev_object*)dev_get_drvdata(dev);
    struct hwmsen_data cali;
    struct hwmsen_context *cxt;
    int err;

    if (!obj) {
        HWM_ERR("null pointer\n");
        return 0;
    }  
    cxt = &obj->dc->cxt[HWM_SENSOR_ACCELERATION];        
    if (!atomic_read(&cxt->init)) {
        HWM_ERR("accelerometer is not initialized!!\n");
        return 0;
    } else if (!cxt->obj.get_cali) {
        HWM_ERR("the function is not provided by sensor\n");
        return 0;    
    } else if ((err = cxt->obj.get_cali(cxt->obj.self, &cali))) {
        HWM_ERR("get calibration fail: %d\n", err);
        return 0;
    } else {
        return snprintf(buf, PAGE_SIZE, "(%6d, %6d, %6d) mg", 
                        cali.raw[HWM_EVT_ACC_X], cali.raw[HWM_EVT_ACC_Y],
                        cali.raw[HWM_EVT_ACC_Z]);
    }
}
/*----------------------------------------------------------------------------*/
static ssize_t hwmsen_store_caliacc(struct device *dev, 
								 struct device_attribute *attr, 
								 const char* buf, size_t count) 
{   /*if the function doesn't return count, the funtion will be called again to consume buffer*/
	struct i2c_client *client = to_i2c_client(dev);
    struct hwmdev_object *obj = i2c_get_clientdata(client);    
    int x, y, z, err;
    struct hwmsen_data cali;
    struct hwmsen_context *cxt;    

    if (!obj) {
        HWM_ERR("null pointer\n");
        return 0;
    }  
    cxt = &obj->dc->cxt[HWM_SENSOR_ACCELERATION];        
    if (!atomic_read(&cxt->init)) {
        HWM_ERR("accelerometer is not initialized!!\n");
    } else if (!cxt->obj.set_cali) {
        HWM_ERR("the function is not provided by sensor\n");
    } else if (3 != sscanf(buf, "%d %d %d", &x, &y, &z)) {
        HWM_ERR("invalid format!!\n");
    } else {
        cali.raw[HWM_EVT_ACC_X] = x;
        cali.raw[HWM_EVT_ACC_Y] = y;
        cali.raw[HWM_EVT_ACC_Z] = z;
        cali.num = 3;
        if ((err = cxt->obj.set_cali(cxt->obj.self, &cali)))
            HWM_ERR("fail to write calibration: %d\n", err);
    }
    return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t hwmsen_show_trace(struct device *dev, 
                                  struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct hwmdev_object *obj = i2c_get_clientdata(client);
    
	return snprintf(buf, PAGE_SIZE, "0x%08X\n", atomic_read(&obj->trace));
}
/*----------------------------------------------------------------------------*/
static ssize_t hwmsen_store_trace(struct device* dev, 
                                   struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
    struct hwmdev_object *obj = i2c_get_clientdata(client);    
    int trc;

    if (1 == sscanf(buf, "0x%x\n", &trc)) 
        atomic_set(&obj->trace, trc);
    else
        HWM_ERR("set trace level fail!!\n");
    return count;
}                                                         
/*----------------------------------------------------------------------------*/
DEVICE_ATTR(hwmdev,     S_IWUGO | S_IRUGO, hwmsen_show_hwmdev, NULL);
DEVICE_ATTR(active,     S_IWUGO | S_IRUGO, hwmsen_show_hwmdev, hwmsen_store_active);
DEVICE_ATTR(delay,      S_IWUGO | S_IRUGO, hwmsen_show_delay,  hwmsen_store_delay);
DEVICE_ATTR(wake,       S_IWUGO | S_IRUGO, hwmsen_show_wake,   hwmsen_store_wake);
DEVICE_ATTR(trace,      S_IWUGO | S_IRUGO, hwmsen_show_trace,  hwmsen_store_trace);
/*calibration of accelerometer*/
DEVICE_ATTR(caliacc,    S_IWUGO | S_IRUGO, hwmsen_show_caliacc,hwmsen_store_caliacc);
/*----------------------------------------------------------------------------*/
static struct device_attribute *hwmsen_attr_list[] = {
    &dev_attr_hwmdev,
    &dev_attr_active,
    &dev_attr_delay,
    &dev_attr_wake,
    &dev_attr_caliacc,
    &dev_attr_trace,
};
/*----------------------------------------------------------------------------*/
static int hwmsen_create_attr(struct device *dev) 
{
    int idx, err = 0;
    int num = (int)(sizeof(hwmsen_attr_list)/sizeof(hwmsen_attr_list[0]));
    if (!dev)
        return -EINVAL;

    HWM_FUN();
    for (idx = 0; idx < num; idx++) {
        if ((err = device_create_file(dev, hwmsen_attr_list[idx]))) {            
            HWM_VER("device_create_file (%s) = %d\n", hwmsen_attr_list[idx]->attr.name, err);        
            break;
        }
    }
    
    return err;
}
/*----------------------------------------------------------------------------*/
static int hwmsen_delete_attr(struct device *dev)
{
    int idx ,err = 0;
    int num = (int)(sizeof(hwmsen_attr_list)/sizeof(hwmsen_attr_list[0]));
    
    if (!dev)
        return -EINVAL;

    for (idx = 0; idx < num; idx++) 
        device_remove_file(dev, hwmsen_attr_list[idx]);

    return err;
}
/*----------------------------------------------------------------------------*/
static struct hwmdev_object* hwmsen_get_object(int minor) 
{
    int idx;
    for (idx = 0; idx < C_MAX_OBJECT_NUM; idx++) {
        if (dev_objlist[idx] == NULL)
            continue;
        if (dev_objlist[idx]->mdev.minor == minor)
            return dev_objlist[idx];
    }
    return NULL;
}
/*----------------------------------------------------------------------------*/
static int hwmsen_add_object(struct hwmdev_object *devobj) 
{
    int idx;
    for (idx = 0; idx < C_MAX_OBJECT_NUM; idx++) {
        if (dev_objlist[idx] == NULL)  {
            dev_objlist[idx] = devobj;
            return 0;
        }
    }
    HWM_ERR("no availble index\n");
    return -EINVAL;
}
/*----------------------------------------------------------------------------*/
static int hwmsen_del_object(struct hwmdev_object *devobj) 
{
    int idx;
    for (idx = 0; idx < C_MAX_OBJECT_NUM; idx++) {
        if (dev_objlist[idx] == devobj) {
            dev_objlist[idx] = NULL;
            return 0;
        }
    }
    HWM_ERR("invalid index\n");
    return -EINVAL;
}
/*----------------------------------------------------------------------------*/
static int hwmsen_open(struct inode *node , struct file *fp)
{
    int minor = iminor(node);
    fp->private_data = hwmsen_get_object(minor);

    if (!fp->private_data) {
        HWM_ERR("null pointer!!\n");
        return -EINVAL;
    }
    return nonseekable_open(node,fp);
}
/*----------------------------------------------------------------------------*/
static int hwmsen_release(struct inode *node, struct file *fp)
{
    fp->private_data = NULL;
    return 0;
}
/*----------------------------------------------------------------------------*/
static int hwmsen_ioctl(struct inode *node, struct file *fp, 
                        unsigned int cmd, unsigned long arg)
{
    struct hwmdev_object *obj = (struct hwmdev_object*)fp->private_data;
    void __user *argp = (void __user*)arg;
    uint32_t flag;
    HWM_SENSOR sid = HWM_SENSOR_MAX;
    int err;

    if (!obj) {
        HWM_ERR("null pointer!!\n");
        return -EINVAL;
    }

    switch (cmd) {
    case HWM_IOCG_ACC_MODE:
        sid = HWM_SENSOR_ACCELERATION;
        break;
    case HWM_IOCG_MAG_MODE:
        sid = HWM_SENSOR_MAGNETIC_FIELD;
        break;
    case HWM_IOCG_ORI_MODE:
        sid = HWM_SENSOR_ORIENTATION;
        break;
    case HWM_IOCG_GYR_MODE:
        sid = HWM_SENSOR_GYROSCOPE;
        break;
    case HWM_IOCG_LIG_MODE:
        sid = HWM_SENSOR_LIGHT;
        break;
    case HWM_IOCG_PRE_MODE:
        sid = HWM_SENSOR_PRESSURE;
        break;
    case HWM_IOCG_TEM_MODE:
        sid = HWM_SENSOR_TEMPERATURE;
        break;
    case HWM_IOCG_PRO_MODE:
        sid = HWM_SENSOR_PROXIMITY;
        break;
    }
    if (sid < HWM_SENSOR_MAX) {
        flag = atomic_read(&obj->dc->cxt[sid].enable);
        if (copy_to_user(argp, &flag, sizeof(flag)))
            return -EFAULT;
        else
            return 0;
    }

    switch (cmd) {
    case HWM_IOCS_ACC_MODE:
        sid = HWM_SENSOR_ACCELERATION;
        break;
    case HWM_IOCS_MAG_MODE:
        sid = HWM_SENSOR_MAGNETIC_FIELD;
        break;
    case HWM_IOCS_ORI_MODE:
        sid = HWM_SENSOR_ORIENTATION;
        break;
    case HWM_IOCS_GYR_MODE:
        sid = HWM_SENSOR_GYROSCOPE;
        break;
    case HWM_IOCS_LIG_MODE:
        sid = HWM_SENSOR_LIGHT;
        break;
    case HWM_IOCS_PRE_MODE:
        sid = HWM_SENSOR_PRESSURE;
        break;
    case HWM_IOCS_TEM_MODE:
        sid = HWM_SENSOR_TEMPERATURE;
        break;
    case HWM_IOCS_PRO_MODE:
        sid = HWM_SENSOR_PROXIMITY;
        break;
    }
    if (sid < HWM_SENSOR_MAX) {
        if (copy_from_user(&flag, argp, sizeof(flag))) {
            HWM_ERR("copy_from_user fail!!\n");
            return -EFAULT;
        } else if (flag != 0 && flag != 1) {
            HWM_ERR("invalid flag: %d\n", flag);
            return -EINVAL;
        } else {
            return hwmsen_enable(obj, sid, flag);
        }
    }

    /*set/get delay*/
    if (cmd == HWM_IOCG_DELAY) {
        flag = atomic_read(&obj->delay);
        if (copy_to_user(argp, &flag, sizeof(flag))) {
            HWM_ERR("copy_to_user fail!!\n");
            return -EFAULT;
        } else {
            HWM_VER("Get delay: %d\n", flag);
            return 0;
        }
    } else if (cmd == HWM_IOCS_DELAY) {
        if (copy_from_user(&flag, argp, sizeof(flag))) {
            HWM_ERR("copy_from_user fail!!\n");
            return -EFAULT;
        }
        atomic_set(&obj->delay, flag);
        HWM_VER("Set delay: %d\n", atomic_read(&obj->delay));
        return 0;
    }

    /*wake*/
    if (cmd == HWM_IOCS_WAKE) {
        if (copy_from_user(&flag, argp, sizeof(flag))) {
            HWM_ERR("copy_from_user fail!!\n");
            return -EFAULT;
        } else if (flag != 0 && flag != 1) {
            HWM_ERR("invalid flag: %d\n", flag);
            return -EINVAL;
        } else {
            return hwmsen_wakeup(obj);
        }
    }

    /*get sensor specification*/
    if (cmd == HWM_IOCG_SPEC) {
        struct hwmsen_spec spec;
        int idx;
        memset(&spec, 0x00, sizeof(spec));
        for (idx = 0; idx < HWM_SENSOR_MAX; idx++) {
            /*the sensor is not attached*/
            if (!atomic_read(&obj->dc->cxt[idx].init))
                continue;
            memcpy(&spec.info[idx], obj->dc->cxt[idx].info, sizeof(spec.info[idx]));
            spec.avail |= (1 << idx); /*this corresponds to HWM_ID_XXX*/
        }
        if (copy_to_user(argp, &spec, sizeof(spec))) {
            HWM_ERR("copy_to_user fail!!\n");
            return -EFAULT;
        } 
        return 0;
    }

    /*calibration*/
    if (cmd == HWM_IOCG_ACC_CALI) {
        struct hwmsen_data cali;
        struct hwmsen_context *cxt = &obj->dc->cxt[HWM_SENSOR_ACCELERATION];
        if (!atomic_read(&cxt->init) || !atomic_read(&cxt->enable) || !cxt->obj.get_cali) {
            HWM_ERR("cali_acc: disabled (%d, %d, %p)\n", atomic_read(&cxt->init), 
                    atomic_read(&cxt->enable), cxt->obj.get_cali);
            return -EINVAL;
        } else if ((err = cxt->obj.get_cali(cxt->obj.self, &cali))) {
            HWM_ERR("get cali_acc fail: %d\n", err);
            return -EINVAL;            
        } else if (copy_to_user(argp, &cali, sizeof(cali))) {
            HWM_ERR("copy_to_user fail!!\n");
            return -EFAULT;
        } else {
            return 0;
        }
    } else if (cmd == HWM_IOCS_ACC_CALI) {
        struct hwmsen_data cali;
        struct hwmsen_context *cxt = &obj->dc->cxt[HWM_SENSOR_ACCELERATION];
        if (!atomic_read(&cxt->init) || !atomic_read(&cxt->enable) || !cxt->obj.set_cali) {
            HWM_ERR("cali_acc: disabled (%d, %d, %p)\n", atomic_read(&cxt->init), 
                    atomic_read(&cxt->enable), cxt->obj.set_cali);
            return -EINVAL;
        } else if (copy_from_user(&cali, argp, sizeof(cali))) {
            HWM_ERR("copy_from_user fail!!\n");
            return -EFAULT;
        } else if ((err = cxt->obj.set_cali(cxt->obj.self, &cali))) {
            HWM_ERR("set cali_acc fail: %d\n", err);
            return -EINVAL;            
        } else {
            return 0;
        }
    }
    
    return -ENOIOCTLCMD;        
}
/*----------------------------------------------------------------------------*/
static struct file_operations hwmsen_fops = {
    .owner  = THIS_MODULE,
    .open   = hwmsen_open,
    .release= hwmsen_release,
    .ioctl  = hwmsen_ioctl,
};
/*----------------------------------------------------------------------------*/
static int hwmsen_probe(struct platform_device *pdev) 
{
    struct hwmdev_object *devobj = NULL;
    int idx, pos, err;
    struct hwmsen_context *cxt = NULL;
    struct hwmsen_prop prop;

    devobj = hwmsen_alloc_object();
    if (!devobj) {
        err = -ENOMEM;
        HWM_ERR("unable to allocate devobj!!\n");
        goto exit_alloc_data_failed;
    }
    
    devobj->idev = input_allocate_device();
    if (!devobj->idev) {
        err = -ENOMEM;
        HWM_ERR("unable to allocate input device!!\n");
        goto exit_alloc_input_dev_failed;
    }
    set_bit(EV_ABS, devobj->idev->evbit);
    set_bit(EV_SYN, devobj->idev->evbit);

    for (idx = 0; idx < C_MAX_HWMSEN_NUM; idx++) {
        cxt = &devobj->dc->cxt[idx];
        if (!atomic_read(&cxt->init))
            continue;
        if (!cxt->obj.get_prop)
            continue;
        
        cxt->info = &sen_info[idx];        
        err = cxt->obj.get_prop(cxt->obj.self, &prop);
        if (err)
            goto exit_get_hwmsen_info_failed;        

        for (pos = 0; pos < prop.num; pos++) {
            cxt->info->evt[pos].max = prop.evt_max[pos];
            cxt->info->evt[pos].min = prop.evt_min[pos];            
            input_set_abs_params(devobj->idev, cxt->info->evt[pos].code, 
                                 cxt->info->evt[pos].min, cxt->info->evt[pos].max, 0, 0);
            printk("%s: input_set_abs_params(%d, %d, %d)\n", __func__, cxt->info->evt[pos].code,
                   cxt->info->evt[pos].min, cxt->info->evt[pos].max);
        }
    }
    devobj->idev->name = HWM_INPUTDEV_NAME;
    if ((err = input_register_device(devobj->idev))) {
        HWM_ERR("unable to register input device!!\n");
        goto exit_input_register_device_failed;
    }
    input_set_drvdata(devobj->idev, devobj);
    
    devobj->mdev.minor = MISC_DYNAMIC_MINOR;
    devobj->mdev.name  = HWM_SENSOR_DEV_NAME;
    devobj->mdev.fops  = &hwmsen_fops;
    if ((err = misc_register(&devobj->mdev))) {
        HWM_ERR("unable to register sensor device!!\n");
        goto exit_misc_register_failed;
    }
    dev_set_drvdata(devobj->mdev.this_device, devobj);

    if ((err = hwmsen_add_object(devobj))) {
        HWM_ERR("unable to attach object!!\n");
        goto exit_attach_object;        
    }
    if ((err = hwmsen_create_attr(devobj->mdev.this_device))) {
        HWM_ERR("unable to create attributes!!\n");
        goto exit_hwmsen_create_attr_failed;
    }
    return 0;

exit_hwmsen_create_attr_failed:
exit_attach_object:
exit_misc_register_failed:    
exit_get_hwmsen_info_failed:
exit_input_register_device_failed:
    input_free_device(devobj->idev);
exit_alloc_input_dev_failed:    
    kfree(devobj);
exit_alloc_data_failed:
    return err;
}
/*----------------------------------------------------------------------------*/
static int hwmsen_remove(struct platform_device *pdev)
{
    struct hwmdev_object *devobj = (struct hwmdev_object*)dev_get_drvdata(&pdev->dev);

    if (devobj) {
        input_unregister_device(devobj->idev);
        hwmsen_del_object(devobj);        
        hwmsen_delete_attr(devobj->mdev.this_device);
        misc_deregister(&devobj->mdev);
        kfree(devobj);
    }
    return 0;
}
/*----------------------------------------------------------------------------*/
static int hwmsen_suspend(struct platform_device *dev, pm_message_t state) 
{
    return 0;
}
/*----------------------------------------------------------------------------*/
static int hwmsen_resume(struct platform_device *dev)
{
    return 0;
}
/*----------------------------------------------------------------------------*/
static struct platform_driver hwmsen_driver = {
    .probe      = hwmsen_probe,
    .remove     = hwmsen_remove,    
    .suspend    = hwmsen_suspend,
    .resume     = hwmsen_resume,
    .driver     = {
        .name = HWM_SENSOR_DEV_NAME,
        .owner = THIS_MODULE,
     }
};
/*----------------------------------------------------------------------------*/
static int __init hwmsen_init(void) 
{
    if (platform_driver_register(&hwmsen_driver)) {
        HWM_ERR("failed to register sensor driver");
        return -ENODEV;
    }    
    return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit hwmsen_exit(void)
{
    platform_driver_unregister(&hwmsen_driver);    
}
/*----------------------------------------------------------------------------*/
module_init(hwmsen_init);
module_exit(hwmsen_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("sensor device driver");
MODULE_AUTHOR("MingHsien Hsieh<minghsien.hsieh@mediatek.com");

