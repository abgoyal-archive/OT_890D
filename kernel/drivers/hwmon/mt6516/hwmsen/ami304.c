
#define __AMI304_C__

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/ctype.h>
#include <linux/hwmon-sysfs.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

#include <mach/mt6516_devs.h>
#include <mach/mt6516_typedefs.h>
#include <mach/mt6516_gpio.h>
#include <mach/mt6516_pll.h>

#include "hwmsen_helper.h"
#include "hwmsen_dev.h"
#include "ami304.h"
/*the version string is automatically modified by Perforce*/
#define AMI_DRV_VERSION		"$Revision: #1 $"
extern void MT6516_EINTIRQUnmask(unsigned int line);
extern void MT6516_EINTIRQMask(unsigned int line);
extern void MT6516_EINT_Set_Polarity(kal_uint8 eintno, kal_bool ACT_Polarity);
extern void MT6516_EINT_Set_HW_Debounce(kal_uint8 eintno, kal_uint32 ms);
extern kal_uint32 MT6516_EINT_Set_Sensitivity(kal_uint8 eintno, kal_bool sens);
extern void MT6516_EINT_Registration(kal_uint8 eintno, kal_bool Dbounce_En,
                                     kal_bool ACT_Polarity, void (EINT_FUNC_PTR)(void),
                                     kal_bool auto_umask);

typedef enum {
    AMI_TRC_GET_DATA    = 0x0001,
    AMI_TRC_EINT        = 0x0002,
} AMI_TRC;
/*----------------------------------------------------------------------------*/
typedef enum {
    AMI_OP_POLL,
    AMI_OP_EINT,
} AMI_OP_MODE;
/*----------------------------------------------------------------------------*/
struct ami_priv{
	struct i2c_client	    client;
    struct work_struct      eint_work;
    struct msensor_hardware *hw;
    struct hwmsen_convert   cvt;
    struct hwmsen_conf      cnf;
    int                     opmode;
    
    /*misc*/
    atomic_t                enable;
    atomic_t                suspend;
    atomic_t                trace;

    /*data*/
    s16                     data[AMI304_AXES_NUM+1];
    s16                     b0[AMI304_AXES_NUM+1];
    s16                     offset[AMI304_AXES_NUM+1];
    s8                      delay[AMI304_AXES_NUM+1];
};
/*----------------------------------------------------------------------------*/
#define AMI_SENISITIVITY    600
static int ami_attach_adapter(struct i2c_adapter *adapter);
static int ami_detach_client(struct i2c_client* client);
static int ami_suspend(struct i2c_client *client, pm_message_t msg); 
static int ami_resume(struct i2c_client *client);
static ssize_t ami_show_offset(struct device *device, struct device_attribute *attr, char *buf);
static ssize_t ami_show_data(struct device *device, struct device_attribute *attr, char* buf);
static ssize_t ami_show_dump(struct device *device, struct device_attribute *attr, char *buf);
static ssize_t ami_show_trace(struct device *device, struct device_attribute *attr, char *buf);
static ssize_t ami_store_trace(struct device *device, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t ami_show_test(struct device *device, struct device_attribute *attr, char *buf);
static int ami_check_device(struct ami_priv *obj);
static void ami_eint_handler(void);
int ami_set_standby(struct ami_priv *obj);
int ami_set_active(struct ami_priv *obj);
/*----------------------------------------------------------------------------*/
static ssize_t ami_show_byte(struct device *device, struct device_attribute *attr, char *buf)
{
    return hwmsen_show_byte(device, attr, buf, PAGE_SIZE);
}
/*----------------------------------------------------------------------------*/
static ssize_t ami_store_byte(struct device *device, struct device_attribute *attr, 
                             const char *buf, size_t count)
{
    return hwmsen_store_byte(device, attr, buf, count);
}
/*----------------------------------------------------------------------------*/
static ssize_t ami_show_word(struct device *device, struct device_attribute *attr, char *buf)
{
    return hwmsen_show_word(device, attr, buf, PAGE_SIZE);
}
/*----------------------------------------------------------------------------*/
static ssize_t ami_store_word(struct device *device, struct device_attribute *attr, 
                             const char *buf, size_t count)
{
    return hwmsen_store_word(device, attr, buf, count);
}
static struct msensor_hardware ami_platform_data = {
    .i2c_num = 2,
    .direction = 0,
};
/*----------------------------------------------------------------------------*/
static SENSOR_DEVICE_ATTR(offset,         S_IWUSR | S_IRUGO, ami_show_offset, NULL,             0);
static SENSOR_DEVICE_ATTR(data          ,           S_IRUGO, ami_show_data,   NULL,             0);
static SENSOR_DEVICE_ATTR(trace         , S_IWUSR | S_IRUGO, ami_show_trace,  ami_store_trace,  0);
static SENSOR_DEVICE_ATTR(test          , S_IWUSR | S_IRUGO, ami_show_test,   NULL,  0);
#if defined(AMI304_TEST_MODE)
static SENSOR_DEVICE_ATTR(dump          ,           S_IRUGO, ami_show_dump,   NULL         ,    0); 
static SENSOR_DEVICE_ATTR(WIA           ,           S_IRUGO, ami_show_byte,   NULL         ,    AMI304_REG_WIA      );
static SENSOR_DEVICE_ATTR(DATAX         ,           S_IRUGO, ami_show_word,   NULL         ,    AMI304_REG_DATAX    ); 
static SENSOR_DEVICE_ATTR(DATAY         ,           S_IRUGO, ami_show_word,   NULL         ,    AMI304_REG_DATAY    ); 
static SENSOR_DEVICE_ATTR(DATAZ         ,           S_IRUGO, ami_show_word,   NULL         ,    AMI304_REG_DATAZ    ); 
static SENSOR_DEVICE_ATTR(INS1          ,           S_IRUGO, ami_show_byte,   NULL         ,    AMI304_REG_INS1     );
static SENSOR_DEVICE_ATTR(STAT1         ,           S_IRUGO, ami_show_byte,   NULL         ,    AMI304_REG_STAT1    );
static SENSOR_DEVICE_ATTR(INL           ,           S_IRUGO, ami_show_byte,   NULL         ,    AMI304_REG_INL      );
static SENSOR_DEVICE_ATTR(CNTL1         , S_IWUSR | S_IRUGO, ami_show_byte,   ami_store_byte,   AMI304_REG_CNTL1    );
static SENSOR_DEVICE_ATTR(CNTL2         , S_IWUSR | S_IRUGO, ami_show_byte,   ami_store_byte,   AMI304_REG_CNTL2    );
static SENSOR_DEVICE_ATTR(CNTL3         , S_IWUSR | S_IRUGO, ami_show_byte,   ami_store_byte,   AMI304_REG_CNTL3    );
static SENSOR_DEVICE_ATTR(INC1          , S_IWUSR | S_IRUGO, ami_show_byte,   ami_store_byte,   AMI304_REG_INC1     );
static SENSOR_DEVICE_ATTR(B0X           , S_IWUSR | S_IRUGO, ami_show_word,   ami_store_word,   AMI304_REG_B0X      ); 
static SENSOR_DEVICE_ATTR(B0Y           , S_IWUSR | S_IRUGO, ami_show_word,   ami_store_word,   AMI304_REG_B0Y      ); 
static SENSOR_DEVICE_ATTR(B0Z           , S_IWUSR | S_IRUGO, ami_show_word,   ami_store_word,   AMI304_REG_B0Z      ); 
static SENSOR_DEVICE_ATTR(ITHR1         , S_IWUSR | S_IRUGO, ami_show_word,   ami_store_word,   AMI304_REG_ITHR1    ); 
static SENSOR_DEVICE_ATTR(PRET          , S_IWUSR | S_IRUGO, ami_show_byte,   ami_store_byte,   AMI304_REG_PRET     );
static SENSOR_DEVICE_ATTR(CNTL4         , S_IWUSR | S_IRUGO, ami_show_word,   ami_store_word,   AMI304_REG_CNTL4    );
static SENSOR_DEVICE_ATTR(TEMP          , S_IWUSR | S_IRUGO, ami_show_word,   NULL          ,   AMI304_REG_TEMP     );
static SENSOR_DEVICE_ATTR(DELAYX        , S_IWUSR | S_IRUGO, ami_show_byte,   ami_store_byte,   AMI304_REG_DELAYX   );
static SENSOR_DEVICE_ATTR(DELAYY        , S_IWUSR | S_IRUGO, ami_show_byte,   ami_store_byte,   AMI304_REG_DELAYY   );
static SENSOR_DEVICE_ATTR(DELAYZ        , S_IWUSR | S_IRUGO, ami_show_byte,   ami_store_byte,   AMI304_REG_DELAYZ   );
static SENSOR_DEVICE_ATTR(OFFX          , S_IWUSR | S_IRUGO, ami_show_word,   ami_store_word,   AMI304_REG_OFFX     ); 
static SENSOR_DEVICE_ATTR(OFFY          , S_IWUSR | S_IRUGO, ami_show_word,   ami_store_word,   AMI304_REG_OFFY     ); 
static SENSOR_DEVICE_ATTR(OFFZ          , S_IWUSR | S_IRUGO, ami_show_word,   ami_store_word,   AMI304_REG_OFFZ     ); 
static SENSOR_DEVICE_ATTR(VER           , S_IWUSR | S_IRUGO, ami_show_word,   NULL          ,   AMI304_REG_VER      ); 
static SENSOR_DEVICE_ATTR(SN            , S_IWUSR | S_IRUGO, ami_show_word,   NULL          ,   AMI304_REG_SN       ); 
#endif /*ADXL345_TEST_MODE*/
/*----------------------------------------------------------------------------*/
static struct attribute *ami_attributes[] = {
	&sensor_dev_attr_offset.dev_attr.attr,
	&sensor_dev_attr_data.dev_attr.attr,
	&sensor_dev_attr_trace.dev_attr.attr,
	&sensor_dev_attr_test.dev_attr.attr,
#if defined(AMI304_TEST_MODE)	
	&sensor_dev_attr_dump.dev_attr.attr,
	&sensor_dev_attr_WIA.dev_attr.attr,
	&sensor_dev_attr_DATAX.dev_attr.attr, 
	&sensor_dev_attr_DATAY.dev_attr.attr,
	&sensor_dev_attr_DATAZ.dev_attr.attr,
	&sensor_dev_attr_INS1.dev_attr.attr,
	&sensor_dev_attr_STAT1.dev_attr.attr,
	&sensor_dev_attr_INL.dev_attr.attr,
	&sensor_dev_attr_CNTL1.dev_attr.attr,
	&sensor_dev_attr_CNTL2.dev_attr.attr,
	&sensor_dev_attr_CNTL3.dev_attr.attr,
	&sensor_dev_attr_INC1.dev_attr.attr,
	&sensor_dev_attr_B0X.dev_attr.attr,
	&sensor_dev_attr_B0Y.dev_attr.attr, 
	&sensor_dev_attr_B0Z.dev_attr.attr, 
	&sensor_dev_attr_ITHR1.dev_attr.attr,
	&sensor_dev_attr_PRET.dev_attr.attr,
	&sensor_dev_attr_CNTL4.dev_attr.attr,
	&sensor_dev_attr_TEMP.dev_attr.attr,	
	&sensor_dev_attr_DELAYX.dev_attr.attr,
	&sensor_dev_attr_DELAYY.dev_attr.attr,
	&sensor_dev_attr_DELAYZ.dev_attr.attr,
	&sensor_dev_attr_OFFX.dev_attr.attr, 
	&sensor_dev_attr_OFFY.dev_attr.attr, 
	&sensor_dev_attr_OFFZ.dev_attr.attr, 
	&sensor_dev_attr_VER.dev_attr.attr, 
	&sensor_dev_attr_SN.dev_attr.attr, 
#endif /*ADXL345_TEST_MODE*/	
	NULL
};
/*----------------------------------------------------------------------------*/
static const struct attribute_group ami_group = {
	.attrs = ami_attributes,
};
/*----------------------------------------------------------------------------*/
#if defined(AMI304_TEST_MODE)
/*----------------------------------------------------------------------------*/
static struct hwmsen_reg ami_regs[] = {
    {"WIA",         AMI304_REG_WIA,       REG_RO, 0x00FF, 1},
    {"DATAX",       AMI304_REG_DATAX,     REG_RO, 0xFFFF, 2},
    {"DATAY",       AMI304_REG_DATAY,     REG_RO, 0xFFFF, 2},
    {"DATAZ",       AMI304_REG_DATAZ,     REG_RO, 0xFFFF, 2},
    {"INS1",        AMI304_REG_INS1,      REG_RO, 0x00FF, 1},
    {"STAT1",       AMI304_REG_STAT1,     REG_RO, 0x00FF, 1},
    {"INL",         AMI304_REG_INL,       REG_RO, 0x00FF, 1},
    {"CNTL1",       AMI304_REG_CNTL1,     REG_RW|REG_LK, 0x0092, 1},
    {"CNTL2",       AMI304_REG_CNTL2 ,    REG_RW|REG_LK, 0x001C, 1},
    {"CNTL3",       AMI304_REG_CNTL3,     REG_RW|REG_LK, 0x00F0, 1},
    {"INC1",        AMI304_REG_INC1,      REG_RW, 0x00EC, 1},
    {"B0X" ,        AMI304_REG_B0X,       REG_RW, 0xFFFF, 2}, 
    {"B0Y",         AMI304_REG_B0Y,       REG_RW, 0xFFFF, 2}, 
    {"B0Z",         AMI304_REG_B0Z,       REG_RW, 0xFFFF, 2}, 
    {"ITHR1",       AMI304_REG_ITHR1,     REG_RW, 0xFFFF, 2}, 
    {"PRET",        AMI304_REG_PRET,      REG_RW, 0x000F, 1},
    {"CNTL4",       AMI304_REG_CNTL4,     REG_RW, 0x0001, 2}, 
    {"TEMP",        AMI304_REG_TEMP,      REG_RO, 0xFFFF, 2},         
    {"DELAYX",      AMI304_REG_DELAYX,    REG_RW, 0x00FF, 1},
    {"DELAYY",      AMI304_REG_DELAYY,    REG_RW, 0x00FF, 1},
    {"DELAYZ",      AMI304_REG_DELAYZ,    REG_RW, 0x00FF, 1},
    {"OFFX",        AMI304_REG_OFFX,      REG_RW, 0x003F, 2}, 
    {"OFFY",        AMI304_REG_OFFY,      REG_RW, 0x003F, 2}, 
    {"OFFZ",        AMI304_REG_OFFZ,      REG_RW, 0x003F, 2}, 
    {"VER",         AMI304_REG_VER,       REG_RO, 0x007F, 2}, 
    {"SN",          AMI304_REG_SN,        REG_RO, 0xFFFF, 2}, 
};                                                                                                                  
/*----------------------------------------------------------------------------*/
struct hwmsen_reg ami_dummy = {NULL, 0,0,0,0};
/*----------------------------------------------------------------------------*/
struct ami_priv *g_ami_priv = NULL;
/*----------------------------------------------------------------------------*/
static void ami_single_rw(struct i2c_client *client) 
{
    hwmsen_single_rw(client, ami_regs, 
                     sizeof(ami_regs)/sizeof(ami_regs[0]));
}
/*----------------------------------------------------------------------------*/
struct hwmsen_reg* ami_get_reg(int reg) 
{
    int idx;    
    for (idx = 0; idx < sizeof(ami_regs)/sizeof(ami_regs[0]); idx++)
        if (ami_regs[idx].addr == reg)
            return &ami_regs[idx];
    return &ami_dummy;    
}
/*----------------------------------------------------------------------------*/
static void ami_multi_rw(struct i2c_client *client) 
{
    struct hwmsen_reg_test_multi test_list[] = {
        {AMI304_REG_B0X  , 6, REG_RW},
        {AMI304_REG_B0X  , 4, REG_RW},
        {AMI304_REG_B0X  , 2, REG_RW},
    };

    hwmsen_multi_rw(client, ami_get_reg,
                    test_list, sizeof(test_list)/sizeof(test_list[0]));
}
/*----------------------------------------------------------------------------*/
static void ami_test(struct ami_priv* obj) 
{
    ami_set_active(obj);
    ami_single_rw(&obj->client);
    ami_multi_rw(&obj->client);
}
/*----------------------------------------------------------------------------*/
static ssize_t ami_show_test(struct device *dev,
                             struct device_attribute *attr, char* buf)
{
	struct i2c_client *client = to_i2c_client(dev); 
    struct ami_priv *obj = i2c_get_clientdata(client);    
    int err = ami_check_device(obj);
    ami_test(obj);
    return snprintf(buf, PAGE_SIZE, (err) ? (" *** Fail ***\n") : (" *** OKAY ***\n"));
}
/*----------------------------------------------------------------------------*/
static ssize_t ami_show_dump(struct device *dev, 
								struct device_attribute *attr, char* buf)
{
	struct i2c_client *client = to_i2c_client(dev);    
    
    return hwmsen_read_all_regs(client, ami_regs, sizeof(ami_regs)/sizeof(ami_regs[0]),
                                buf, PAGE_SIZE);
}
/*----------------------------------------------------------------------------*/
#endif /*ADXL345_TEST_MODE*/                                                         
/*----------------------------------------------------------------------------*/
static int ami_read_offset(struct ami_priv *obj, s16 offset[AMI304_AXES_NUM])
{
    struct i2c_client *client = &obj->client;
    u8 buf[AMI304_OFFSET_LEN] = {0};
    int err = 0;

    if (!client) {
        err = -EINVAL;
    } else if ((err = hwmsen_read_block(client, AMI304_REG_DATAX, buf, AMI304_DATA_LEN))) {
        AMI_ERR("error: %d\n", err);
    } else {
		offset[AMI304_AXIS_X] = (s16)((buf[AMI304_AXIS_X*2]) |
									 (buf[AMI304_AXIS_X*2+1] << 8));
		offset[AMI304_AXIS_Y] = (s16)((buf[AMI304_AXIS_Y*2]) |
									 (buf[AMI304_AXIS_Y*2+1] << 8));
		offset[AMI304_AXIS_Z] = (s16)((buf[AMI304_AXIS_Z*2]) |
									 (buf[AMI304_AXIS_Z*2+1] << 8));
    }
    return err;
}
/*----------------------------------------------------------------------------*/
static void ami_correct_data(struct ami_priv* obj, s16 data[AMI304_AXES_NUM])
{
    int idx = 0;
    s16 tmp;

    for (idx = 0; idx < AMI304_AXES_NUM; idx++) {
        if ((data[idx] & 0x0800) && ((data[idx] & 0xF000) != 0xF000)) {            
            tmp = 0xF000 | (data[idx] & 0x0FFF);
            AMI_LOG("correct value: 0x%04X -> 0x%04X\n", tmp, data[idx]);
            data[idx] = tmp;
        } 
        if (!(data[idx] & 0x0800) && ((data[idx] & 0xF000) != 0x0000)) {
            tmp = (data[idx] & 0x0FFF);
            AMI_LOG("correct value: 0x%04X -> 0x%04X\n", tmp, data[idx]);            
            data[idx] = tmp;
        }
    }
}
/*----------------------------------------------------------------------------*/
static int ami_read_data(struct ami_priv *obj, s16 data[AMI304_AXES_NUM])
{
    struct i2c_client *client = &obj->client;
    u8 buf[AMI304_DATA_LEN] = {0};
    int err = 0;

    if (!client) {
        err = -EINVAL;
    } else if ((err = hwmsen_read_block(client, AMI304_REG_DATAX, buf, AMI304_DATA_LEN))) {
        AMI_ERR("error: %d\n", err);
    } else {
		data[AMI304_AXIS_X] = (s16)((buf[AMI304_AXIS_X*2]) |
									 (buf[AMI304_AXIS_X*2+1] << 8));
		data[AMI304_AXIS_Y] = (s16)((buf[AMI304_AXIS_Y*2]) |
									 (buf[AMI304_AXIS_Y*2+1] << 8));
		data[AMI304_AXIS_Z] = (s16)((buf[AMI304_AXIS_Z*2]) |
									 (buf[AMI304_AXIS_Z*2+1] << 8));
        ami_correct_data(obj, data);
    }
    return err;
}
/*----------------------------------------------------------------------------*/
static int ami_update_data(struct ami_priv *obj)
{
    if (obj->opmode == AMI_OP_POLL) { 
        struct i2c_client *client = &obj->client;
        u8 dat, buf[AMI304_DATA_LEN] = {0};
        int err = 0, retry = 3;

        if (!client) 
            return -EFAULT;

        while(retry--) {
            if (1 == mt_get_gpio_mode(GPIO_MSESNOR_RDY_PIN)) {
                if ((err = hwmsen_read_byte(client, AMI304_REG_STAT1, &dat))) {
                    AMI_ERR("read status: %d\n", err);
                    return err;
                } else if (dat & STA1_DRDY) {
                    if ((err = hwmsen_read_block(client, AMI304_REG_DATAX, buf, AMI304_DATA_LEN))) {
                        AMI_ERR("read data: %d\n", err);
                        return err;
                    } else {
                		obj->data[AMI304_AXIS_X] = (s16)((buf[AMI304_AXIS_X*2]) |
                									     (buf[AMI304_AXIS_X*2+1] << 8));
                		obj->data[AMI304_AXIS_Y] = (s16)((buf[AMI304_AXIS_Y*2]) |
                									     (buf[AMI304_AXIS_Y*2+1] << 8));
                		obj->data[AMI304_AXIS_Z] = (s16)((buf[AMI304_AXIS_Z*2]) |
                									     (buf[AMI304_AXIS_Z*2+1] << 8));
                        return 0;                
                    }
                }
            }
            ssleep(1);
        } 
        return -ETIME;
    } else if (obj->opmode == AMI_OP_EINT) {
        /*the read data is done in eint handler*/
    }    
    return 0;
}
/*----------------------------------------------------------------------------*/
static int ami_to_standard_format(struct ami_priv *obj, s16 dat[AMI304_AXES_NUM],
                                  s16 out[AMI304_AXES_NUM])
{   /*align layout, convert to requested resolution*/
    int lsb = AMI304_SENSITIVITY;
    s16 tmp[AMI304_AXES_NUM];

    tmp[AMI304_AXIS_X] = (obj->cvt.sign[AMI304_AXIS_X]*obj->cnf.sensitivity[AMI304_AXIS_X]*(dat[AMI304_AXIS_X]))/lsb;
    tmp[AMI304_AXIS_Y] = (obj->cvt.sign[AMI304_AXIS_Y]*obj->cnf.sensitivity[AMI304_AXIS_Y]*(dat[AMI304_AXIS_Y]))/lsb;
    tmp[AMI304_AXIS_Z] = (obj->cvt.sign[AMI304_AXIS_Z]*obj->cnf.sensitivity[AMI304_AXIS_Z]*(dat[AMI304_AXIS_Z]))/lsb;
    out[obj->cvt.map[AMI304_AXIS_X]] = tmp[AMI304_AXIS_X] ;
    out[obj->cvt.map[AMI304_AXIS_Y]] = tmp[AMI304_AXIS_Y] ;
    out[obj->cvt.map[AMI304_AXIS_Z]] = tmp[AMI304_AXIS_Z] ;
    return 0;
}
/*----------------------------------------------------------------------------*/
static void ami_power(unsigned int on) 
{
    static unsigned int power_on = 0;

    AMI_LOG("power %s\n", on ? "on" : "off");
    /*AMI-304's power is supplied from VIO, which is never turned off*/    
    power_on = on;   
}
/*----------------------------------------------------------------------------*/
static ssize_t ami_show_offset(struct device *dev, 
								struct device_attribute *attr, char* buf)
{   
    struct i2c_client *client = to_i2c_client(dev);
    struct ami_priv *obj = i2c_get_clientdata(client);
    int err;
    if ((err = ami_read_offset(obj, obj->offset)))
        return -EINVAL;

	return snprintf(buf, PAGE_SIZE, "data: (%5d, %5d, %5d)\n", 
                    obj->offset[AMI304_AXIS_X], obj->offset[AMI304_AXIS_Y],
                    obj->offset[AMI304_AXIS_Z]);
}
/*----------------------------------------------------------------------------*/
static ssize_t ami_show_data(struct device *dev, 
								struct device_attribute *attr, char* buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct ami_priv *obj = i2c_get_clientdata(client);
    int err, factor;

    if ((err = ami_read_data(obj, obj->data))) {
        AMI_ERR("update data failed!!");
        return -EINVAL;
    }

    factor = obj->data[AMI304_AXIS_X]*obj->data[AMI304_AXIS_X] +
             obj->data[AMI304_AXIS_Y]*obj->data[AMI304_AXIS_Y] +
             obj->data[AMI304_AXIS_Z]*obj->data[AMI304_AXIS_Z];
	return snprintf(buf, PAGE_SIZE, "data: (%5d, %5d, %5d) = %5d\n", 
                    obj->data[AMI304_AXIS_X], obj->data[AMI304_AXIS_Y],
                    obj->data[AMI304_AXIS_Z], factor/3600);

}
/*----------------------------------------------------------------------------*/
static ssize_t ami_show_trace(struct device *dev, 
                                  struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct ami_priv *obj = i2c_get_clientdata(client);
    
	return snprintf(buf, PAGE_SIZE, "0x%08X\n", atomic_read(&obj->trace));
}
/*----------------------------------------------------------------------------*/
static ssize_t ami_store_trace(struct device* dev, 
                                   struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
    struct ami_priv *obj = i2c_get_clientdata(client);    
    int trc;

    if (1 == sscanf(buf, "0x%x\n", &trc)) 
        atomic_set(&obj->trace, trc);
    else
        AMI_ERR("set trace level fail!!\n");
    return count;
}                                                         
int ami_set_standby(struct ami_priv *obj)
{
    return hwmsen_write_byte(&obj->client, AMI304_REG_CNTL1, CNTL1_PC1_STDBY|CNTL1_ODR1_20|CNTL1_FS1_NORM);    
}
/*----------------------------------------------------------------------------*/
int ami_set_active(struct ami_priv *obj)
{
    return hwmsen_write_byte(&obj->client, AMI304_REG_CNTL1, CNTL1_PC1_ACT|CNTL1_ODR1_20|CNTL1_FS1_NORM);    
}
/*----------------------------------------------------------------------------*/
int ami_enable(struct ami_priv *obj, u8 enable) 
{
    int err;
    u8 nxt;

    /* NOTE: 
     * register R/W fails in standby mode, hence, when enable, we can not
     * read the byte first.
     */
     
    if (enable)
        nxt = (CNTL1_DEFAULT) | (CNTL1_PC1_ACT);
    else
        nxt = (CNTL1_DEFAULT) & (~CNTL1_PC1_ACT);

    //if (cur ^ nxt) {    /*update if changed*/    
    if (nxt) {

        if ((err = hwmsen_write_byte(&obj->client, AMI304_REG_CNTL1, nxt))) {
            AMI_ERR("write power control fail!!\n");
            return err;
        }
        
        if (obj->opmode == AMI_OP_EINT) {
            if (enable)
                MT6516_EINT_Registration(EINT_MSENSOR_INT, TRUE, EINT_CON_HIGHLEVEL, ami_eint_handler, 0);        
            else
                MT6516_EINT_Registration(EINT_MSENSOR_INT, TRUE, EINT_CON_HIGHLEVEL, NULL, 0);
        }
    }
    return 0;    
}
/*----------------------------------------------------------------------------*/
int ami_activate(void* self, u8 enable) 
{
    struct ami_priv* obj = (struct ami_priv*)self;

    AMI_LOG("%s (%d)\n", __func__, enable);
    if (!obj)
        return -EINVAL;

    if (enable)
        atomic_set(&obj->enable, 1);
    else
        atomic_set(&obj->enable, 0);

    return ami_enable(obj, enable);
}
/*----------------------------------------------------------------------------*/
int ami_get_data(void* self, struct hwmsen_data *data) 
{
    struct ami_priv* obj = (struct ami_priv*)self;
    int idx, err;
    s16 output[AMI304_AXES_NUM];

    if (!obj || !data) {
        AMI_ERR("null pointer");
        return -EINVAL;
    }

    if (!atomic_read(&obj->enable) || atomic_read(&obj->suspend)) {
        AMI_ERR("the sensor is not activated: %d, %d\n", 
                atomic_read(&obj->enable), atomic_read(&obj->suspend));
        return -EINVAL;
    }
        
    //if ((err = ami_update_data(obj))) {
    if ((err = ami_read_data(obj, obj->data))) {
        AMI_ERR("update data failed : %d!!\n", err);
        return -EINVAL;
    }

    if ((err = ami_to_standard_format(obj, obj->data, output))) {
        AMI_ERR("convert to standard format fail:%d\n",err);
        return -EINVAL;
    }

    data->num = AMI304_AXES_NUM;
    for (idx = 0; idx < data->num; idx++)
        data->raw[idx] = (int)output[idx];

    if (atomic_read(&obj->trace) & AMI_TRC_GET_DATA) {
        AMI_LOG("(0x%08X, 0x%08X, 0x%08X) -> (%5d, %5d, %5d)\n",
                obj->data[AMI304_AXIS_X], obj->data[AMI304_AXIS_Y], obj->data[AMI304_AXIS_Z],
                data->raw[HWM_EVT_ACC_X], data->raw[HWM_EVT_ACC_Y], data->raw[HWM_EVT_ACC_Z]);
    }
    return 0;
}
/*----------------------------------------------------------------------------*/
int ami_set_cali(void* self, struct hwmsen_data *data)
{
    struct ami_priv* obj = (struct ami_priv*)self;
    
    AMI_FUN();
    if (!obj || ! data) {
        AMI_ERR("null ptr!!\n");
        return -EINVAL;
    } else {

        /*not implemented*/
        return 0;
    }        
}  
/*----------------------------------------------------------------------------*/
int ami_get_cali(void* self, struct hwmsen_data *data)
{
    struct ami_priv* obj = (struct ami_priv*)self;
    
    AMI_FUN();
    if (!obj || ! data) {
        AMI_ERR("null ptr!!\n");
        return -EINVAL;
    } else {
        /*not implemented*/
        return 0;
    }    
}
/*----------------------------------------------------------------------------*/
int ami_get_prop(void *self, struct hwmsen_prop *prop)
{
    struct ami_priv* obj = (struct ami_priv*)self;
    
    AMI_FUN();
    if (!obj) {
        AMI_ERR("null pointer");
        return -EINVAL;
    }   

    prop->evt_max[obj->cvt.map[AMI304_AXIS_X]] =  2047 * (obj->cnf.sensitivity[AMI304_AXIS_X]/AMI_SENISITIVITY);
    prop->evt_min[obj->cvt.map[AMI304_AXIS_X]] = -2048 * (obj->cnf.sensitivity[AMI304_AXIS_X]/AMI_SENISITIVITY);
    prop->evt_max[obj->cvt.map[AMI304_AXIS_Y]] =  2047 * (obj->cnf.sensitivity[AMI304_AXIS_Y]/AMI_SENISITIVITY);
    prop->evt_min[obj->cvt.map[AMI304_AXIS_Y]] = -2048 * (obj->cnf.sensitivity[AMI304_AXIS_Y]/AMI_SENISITIVITY);
    prop->evt_max[obj->cvt.map[AMI304_AXIS_Z]] =  2047 * (obj->cnf.sensitivity[AMI304_AXIS_Z]/AMI_SENISITIVITY);
    prop->evt_min[obj->cvt.map[AMI304_AXIS_Z]] = -2048 * (obj->cnf.sensitivity[AMI304_AXIS_Z]/AMI_SENISITIVITY);
    prop->num = 3;
    return 0;
}
static unsigned short normal_i2c[] = {AMI304_WR_SLAVE_ADDR, I2C_CLIENT_END};
static unsigned short ignore = I2C_CLIENT_END;
static struct i2c_client_address_data ami_addr_data = {
	.normal_i2c = normal_i2c,
	.probe  = &ignore,
	.ignore = &ignore,
};
/*----------------------------------------------------------------------------*/
static struct i2c_driver ami_driver = {
	.attach_adapter = ami_attach_adapter,
	.detach_client  = ami_detach_client,
	.suspend        = ami_suspend,
	.resume         = ami_resume,
	.driver = {
		.name = AMI_DEV_NAME,
	},
};
/*----------------------------------------------------------------------------*/
static void ami_eint_work(struct work_struct *work) 
{
    struct ami_priv *obj = container_of(work, struct ami_priv, eint_work);    
    s16 dat[AMI304_AXES_NUM];
    u8 tmp;
    int err;

    if ((err = hwmsen_read_byte(&obj->client, AMI304_REG_STAT1, &tmp))) {
        AMI_ERR("read STAT1 fail: %d\n", err);
    } else if (!(tmp & STA1_DRDY)) {
        AMI_ERR("Status not ready: 0x%2x\n", tmp);
        err = -EINVAL;
    } else if ((err = ami_read_data(obj, dat))) {
        AMI_ERR("read data err = %d\n", err);
    } else {
        memcpy(obj->data, dat, sizeof(dat));
    }

    /*read INL to clear interrupt*/
    err = hwmsen_read_byte(&obj->client, AMI304_REG_INL, &tmp);
    MT6516_EINTIRQUnmask(GPIO_MSESNOR_INT_PIN);    
}
/*----------------------------------------------------------------------------*/
static int ami_check_device(struct ami_priv *obj)
{
    #define C_AMI_MAX_RETRY 10
    int err;
    u8 cur;
    int retry = 0;
    
    if ((err = ami_set_active(obj))) {
        AMI_ERR("set active fail: %d\n", err);
        return err;
    }

    while (retry++ < C_AMI_MAX_RETRY)  {
    	/*check device ID*/
    	if ((err = hwmsen_read_byte(&obj->client, AMI304_REG_WIA, &cur))) {
    		AMI_ERR("read device id fail!!\n");
    		return err;
    	}

    	if (cur != 0x47) {
    		AMI_ERR("device id 0x%X not match!!\n", (unsigned int)cur);
    		err = -ENODEV; /*no such device*/
    	} else {
    		AMI_LOG("[%2d] device is found, driver version: %s\n", retry, AMI_DRV_VERSION);
    	}
        if (!err)
            break;
        else 
            ssleep(1);
    }

    //if ((err = ami_set_standby(obj))) {
    //    AMI_ERR("set active fail: %d\n", err);
    //    return err;
    //}


    return 0;    
}
/*----------------------------------------------------------------------------*/
static void ami_eint_handler(void)
{
    struct ami_priv *obj = g_ami_priv;
    if (!obj)
        return;
    schedule_work(&obj->eint_work);
    if (atomic_read(&obj->trace) & AMI_TRC_EINT)
        AMI_LOG("eint ami\n");
}
/*----------------------------------------------------------------------------*/
static int ami_init_gpio(struct ami_priv *obj)
{
    if (!obj) {
        return -EINVAL;
    } else if (obj->opmode == AMI_OP_POLL) {
        /*data ready pin*/
        mt_set_gpio_dir(GPIO_MSESNOR_RDY_PIN, GPIO_DIR_IN);
        mt_set_gpio_pull_enable(GPIO_MSESNOR_RDY_PIN, FALSE);
        mt_set_gpio_mode(GPIO_MSESNOR_RDY_PIN, GPIO_MSESNOR_RDY_PIN_M_GPIO);
        /*eint pin*/
        mt_set_gpio_dir(GPIO_MSESNOR_INT_PIN, GPIO_DIR_IN);
        mt_set_gpio_pull_enable(GPIO_MSESNOR_INT_PIN, FALSE);
        mt_set_gpio_mode(GPIO_MSESNOR_INT_PIN, GPIO_MSESNOR_INT_PIN_M_GPIO);        
    } else if (obj->opmode == AMI_OP_EINT) {
        g_ami_priv = obj;
        /*data ready pin*/
        mt_set_gpio_dir(GPIO_MSESNOR_RDY_PIN, GPIO_DIR_IN);
        mt_set_gpio_pull_enable(GPIO_MSESNOR_RDY_PIN, FALSE);
        mt_set_gpio_mode(GPIO_MSESNOR_RDY_PIN, GPIO_MSESNOR_RDY_PIN_M_GPIO);
        /*eint pin*/
        mt_set_gpio_dir(GPIO_MSESNOR_INT_PIN, GPIO_DIR_IN);
        mt_set_gpio_pull_enable(GPIO_MSESNOR_INT_PIN, FALSE);
        mt_set_gpio_mode(GPIO_MSESNOR_INT_PIN, GPIO_MSESNOR_INT_PIN_M_GPIO); 
        MT6516_EINT_Set_Sensitivity(EINT_MSENSOR_INT, MT6516_LEVEL_SENSITIVE);
    }
    return 0;
}
/*----------------------------------------------------------------------------*/
static int ami_init_client(struct i2c_client* client)
{
	/* 1. Power On/Off is done by I2C driver
	 * 2. GPIO configuration is done in board-phone.c
	 */
    struct ami_priv *obj = i2c_get_clientdata(client);    	 
	int err;

    if ((err = ami_init_gpio(obj))) {
        AMI_ERR("setup gpio fail!!\n");
        return err;
    }

    if ((err = ami_check_device(obj))) {
        AMI_ERR("check device!!\n");
        return err;
    }
    
	/*standby mode, output = 20 Hz, normal mode*/
	if ((err = hwmsen_write_byte(client, AMI304_REG_CNTL1, CNTL1_PC1_STDBY|CNTL1_ODR1_20|CNTL1_FS1_NORM))) {
		AMI_ERR("write data format fail!!\n");
		return err;
	}

    /*interrupt enable, enable data ready signal*/
    if ((err = hwmsen_write_byte(client, AMI304_REG_CNTL2, CNTL2_IEN|CNTL2_DREN|CNTL2_DRP))) {
        AMI_ERR("write power control fail!!\n");
        return err;
    }

    /*interrupt enable, load B0 from OTP-ROM to RAM*/
    if ((err = hwmsen_write_byte(client, AMI304_REG_CNTL3, CNTL3_B0_LO))) {
        AMI_ERR("write power control fail!!\n");
        return err;
    }

    if (obj->opmode == AMI_OP_POLL) {
        if ((err = hwmsen_write_byte(client, AMI304_REG_CNTL3, CNTL3_FORCE))) {
            AMI_ERR("write byte fail!!\n");
            return err;
        }
    }
            
	return 0;
}
/*----------------------------------------------------------------------------*/
static int ami_detect(struct i2c_adapter* adapter, int address, int kind)
{
	int err = 0;
	struct i2c_client *client;
	struct ami_priv *obj;
    struct hwmsen_object sobj;
	
	AMI_FUN();

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		goto exit;

	if (!(obj = kmalloc(sizeof(*obj), GFP_KERNEL))){
		err = -ENOMEM;
		goto exit;
	}

	memset(obj, 0 ,sizeof(*obj));

    obj->hw = &ami_platform_data;
    if ((err = hwmsen_get_convert(obj->hw->direction, &obj->cvt))) {
        AMI_ERR("invalid direction: %d\n", obj->hw->direction);
        goto exit;
    }
    if ((err = hwmsen_get_conf(HWM_SENSOR_MAGNETIC_FIELD, &obj->cnf))) {
        AMI_ERR("get conf fail: %d\n", err);
        goto exit;
    }    
    atomic_set(&obj->enable, 0);
    atomic_set(&obj->suspend, 0);
    atomic_set(&obj->trace, 0);     /*disable all log by default*/
    obj->opmode = AMI_OP_POLL;
    INIT_WORK(&(obj->eint_work), ami_eint_work);
	client = &obj->client;
	client->addr = address;
	client->adapter = adapter;
	client->driver = &ami_driver;
	client->flags = 0;
	strncpy(client->name, AMI_DEV_NAME , I2C_NAME_SIZE);
	i2c_set_clientdata(client, obj);

	if ((err = i2c_attach_client(client)))
		goto exit_kfree;

	if ((err = ami_init_client(client)))
		goto exit_kfree;
        
	if ((err = sysfs_create_group(&client->dev.kobj, &ami_group))) {
		AMI_ERR("sysfs_create_group error = %d\n", err);
		goto exit_kfree;
	}

    sobj.self = obj;
    sobj.activate = ami_activate;
    sobj.get_prop = ami_get_prop;
    sobj.get_data = ami_get_data;
    sobj.set_cali = ami_set_cali;
    sobj.get_cali = ami_get_cali;
    if ((err = hwmsen_attach(HWM_SENSOR_MAGNETIC_FIELD, &sobj))) {
        AMI_ERR("attach fail = %d\n", err);
        goto exit_kfree;
    }

	return 0;

exit_kfree:
	kfree(obj);
exit:
	return err;
}
/*----------------------------------------------------------------------------*/
static int ami_suspend(struct i2c_client *client, pm_message_t msg) 
{
    struct ami_priv *obj = i2c_get_clientdata(client);    
    int err;
    AMI_FUN();    

    if (msg.event == PM_EVENT_SUSPEND) {   
        if (!obj) {
            AMI_ERR("null pointer!!\n");
            return -EINVAL;
        }
        if ((err = ami_enable(obj, 0))) {
            AMI_ERR("disable fail!!\n");
            return err;
        }        
        atomic_set(&obj->suspend, 1);
        ami_power(0);
    }
    return 0;
}
/*----------------------------------------------------------------------------*/
static int ami_resume(struct i2c_client *client)
{
    struct ami_priv *obj = i2c_get_clientdata(client);        
    int err;
    AMI_FUN();

    if (!obj) {
        AMI_ERR("null pointer!!\n");
        return -EINVAL;
    }
    
    ami_power(1);
    if ((err = ami_init_client(client))) {
        AMI_ERR("initialize client fail!!\n");
        return err;        
    }

    atomic_set(&obj->suspend, 0);

    if (atomic_read(&obj->enable)) 
        return ami_enable(obj, 1);
    
    return 0;
}
/*----------------------------------------------------------------------------*/
static int ami_attach_adapter(struct i2c_adapter *adapter)
{
 	if (adapter->id == ami_platform_data.i2c_num)
		return i2c_probe(adapter, &ami_addr_data, ami_detect);
	return -1;	
}
/*----------------------------------------------------------------------------*/
static int ami_detach_client(struct i2c_client *client)
{
	int err;
	struct ami_priv *obj = i2c_get_clientdata(client);

	AMI_FUN();

    err = hwmsen_detach(HWM_SENSOR_MAGNETIC_FIELD);
    if (err) 
        AMI_ERR("detach fail:%d\n", err);
	err = i2c_detach_client(client);
	if (err){
		AMI_ERR("Client deregistration failed, client not detached.\n");
		return err;
	}
	sysfs_remove_group(&client->dev.kobj, &ami_group);
	i2c_unregister_device(client);	
	kfree(obj);
	return 0;
}
/*----------------------------------------------------------------------------*/
static int ami_probe(struct platform_device *pdev) 
{
    struct msensor_hardware *hw = (struct msensor_hardware*)pdev->dev.platform_data;   
    AMI_FUN();    
    if (hw)
        memcpy(&ami_platform_data, hw, sizeof(*hw));

    ami_power(1);    
    if (i2c_add_driver(&ami_driver)) {
        AMI_ERR("add driver error\n");
        return -1;
    } 
    return 0;
}
/*----------------------------------------------------------------------------*/
static int ami_remove(struct platform_device *pdev)
{
    AMI_FUN();    
    ami_power(0);    
    i2c_del_driver(&ami_driver);
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
static int __init ami_init(void)
{
    AMI_FUN();
    if (platform_driver_register(&ami_sensor_driver)) {
        AMI_ERR("failed to register driver");
        return -ENODEV;
    }
    return 0;    
}
/*----------------------------------------------------------------------------*/
static void __exit ami_exit(void)
{
    AMI_FUN();
    platform_driver_unregister(&ami_sensor_driver);
}
/*----------------------------------------------------------------------------*/
module_init(ami_init);
module_exit(ami_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("AMI304 driver");
MODULE_AUTHOR("MingHsien Hsieh<minghsien.hsieh@mediatek.com");

