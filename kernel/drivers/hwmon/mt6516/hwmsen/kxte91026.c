
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/ctype.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/hwmon-sysfs.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>

#include <mach/mt6516_devs.h>
#include <mach/mt6516_typedefs.h>
#include <mach/mt6516_gpio.h>
#include <mach/mt6516_pll.h>

#include <cust_acc.h>
#include "hwmsen_helper.h"
#include "hwmsen_dev.h"
#include "kxte91026.h"
/*the version string is automatically modified by Perforce*/
#define KXTE9_DRV_VERSION           "$Revision: #3 $"
struct kxte9_object{
	struct i2c_client	    client;
    struct acc_hw *hw;
    struct hwmsen_convert   cvt;
    struct hwmsen_conf      cnf;    

    /*software calibration: the unit follows standard format*/
    s16                     cali_sw[KXTE9_AXES_NUM+1];     
    
    /*misc*/
    atomic_t                enable;
    atomic_t                suspend;
    atomic_t                trace;

    /*data*/
    u8                      data[KXTE9_DATA_LEN+1];
    
};
/*----------------------------------------------------------------------------*/
#define KXTE9_TRC_GET_DATA  0x0001
static int kxte9_attach_adapter(struct i2c_adapter *adapter);
static int kxte9_detach_client(struct i2c_client* client);
static ssize_t kxte9_show_data(struct device *device, struct device_attribute *attr, char* buf);
static ssize_t kxte9_show_reg(struct device *device, struct device_attribute *attr, char *buf);
static ssize_t kxte9_store_reg(struct device* device, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t kxte9_show_dump(struct device *device, struct device_attribute *attr, char *buf);
static ssize_t kxte9_show_calibr(struct device *device, struct device_attribute *attr, char *buf);
static ssize_t kxte9_store_calibr(struct device* device, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t kxte9_show_trace(struct device *device, struct device_attribute *attr, char *buf);
static ssize_t kxte9_store_trace(struct device* device, struct device_attribute *attr, const char *buf, size_t count);
static unsigned short normal_i2c[] = {KXTE9_WR_SLAVE_ADDR, I2C_CLIENT_END};
/*----------------------------------------------------------------------------*/
static unsigned short ignore = I2C_CLIENT_END;
/*----------------------------------------------------------------------------*/
static struct i2c_client_address_data kxte9_addr_data = {
    .normal_i2c = normal_i2c,
    .probe  = &ignore,
    .ignore = &ignore,
};
/*----------------------------------------------------------------------------*/
static struct i2c_driver kxte9_driver = {
    .attach_adapter = kxte9_attach_adapter,
    .detach_client  = kxte9_detach_client,
    .driver = {
        .name = KXT_DEV_NAME,
    },
};
/*----------------------------------------------------------------------------*/
static SENSOR_DEVICE_ATTR(calibr,          S_IWUSR | S_IRUGO, kxte9_show_calibr, kxte9_store_calibr, 0                        );
static SENSOR_DEVICE_ATTR(data           ,           S_IRUGO, kxte9_show_data , NULL,              0                          );
static SENSOR_DEVICE_ATTR(trace,           S_IWUSR | S_IRUGO, kxte9_show_trace, kxte9_store_trace, 0                          ); 
#if defined(KXTE9_TEST_MODE)
static SENSOR_DEVICE_ATTR(dump           ,           S_IRUGO, kxte9_show_dump,  NULL,              0x00);
static SENSOR_DEVICE_ATTR(ST_RESP        ,           S_IRUGO, kxte9_show_reg  , NULL,              KXTE9_REG_ST_RESP          );
static SENSOR_DEVICE_ATTR(WHO_AM_I       ,           S_IRUGO, kxte9_show_reg  , NULL             , KXTE9_REG_WHO_AM_I         );
static SENSOR_DEVICE_ATTR(TILE_POS_CUR   , S_IWUSR | S_IRUGO, kxte9_show_reg  , NULL             , KXTE9_REG_TILE_POS_CUR     );
static SENSOR_DEVICE_ATTR(TILT_POS_PRE   , S_IWUSR | S_IRUGO, kxte9_show_reg  , NULL             , KXTE9_REG_TILT_POS_PRE     );
static SENSOR_DEVICE_ATTR(XOUT           , S_IWUSR | S_IRUGO, kxte9_show_reg  , NULL             , KXTE9_REG_XOUT             );
static SENSOR_DEVICE_ATTR(YOUT           , S_IWUSR | S_IRUGO, kxte9_show_reg  , NULL             , KXTE9_REG_YOUT             );
static SENSOR_DEVICE_ATTR(ZOUT           , S_IWUSR | S_IRUGO, kxte9_show_reg  , NULL             , KXTE9_REG_ZOUT             );
static SENSOR_DEVICE_ATTR(INT_SRC_REG1   , S_IWUSR | S_IRUGO, kxte9_show_reg  , NULL             , KXTE9_REG_INT_SRC_REG1     );
static SENSOR_DEVICE_ATTR(INT_SRC_REG2   ,           S_IRUGO, kxte9_show_reg  , NULL             , KXTE9_REG_INT_SRC_REG2     );
static SENSOR_DEVICE_ATTR(STATUS_REG     , S_IWUSR | S_IRUGO, kxte9_show_reg  , NULL             , KXTE9_REG_STATUS_REG       );
static SENSOR_DEVICE_ATTR(INT_REL        , S_IWUSR | S_IRUGO, kxte9_show_reg  , NULL             , KXTE9_REG_INT_REL          );
static SENSOR_DEVICE_ATTR(CTRL_REG1      , S_IWUSR | S_IRUGO, kxte9_show_reg  , kxte9_store_reg  , KXTE9_REG_CTRL_REG1        );
static SENSOR_DEVICE_ATTR(CTRL_REG2      , S_IWUSR | S_IRUGO, kxte9_show_reg  , kxte9_store_reg  , KXTE9_REG_CTRL_REG2        );
static SENSOR_DEVICE_ATTR(CTRL_REG3      , S_IWUSR | S_IRUGO, kxte9_show_reg  , kxte9_store_reg  , KXTE9_REG_CTRL_REG3        );
static SENSOR_DEVICE_ATTR(INT_CTRL_REG1  , S_IWUSR | S_IRUGO, kxte9_show_reg  , kxte9_store_reg  , KXTE9_REG_INT_CTRL_REG1    );
static SENSOR_DEVICE_ATTR(INT_CTRL_REG2  ,           S_IRUGO, kxte9_show_reg  , kxte9_store_reg  , KXTE9_REG_INT_CTRL_REG2    );
static SENSOR_DEVICE_ATTR(TILT_TIMER     , S_IWUSR | S_IRUGO, kxte9_show_reg  , kxte9_store_reg  , KXTE9_REG_TILT_TIMER       );
static SENSOR_DEVICE_ATTR(WUF_TIMER      , S_IWUSR | S_IRUGO, kxte9_show_reg  , kxte9_store_reg  , KXTE9_REG_WUF_TIMER        );
static SENSOR_DEVICE_ATTR(B2S_TIMER      , S_IWUSR | S_IRUGO, kxte9_show_reg  , kxte9_store_reg  , KXTE9_REG_B2S_TIMER        );
static SENSOR_DEVICE_ATTR(WUF_THRESH     , S_IWUSR | S_IRUGO, kxte9_show_reg  , kxte9_store_reg  , KXTE9_REG_WUF_THRESH       );
static SENSOR_DEVICE_ATTR(B2S_THRESH     , S_IWUSR | S_IRUGO, kxte9_show_reg  , kxte9_store_reg  , KXTE9_REG_B2S_THRESH       );
static SENSOR_DEVICE_ATTR(TILE_ANGLE     , S_IWUSR | S_IRUGO, kxte9_show_reg  , kxte9_store_reg  , KXTE9_REG_TILE_ANGLE       );
static SENSOR_DEVICE_ATTR(HYST_SET       , S_IWUSR | S_IRUGO, kxte9_show_reg  , kxte9_store_reg  , KXTE9_REG_HYST_SET         );
#endif 
/*----------------------------------------------------------------------------*/
static struct attribute *kxte9_attributes[] = {
    &sensor_dev_attr_calibr.dev_attr.attr,
	&sensor_dev_attr_data.dev_attr.attr,
	&sensor_dev_attr_trace.dev_attr.attr,
#if defined(KXTE9_TEST_MODE)
	&sensor_dev_attr_dump.dev_attr.attr,
	&sensor_dev_attr_ST_RESP.dev_attr.attr,
	&sensor_dev_attr_WHO_AM_I.dev_attr.attr,
	&sensor_dev_attr_TILE_POS_CUR.dev_attr.attr,
	&sensor_dev_attr_TILT_POS_PRE.dev_attr.attr,
	&sensor_dev_attr_XOUT.dev_attr.attr,
	&sensor_dev_attr_YOUT.dev_attr.attr,
	&sensor_dev_attr_ZOUT.dev_attr.attr,
	&sensor_dev_attr_INT_SRC_REG1.dev_attr.attr,
	&sensor_dev_attr_INT_SRC_REG2.dev_attr.attr,
	&sensor_dev_attr_STATUS_REG.dev_attr.attr,
	&sensor_dev_attr_INT_REL.dev_attr.attr,
	&sensor_dev_attr_CTRL_REG1.dev_attr.attr,
	&sensor_dev_attr_CTRL_REG2.dev_attr.attr,
	&sensor_dev_attr_CTRL_REG3.dev_attr.attr,
	&sensor_dev_attr_INT_CTRL_REG1.dev_attr.attr,
	&sensor_dev_attr_INT_CTRL_REG2.dev_attr.attr,
	&sensor_dev_attr_TILT_TIMER.dev_attr.attr,
	&sensor_dev_attr_WUF_TIMER.dev_attr.attr,
	&sensor_dev_attr_B2S_TIMER.dev_attr.attr,
	&sensor_dev_attr_WUF_THRESH.dev_attr.attr,
	&sensor_dev_attr_B2S_THRESH.dev_attr.attr,
	&sensor_dev_attr_TILE_ANGLE.dev_attr.attr,
	&sensor_dev_attr_HYST_SET.dev_attr.attr,
#endif
    NULL
};
/*----------------------------------------------------------------------------*/
static const struct attribute_group kxte9_group = {
	.attrs = kxte9_attributes,
};
/*----------------------------------------------------------------------------*/
#if defined(KXTE9_TEST_MODE) 
/*----------------------------------------------------------------------------*/
static struct hwmsen_reg kxte9_regs[] = {
    /*0x0C ~ 0x1F*/
    {"ST_RESP",         KXTE9_REG_ST_RESP,        REG_RO, 0xFF, 1},
    {"WHO_AM_I",        KXTE9_REG_WHO_AM_I,       REG_RO, 0xFF, 1},
    {"TILE_POS_CUR",    KXTE9_REG_TILE_POS_CUR,   REG_RO, 0x3F, 1},
    {"TILT_POS_PRE",    KXTE9_REG_TILT_POS_PRE,   REG_RO, 0x3F, 1},
    {"XOUT",            KXTE9_REG_XOUT,           REG_RO, 0xFC, 1},
    {"YOUT",            KXTE9_REG_YOUT,           REG_RO, 0xFC, 1},
    {"ZOUT",            KXTE9_REG_ZOUT,           REG_RO, 0xFC, 1},
    {"INT_SRC_REG1",    KXTE9_REG_INT_SRC_REG1,   REG_RO, 0x07, 1},
    {"INT_SRC_REG2",    KXTE9_REG_INT_SRC_REG2,   REG_RO, 0x3F, 1},
    {"STATUS_REG",      KXTE9_REG_STATUS_REG,     REG_RO, 0x3C, 1},
    {"INT_REL",         KXTE9_REG_INT_REL,        REG_RO, 0xFF, 1},
    {"CTRL_REG1",       KXTE9_REG_CTRL_REG1,      REG_RW, 0x9F, 1},
    {"CTRL_REG2",       KXTE9_REG_CTRL_REG2,      REG_RW, 0x3F, 1},        
    {"CTRL_REG3",       KXTE9_REG_CTRL_REG3,      REG_RW, 0x1F, 1}, /*transfer timeout if writing SRST as 1*/
    {"INT_CTRL_REG1",   KXTE9_REG_INT_CTRL_REG1,  REG_RW, 0x1C, 1},
    {"INT_CTRL_REG2",   KXTE9_REG_INT_CTRL_REG2,  REG_RW, 0xE0, 1},
    {"TILE_TIMER",      KXTE9_REG_TILT_TIMER,     REG_RW, 0xFF, 1},
    {"WUF_TIMER",       KXTE9_REG_WUF_TIMER,      REG_RW, 0xFF, 1},
    {"B2S_TIMER",       KXTE9_REG_B2S_TIMER,      REG_RW, 0xFF, 1},
    {"WUF_THRESH",      KXTE9_REG_WUF_THRESH,     REG_RW, 0xFF, 1},
    {"B2S_THRESH",      KXTE9_REG_B2S_THRESH,     REG_RW, 0xFF, 1},
    {"TILE_ANGLE",      KXTE9_REG_TILE_ANGLE,     REG_RW, 0xFF, 1},
    {"HYST_SET",        KXTE9_REG_HYST_SET,       REG_RW, 0xCF, 1},
 };
/*----------------------------------------------------------------------------*/
#endif
/*----------------------------------------------------------------------------*/
#if defined(KXTE9_TEST_MODE)
/*----------------------------------------------------------------------------*/
static void kxte9_single_rw(struct i2c_client *client) 
{
    hwmsen_single_rw(client, kxte9_regs, 
                     sizeof(kxte9_regs)/sizeof(kxte9_regs[0]));   
}
/*----------------------------------------------------------------------------*/
struct hwmsen_reg kxte9_dummy = HWMSEN_DUMMY_REG(0x00);
/*----------------------------------------------------------------------------*/
struct hwmsen_reg* kxte9_get_reg(int reg) 
{
    int idx;
    for (idx = 0; idx < sizeof(kxte9_regs)/sizeof(kxte9_regs[0]); idx++) {
        if (reg == kxte9_regs[idx].addr)
            return &kxte9_regs[idx];
    }
    return &kxte9_dummy;
}
/*----------------------------------------------------------------------------*/
static void kxte9_multi_rw(struct i2c_client *client) 
{
    struct hwmsen_reg_test_multi test_list[] = {
        {KXTE9_REG_XOUT,        3, REG_RO},
        {KXTE9_REG_TILT_TIMER,  3, REG_RW},            
        {KXTE9_REG_WUF_THRESH,  3, REG_RW},                        
    };    

    hwmsen_multi_rw(client, kxte9_get_reg,
                    test_list, sizeof(test_list)/sizeof(test_list[0]));
}
/*----------------------------------------------------------------------------*/
static void kxte9_test(struct i2c_client *client) 
{
    kxte9_single_rw(client);
    kxte9_multi_rw(client);
}
/*----------------------------------------------------------------------------*/
static ssize_t kxte9_show_dump(struct device *dev, 
                               struct device_attribute *attr, char* buf)
{
    /*the register name starting from THRESH_TAP*/
    #define REG_OFFSET      0x00 
    #define REG_TABLE_LEN   0x5F 
    static u8 kxte9_reg_value[REG_TABLE_LEN] = {0};
	struct i2c_client *client = to_i2c_client(dev);    
    
    return hwmsen_show_dump(client, REG_OFFSET, 
                            kxte9_reg_value, REG_TABLE_LEN,
                            kxte9_get_reg, buf, PAGE_SIZE);
}
/*----------------------------------------------------------------------------*/
#endif /*KXTE9_TEST_MODE*/
/*----------------------------------------------------------------------------*/
static int kxte9_to_standard_format(struct kxte9_object *obj, u8 dat[KXTE9_AXES_NUM],
                                    s16 out[KXTE9_AXES_NUM])
{   
    int lsb = KXTE9_SENSITIVITY;
    s16 tmp[KXTE9_AXES_NUM];

    /*convert original KXTE9 data t*/
    tmp[KXTE9_AXIS_X] = (KXTE9_DATA_COUNT(dat[KXTE9_AXIS_X]) - KXTE9_0G_OFFSET);
    tmp[KXTE9_AXIS_Y] = (KXTE9_DATA_COUNT(dat[KXTE9_AXIS_Y]) - KXTE9_0G_OFFSET);
    tmp[KXTE9_AXIS_Z] = (KXTE9_DATA_COUNT(dat[KXTE9_AXIS_Z]) - KXTE9_0G_OFFSET);
    
    tmp[KXTE9_AXIS_X] = (obj->cvt.sign[KXTE9_AXIS_X]*obj->cnf.sensitivity[KXTE9_AXIS_X]*(tmp[KXTE9_AXIS_X]))/lsb;
    tmp[KXTE9_AXIS_Y] = (obj->cvt.sign[KXTE9_AXIS_Y]*obj->cnf.sensitivity[KXTE9_AXIS_Y]*(tmp[KXTE9_AXIS_Y]))/lsb;
    tmp[KXTE9_AXIS_Z] = (obj->cvt.sign[KXTE9_AXIS_Z]*obj->cnf.sensitivity[KXTE9_AXIS_Z]*(tmp[KXTE9_AXIS_Z]))/lsb;
    
    out[obj->cvt.map[KXTE9_AXIS_X]] = tmp[KXTE9_AXIS_X] + obj->cali_sw[KXTE9_AXIS_X];
    out[obj->cvt.map[KXTE9_AXIS_Y]] = tmp[KXTE9_AXIS_Y] + obj->cali_sw[KXTE9_AXIS_Y];
    out[obj->cvt.map[KXTE9_AXIS_Z]] = tmp[KXTE9_AXIS_Z] + obj->cali_sw[KXTE9_AXIS_Z];
    return 0;
}
/*----------------------------------------------------------------------------*/
static int kxte9_write_calibration(struct kxte9_object *obj, s16 dat[KXTE9_AXES_NUM])
{
    obj->cali_sw[KXTE9_AXIS_X] = obj->cvt.sign[KXTE9_AXIS_X]*dat[obj->cvt.map[KXTE9_AXIS_X]];
    obj->cali_sw[KXTE9_AXIS_Y] = obj->cvt.sign[KXTE9_AXIS_Y]*dat[obj->cvt.map[KXTE9_AXIS_Y]];
    obj->cali_sw[KXTE9_AXIS_Z] = obj->cvt.sign[KXTE9_AXIS_Z]*dat[obj->cvt.map[KXTE9_AXIS_Z]];
    return 0;
}
/*----------------------------------------------------------------------------*/
static int kxte9_read_data(struct i2c_client *client, u8 data[KXTE9_AXES_NUM])
{
    u8 addr = KXTE9_REG_XOUT;
    int err = 0;

    if (!client) {
        err = -EINVAL;
    } else if ((err = hwmsen_read_block(client, addr, data, KXTE9_DATA_LEN))) {
        KXT_ERR("error: %d\n", err);
    }
    return err;
}
/*----------------------------------------------------------------------------*/
static ssize_t kxte9_show_data(struct device *dev, 
								struct device_attribute *attr, char* buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct kxte9_object *obj = i2c_get_clientdata(client);
    int err;
    if ((err = kxte9_read_data(&obj->client, obj->data)))
        return -EINVAL;
    
	return snprintf(buf, PAGE_SIZE, "data: (%5d, %5d, %5d)\n", 
                    obj->data[KXTE9_AXIS_X], obj->data[KXTE9_AXIS_Y],
                    obj->data[KXTE9_AXIS_Z]);
}
/*----------------------------------------------------------------------------*/
static ssize_t kxte9_show_calibr(struct device *dev, 
								   struct device_attribute *attr, char* buf) 
{
	struct i2c_client *client = to_i2c_client(dev);
    struct kxte9_object *obj = i2c_get_clientdata(client);    
    int len = 0;
    s16 cali[KXTE9_AXES_NUM];

    cali[obj->cvt.map[KXTE9_AXIS_X]] = obj->cvt.sign[KXTE9_AXIS_X]*obj->cali_sw[KXTE9_AXIS_X];
    cali[obj->cvt.map[KXTE9_AXIS_Y]] = obj->cvt.sign[KXTE9_AXIS_Y]*obj->cali_sw[KXTE9_AXIS_Y];
    cali[obj->cvt.map[KXTE9_AXIS_Z]] = obj->cvt.sign[KXTE9_AXIS_Z]*obj->cali_sw[KXTE9_AXIS_Z];

    len += snprintf(buf+len, PAGE_SIZE-len, "all: (0x%08X, 0x%08X, 0x%08X)\n",
                    cali[HWM_EVT_ACC_X], cali[HWM_EVT_ACC_Y], cali[HWM_EVT_ACC_Z]);
    return len;                    
}
/*----------------------------------------------------------------------------*/
static ssize_t kxte9_store_calibr(struct device *dev, 
								 struct device_attribute *attr, 
								 const char* buf, size_t count) 
{   /*if the function doesn't return count, the funtion will be called again to consume buffer*/
	struct i2c_client *client = to_i2c_client(dev);
    struct kxte9_object *obj = i2c_get_clientdata(client);    
    int x, y, z, err;

    if (3 == sscanf(buf, "%d %d %d", &x, &y, &z)) {
        s16 dat[KXTE9_AXES_NUM];
        dat[KXTE9_AXIS_X] = (s16)x;
        dat[KXTE9_AXIS_Y] = (s16)y;
        dat[KXTE9_AXIS_Z] = (s16)z;
        if ((err = kxte9_write_calibration(obj, dat)))
            KXT_ERR("fail to write calibration: %d\n", err);
    } else {
        KXT_ERR("invalid format!!\n");
    }
    return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t kxte9_show_reg(struct device *dev, 
								struct device_attribute *attr, char* buf) 
{
	int index = to_sensor_dev_attr(attr)->index;
	struct i2c_client *client = to_i2c_client(dev);
    u8 addr = (u8)index;

    if (addr > KXTE9_ADDR_MAX) {
        KXT_ERR("invalid address: 0x%02X\n", addr);
        return 0;
    } else {
        return hwmsen_show_reg(client, addr, buf, PAGE_SIZE);
    }
}
/*----------------------------------------------------------------------------*/
static ssize_t kxte9_store_reg(struct device *dev, 
								 struct device_attribute *attr, 
								 const char* buf, size_t count) 
{   /*if the function doesn't return count, the funtion will be called again to consume buffer*/
	int index = to_sensor_dev_attr(attr)->index;
	struct i2c_client *client = to_i2c_client(dev);
    u8 addr = (u8)index;

    if (addr > KXTE9_ADDR_MAX) {
        KXT_ERR("invalid address: 0x%02X\n", addr);
        return count;
    } else {
        return hwmsen_store_reg(client, addr, buf, PAGE_SIZE);
    }    
}
/*----------------------------------------------------------------------------*/
static ssize_t kxte9_show_trace(struct device *dev, 
                                  struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct kxte9_object *obj = i2c_get_clientdata(client);
    
	return snprintf(buf, PAGE_SIZE, "0x%08X\n", atomic_read(&obj->trace));
}
/*----------------------------------------------------------------------------*/
static ssize_t kxte9_store_trace(struct device* dev, 
                                   struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
    struct kxte9_object *obj = i2c_get_clientdata(client);    
    int trc;

    if (1 == sscanf(buf, "0x%x\n", &trc)) 
        atomic_set(&obj->trace, trc);
    else
        KXT_ERR("set trace level fail!!\n");
    return count;
}
/*----------------------------------------------------------------------------*/
static inline void kxte9_power(unsigned int on) 
{
    static unsigned int power_on = 0;

    KXT_LOG("power %s\n", on ? "on" : "off");
    
    if (power_on == on) {
        KXT_LOG("ignore power control: %d\n", on);
    } else if (on) {
        if (!hwPowerOn(MT6516_POWER_VGP, VOL_2500,"KXTE9")) 
            KXT_ERR("power on fails!!\n");
    } else {
        if (!hwPowerDown(MT6516_POWER_VGP,"KXTE9")) 
            KXT_ERR("power off fail!!\n");   
    }
    power_on = on;
}
/*----------------------------------------------------------------------------*/
static int kxte9_init_hw(struct i2c_client* client)
{
    int err = 0;

    if (!client)
        return -EINVAL;
    
    if ((err = hwmsen_write_byte(client, KXTE9_REG_CTRL_REG2, KXTE9_TILE_POS_MASK))) {
        KXT_ERR("write KXTE9_REG_CTRL_REG3 fail!!\n");
        return err;
    }

    /*configure output rate*/
    if ((err = hwmsen_write_byte(client, KXTE9_REG_CTRL_REG3, KXTE9_ODR_ACTIVE|KXTE9_ODR_INACTIVE))) {
        KXT_ERR("write KXTE9_REG_CTRL_REG2 fail!!\n");
        return err;
    }

    /*disable all interrupt*/
    if ((err = hwmsen_write_byte(client, KXTE9_REG_INT_CTRL_REG2, 0x00))) {
        KXT_ERR("write KXTE9_REG_INT_CTRL_REG2 fail!!\n");
        return err;
    }
    
    /*setup default ODR*/
    if ((err = hwmsen_write_byte(client, KXTE9_REG_CTRL_REG1, KXTE9_ODR_DEFAULT))) {
        KXT_ERR("write KXTE9_REG_CTRL_REG1 fail!!\n");
        return err;
    }
    return err;
}
/*----------------------------------------------------------------------------*/
static inline int kxte9_enable(struct i2c_client *client, u8 enable) 
{
    int err;
    u8 cur, nxt;

	if ((err = hwmsen_read_byte(client, KXTE9_REG_CTRL_REG1, &cur))) {
		KXT_ERR("write data format fail!!\n");
		return err;
	}

    if (enable)
        nxt = cur | (KXTE9_CTRL_PC1);
    else
        nxt = cur & (~KXTE9_CTRL_PC1);

    if (cur ^ nxt) {    /*update CTRL_PC1 if changed*/    
        if ((err = hwmsen_write_byte(client, KXTE9_REG_CTRL_REG1, nxt))) {
            KXT_ERR("write power control fail!!\n");
            return err;
        }
    }
    return 0;        
}
/*----------------------------------------------------------------------------*/
static inline int kxte9_activate(void* self, u8 enable) 
{
    struct kxte9_object* obj = (struct kxte9_object*)self;
    struct i2c_client *client = &obj->client;

    KXT_LOG("%s (%d)\n", __func__, enable);
    if (!obj)
        return -EINVAL;

    if (enable)
        atomic_set(&obj->enable, 1);
    else
        atomic_set(&obj->enable, 0);

    return kxte9_enable(client, enable);
}
/*----------------------------------------------------------------------------*/
static ssize_t kxte9_get_data(void* self, struct hwmsen_data *data)
{
    struct kxte9_object* obj = (struct kxte9_object*)self;
    int idx,err;
    s16 output[KXTE9_AXES_NUM];

    if (!obj || !data) {
        KXT_ERR("null pointer");
        return -EINVAL;
    }

    if (!atomic_read(&obj->enable) || atomic_read(&obj->suspend)) {
        KXT_ERR("the sensor is not activated: %d, %d\n", 
                atomic_read(&obj->enable), atomic_read(&obj->suspend));
        return -EINVAL;
    }
        
    if ((err = kxte9_read_data(&obj->client, obj->data))) {
        KXT_ERR("read data failed!!");
        return -EINVAL;
    }

    /*convert to mg: 1000 = 1g*/
    if ((err = kxte9_to_standard_format(obj, obj->data, output))) {
        KXT_ERR("convert to standard format fail:%d\n",err);
        return -EINVAL;
    }

    data->num = 3;
    for (idx = 0; idx < data->num; idx++)
        data->raw[idx] = (int)output[idx];

    if (atomic_read(&obj->trace) & KXTE9_TRC_GET_DATA) {
        KXT_LOG("%d (0x%08X, 0x%08X, 0x%08X) -> (%5d, %5d, %5d)\n", KXTE9_SENSITIVITY,
                obj->data[KXTE9_AXIS_X], obj->data[KXTE9_AXIS_Y], obj->data[KXTE9_AXIS_Z],
                data->raw[HWM_EVT_ACC_X], data->raw[HWM_EVT_ACC_Y], data->raw[HWM_EVT_ACC_Z]);
    }   
    return 0;
}
/*----------------------------------------------------------------------------*/
int kxte9_set_cali(void* self, struct hwmsen_data *data)
{
    struct kxte9_object* obj = (struct kxte9_object*)self;
    
    KXT_FUN();
    if (!obj || ! data) {
        KXT_ERR("null ptr!!\n");
        return -EINVAL;
    } else {        
        s16 cali[KXTE9_AXES_NUM];
        cali[HWM_EVT_ACC_X] = data->raw[HWM_EVT_ACC_X];
        cali[HWM_EVT_ACC_Y] = data->raw[HWM_EVT_ACC_Y];
        cali[HWM_EVT_ACC_Z] = data->raw[HWM_EVT_ACC_Z];
        return kxte9_write_calibration(obj, cali);
    }        
}
/*----------------------------------------------------------------------------*/
int kxte9_get_cali(void *self, struct hwmsen_data *data)
{
    struct kxte9_object* obj = (struct kxte9_object*)self;
    
    KXT_FUN();
    if (!obj || ! data) {
        KXT_ERR("null ptr!!\n");
        return -EINVAL;
    } else {
        data->raw[obj->cvt.map[KXTE9_AXIS_X]] = obj->cvt.sign[KXTE9_AXIS_X]*obj->cali_sw[KXTE9_AXIS_X];
        data->raw[obj->cvt.map[KXTE9_AXIS_Y]] = obj->cvt.sign[KXTE9_AXIS_Y]*obj->cali_sw[KXTE9_AXIS_Y];
        data->raw[obj->cvt.map[KXTE9_AXIS_Z]] = obj->cvt.sign[KXTE9_AXIS_Z]*obj->cali_sw[KXTE9_AXIS_Z];
        data->num = 3;
        return 0;
    }    
}
/*----------------------------------------------------------------------------*/
int kxte9_get_prop(void *self, struct hwmsen_prop *prop)
{
    struct kxte9_object* obj = (struct kxte9_object*)self;
    
    KXT_FUN();
    if (!obj) {
        KXT_ERR("null pointer");
        return -EINVAL;
    }   

    prop->evt_max[obj->cvt.map[KXTE9_AXIS_X]] =  4095 * (obj->cnf.sensitivity[KXTE9_AXIS_X]/256);
    prop->evt_min[obj->cvt.map[KXTE9_AXIS_X]] = -4096 * (obj->cnf.sensitivity[KXTE9_AXIS_X]/256);
    prop->evt_max[obj->cvt.map[KXTE9_AXIS_Y]] =  4095 * (obj->cnf.sensitivity[KXTE9_AXIS_Y]/256);
    prop->evt_min[obj->cvt.map[KXTE9_AXIS_Y]] = -4096 * (obj->cnf.sensitivity[KXTE9_AXIS_Y]/256);
    prop->evt_max[obj->cvt.map[KXTE9_AXIS_Z]] =  4095 * (obj->cnf.sensitivity[KXTE9_AXIS_Y]/256);
    prop->evt_min[obj->cvt.map[KXTE9_AXIS_Z]] = -4096 * (obj->cnf.sensitivity[KXTE9_AXIS_Y]/256);
    prop->num = 3;
    return 0;}
/*----------------------------------------------------------------------------*/
static int kxte9_init_client(struct i2c_client* client)
{
    /* 1. Power On/Off is done by I2C driver
     * 2. GPIO configuration is done in board-phone.c
     */
#if defined(KXTE9_TEST_MODE)
    kxte9_test(client);
#endif 
    return kxte9_init_hw(client);
}
/*----------------------------------------------------------------------------*/
static int kxte9_detect(struct i2c_adapter* adapter, int address, int kind)
{
    int err = 0;
    struct i2c_client *client;
    struct kxte9_object *obj = NULL;
    struct hwmsen_object sobj;    
        
    KXT_FUN();

    if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
        return -EPERM;

    if (!(obj = kmalloc(sizeof(*obj), GFP_KERNEL)))
        return -ENOMEM;

    memset(obj, 0 ,sizeof(*obj));
    obj->hw = get_cust_acc_hw();
    if ((err = hwmsen_get_convert(obj->hw->direction, &obj->cvt))) {
        KXT_ERR("invalid direction: %d\n", obj->hw->direction);
        goto exit;
    }
    if ((err = hwmsen_get_conf(HWM_SENSOR_ACCELERATION, &obj->cnf))) {
        KXT_ERR("get conf fail: %d\n", err);
        goto exit;
    }
    
    atomic_set(&obj->enable, 0);
    atomic_set(&obj->suspend, 0);
    atomic_set(&obj->trace, 0);   /*disable all log by default*/
    
    client = &obj->client;
    client->addr = address;
    client->adapter = adapter;
    client->driver = &kxte9_driver;
    client->flags = 0;
    strncpy(client->name, KXT_DEV_NAME , I2C_NAME_SIZE);
    i2c_set_clientdata(client, obj);

    if ((err = i2c_attach_client(client)))
        goto exit;

    if ((err = kxte9_init_client(client)))
        goto exit;
        
	if ((err = sysfs_create_group(&client->dev.kobj, &kxte9_group))) {
		KXT_ERR("sysfs_create_group error = %d\n", err);
		goto exit_kfree;
	}

    sobj.self = obj;
    sobj.activate = kxte9_activate;
    sobj.get_prop = kxte9_get_prop;
    sobj.get_data = kxte9_get_data;
    sobj.set_cali = kxte9_set_cali;
    sobj.get_cali = kxte9_get_cali;
    if ((err = hwmsen_attach(HWM_SENSOR_ACCELERATION, &sobj))) {
        KXT_ERR("attach fail = %d\n", err);
        goto exit_kfree;
    }

    KXT_LOG("device is found, driver version: %s\n", KXTE9_DRV_VERSION);
    return 0;

exit_kfree:
    kfree(obj);
exit: 
    return err;
}
/*----------------------------------------------------------------------------*/
static int kxte9_attach_adapter(struct i2c_adapter *adapter)
{
    struct acc_hw *hw = get_cust_acc_hw();
    if (adapter->id == hw->i2c_num) {
        KXT_FUN();
        return i2c_probe(adapter, &kxte9_addr_data, kxte9_detect);
    }
    return -1;    
}
/*----------------------------------------------------------------------------*/
static int kxte9_detach_client(struct i2c_client *client)
{
    int err;
    struct kxte9_object *obj = i2c_get_clientdata(client);

    KXT_FUN();

    err = hwmsen_detach(HWM_SENSOR_ACCELERATION);
    if (err) 
        KXT_ERR("detach fail:%d\n", err);
    err = i2c_detach_client(client);
    if (err){
        KXT_ERR("Client deregistration failed, client not attached.\n");
        return err;
    }
	sysfs_remove_group(&client->dev.kobj, &kxte9_group);    
    i2c_unregister_device(client);    
    kfree(obj);
    return 0;
}
/*----------------------------------------------------------------------------*/
static int kxte9_probe(struct platform_device *pdev) 
{
    struct acc_hw *hw = get_cust_acc_hw();
    
    kxte9_power(1);
    KXT_LOG("%s: %p, %d, %d\n", __func__, hw, hw->i2c_num, hw->direction);
    if (i2c_add_driver(&kxte9_driver)) {
        KXT_ERR("add driver error\n");
        return -1;
    } 
    return 0;
}
/*----------------------------------------------------------------------------*/
static int kxte9_remove(struct platform_device *pdev)
{
    KXT_FUN();    
    kxte9_power(0);
    i2c_del_driver(&kxte9_driver);
    return 0;
}
/*----------------------------------------------------------------------------*/
static int kxte9_suspend(struct platform_device *dev, pm_message_t state) 
{
    int err = 0;
    KXT_LOG("dev = %p, event = %u,", dev, state.event);
    if (state.event == PM_EVENT_SUSPEND) {
        // TODO: to be implemented
    }
    return err;
}
/*----------------------------------------------------------------------------*/
static int kxte9_resume(struct platform_device *dev)
{
    /*it should be replaced by waking up from standby mode*/
    KXT_LOG("");
    // TODO: to be implemented
    return 0;
}
/*----------------------------------------------------------------------------*/
static struct platform_driver kxte9_gsensor_driver = {
    .probe      = kxte9_probe,
    .remove     = kxte9_remove,    
    .suspend    = kxte9_suspend,
    .resume     = kxte9_resume,
    .driver     = {
        .name = "kxte91026",
        .owner = THIS_MODULE,
     }
};
/*----------------------------------------------------------------------------*/
static int __init kxte9_init(void)
{
    KXT_FUN();
    if (platform_driver_register(&kxte9_gsensor_driver)) {
        KXT_ERR("failed to register KXTE9");
        return -ENODEV;
    }
    return 0;    
}
/*----------------------------------------------------------------------------*/
static void __exit kxte9_exit(void)
{
    KXT_FUN();
    platform_driver_unregister(&kxte9_gsensor_driver);
}
/*----------------------------------------------------------------------------*/
module_init(kxte9_init);
module_exit(kxte9_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("KXTE9 I2C driver");
MODULE_AUTHOR("MingHsien Hsieh<minghsien.hsieh@mediatek.com");
