

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <linux/earlysuspend.h>

#include <mach/mt6516_devs.h>
#include <mach/mt6516_typedefs.h>
#include <mach/mt6516_gpio.h>
#include <mach/mt6516_pll.h>

#include <cust_mag.h>
#include "ami304.h"
#include "hwmsen_helper.h"
/*----------------------------------------------------------------------------*/
#define I2C_DRIVERID_AMI304 304
#define DEBUG 1
#define AMI304_DRV_NAME         "ami304"
#define DRIVER_VERSION          "1.0.6.11"
/*----------------------------------------------------------------------------*/
#define AMI304_AXIS_X            0
#define AMI304_AXIS_Y            1
#define AMI304_AXIS_Z            2
#define AMI304_AXES_NUM          3
/*----------------------------------------------------------------------------*/
#define AMI_TAG                  "[AMI304] "
#define AMI_FUN(f)               printk(KERN_INFO AMI_TAG"%s\n", __FUNCTION__)
#define AMI_ERR(fmt, args...)    printk(KERN_ERR AMI_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define AMI_LOG(fmt, args...)    printk(KERN_INFO AMI_TAG fmt, ##args)
#define AMI_VER(fmt, args...)   ((void)0)
/*----------------------------------------------------------------------------*/
static struct i2c_client *ami304_i2c_client = NULL;
/*----------------------------------------------------------------------------*/
/* Addresses to scan */
static unsigned short normal_i2c[] = { AMI304_I2C_ADDRESS, I2C_CLIENT_END };
/*----------------------------------------------------------------------------*/
/* Insmod parameters */
I2C_CLIENT_INSMOD_1(ami304);
/*----------------------------------------------------------------------------*/
static int ami304_i2c_attach_adapter(struct i2c_adapter *adapter);
static int ami304_i2c_detect(struct i2c_adapter *adapter, int address, int kind);
static int ami304_i2c_detach_client(struct i2c_client *client);
/*----------------------------------------------------------------------------*/
typedef enum {
    AMI_TRC_DEBUG  = 0x01,
} AMI_TRC;
/*----------------------------------------------------------------------------*/
struct _ami302_data {
    rwlock_t lock;
    int mode;
    int rate;
    volatile int updated;
} ami304_data;
/*----------------------------------------------------------------------------*/
struct _ami304mid_data {
    rwlock_t datalock;
    rwlock_t ctrllock;    
    int controldata[10];
    unsigned int debug;
    int yaw;
    int roll;
    int pitch;
    int nmx;
    int nmy;
    int nmz;
    int nax;
    int nay;
    int naz;
    int mag_status;
} ami304mid_data;
/*----------------------------------------------------------------------------*/
struct ami304_i2c_data {
    struct i2c_client client;
    struct mag_hw *hw;
    struct hwmsen_convert   cvt;
    atomic_t layout;   
    atomic_t trace;
#if defined(CONFIG_HAS_EARLYSUSPEND)    
    struct early_suspend    early_drv;
#endif 
};
/*----------------------------------------------------------------------------*/
static struct i2c_driver ami304_i2c_driver = {
    .driver = {
        .owner = THIS_MODULE, 
        .name  = AMI304_DRV_NAME,
    },
    .attach_adapter = ami304_i2c_attach_adapter,
    .detach_client  = ami304_i2c_detach_client,    
#if !defined(CONFIG_HAS_EARLYSUSPEND)
    .suspend    = ami304_suspend,
    .resume     = ami304_resume,
#endif 
    .id         = I2C_DRIVERID_AMI304,
};
/*----------------------------------------------------------------------------*/
static atomic_t dev_open_count;
static atomic_t hal_open_count;
static atomic_t daemon_open_count;
/*----------------------------------------------------------------------------*/
static void ami304_power(struct mag_hw *hw, unsigned int on) 
{
    static unsigned int power_on = 0;

    if (hw->power_id != MT6516_POWER_NONE) {        
        AMI_LOG("power %s\n", on ? "on" : "off");
        if (power_on == on) {
            AMI_LOG("ignore power control: %d\n", on);
        } else if (on) {
            if (!hwPowerOn(hw->power_id, hw->power_vol, "AMI304")) 
                AMI_ERR("power on fails!!\n");
        } else {
            if (!hwPowerDown(hw->power_id, "AMI304")) 
                AMI_ERR("power off fail!!\n");   
        }
    }
    power_on = on;    
}
/*----------------------------------------------------------------------------*/
static int AMI304_Chipset_Init(int mode)
{
    u8 databuf[10];
    u8 ctrl1, ctrl2, ctrl3;
    int err;

    if ((err = hwmsen_read_byte(ami304_i2c_client, AMI304_REG_CTRL1, &ctrl1))) {
        AMI_ERR("read CTRL1 fail: %d\n", err);
        return err;
    }
    if ((err = hwmsen_read_byte(ami304_i2c_client, AMI304_REG_CTRL2, &ctrl2))) {
        AMI_ERR("read CTRL2 fail: %d\n", err);
        return err;        
    }
    if ((err = hwmsen_read_byte(ami304_i2c_client, AMI304_REG_CTRL3, &ctrl3))) {
        AMI_ERR("read CTRL3 fail: %d\n", err);
        return err;        
    }     
    
    databuf[0] = AMI304_REG_CTRL1;
    if( mode==AMI304_FORCE_MODE )
    {
        databuf[1] = ctrl1 | AMI304_CTRL1_PC1 | AMI304_CTRL1_FS1_FORCE;
        write_lock(&ami304_data.lock);
        ami304_data.mode = AMI304_FORCE_MODE;
        write_unlock(&ami304_data.lock);            
    }
    else    
    {
        databuf[1] = ctrl1 | AMI304_CTRL1_PC1 | AMI304_CTRL1_FS1_NORMAL | AMI304_CTRL1_ODR1;
        write_lock(&ami304_data.lock);
        ami304_data.mode = AMI304_NORMAL_MODE;
        write_unlock(&ami304_data.lock);            
    }
    i2c_master_send(ami304_i2c_client, databuf, 2);        
    
    databuf[0] = AMI304_REG_CTRL2;
    databuf[1] = ctrl2 | AMI304_CTRL2_DREN;
    i2c_master_send(ami304_i2c_client, databuf, 2);        
    
    databuf[0] = AMI304_REG_CTRL3;
    databuf[1] = ctrl3 | AMI304_CTRL3_B0_LO_CLR;
    i2c_master_send(ami304_i2c_client, databuf, 2);                
    return 0;
}
/*----------------------------------------------------------------------------*/
static int AMI304_SetMode(int newmode)
{
    int mode = 0;
    
    read_lock(&ami304_data.lock);
    mode = ami304_data.mode;
    read_unlock(&ami304_data.lock);        
    
    if (mode == newmode) 
        return 0;    
            
   return AMI304_Chipset_Init(newmode);
}
/*----------------------------------------------------------------------------*/
static int AMI304_ReadChipInfo(char *buf, int bufsize)
{
    if ((!buf)||(bufsize<=30))
        return -1;
    if (!ami304_i2c_client)
    {
        *buf = 0;
        return -2;
    }

    sprintf(buf, "AMI304 Chip");
    return 0;
}
/*----------------------------------------------------------------------------*/
static int AMI304_ReadSensorData(char *buf, int bufsize)
{
    struct ami304_i2c_data *data = i2c_get_clientdata(ami304_i2c_client);
    char cmd;
    int mode = 0;    
    unsigned char databuf[10];
    short output[3];
    int mag[AMI304_AXES_NUM];

    if ((!buf)||(bufsize<=80))
        return -1;
    if (!ami304_i2c_client)
    {
        *buf = 0;
        return -2;
    }
    
    read_lock(&ami304_data.lock);    
    mode = ami304_data.mode;
    read_unlock(&ami304_data.lock);        

    databuf[0] = AMI304_REG_CTRL3;
    databuf[1] = AMI304_CTRL3_FORCE_BIT;
    i2c_master_send(ami304_i2c_client, databuf, 2);    
    // We can read all measured data in once
    cmd = AMI304_REG_DATAXH;
    i2c_master_send(ami304_i2c_client, &cmd, 1);    
    i2c_master_recv(ami304_i2c_client, &(databuf[0]), 6);
    
    output[0] = ((int) databuf[1]) << 8 | ((int) databuf[0]);
    output[1] = ((int) databuf[3]) << 8 | ((int) databuf[2]);
    output[2] = ((int) databuf[5]) << 8 | ((int) databuf[4]);

    mag[data->cvt.map[AMI304_AXIS_X]] = data->cvt.sign[AMI304_AXIS_X]*output[AMI304_AXIS_X];
    mag[data->cvt.map[AMI304_AXIS_Y]] = data->cvt.sign[AMI304_AXIS_Y]*output[AMI304_AXIS_Y];
    mag[data->cvt.map[AMI304_AXIS_Z]] = data->cvt.sign[AMI304_AXIS_Z]*output[AMI304_AXIS_Z];
    
    sprintf(buf, "%04x %04x %04x", mag[AMI304_AXIS_X], mag[AMI304_AXIS_Y], mag[AMI304_AXIS_Z]);
    
    return 0;
}
/*----------------------------------------------------------------------------*/
static int AMI304_ReadPostureData(char *buf, int bufsize)
{
    if ((!buf)||(bufsize<=80))
        return -1;

    read_lock(&ami304mid_data.datalock);
    sprintf(buf, "%d %d %d %d", ami304mid_data.yaw, ami304mid_data.pitch, ami304mid_data.roll, ami304mid_data.mag_status);
    read_unlock(&ami304mid_data.datalock);
    return 0;
}
/*----------------------------------------------------------------------------*/
static int AMI304_ReadCaliData(char *buf, int bufsize)
{
    if ((!buf)||(bufsize<=80))
        return -1;

    read_lock(&ami304mid_data.datalock);
    sprintf(buf, "%d %d %d %d %d %d %d", ami304mid_data.nmx, ami304mid_data.nmy, ami304mid_data.nmz,ami304mid_data.nax,ami304mid_data.nay,ami304mid_data.naz,ami304mid_data.mag_status);
    read_unlock(&ami304mid_data.datalock);
    return 0;
}
/*----------------------------------------------------------------------------*/
static int AMI304_ReadMiddleControl(char *buf, int bufsize)
{
    if ((!buf)||(bufsize<=80))
        return -1;

    read_lock(&ami304mid_data.ctrllock);
    sprintf(buf, "%d %d %d %d %d %d %d %d %d %d", 
        ami304mid_data.controldata[0], ami304mid_data.controldata[1], ami304mid_data.controldata[2],ami304mid_data.controldata[3],ami304mid_data.controldata[4],
        ami304mid_data.controldata[5], ami304mid_data.controldata[6], ami304mid_data.controldata[7], ami304mid_data.controldata[8], ami304mid_data.controldata[9]);
    read_unlock(&ami304mid_data.ctrllock);
    return 0;
}
/*----------------------------------------------------------------------------*/
static ssize_t show_chipinfo_value(struct device *dev, struct device_attribute *attr, char *buf)
{
    char strbuf[AMI304_BUFSIZE];
    AMI304_ReadChipInfo(strbuf, AMI304_BUFSIZE);
    return sprintf(buf, "%s\n", strbuf);        
}
/*----------------------------------------------------------------------------*/
static ssize_t show_sensordata_value(struct device *dev, struct device_attribute *attr, char *buf)
{
    char strbuf[AMI304_BUFSIZE];
    AMI304_ReadSensorData(strbuf, AMI304_BUFSIZE);
    return sprintf(buf, "%s\n", strbuf);            
}
/*----------------------------------------------------------------------------*/
static ssize_t show_posturedata_value(struct device *dev, struct device_attribute *attr, char *buf)
{
    char strbuf[AMI304_BUFSIZE];
    AMI304_ReadPostureData(strbuf, AMI304_BUFSIZE);
    return sprintf(buf, "%s\n", strbuf);            
}
/*----------------------------------------------------------------------------*/
static ssize_t show_calidata_value(struct device *dev, struct device_attribute *attr, char *buf)
{
    char strbuf[AMI304_BUFSIZE];
    AMI304_ReadCaliData(strbuf, AMI304_BUFSIZE);
    return sprintf(buf, "%s\n", strbuf);            
}
/*----------------------------------------------------------------------------*/
static ssize_t show_midcontrol_value(struct device *dev, struct device_attribute *attr, char *buf)
{
    char strbuf[AMI304_BUFSIZE];
    AMI304_ReadMiddleControl(strbuf, AMI304_BUFSIZE);
    return sprintf(buf, "%s\n", strbuf);            
}
/*----------------------------------------------------------------------------*/
static ssize_t store_midcontrol_value(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{   
    int p[10];
    if (10 == sscanf(buf, "%d %d %d %d %d %d %d %d %d %d", 
                     &p[0], &p[1], &p[2], &p[3], &p[4], &p[5], &p[6], &p[7], &p[8], &p[9])) {
        write_lock(&ami304mid_data.ctrllock);
        memcpy(&ami304mid_data.controldata[0], &p, sizeof(int)*10);    
        write_unlock(&ami304mid_data.ctrllock);        
    } else {
        AMI_ERR("invalid format\n");     
    }
    return count;            
}
/*----------------------------------------------------------------------------*/
static ssize_t show_middebug_value(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t len;
    read_lock(&ami304mid_data.ctrllock);
    len = sprintf(buf, "0x%08X\n", ami304mid_data.debug);
    read_unlock(&ami304mid_data.ctrllock);

    return len;            
}
/*----------------------------------------------------------------------------*/
static ssize_t store_middebug_value(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{   
    int debug;
    if (1 == sscanf(buf, "0x%x", &debug)) {
        write_lock(&ami304mid_data.ctrllock);
        ami304mid_data.debug = debug;
        write_unlock(&ami304mid_data.ctrllock);        
    } else {
        AMI_ERR("invalid format\n");     
    }
    return count;            
}
/*----------------------------------------------------------------------------*/
static ssize_t show_mode_value(struct device *dev, struct device_attribute *attr, char *buf)
{
    int mode=0;
    read_lock(&ami304_data.lock);
    mode = ami304_data.mode;
    read_unlock(&ami304_data.lock);        
    return sprintf(buf, "%d\n", mode);            
}
/*----------------------------------------------------------------------------*/
static ssize_t store_mode_value(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int mode = 0;
    sscanf(buf, "%d", &mode);    
    AMI304_SetMode(mode);
    return count;            
}
/*----------------------------------------------------------------------------*/
static ssize_t show_layout_value(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);  
    struct ami304_i2c_data *data = i2c_get_clientdata(client);
    
    return sprintf(buf, "(%d, %d)\n[%+2d %+2d %+2d]\n[%+2d %+2d %+2d]\n", 
                   data->hw->direction, atomic_read(&data->layout),
                   data->cvt.sign[0], data->cvt.sign[1], data->cvt.sign[2],
                   data->cvt.map[0], data->cvt.map[1], data->cvt.map[2]
                   );            
}
/*----------------------------------------------------------------------------*/
static ssize_t store_layout_value(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);  
    struct ami304_i2c_data *data = i2c_get_clientdata(client);
    int layout = 0;
    
    if (1 == sscanf(buf, "%d", &layout)) {
        atomic_set(&data->layout, layout);
        if (!hwmsen_get_convert(layout, &data->cvt)) {
        } else if (!hwmsen_get_convert(data->hw->direction, &data->cvt)) {
            AMI_ERR("invalid layout: %d, restore to %d\n", layout, data->hw->direction);
        } else {
            AMI_ERR("invalid layout: (%d, %d)\n", layout, data->hw->direction);
            hwmsen_get_convert(0, &data->cvt);
        }
    } else {
        AMI_ERR("invalid format = '%s'\n", buf);
    }
    return count;            
}
/*----------------------------------------------------------------------------*/
static ssize_t show_status_value(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);  
    struct ami304_i2c_data *data = i2c_get_clientdata(client);
    ssize_t len = 0;

    if (data->hw) {
        len += snprintf(buf+len, PAGE_SIZE-len, "CUST: %d %d (%d %d)\n", 
                        data->hw->i2c_num, data->hw->direction, data->hw->power_id, data->hw->power_vol);
    } else {
        len += snprintf(buf+len, PAGE_SIZE-len, "CUST: NULL\n");
    }
    len += snprintf(buf+len, PAGE_SIZE-len, "OPEN: %d %d %d\n", atomic_read(&dev_open_count),
           atomic_read(&daemon_open_count), atomic_read(&hal_open_count));
    return len;
}
/*----------------------------------------------------------------------------*/
static ssize_t show_trace_value(struct device* dev, 
                             struct device_attribute *attr, char *buf)
{
    ssize_t res;
    struct ami304_i2c_data *obj;
    if (!dev) {
        AMI_ERR("dev is null!!\n");
        return 0;
    } else if (!(obj = (struct ami304_i2c_data*)dev_get_drvdata(dev))) {
        AMI_ERR("drv data is null!!\n");
        return 0;
    }
    res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&obj->trace));     
    return res;    
}
/*----------------------------------------------------------------------------*/
static ssize_t store_trace_value(struct device* dev, struct device_attribute *attr,
                              const char *buf, size_t count)
{
    struct ami304_i2c_data *obj;
    int trace;
    if (!dev) {
        AMI_ERR("dev is null!!\n");
        return 0;
    } else if (!(obj = (struct ami304_i2c_data*)dev_get_drvdata(dev))) {
        AMI_ERR("drv data is null!!\n");
        return 0;
    }
    if (1 == sscanf(buf, "0x%x", &trace)) 
        atomic_set(&obj->trace, trace);
    else 
        AMI_ERR("invalid content: '%s', length = %d\n", buf, count);
    return count;    
}
/*----------------------------------------------------------------------------*/
static DEVICE_ATTR(chipinfo,    S_IRUGO, show_chipinfo_value, NULL);
static DEVICE_ATTR(sensordata,  S_IRUGO, show_sensordata_value, NULL);
static DEVICE_ATTR(posturedata, S_IRUGO, show_posturedata_value, NULL);
static DEVICE_ATTR(calidata,    S_IRUGO, show_calidata_value, NULL);
static DEVICE_ATTR(midcontrol,  S_IRUGO | S_IWUSR, show_midcontrol_value, store_midcontrol_value );
static DEVICE_ATTR(middebug,    S_IRUGO | S_IWUSR, show_middebug_value, store_middebug_value );
static DEVICE_ATTR(mode,        S_IRUGO | S_IWUSR, show_mode_value, store_mode_value );
static DEVICE_ATTR(layout,      S_IRUGO | S_IWUSR, show_layout_value, store_layout_value );
static DEVICE_ATTR(status,      S_IRUGO, show_status_value, NULL);
static DEVICE_ATTR(trace,       S_IRUGO | S_IWUSR, show_trace_value, store_trace_value );
/*----------------------------------------------------------------------------*/
static struct attribute *ami304_attributes[] = {
   &dev_attr_chipinfo.attr,
   &dev_attr_sensordata.attr,
   &dev_attr_posturedata.attr,
   &dev_attr_calidata.attr,
   &dev_attr_midcontrol.attr,
   &dev_attr_middebug.attr,
   &dev_attr_mode.attr,
   &dev_attr_layout.attr,
   &dev_attr_status.attr,
   &dev_attr_trace.attr,
   NULL
};
/*----------------------------------------------------------------------------*/
static struct attribute_group ami304_attribute_group = {
   .attrs = ami304_attributes
};
/*----------------------------------------------------------------------------*/
static int ami304_open(struct inode *inode, struct file *file)
{    
    struct ami304_i2c_data *obj = i2c_get_clientdata(ami304_i2c_client);    
    int ret = -1;
    if( atomic_cmpxchg(&dev_open_count, 0, 1)==0 ) {
        if (atomic_read(&obj->trace) & AMI_TRC_DEBUG)
            AMI_LOG("Open device node:ami304\n");
        ret = nonseekable_open(inode, file);
    } else {
        AMI_ERR("open ami304 fail\n");
    }    
    return ret;
}
/*----------------------------------------------------------------------------*/
static int ami304_release(struct inode *inode, struct file *file)
{
    struct ami304_i2c_data *obj = i2c_get_clientdata(ami304_i2c_client);    
    atomic_set(&dev_open_count, 0);
    if (atomic_read(&obj->trace) & AMI_TRC_DEBUG)
        AMI_LOG("Release device node:ami304\n");
    return 0;
}
/*----------------------------------------------------------------------------*/
static int ami304_ioctl(struct inode *inode, struct file *file, unsigned int cmd,unsigned long arg)
{
    char strbuf[AMI304_BUFSIZE];
    int controlbuf[10];
    void __user *data;
    int retval=0;
    int mode=0;

    //check the authority is root or not
    //if(!capable(CAP_SYS_ADMIN)) { 
    //    retval = -EPERM;
    //    goto err_out;
    //}
        
    switch (cmd) {
        case AMI304_IOCTL_INIT:
            read_lock(&ami304_data.lock);
            mode = ami304_data.mode;
            read_unlock(&ami304_data.lock);
            AMI304_Chipset_Init(mode);         
            break;
        
        case AMI304_IOCTL_READ_CHIPINFO:
            data = (void __user *) arg;
            if (data == NULL)
                break;    
            AMI304_ReadChipInfo(strbuf, AMI304_BUFSIZE);
            if (copy_to_user(data, strbuf, strlen(strbuf)+1)) {
                retval = -EFAULT;
                goto err_out;
            }                
            break;

        case AMI304_IOCTL_READ_SENSORDATA:
            data = (void __user *) arg;
            if (data == NULL)
                break;    
            AMI304_ReadSensorData(strbuf, AMI304_BUFSIZE);
            if (copy_to_user(data, strbuf, strlen(strbuf)+1)) {
                retval = -EFAULT;
                goto err_out;
            }                
            break;                
                        
        case AMI304_IOCTL_READ_POSTUREDATA:
            data = (void __user *) arg;
            if (data == NULL)
                break;    
            AMI304_ReadPostureData(strbuf, AMI304_BUFSIZE);
            if (copy_to_user(data, strbuf, strlen(strbuf)+1)) {
                retval = -EFAULT;
                goto err_out;
            }                
            break;            
     
        case AMI304_IOCTL_READ_CALIDATA:
            data = (void __user *) arg;
            if (data == NULL)
                break;    
            AMI304_ReadCaliData(strbuf, AMI304_BUFSIZE);
            if (copy_to_user(data, strbuf, strlen(strbuf)+1)) {
                retval = -EFAULT;
                goto err_out;
            }                
                break;
            
        case AMI304_IOCTL_READ_CONTROL:
            read_lock(&ami304mid_data.ctrllock);
            memcpy(controlbuf, &ami304mid_data.controldata[0], sizeof(controlbuf));
            read_unlock(&ami304mid_data.ctrllock);            
            data = (void __user *) arg;
            if (data == NULL)
                break;    
            if (copy_to_user(data, controlbuf, sizeof(controlbuf))) {
                retval = -EFAULT;
                goto err_out;
            }                                
            break;

        case AMI304_IOCTL_SET_CONTROL:
            data = (void __user *) arg;
            if (data == NULL)
                break;    
            if (copy_from_user(controlbuf, data, sizeof(controlbuf))) {
                retval = -EFAULT;
                goto err_out;
            }    
            write_lock(&ami304mid_data.ctrllock);
            memcpy(&ami304mid_data.controldata[0], controlbuf, sizeof(controlbuf));
            write_unlock(&ami304mid_data.ctrllock);        
            break;
            
        case AMI304_IOCTL_SET_MODE:
            data = (void __user *) arg;
            if (data == NULL)
                break;    
            if (copy_from_user(&mode, data, sizeof(mode))) {
                retval = -EFAULT;
                goto err_out;
            }        
            AMI304_SetMode(mode);                
            break;
                                            
        default:
            AMI_ERR("%s not supported = 0x%04x", __FUNCTION__, cmd);
            retval = -ENOIOCTLCMD;
            break;
    }
    
err_out:
    return retval;    
}
/*----------------------------------------------------------------------------*/
static int ami304daemon_open(struct inode *inode, struct file *file)
{
    struct ami304_i2c_data *obj = i2c_get_clientdata(ami304_i2c_client);    
    int ret = -1;
    if( atomic_cmpxchg(&daemon_open_count, 0, 1)==0 ) {
        if (atomic_read(&obj->trace) & AMI_TRC_DEBUG)
            AMI_LOG("Open device node:ami304daemon\n");
        ret = 0;
    } else {
        AMI_ERR("open daemon fail\n");
    }
    return ret;    
}
/*----------------------------------------------------------------------------*/
static int ami304daemon_release(struct inode *inode, struct file *file)
{
    struct ami304_i2c_data *obj = i2c_get_clientdata(ami304_i2c_client);    
    atomic_set(&daemon_open_count, 0);
    if (atomic_read(&obj->trace) & AMI_TRC_DEBUG)
        AMI_LOG("Release device node:ami304daemon\n");    
    return 0;
}
/*----------------------------------------------------------------------------*/
static int ami304daemon_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
       unsigned long arg)
{
    int valuebuf[4];
    int calidata[7];
    int controlbuf[10];
    char strbuf[AMI304_BUFSIZE];
    void __user *data;
    int retval=0;
    int mode;

    //check the authority is root or not
    //if(!capable(CAP_SYS_ADMIN)) { /*remove this because sensor library will set/get control*/
    //    retval = -EPERM;
    //    goto err_out;
    //}
 
    switch (cmd) {
            
        case AMI304MID_IOCTL_GET_SENSORDATA:
            data = (void __user *) arg;
            if (data == NULL)
                break;    
            AMI304_ReadSensorData(strbuf, AMI304_BUFSIZE);
            if (copy_to_user(data, strbuf, strlen(strbuf)+1)) {
                retval = -EFAULT;
                goto err_out;
            }
            break;
                
        case AMI304MID_IOCTL_SET_POSTURE:
            data = (void __user *) arg;
            if (data == NULL)
                break;    
            if (copy_from_user(&valuebuf, data, sizeof(valuebuf))) {
                retval = -EFAULT;
                goto err_out;
            }                
            write_lock(&ami304mid_data.datalock);
            ami304mid_data.yaw   = valuebuf[0];
            ami304mid_data.pitch = valuebuf[1];
            ami304mid_data.roll  = valuebuf[2];
            ami304mid_data.mag_status = valuebuf[3];
            write_unlock(&ami304mid_data.datalock);    
            break;        
            
        case AMI304MID_IOCTL_SET_CALIDATA:
            data = (void __user *) arg;
            if (data == NULL)
                break;    
            if (copy_from_user(&calidata, data, sizeof(calidata))) {
                retval = -EFAULT;
                goto err_out;
            }    
            write_lock(&ami304mid_data.datalock);            
            ami304mid_data.nmx = calidata[0];
            ami304mid_data.nmy = calidata[1];
            ami304mid_data.nmz = calidata[2];
            ami304mid_data.nax = calidata[3];
            ami304mid_data.nay = calidata[4];
            ami304mid_data.naz = calidata[5];
            ami304mid_data.mag_status = calidata[6];
            write_unlock(&ami304mid_data.datalock);    
            break;                                

        case AMI304MID_IOCTL_GET_CONTROL:
            read_lock(&ami304mid_data.ctrllock);
            memcpy(controlbuf, &ami304mid_data.controldata[0], sizeof(controlbuf));
            read_unlock(&ami304mid_data.ctrllock);            
            data = (void __user *) arg;
            if (data == NULL)
                break;    
            if (copy_to_user(data, controlbuf, sizeof(controlbuf))) {
                retval = -EFAULT;
                goto err_out;
            }                    
            break;        
            
        case AMI304MID_IOCTL_SET_CONTROL:
            data = (void __user *) arg;
            if (data == NULL)
                break;    
            if (copy_from_user(controlbuf, data, sizeof(controlbuf))) {
                retval = -EFAULT;
                goto err_out;
            }    
            write_lock(&ami304mid_data.ctrllock);
            memcpy(&ami304mid_data.controldata[0], controlbuf, sizeof(controlbuf));
            write_unlock(&ami304mid_data.ctrllock);
            break;    
    
        case AMI304MID_IOCTL_SET_MODE:
            data = (void __user *) arg;
            if (data == NULL)
                break;    
            if (copy_from_user(&mode, data, sizeof(mode))) {
                retval = -EFAULT;
                goto err_out;
            }        
            AMI304_SetMode(mode);                
            break;
            
        default:
            AMI_ERR("%s not supported = 0x%04x", __FUNCTION__, cmd);
            retval = -ENOIOCTLCMD;
            break;
    }
    
err_out:
    return retval;    
}
/*----------------------------------------------------------------------------*/
static int ami304hal_open(struct inode *inode, struct file *file)
{
    struct ami304_i2c_data *obj = i2c_get_clientdata(ami304_i2c_client);    
    atomic_inc(&hal_open_count);
    if (atomic_read(&obj->trace) & AMI_TRC_DEBUG)
        AMI_LOG("Open ami304hal %d times.\n", atomic_read(&hal_open_count));    
    return 0;
}
/*----------------------------------------------------------------------------*/
static int ami304hal_release(struct inode *inode, struct file *file)
{
    struct ami304_i2c_data *obj = i2c_get_clientdata(ami304_i2c_client);    
    atomic_dec(&hal_open_count);
    if (atomic_read(&obj->trace) & AMI_TRC_DEBUG)
        AMI_LOG("Release ami304hal, remainder is %d times.\n", atomic_read(&hal_open_count));    
    return 0;
}
/*----------------------------------------------------------------------------*/
static int ami304hal_ioctl(struct inode *inode, struct file *file, unsigned int cmd,unsigned long arg)
{
    int controlbuf[10];
    char strbuf[AMI304_BUFSIZE];
    void __user *data;
    int retval=0;
        
    switch (cmd) {
        
        case AMI304HAL_IOCTL_GET_SENSORDATA:
            data = (void __user *) arg;
            if (data == NULL)
                break;    
            AMI304_ReadSensorData(strbuf, AMI304_BUFSIZE);
            if (copy_to_user(data, strbuf, strlen(strbuf)+1)) {
                retval = -EFAULT;
                goto err_out;
            }        
            break;
                                    
        case AMI304HAL_IOCTL_GET_POSTURE:
            data = (void __user *) arg;
            if (data == NULL)
                break;    
            AMI304_ReadPostureData(strbuf, AMI304_BUFSIZE);
            if (copy_to_user(data, strbuf, strlen(strbuf)+1)) {
                retval = -EFAULT;
                goto err_out;
            }                
            break;            
     
        case AMI304HAL_IOCTL_GET_CALIDATA:
            data = (void __user *) arg;
            if (data == NULL)
                break;    
            AMI304_ReadCaliData(strbuf, AMI304_BUFSIZE);
            if (copy_to_user(data, strbuf, strlen(strbuf)+1)) {
                retval = -EFAULT;
                goto err_out;
            }                
                break;
        case AMI304HAL_IOCTL_GET_CONTROL:
            read_lock(&ami304mid_data.ctrllock);
            memcpy(controlbuf, &ami304mid_data.controldata[0], sizeof(controlbuf));
            read_unlock(&ami304mid_data.ctrllock);         
            data = (void __user *) arg;
            if (data == NULL)
                break;   
            if (copy_to_user(data, controlbuf, sizeof(controlbuf))) {
                retval = -EFAULT;
                goto err_out;
            }         
            break;

        case AMI304HAL_IOCTL_SET_CONTROL:
            data = (void __user *) arg;
            if (data == NULL)
               break;   
            if (copy_from_user(controlbuf, data, sizeof(controlbuf))) {
               retval = -EFAULT;
               goto err_out;
            }   
            write_lock(&ami304mid_data.ctrllock);
            memcpy(&ami304mid_data.controldata[0], controlbuf, sizeof(controlbuf));
            write_unlock(&ami304mid_data.ctrllock);
            break;
        default:
            AMI_ERR("%s not supported = 0x%04x", __FUNCTION__, cmd);
            retval = -ENOIOCTLCMD;
            break;
    }
    
err_out:
    return retval;    
}
/*----------------------------------------------------------------------------*/
static struct file_operations ami304_fops = {
    .owner = THIS_MODULE,
    .open = ami304_open,
    .release = ami304_release,
    .ioctl = ami304_ioctl,
};
/*----------------------------------------------------------------------------*/
static struct miscdevice ami304_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "ami304",
    .fops = &ami304_fops,
};
/*----------------------------------------------------------------------------*/
static struct file_operations ami304daemon_fops = {
    .owner = THIS_MODULE,
    .open = ami304daemon_open,
    .release = ami304daemon_release,
    .ioctl = ami304daemon_ioctl,
};
/*----------------------------------------------------------------------------*/
static struct miscdevice ami304daemon_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "ami304daemon",
    .fops = &ami304daemon_fops,
};
/*----------------------------------------------------------------------------*/
static struct file_operations ami304hal_fops = {
    .owner = THIS_MODULE,
    .open = ami304hal_open,
    .release = ami304hal_release,
    .ioctl = ami304hal_ioctl,
};
/*----------------------------------------------------------------------------*/
static struct miscdevice ami304hal_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "ami304hal",
    .fops = &ami304hal_fops,
};
/*----------------------------------------------------------------------------*/
#if !defined(CONFIG_HAS_EARLYSUSPEND)
/*----------------------------------------------------------------------------*/
static int ami304_suspend(struct i2c_client *client, pm_message_t msg) 
{
    int err;
    struct ami304_i2c_data *obj = i2c_get_clientdata(client)
    AMI_FUN();    

    if (msg.event == PM_EVENT_SUSPEND) {   
        if ((err = hwmsen_write_byte(client, AMI304_REG_POWER_CTL, 0x00))) {
            AMI_ERR("write power control fail!!\n");
            return err;
        }        
        ami304_power(obj->hw, 0);
    }
    return 0;
}
/*----------------------------------------------------------------------------*/
static int ami304_resume(struct i2c_client *client)
{
    int err;
    struct ami304_i2c_data *obj = i2c_get_clientdata(client)
    AMI_FUN();
    
    ami304_power(obj->hw, 1);
    if ((err = AMI304_Init(AMI304_FORCE_MODE))) {
        AMI_ERR("initialize client fail!!\n");
        return err;        
    }

    return 0;
}
/*----------------------------------------------------------------------------*/
#else /*CONFIG_HAS_EARLY_SUSPEND is defined*/
/*----------------------------------------------------------------------------*/
static void ami304_early_suspend(struct early_suspend *h) 
{
    struct ami304_i2c_data *obj = container_of(h, struct ami304_i2c_data, early_drv);   
    int err;
    AMI_FUN();    

    if (!obj) {
        AMI_ERR("null pointer!!\n");
        return;
    }
    if ((err = hwmsen_write_byte(&obj->client, AMI304_REG_CTRL1, 0x00))) {
        AMI_ERR("write power control fail!!\n");
        return;
    }        
}
/*----------------------------------------------------------------------------*/
static void ami304_late_resume(struct early_suspend *h)
{
    struct ami304_i2c_data *obj = container_of(h, struct ami304_i2c_data, early_drv);         
    int err;
    AMI_FUN();

    if (!obj) {
        AMI_ERR("null pointer!!\n");
        return;
    }
    
    ami304_power(obj->hw, 1);
    if ((err = AMI304_Chipset_Init(AMI304_FORCE_MODE))) {
        AMI_ERR("initialize client fail!!\n");
        return;        
    }    
}
/*----------------------------------------------------------------------------*/
#endif /*CONFIG_HAS_EARLYSUSPEND*/
/*----------------------------------------------------------------------------*/
static int ami304_i2c_attach_adapter(struct i2c_adapter *adapter)
{
    struct mag_hw *hw = get_cust_mag_hw();
    if (adapter->id == hw->i2c_num)    
        return i2c_probe(adapter, &addr_data, ami304_i2c_detect);
    return -1;
}
/*----------------------------------------------------------------------------*/
static int ami304_i2c_detect(struct i2c_adapter *adapter, int address, int kind)
{
    struct i2c_client *new_client;
    struct ami304_i2c_data *data;
    int err = 0;

    if (!(data = kmalloc(sizeof(struct ami304_i2c_data), GFP_KERNEL))) {
        err = -ENOMEM;
        goto exit;
    }
    memset(data, 0, sizeof(struct ami304_i2c_data));

    data->hw = get_cust_mag_hw();
    if ((err = hwmsen_get_convert(data->hw->direction, &data->cvt))) {
        AMI_ERR("invalid direction: %d\n", data->hw->direction);
        goto exit;
    }
    atomic_set(&data->layout, data->hw->direction);
    atomic_set(&data->trace, 0);
    
    new_client = &data->client;
    i2c_set_clientdata(new_client, data);
    new_client->addr = address;
    new_client->adapter = adapter;
    new_client->driver = &ami304_i2c_driver;
    new_client->flags = 0;

    strlcpy(new_client->name, "ami304_i2c", I2C_NAME_SIZE);
    ami304_i2c_client = new_client;
        
    if ((err = i2c_attach_client(new_client)))
        goto exit_kfree;

    if ((err = AMI304_Chipset_Init(AMI304_FORCE_MODE)))
        goto exit_init_failed;

   /* Register sysfs hooks */
   err = sysfs_create_group(&new_client->dev.kobj, &ami304_attribute_group);
   if (err)
      goto exit_sysfs_create_group_failed;
    
    if ((err = misc_register(&ami304_device))) {
        AMI_ERR("ami304_device register failed\n");
        goto exit_misc_device_register_failed;
    }    
    
    err = misc_register(&ami304daemon_device);
    if (err) {
        AMI_ERR("ami304daemon_device register failed\n");
        goto exit_misc_device_register_failed;
    }    

    err = misc_register(&ami304hal_device);
    if (err) {
        AMI_ERR("ami304hal_device register failed\n");
        goto exit_misc_device_register_failed;
    }    
#if defined(CONFIG_HAS_EARLYSUSPEND)
    data->early_drv.level    = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1,
    data->early_drv.suspend  = ami304_early_suspend,
    data->early_drv.resume   = ami304_late_resume,    
    register_early_suspend(&data->early_drv);
#endif 
    AMI_LOG("%s: OK\n", __func__);
    return 0;

exit_sysfs_create_group_failed:   
exit_init_failed:
    i2c_detach_client(new_client);
exit_misc_device_register_failed:
exit_kfree:
    kfree(data);
exit:
    AMI_ERR("%s: err = %d\n", __func__, err);
    return err;
}
/*----------------------------------------------------------------------------*/
static int ami304_i2c_detach_client(struct i2c_client *client)
{
    int err;

    if ((err = i2c_detach_client(client)))
        return err;

    ami304_i2c_client = NULL;
    kfree(i2c_get_clientdata(client));
    misc_deregister(&ami304hal_device);
    misc_deregister(&ami304daemon_device);
    misc_deregister(&ami304_device);    
    return 0;
}
/*----------------------------------------------------------------------------*/
static int ami_probe(struct platform_device *pdev) 
{
    struct mag_hw *hw = get_cust_mag_hw();

    ami304_power(hw, 1);    
    rwlock_init(&ami304mid_data.ctrllock);
    rwlock_init(&ami304mid_data.datalock);
    rwlock_init(&ami304_data.lock);
    memset(&ami304mid_data.controldata[0], 0, sizeof(int)*10);    
    ami304mid_data.controldata[0] =    20;  // Loop Delay
    ami304mid_data.controldata[1] =     0;  // Run   
    ami304mid_data.controldata[2] =     0;  // Disable Start-AccCali
    ami304mid_data.controldata[3] =     1;  // Enable Start-Cali
    ami304mid_data.controldata[4] =   350;  // MW-Timout
    ami304mid_data.controldata[5] =    10;  // MW-IIRStrength_M
    ami304mid_data.controldata[6] =    10;  // MW-IIRStrength_G   
    ami304mid_data.controldata[7] =     0;  // Active Sensors
    ami304mid_data.controldata[8] =     0;  // Wait for define
    ami304mid_data.controldata[9] =     0;  // Wait for define   
    atomic_set(&dev_open_count, 0);    
    atomic_set(&hal_open_count, 0);
    atomic_set(&daemon_open_count, 0);    
    if (i2c_add_driver(&ami304_i2c_driver)) {
        AMI_ERR("add driver error\n");
        return -1;
    } 
    return 0;
}
/*----------------------------------------------------------------------------*/
static int ami_remove(struct platform_device *pdev)
{
    struct mag_hw *hw = get_cust_mag_hw();

    AMI_FUN();    
    ami304_power(hw, 0);    
    atomic_set(&dev_open_count, 0);
    atomic_set(&hal_open_count, 0);
    atomic_set(&daemon_open_count, 0);    
    i2c_del_driver(&ami304_i2c_driver);
    return 0;
}
/*----------------------------------------------------------------------------*/
static struct platform_driver ami_sensor_driver = {
    .probe      = ami_probe,
    .remove     = ami_remove,    
    .driver     = {
        .name  = "ami304",
        .owner = THIS_MODULE,
     }
};

/*----------------------------------------------------------------------------*/
static int __init ami304_init(void)
{
    AMI_FUN();
    if (platform_driver_register(&ami_sensor_driver)) {
        AMI_ERR("failed to register driver");
        return -ENODEV;
    }
    return 0;    
}
/*----------------------------------------------------------------------------*/
static void __exit ami304_exit(void)
{
    AMI_FUN();
    platform_driver_unregister(&ami_sensor_driver);
}
/*----------------------------------------------------------------------------*/
module_init(ami304_init);
module_exit(ami304_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("Kyle K.Y. Chen");
MODULE_DESCRIPTION("AMI304 MI-Sensor driver without DRDY");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRIVER_VERSION);
