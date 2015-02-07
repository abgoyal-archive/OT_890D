
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
#include <linux/delay.h>
#include <linux/earlysuspend.h>

#include <mach/mt6516_devs.h>
#include <mach/mt6516_typedefs.h>
#include <mach/mt6516_gpio.h>
#include <mach/mt6516_pll.h>

#include "hwmsen_helper.h"
#include "hwmsen_dev.h"
#include "adxl345.h"
/*the version string is automatically modified by Perforce*/
#define ADXL345_DRV_VERSION		"$Revision: #1 $"
struct scale_factor{
    u8  whole;
    u8  fraction;
};
/*----------------------------------------------------------------------------*/
struct data_resolution {
    struct scale_factor scalefactor;
    int                 sensitivity;
};
/*----------------------------------------------------------------------------*/
struct calibration_item {
    u32    time;
    s16    dat[ADXL345_AXES_NUM];
};
/*----------------------------------------------------------------------------*/
#define ADXL345_TRC_GET_DATA    0x0001
/*----------------------------------------------------------------------------*/
struct adxl345_object{
	struct i2c_client	    client;
    struct gsensor_hardware *hw;
    struct hwmsen_convert   cvt;
    struct hwmsen_conf      cnf;
    
    /*calibration*/
    struct work_struct      cali_work;   
    s16                     cali_period;
    s16                     cali_number;
    /*software calibration: the unit follows standard format*/
    s16                     cali_sw[ADXL345_AXES_NUM+1]; 

    /*misc*/
    atomic_t                enable;
    atomic_t                suspend;
    atomic_t                trace;
    struct data_resolution *reso;

    /*data*/
	s8					    offset[ADXL345_AXES_NUM+1];  /*+1: for 4-byte alignment*/
    s16                     data[ADXL345_AXES_NUM+1];

    /*early suspend*/
#if defined(CONFIG_HAS_EARLYSUSPEND)
    struct early_suspend    early_drv;
#endif 
};
static int adxl345_attach_adapter(struct i2c_adapter *adapter);
static int adxl345_detach_client(struct i2c_client* client);
#if !defined(CONFIG_HAS_EARLYSUSPEND)
static int adxl345_suspend(struct i2c_client *client, pm_message_t msg); 
static int adxl345_resume(struct i2c_client *client);
#else
static void adxl345_early_suspend(struct early_suspend *h); 
static void adxl345_late_resume(struct early_suspend *h);
#endif 
static int adxl345_set_data_resolution(struct adxl345_object *obj);
static ssize_t adxl345_show_offset(struct device *device, struct device_attribute *attr, char *buf);
static ssize_t adxl345_show_data(struct device *device, struct device_attribute *attr, char* buf);
static ssize_t adxl345_show_reg(struct device *device, struct device_attribute *attr, char *buf);
static ssize_t adxl345_store_reg(struct device* device, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t adxl345_show_dump(struct device *device, struct device_attribute *attr, char *buf);
static ssize_t adxl345_show_calibr(struct device *device, struct device_attribute *attr, char *buf);
static ssize_t adxl345_store_calibr(struct device* device, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t adxl345_show_trace(struct device *device, struct device_attribute *attr, char *buf);
static ssize_t adxl345_store_trace(struct device* device, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t adxl345_show_test(struct device *device, struct device_attribute *attr, char *buf);
static struct gsensor_hardware adxl345_platform_data = {
    .i2c_num = 2,
    .direction = 6,
    .offset = {60, -64, -403},
};
/*----------------------------------------------------------------------------*/
static struct data_resolution adxl345_data_resolution[] = {
 /*8 combination by {FULL_RES,RANGE}*/
    {{ 3, 9}, 256},   /*+/-2g  in 10-bit resolution:  3.9 mg/LSB*/
    {{ 7, 8}, 128},   /*+/-4g  in 10-bit resolution:  7.8 mg/LSB*/
    {{15, 6},  64},   /*+/-8g  in 10-bit resolution: 15.6 mg/LSB*/
    {{31, 2},  32},   /*+/-16g in 10-bit resolution: 31.2 mg/LSB*/
    {{ 3, 9}, 256},   /*+/-2g  in 10-bit resolution:  3.9 mg/LSB (full-resolution)*/
    {{ 3, 9}, 256},   /*+/-4g  in 11-bit resolution:  3.9 mg/LSB (full-resolution)*/
    {{ 3, 9}, 256},   /*+/-8g  in 12-bit resolution:  3.9 mg/LSB (full-resolution)*/
    {{ 3, 9}, 256},   /*+/-16g in 13-bit resolution:  3.9 mg/LSB (full-resolution)*/            
};
/*----------------------------------------------------------------------------*/
static struct data_resolution adxl345_offset_resolution = {{15, 6}, 64};
/*----------------------------------------------------------------------------*/
static SENSOR_DEVICE_ATTR(calibr,          S_IWUSR | S_IRUGO, adxl345_show_calibr, adxl345_store_calibr, 0);
static SENSOR_DEVICE_ATTR(offset,          S_IWUSR | S_IRUGO, adxl345_show_offset, NULL,           0);
static SENSOR_DEVICE_ATTR(data           ,           S_IRUGO, adxl345_show_data,   NULL,             0);
static SENSOR_DEVICE_ATTR(trace,           S_IWUSR | S_IRUGO, adxl345_show_trace,  adxl345_store_trace, 0); 
static SENSOR_DEVICE_ATTR(test,            S_IWUSR | S_IRUGO, adxl345_show_test,   NULL, 0); 
#if defined(ADXL345_TEST_MODE)
static SENSOR_DEVICE_ATTR(dump           ,           S_IRUGO, adxl345_show_dump, NULL,             ADXL345_ATTR_REGS);
static SENSOR_DEVICE_ATTR(DEVID		     ,           S_IRUGO, adxl345_show_reg, NULL             , ADXL345_REG_DEVID		  );
static SENSOR_DEVICE_ATTR(THRESH_TAP     , S_IWUSR | S_IRUGO, adxl345_show_reg, adxl345_store_reg, ADXL345_REG_THRESH_TAP     );
static SENSOR_DEVICE_ATTR(OFSX           , S_IWUSR | S_IRUGO, adxl345_show_reg, adxl345_store_reg, ADXL345_REG_OFSX           );
static SENSOR_DEVICE_ATTR(OFSY           , S_IWUSR | S_IRUGO, adxl345_show_reg, adxl345_store_reg, ADXL345_REG_OFSY           );
static SENSOR_DEVICE_ATTR(OFSZ           , S_IWUSR | S_IRUGO, adxl345_show_reg, adxl345_store_reg, ADXL345_REG_OFSZ           );
static SENSOR_DEVICE_ATTR(DUR            , S_IWUSR | S_IRUGO, adxl345_show_reg, adxl345_store_reg, ADXL345_REG_DUR            );
static SENSOR_DEVICE_ATTR(LATENT         , S_IWUSR | S_IRUGO, adxl345_show_reg, adxl345_store_reg, ADXL345_REG_LATENT         );
static SENSOR_DEVICE_ATTR(WINDOW         , S_IWUSR | S_IRUGO, adxl345_show_reg, adxl345_store_reg, ADXL345_REG_WINDOW         );
static SENSOR_DEVICE_ATTR(THRESH_ACT     , S_IWUSR | S_IRUGO, adxl345_show_reg, adxl345_store_reg, ADXL345_REG_THRESH_ACT     );
static SENSOR_DEVICE_ATTR(THRESH_INACT   , S_IWUSR | S_IRUGO, adxl345_show_reg, adxl345_store_reg, ADXL345_REG_THRESH_INACT   );
static SENSOR_DEVICE_ATTR(TIME_INACT     , S_IWUSR | S_IRUGO, adxl345_show_reg, adxl345_store_reg, ADXL345_REG_TIME_INACT     );
static SENSOR_DEVICE_ATTR(ACT_INACT_CTL  , S_IWUSR | S_IRUGO, adxl345_show_reg, adxl345_store_reg, ADXL345_REG_ACT_INACT_CTL  );
static SENSOR_DEVICE_ATTR(THRESH_FF      , S_IWUSR | S_IRUGO, adxl345_show_reg, adxl345_store_reg, ADXL345_REG_THRESH_FF      );
static SENSOR_DEVICE_ATTR(TIME_FF        , S_IWUSR | S_IRUGO, adxl345_show_reg, adxl345_store_reg, ADXL345_REG_TIME_FF        );
static SENSOR_DEVICE_ATTR(TAP_AXES       , S_IWUSR | S_IRUGO, adxl345_show_reg, adxl345_store_reg, ADXL345_REG_TAP_AXES       );
static SENSOR_DEVICE_ATTR(ACT_TAP_STATUS ,           S_IRUGO, adxl345_show_reg, NULL             , ADXL345_REG_ACT_TAP_STATUS );
static SENSOR_DEVICE_ATTR(BW_RATE        , S_IWUSR | S_IRUGO, adxl345_show_reg, adxl345_store_reg, ADXL345_REG_BW_RATE        );
static SENSOR_DEVICE_ATTR(POWER_CTL      , S_IWUSR | S_IRUGO, adxl345_show_reg, adxl345_store_reg, ADXL345_REG_POWER_CTL      );
static SENSOR_DEVICE_ATTR(INT_ENABLE     , S_IWUSR | S_IRUGO, adxl345_show_reg, adxl345_store_reg, ADXL345_REG_INT_ENABLE     );
static SENSOR_DEVICE_ATTR(INT_MAP        , S_IWUSR | S_IRUGO, adxl345_show_reg, adxl345_store_reg, ADXL345_REG_INT_MAP        );
static SENSOR_DEVICE_ATTR(INT_SOURCE     ,           S_IRUGO, adxl345_show_reg, NULL             , ADXL345_REG_INT_SOURCE     );
static SENSOR_DEVICE_ATTR(DATA_FORMAT    , S_IWUSR | S_IRUGO, adxl345_show_reg, adxl345_store_reg, ADXL345_REG_DATA_FORMAT    );
static SENSOR_DEVICE_ATTR(DATAX0         ,           S_IRUGO, adxl345_show_reg, NULL             , ADXL345_REG_DATAX0         );
static SENSOR_DEVICE_ATTR(DATAX1         ,           S_IRUGO, adxl345_show_reg, NULL             , ADXL345_REG_DATAX1         );
static SENSOR_DEVICE_ATTR(DATAY0         ,           S_IRUGO, adxl345_show_reg, NULL             , ADXL345_REG_DATAY0         );
static SENSOR_DEVICE_ATTR(DATAY1         ,           S_IRUGO, adxl345_show_reg, NULL             , ADXL345_REG_DATAY1         );
static SENSOR_DEVICE_ATTR(DATAZ0         ,           S_IRUGO, adxl345_show_reg, NULL             , ADXL345_REG_DATAZ0         );
static SENSOR_DEVICE_ATTR(DATAZ1         ,           S_IRUGO, adxl345_show_reg, NULL             , ADXL345_REG_DATAZ1         );
static SENSOR_DEVICE_ATTR(FIFO_CTL       , S_IWUSR | S_IRUGO, adxl345_show_reg, adxl345_store_reg, ADXL345_REG_FIFO_CTL       );
static SENSOR_DEVICE_ATTR(FIFO_STATUS    ,           S_IRUGO, adxl345_show_reg, NULL             , ADXL345_REG_FIFO_STATUS    );
#endif /*ADXL345_TEST_MODE*/
/*----------------------------------------------------------------------------*/
static struct attribute *adxl345_attributes[] = {
    &sensor_dev_attr_calibr.dev_attr.attr,
	&sensor_dev_attr_offset.dev_attr.attr,
	&sensor_dev_attr_data.dev_attr.attr,
	&sensor_dev_attr_trace.dev_attr.attr,
	&sensor_dev_attr_test.dev_attr.attr,
#if defined(ADXL345_TEST_MODE)	
	&sensor_dev_attr_dump.dev_attr.attr,
	&sensor_dev_attr_DEVID.dev_attr.attr,
	&sensor_dev_attr_THRESH_TAP.dev_attr.attr,
	&sensor_dev_attr_OFSX.dev_attr.attr,
	&sensor_dev_attr_OFSY.dev_attr.attr,
	&sensor_dev_attr_OFSZ.dev_attr.attr,
	&sensor_dev_attr_DUR.dev_attr.attr,
	&sensor_dev_attr_LATENT.dev_attr.attr,
	&sensor_dev_attr_WINDOW.dev_attr.attr,
	&sensor_dev_attr_THRESH_ACT.dev_attr.attr,
	&sensor_dev_attr_THRESH_INACT.dev_attr.attr,
	&sensor_dev_attr_TIME_INACT.dev_attr.attr,
	&sensor_dev_attr_ACT_INACT_CTL.dev_attr.attr,
	&sensor_dev_attr_THRESH_FF.dev_attr.attr,
	&sensor_dev_attr_TIME_FF.dev_attr.attr,
	&sensor_dev_attr_TAP_AXES.dev_attr.attr,
	&sensor_dev_attr_ACT_TAP_STATUS.dev_attr.attr,
	&sensor_dev_attr_BW_RATE.dev_attr.attr,
	&sensor_dev_attr_POWER_CTL.dev_attr.attr,
	&sensor_dev_attr_INT_ENABLE.dev_attr.attr,
	&sensor_dev_attr_INT_MAP.dev_attr.attr,
	&sensor_dev_attr_INT_SOURCE.dev_attr.attr,
	&sensor_dev_attr_DATA_FORMAT.dev_attr.attr,
	&sensor_dev_attr_DATAX0.dev_attr.attr,
	&sensor_dev_attr_DATAX1.dev_attr.attr,
	&sensor_dev_attr_DATAY0.dev_attr.attr,
	&sensor_dev_attr_DATAY1.dev_attr.attr,
	&sensor_dev_attr_DATAZ0.dev_attr.attr,
	&sensor_dev_attr_DATAZ1.dev_attr.attr,
	&sensor_dev_attr_FIFO_CTL.dev_attr.attr,
	&sensor_dev_attr_FIFO_STATUS.dev_attr.attr,
#endif /*ADXL345_TEST_MODE*/	
	NULL
};
/*----------------------------------------------------------------------------*/
static const struct attribute_group adxl345_group = {
	.attrs = adxl345_attributes,
};
/*----------------------------------------------------------------------------*/
#if defined(ADXL345_TEST_MODE)
/*----------------------------------------------------------------------------*/
static struct hwmsen_reg adxl345_regs[] = {
    {"THRESH_TAP",        ADXL345_REG_THRESH_TAP,       REG_RW, 0xFF, 1},
    {"OFSX",              ADXL345_REG_OFSX,             REG_RW, 0xFF, 1},
    {"OFSY",              ADXL345_REG_OFSY,             REG_RW, 0xFF, 1},
    {"OFSZ",              ADXL345_REG_OFSZ,             REG_RW, 0xFF, 1},
    {"DUR",               ADXL345_REG_DUR,              REG_RW, 0xFF, 1},
    {"LATENT",            ADXL345_REG_LATENT,           REG_RW, 0xFF, 1},
    {"WINDOW",            ADXL345_REG_WINDOW,           REG_RW, 0xFF, 1},
    {"THRESH_ACT",        ADXL345_REG_THRESH_ACT,       REG_RW, 0xFF, 1},
        
    {"THRESH_INACT",      ADXL345_REG_THRESH_INACT,     REG_RW, 0xFF, 1},
    {"TIME_INACT",        ADXL345_REG_TIME_INACT,       REG_RW, 0xFF, 1},
    {"ACT_INACT_CTL",     ADXL345_REG_ACT_INACT_CTL,    REG_RW, 0xFF, 1},
    {"THRESH_FF",         ADXL345_REG_THRESH_FF,        REG_RW, 0xFF, 1},
    {"TIME_FF",           ADXL345_REG_TIME_FF,          REG_RW, 0xFF, 1},
    {"TAP_AXES",          ADXL345_REG_TAP_AXES,         REG_RW, 0x0F, 1},
    {"ACT_TAP_STATUS",    ADXL345_REG_ACT_TAP_STATUS,   REG_RO, 0xFF, 1},
    {"BW_RATE",           ADXL345_REG_BW_RATE,          REG_RW, 0x1F, 1},
        
    {"POWER_CTL",         ADXL345_REG_POWER_CTL,        REG_RW, 0x3F, 1},
    {"INT_ENABLE",        ADXL345_REG_INT_ENABLE,       REG_RW, 0xFF, 1},
    {"INT_MAP",           ADXL345_REG_INT_MAP,          REG_RW, 0xFF, 1},
    {"INT_SOURCE",        ADXL345_REG_INT_SOURCE,       REG_RO, 0xFF, 1},
    {"DATA_FORMAT",       ADXL345_REG_DATA_FORMAT,      REG_RW, 0xFF, 1},
    {"DATAX0",            ADXL345_REG_DATAX0,           REG_RO, 0xFF, 1},
    {"DATAX1",            ADXL345_REG_DATAX1,           REG_RO, 0xFF, 1},
    {"DATAY0",            ADXL345_REG_DATAY0,           REG_RO, 0xFF, 1},
    {"DATAY1",            ADXL345_REG_DATAY1,           REG_RO, 0xFF, 1},
    {"DATAZ0",            ADXL345_REG_DATAZ0,           REG_RO, 0xFF, 1},
    {"DATAZ1",            ADXL345_REG_DATAZ1,           REG_RO, 0xFF, 1},
    {"FIFO_CTL",          ADXL345_REG_FIFO_CTL,         REG_RW, 0xFF, 1},
    {"FIFO_STATUS",       ADXL345_REG_FIFO_STATUS,      REG_RO, 0xFF, 1}, 
};
/*----------------------------------------------------------------------------*/
struct hwmsen_reg adxl345_dummy = {"", 0,0,0,0};
/*----------------------------------------------------------------------------*/
static void adxl345_single_rw(struct i2c_client *client) 
{
    hwmsen_single_rw(client, adxl345_regs, 
                     sizeof(adxl345_regs)/sizeof(adxl345_regs[0]));
}
/*----------------------------------------------------------------------------*/
struct hwmsen_reg* adxl345_get_reg(int reg) 
{
    if ((reg >= ADXL345_REG_THRESH_TAP) && (reg <= ADXL345_REG_FIFO_STATUS))
        return &adxl345_regs[reg - ADXL345_REG_THRESH_TAP];
    else
        return &adxl345_dummy;
}
/*----------------------------------------------------------------------------*/
static void adxl345_multi_rw(struct i2c_client *client) 
{
    struct hwmsen_reg_test_multi test_list[] = {
        {ADXL345_REG_THRESH_TAP, 7, REG_RW},
        {ADXL345_REG_THRESH_TAP, 6, REG_RW},
        {ADXL345_REG_THRESH_TAP, 5, REG_RW},
        {ADXL345_REG_THRESH_TAP, 4, REG_RW},
        {ADXL345_REG_THRESH_TAP, 3, REG_RW},
        {ADXL345_REG_THRESH_TAP, 2, REG_RW},
        {ADXL345_REG_THRESH_TAP, 1, REG_RW},
        {ADXL345_REG_THRESH_ACT, 7, REG_RW},
        {ADXL345_REG_THRESH_ACT, 6, REG_RW},
        {ADXL345_REG_THRESH_ACT, 5, REG_RW},
        {ADXL345_REG_THRESH_ACT, 4, REG_RW},
        {ADXL345_REG_THRESH_ACT, 3, REG_RW},
        {ADXL345_REG_THRESH_ACT, 2, REG_RW},
        {ADXL345_REG_THRESH_ACT, 1, REG_RW},
        {ADXL345_REG_BW_RATE, 4, REG_RW},
        {ADXL345_REG_BW_RATE, 3, REG_RW},
        {ADXL345_REG_BW_RATE, 2, REG_RW},
        {ADXL345_REG_BW_RATE, 1, REG_RW},
        {ADXL345_REG_OFSX, 3, REG_RW},
    };

    hwmsen_multi_rw(client, adxl345_get_reg,
                    test_list, sizeof(test_list)/sizeof(test_list[0]));
}
/*----------------------------------------------------------------------------*/
static void adxl345_test(struct i2c_client *client) 
{
    adxl345_single_rw(client);
    adxl345_multi_rw(client);
}
/*----------------------------------------------------------------------------*/
static ssize_t adxl345_show_test(struct device *dev,
                                 struct device_attribute *attr, char* buf)
{
	struct i2c_client *client = to_i2c_client(dev);    
    adxl345_test(client);
    return snprintf(buf, PAGE_SIZE, "done");
}
/*----------------------------------------------------------------------------*/
static ssize_t adxl345_show_dump(struct device *dev, 
								 struct device_attribute *attr, char* buf)
{
    /*the register name starting from THRESH_TAP*/
    #define REG_OFFSET      ADXL345_REG_THRESH_TAP 
    #define REG_TABLE_LEN   0x3A 
    static u8 adxl345_reg_value[REG_TABLE_LEN] = {0};
	struct i2c_client *client = to_i2c_client(dev);    
    
    return hwmsen_show_dump(client, REG_OFFSET, 
                            adxl345_reg_value, REG_TABLE_LEN,
                            adxl345_get_reg, buf, PAGE_SIZE);
}
/*----------------------------------------------------------------------------*/
#endif /*ADXL345_TEST_MODE*/
/*----------------------------------------------------------------------------*/
static int adxl345_read_offset(struct adxl345_object *obj, s8 ofs[ADXL345_AXES_NUM])
{
	struct i2c_client *client = &obj->client;
	int err;

    if ((err = hwmsen_read_block(client, ADXL345_REG_OFSX, ofs, ADXL345_AXES_NUM))) 
        ADX_ERR("error: %d\n", err);

    return err;    
}
/*----------------------------------------------------------------------------*/
static int adxl345_reset_offset(struct adxl345_object *obj)
{
	struct i2c_client *client = &obj->client;
    s8 ofs[ADXL345_AXES_NUM] = {0x00, 0x00, 0x00};
	int err;

    if ((err = hwmsen_write_block(client, ADXL345_REG_OFSX, ofs, ADXL345_AXES_NUM))) 
        ADX_ERR("error: %d\n", err);

    return err;    
}
/*----------------------------------------------------------------------------*/
static int adxl345_read_data(struct adxl345_object *obj, s16 data[ADXL345_AXES_NUM])
{
    struct i2c_client *client = &obj->client;
    u8 addr = ADXL345_REG_DATAX0;
    u8 buf[ADXL345_DATA_LEN] = {0};
    int err = 0;

    if (!client) {
        err = -EINVAL;
    } else if ((err = hwmsen_read_block(client, addr, buf, ADXL345_DATA_LEN))) {
        ADX_ERR("error: %d\n", err);
    } else {
		data[ADXL345_AXIS_X] = (s16)((buf[ADXL345_AXIS_X*2]) |
									 (buf[ADXL345_AXIS_X*2+1] << 8));
		data[ADXL345_AXIS_Y] = (s16)((buf[ADXL345_AXIS_Y*2]) |
									 (buf[ADXL345_AXIS_Y*2+1] << 8));
		data[ADXL345_AXIS_Z] = (s16)((buf[ADXL345_AXIS_Z*2]) |
									 (buf[ADXL345_AXIS_Z*2+1] << 8));
    }
    return err;
}
#if 0
/*----------------------------------------------------------------------------*/
#define ABSDIF(X,Y) ((X > Y) ? (Y - X) : (X - Y))
/*----------------------------------------------------------------------------*/
#define DIVERSE_X   0x0001
#define DIVERSE_Y   0x0002
#define DIVERSE_Z   0x0004
#define DIVERSE_XYZ 0x0008
/*----------------------------------------------------------------------------*/
static int adxl345_check_calibration(struct adxl345_object *obj, 
                                     struct calibration_item *all, u16 num, 
                                     s16 avg[ADXL345_AXES_NUM])
{
    int idx, diverse = 0;
    int maxdif = obj->reso->sensitivity/10; /*the variation should be less than 0.1G*/
    s16 max[ADXL345_AXES_NUM] = {0xFFFF, 0xFFFF, 0xFFFF};
    s16 min[ADXL345_AXES_NUM] = {0x7FFF, 0x7FFF, 0x7FFF};
    s32 diffx = 0, diffy = 0, diffz = 0;

    ADX_LOG("------------------------------------------\n");
    ADX_LOG("             Calibration Data             \n");
    ADX_LOG("------------------------------------------\n");
    for (idx = 0; idx < num; idx++) 
        ADX_LOG("[0x%08X] (0x%08X, 0x%08X, 0x%08X) => (%6d, %6d, %6d)\n", all[idx].time,
                all[idx].dat[ADXL345_AXIS_X], all[idx].dat[ADXL345_AXIS_Y], all[idx].dat[ADXL345_AXIS_Z],
                all[idx].dat[ADXL345_AXIS_X], all[idx].dat[ADXL345_AXIS_Y], all[idx].dat[ADXL345_AXIS_Z]);
    ADX_LOG("------------------------------------------\n");
    ADX_LOG("             Average Data                 \n");
    ADX_LOG("------------------------------------------\n");
    ADX_LOG("(0x%08X, 0x%08X, 0x%08X)\n", avg[ADXL345_AXIS_X], 
            avg[ADXL345_AXIS_Y], avg[ADXL345_AXIS_Z]);
    
    for (idx = 0; idx < num; idx++) {
        if (max[ADXL345_AXIS_X] < all[idx].dat[ADXL345_AXIS_X])
            max[ADXL345_AXIS_X] = all[idx].dat[ADXL345_AXIS_X];
        if (max[ADXL345_AXIS_Y] < all[idx].dat[ADXL345_AXIS_Y])
            max[ADXL345_AXIS_Y] = all[idx].dat[ADXL345_AXIS_Y];
        if (max[ADXL345_AXIS_Z] < all[idx].dat[ADXL345_AXIS_Z])
            max[ADXL345_AXIS_Z] = all[idx].dat[ADXL345_AXIS_Z];
        if (min[ADXL345_AXIS_X] > all[idx].dat[ADXL345_AXIS_X])
            min[ADXL345_AXIS_X] = all[idx].dat[ADXL345_AXIS_X];
        if (min[ADXL345_AXIS_Y] > all[idx].dat[ADXL345_AXIS_Y])
            min[ADXL345_AXIS_Y] = all[idx].dat[ADXL345_AXIS_Y];
        if (min[ADXL345_AXIS_Z] > all[idx].dat[ADXL345_AXIS_Z])
            min[ADXL345_AXIS_Z] = all[idx].dat[ADXL345_AXIS_Z];

        diffx = ABSDIF(avg[ADXL345_AXIS_X], all[idx].dat[ADXL345_AXIS_X]);
        diffy = ABSDIF(avg[ADXL345_AXIS_Y], all[idx].dat[ADXL345_AXIS_Y]);
        diffz = ABSDIF(avg[ADXL345_AXIS_Z], all[idx].dat[ADXL345_AXIS_Z]);
        
        if (diffx > maxdif) {
            ADX_LOG("unstable x-axis calibration: (%02d) 0x%4X -> 0x%4X\n", 
                    idx, avg[ADXL345_AXIS_X], all[idx].dat[ADXL345_AXIS_X]);
            diverse |= DIVERSE_X; 
        }
        if (diffy > maxdif) {
            ADX_LOG("unstable y-axis calibration: (%02d) 0x%4X -> 0x%4X\n", 
                    idx, avg[ADXL345_AXIS_Y], all[idx].dat[ADXL345_AXIS_Y]);
            diverse |= DIVERSE_Y;
        }
        if (diffz > maxdif) {
            ADX_LOG("unstable z-axis calibration: (%02d) 0x%4X -> 0x%4X\n", 
                    idx, avg[ADXL345_AXIS_Z], all[idx].dat[ADXL345_AXIS_Z]);
            diverse |= DIVERSE_Z;
        }
        if ((diffx*diffx + diffy*diffy + diffz*diffz) > maxdif*maxdif) {
            ADX_LOG("unstable calibration: (%02d) %d, %d, %d\n", 
                    idx, diffx, diffy, diffz);
            diverse |= DIVERSE_XYZ;
        }
        
    }
    if (diverse)    
        return -EINVAL;
    return 0;
}
/*----------------------------------------------------------------------------*/
static ssize_t adxl345_store_calibr(struct device *dev, 
								 struct device_attribute *attr, 
								 const char* buf, size_t count) 
{   /*if the function doesn't return count, the funtion will be called again to consume buffer*/
	struct i2c_client *client = to_i2c_client(dev);
    struct adxl345_object *obj = i2c_get_clientdata(client);    
    int delay, num, x, y, z, err;

    if (!strncmp(buf, "autof", 4)) {    /*auto-calibration after reseting offset*/
        /*default value of delay and num*/
        if ((count > 4) && 
            (2 == sscanf(buf+4, "%d %d", &delay, &num)) ) {
        } else {
            delay = 100;
            num = 20;
        }
        if ((err = adxl345_reset_offset(obj))) {
            ADX_ERR("reset offset fail: %d\n", err);
        } else {
            /*create a work to do auto-calibration*/
            obj->cali_number  = num;
            obj->cali_period  = delay;
            schedule_work(&obj->cali_work);
        }
    } else if (!strncmp(buf, "auto", 4)) { /*auto-calibration without reseting offset*/
        /*default value of delay and num*/
        if ((count > 4) && 
            (2 == sscanf(buf+4, "%d %d", &delay, &num)) ) {
        } else {
            delay = 100;
            num = 20;
        }
         /*create a work to do auto-calibration*/
        obj->cali_number  = num;
        obj->cali_period  = delay;
        schedule_work(&obj->cali_work);
    } else if (3 == sscanf(buf, "%d %d %d", &x, &y, &z)) {
        s16 dat[ADXL345_AXES_NUM];
        dat[ADXL345_AXIS_X] = (s16)x;
        dat[ADXL345_AXIS_Y] = (s16)y;
        dat[ADXL345_AXIS_Z] = (s16)z;
        if ((err = adxl345_write_calibration(obj, dat)))
            ADX_ERR("fail to write calibration: %d\n", err);
    } else {
        ADX_ERR("invalid format!!\n");
    }
    return count;
}
/*----------------------------------------------------------------------------*/
static void adxl345_calibration_work(struct work_struct *work) 
{
    struct adxl345_object *obj = container_of(work, struct adxl345_object, cali_work);
    struct calibration_item *raw = NULL;
    s32 sum[ADXL345_AXES_NUM] = {0, 0, 0};
    s16 avg[ADXL345_AXES_NUM] = {0, 0, 0};
    s16 golden[ADXL345_AXES_NUM];
    s16 faceup[ADXL345_AXES_NUM];
    int idx, err;
    u8  mode = 0x00;
    
    if (!obj || !work) {
        ADX_ERR("null pointer");
        return;
    } else if ((err = hwmsen_read_byte(&obj->client, ADXL345_REG_POWER_CTL, &mode))) {
		ADX_ERR("write data format fail!!\n");
		return;
	} else if (!(mode & ADXL345_MEASURE)) {
	    if (adxl345_enable(&obj->client, 1)) {
            ADX_ERR("enable fail!!\n");
            return ;
	    }
	}


    /* the following calibration is for face-up: (0G, 0G, 1G)*/
    raw = kzalloc(obj->cali_number * sizeof(*raw), GFP_KERNEL);

    idx = 0;
    while(idx < obj->cali_number) {
        if (msleep_interruptible(obj->cali_period))
            continue;       
        if ((err = adxl345_read_data(obj, raw[idx].dat))) {
            ADX_ERR("read data failed!!");
            goto exit_read_failed;
        }
        raw[idx].time = jiffies;
        sum[ADXL345_AXIS_X] += raw[idx].dat[ADXL345_AXIS_X];
        sum[ADXL345_AXIS_Y] += raw[idx].dat[ADXL345_AXIS_Y];
        sum[ADXL345_AXIS_Z] += raw[idx].dat[ADXL345_AXIS_Z];        
        idx++;
    }
    avg[ADXL345_AXIS_X] = sum[ADXL345_AXIS_X]/obj->cali_number;
    avg[ADXL345_AXIS_Y] = sum[ADXL345_AXIS_Y]/obj->cali_number;
    avg[ADXL345_AXIS_Z] = sum[ADXL345_AXIS_Z]/obj->cali_number;

    ADX_LOG("sum: %4d %4d %4d\n", sum[ADXL345_AXIS_X], sum[ADXL345_AXIS_Y], sum[ADXL345_AXIS_Z]);
    ADX_LOG("avg: %4d %4d %4d\n", avg[ADXL345_AXIS_X], avg[ADXL345_AXIS_Y], avg[ADXL345_AXIS_Z]);
    if ((err = adxl345_check_calibration(obj, raw, obj->cali_number, avg))) {
        ADX_ERR("invalid calibration!!");
        goto exit_check_calibration;
    }

    /*get standard format of face up*/
    golden[0] = 0;
    golden[1] = 0;
    golden[2] = obj->cnf.sensitivity[ADXL345_AXIS_Z];
    
    if ((err = adxl345_to_device_format(obj, golden, faceup))) {
        ADX_ERR("convert to device format!!");
        goto exit_device_format;        
    }

    ADX_LOG("faceup: %4d, %4d, %4d\n", faceup[ADXL345_AXIS_X], faceup[ADXL345_AXIS_Y], faceup[ADXL345_AXIS_Z]);
    /*calulate offset using (0G, 0G, 1G)*/
    avg[ADXL345_AXIS_X] = faceup[ADXL345_AXIS_X] - avg[ADXL345_AXIS_X];
    avg[ADXL345_AXIS_Y] = faceup[ADXL345_AXIS_Y] - avg[ADXL345_AXIS_Y];
    avg[ADXL345_AXIS_Z] = faceup[ADXL345_AXIS_Z] - avg[ADXL345_AXIS_Z];

    if ((err = adxl345_write_calibration(obj, avg))) {
        ADX_ERR("faile to write calibration!!");
        goto exit_write_calibration;        
    }
        
exit_write_calibration:    
exit_device_format:    
exit_check_calibration:
exit_read_failed:
    kfree(raw);
	if (!(mode & ADXL345_MEASURE)) {
	    if (adxl345_enable(&obj->client, 0)) 
            ADX_ERR("disable fail!!\n");
	}
}
#endif 
/*----------------------------------------------------------------------------*/
/* @dat calibration value with standard format                                */
/*----------------------------------------------------------------------------*/
static int adxl345_write_calibration(struct adxl345_object *obj, s16 dat[ADXL345_AXES_NUM])
{
    int err;
    int cali[ADXL345_AXES_NUM];
    int lsb = adxl345_offset_resolution.sensitivity;

    if ((err = adxl345_read_offset(obj, obj->offset))) {
        ADX_ERR("read offset fail, %d\n", err);
        return err;
    }
    /*convert standard calibration to device format*/    
    cali[ADXL345_AXIS_X] = obj->cvt.sign[ADXL345_AXIS_X]*(dat[obj->cvt.map[ADXL345_AXIS_X]])/(obj->cnf.sensitivity[ADXL345_AXIS_X]/lsb);
    cali[ADXL345_AXIS_Y] = obj->cvt.sign[ADXL345_AXIS_Y]*(dat[obj->cvt.map[ADXL345_AXIS_Y]])/(obj->cnf.sensitivity[ADXL345_AXIS_Y]/lsb);
    cali[ADXL345_AXIS_Z] = obj->cvt.sign[ADXL345_AXIS_Z]*(dat[obj->cvt.map[ADXL345_AXIS_Z]])/(obj->cnf.sensitivity[ADXL345_AXIS_Z]/lsb);   
    ADX_LOG("%4d = (%2d * %4d * %4d) / %4d\n", cali[ADXL345_AXIS_X], obj->cvt.sign[ADXL345_AXIS_X], dat[obj->cvt.map[ADXL345_AXIS_X]], lsb, obj->cnf.sensitivity[ADXL345_AXIS_X]);
    ADX_LOG("%4d = (%2d * %4d * %4d) / %4d\n", cali[ADXL345_AXIS_Y], obj->cvt.sign[ADXL345_AXIS_Y], dat[obj->cvt.map[ADXL345_AXIS_Y]], lsb, obj->cnf.sensitivity[ADXL345_AXIS_Y]);
    ADX_LOG("%4d = (%2d * %4d * %4d) / %4d\n", cali[ADXL345_AXIS_Z], obj->cvt.sign[ADXL345_AXIS_Z], dat[obj->cvt.map[ADXL345_AXIS_Z]], lsb, obj->cnf.sensitivity[ADXL345_AXIS_Z]);    

    obj->offset[ADXL345_AXIS_X] += (s8)cali[ADXL345_AXIS_X];
    obj->offset[ADXL345_AXIS_Y] += (s8)cali[ADXL345_AXIS_Y];
    obj->offset[ADXL345_AXIS_Z] += (s8)cali[ADXL345_AXIS_Z];
    ADX_LOG("offset = (%4d, %4d, %4d) => (0x%02X, 0x%02X, 0x%02X)\n", 
            obj->offset[ADXL345_AXIS_X], obj->offset[ADXL345_AXIS_Y], obj->offset[ADXL345_AXIS_Z],
            obj->offset[ADXL345_AXIS_X], obj->offset[ADXL345_AXIS_Y], obj->offset[ADXL345_AXIS_Z]);
    if ((err = hwmsen_write_block(&obj->client, ADXL345_REG_OFSX, obj->offset, ADXL345_AXES_NUM))) {
        ADX_ERR("write offset fail: %d\n", err);
        return err;
    }
    /*convert software calibration using standard calibration*/
    obj->cali_sw[ADXL345_AXIS_X] = obj->cvt.sign[ADXL345_AXIS_X]*(dat[obj->cvt.map[ADXL345_AXIS_X]])%(obj->cnf.sensitivity[ADXL345_AXIS_X]/lsb);
    obj->cali_sw[ADXL345_AXIS_Y] = obj->cvt.sign[ADXL345_AXIS_Y]*(dat[obj->cvt.map[ADXL345_AXIS_Y]])%(obj->cnf.sensitivity[ADXL345_AXIS_Y]/lsb);
    obj->cali_sw[ADXL345_AXIS_Z] = obj->cvt.sign[ADXL345_AXIS_Z]*(dat[obj->cvt.map[ADXL345_AXIS_Z]])%(obj->cnf.sensitivity[ADXL345_AXIS_Z]/lsb);
     
    ADX_LOG("calibr: %4d, %4d, %4d\n", dat[0], dat[1], dat[2]);
    ADX_LOG("hwcali: %4d, %4d, %4d\n", obj->offset[0], obj->offset[1], obj->offset[2]);
    ADX_LOG("swcali: %4d, %4d, %4d\n", obj->cali_sw[0], obj->cali_sw[1], obj->cali_sw[2]);    

    return err;
}
/*----------------------------------------------------------------------------*/
static int adxl345_to_standard_format(struct adxl345_object *obj, s16 dat[ADXL345_AXES_NUM],
                                      s16 out[ADXL345_AXES_NUM])
{   /*align layout, convert to requested resolution*/
    int lsb = obj->reso->sensitivity;
    s16 tmp[ADXL345_AXES_NUM];

    tmp[ADXL345_AXIS_X] = (obj->cvt.sign[ADXL345_AXIS_X]*obj->cnf.sensitivity[ADXL345_AXIS_X]*(dat[ADXL345_AXIS_X]))/lsb;
    tmp[ADXL345_AXIS_Y] = (obj->cvt.sign[ADXL345_AXIS_Y]*obj->cnf.sensitivity[ADXL345_AXIS_Y]*(dat[ADXL345_AXIS_Y]))/lsb;
    tmp[ADXL345_AXIS_Z] = (obj->cvt.sign[ADXL345_AXIS_Z]*obj->cnf.sensitivity[ADXL345_AXIS_Z]*(dat[ADXL345_AXIS_Z]))/lsb;
    out[obj->cvt.map[ADXL345_AXIS_X]] = tmp[ADXL345_AXIS_X] + obj->cali_sw[ADXL345_AXIS_X];
    out[obj->cvt.map[ADXL345_AXIS_Y]] = tmp[ADXL345_AXIS_Y] + obj->cali_sw[ADXL345_AXIS_Y];
    out[obj->cvt.map[ADXL345_AXIS_Z]] = tmp[ADXL345_AXIS_Z] + obj->cali_sw[ADXL345_AXIS_Z];
    return 0;
}
/*----------------------------------------------------------------------------*/
static void adxl345_power(unsigned int on) 
{
    static unsigned int power_on = 0;

    ADX_LOG("power %s\n", on ? "on" : "off");
    
    if (power_on == on) {
        ADX_LOG("ignore power control: %d\n", on);
    } else if (on) {
        if (!hwPowerOn(MT6516_POWER_VGP, VOL_2500, "ADXL345")) 
            ADX_ERR("power on fails!!\n");
    } else {
        if (!hwPowerDown(MT6516_POWER_VGP,"ADXL345")) 
            ADX_ERR("power off fail!!\n");   
    }
    power_on = on;
}
/*----------------------------------------------------------------------------*/
static int adxl345_set_data_resolution(struct adxl345_object *obj)
{
    int err;
    u8  dat, reso;
    
	if ((err = hwmsen_read_byte(&obj->client, ADXL345_REG_DATA_FORMAT, &dat))) {
		ADX_ERR("write data format fail!!\n");
		return err;
	}

    /*the data_reso is combined by 3 bits: {FULL_RES, DATA_RANGE}*/
    reso  = (dat & ADXL345_FULL_RESOLUTION) ? (0x04) : (0x00);
    reso |= (dat & ADXL345_RANGE_16G); 

    if (reso < sizeof(adxl345_data_resolution)/sizeof(adxl345_data_resolution[0])) {        
        obj->reso = &adxl345_data_resolution[reso];
        return 0;
    } else {
        return -EINVAL;
    }
}
/*----------------------------------------------------------------------------*/
static void twocomp_2_fraction(s16 data, u8 int_scale, u8 fra_scale, 
                               int *whole, int* fraction)
{
    /*convert from 2's complement to whole integer*/
    s16 val = (data & 0x8000) ? (~(data-1))*(-1) : (data);
    int deci  = val * int_scale * 10;
    int frac = val * fra_scale;
    int temp;
    int sign;    

    /* handle negative fraction */
	if (frac < 0) {
		sign = -1;
		frac *= sign;
	}

	/* carry if necessary*/
	if (frac >= 10) {
		deci += sign * frac;
		temp = frac / 10;
		frac -= temp * 10;
	}
	deci /= 10;

	/* If at least 10 still remains in the fractional part, one last carry*/
	 
	if (frac >= 10) {
		deci += sign;
		frac -= 10;
	}

	/* Pass the values up */
	*whole = deci;
	*fraction = frac;    
    printk("0x%4X:%d, (%d,%d) <-> (%d, %d)\n", data, val, int_scale, fra_scale, *whole, *fraction);
}
/*----------------------------------------------------------------------------*/
/*show calibration with standard format                                       */
/*----------------------------------------------------------------------------*/
static ssize_t adxl345_show_calibr(struct device *dev, 
								   struct device_attribute *attr, char* buf) 
{
	struct i2c_client *client = to_i2c_client(dev);
    struct adxl345_object *obj = i2c_get_clientdata(client);    
    int len = 0, err, lsb;
    s16 cali[ADXL345_AXES_NUM], tmp[ADXL345_AXES_NUM];

    if ((err = hwmsen_read_block(client, ADXL345_REG_OFSX, obj->offset, ADXL345_AXES_NUM))) {        
        ADX_ERR("error: %d\n", err);
        return 0;
    }
    len += snprintf(buf+len, PAGE_SIZE-len, "hw > sensitivity : %4d\n", adxl345_offset_resolution.sensitivity);
    len += snprintf(buf+len, PAGE_SIZE-len, "hw > (0x%08X, 0x%08X, 0x%08X) => (%4d, %4d, %4d)\n",  
                    obj->offset[ADXL345_AXIS_X], obj->offset[ADXL345_AXIS_Y], obj->offset[ADXL345_AXIS_Z],        
                    obj->offset[ADXL345_AXIS_X], obj->offset[ADXL345_AXIS_Y], obj->offset[ADXL345_AXIS_Z]);
    len += snprintf(buf+len, PAGE_SIZE-len, "sw > sensitivity : (%4d, %4d, %4d)\n", 
                    obj->cnf.sensitivity[obj->cvt.map[ADXL345_AXIS_X]],
                    obj->cnf.sensitivity[obj->cvt.map[ADXL345_AXIS_Y]],
                    obj->cnf.sensitivity[obj->cvt.map[ADXL345_AXIS_Z]]);
    len += snprintf(buf+len, PAGE_SIZE-len, "sw : (0x%08X, 0x%08X, 0x%08X) => (%4d, %4d, %4d)\n",
                    obj->cali_sw[ADXL345_AXIS_X], obj->cali_sw[ADXL345_AXIS_Y], obj->cali_sw[ADXL345_AXIS_Z],
                    obj->cali_sw[ADXL345_AXIS_X], obj->cali_sw[ADXL345_AXIS_Y], obj->cali_sw[ADXL345_AXIS_Z]);     
    
    lsb = adxl345_offset_resolution.sensitivity;
    tmp[ADXL345_AXIS_X] = (obj->cvt.sign[ADXL345_AXIS_X]*obj->cnf.sensitivity[ADXL345_AXIS_X]*(obj->offset[ADXL345_AXIS_X]))/lsb;
    tmp[ADXL345_AXIS_Y] = (obj->cvt.sign[ADXL345_AXIS_Y]*obj->cnf.sensitivity[ADXL345_AXIS_Y]*(obj->offset[ADXL345_AXIS_Y]))/lsb;
    tmp[ADXL345_AXIS_Z] = (obj->cvt.sign[ADXL345_AXIS_Z]*obj->cnf.sensitivity[ADXL345_AXIS_Z]*(obj->offset[ADXL345_AXIS_Z]))/lsb;
    cali[obj->cvt.map[ADXL345_AXIS_X]] = tmp[ADXL345_AXIS_X] + obj->cali_sw[ADXL345_AXIS_X];
    cali[obj->cvt.map[ADXL345_AXIS_Y]] = tmp[ADXL345_AXIS_Y] + obj->cali_sw[ADXL345_AXIS_Y];
    cali[obj->cvt.map[ADXL345_AXIS_Z]] = tmp[ADXL345_AXIS_Z] + obj->cali_sw[ADXL345_AXIS_Z];

    len += snprintf(buf+len, PAGE_SIZE-len, "all: (0x%08X, 0x%08X, 0x%08X)\n",
                    cali[HWM_EVT_ACC_X], cali[HWM_EVT_ACC_Y], cali[HWM_EVT_ACC_Z]);
    return len;                    
}
/*----------------------------------------------------------------------------*/
static ssize_t adxl345_store_calibr(struct device *dev, 
								 struct device_attribute *attr, 
								 const char* buf, size_t count) 
{   /*if the function doesn't return count, the funtion will be called again to consume buffer*/
	struct i2c_client *client = to_i2c_client(dev);
    struct adxl345_object *obj = i2c_get_clientdata(client);    
    int x, y, z, err;

    if (3 == sscanf(buf, "%d %d %d", &x, &y, &z)) {
        s16 dat[ADXL345_AXES_NUM];
        dat[ADXL345_AXIS_X] = (s16)x;
        dat[ADXL345_AXIS_Y] = (s16)y;
        dat[ADXL345_AXIS_Z] = (s16)z;
        if ((err = adxl345_write_calibration(obj, dat)))
            ADX_ERR("fail to write calibration: %d\n", err);
    } else {
        ADX_ERR("invalid format!!\n");
    }
    return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t adxl345_show_reg(struct device *dev, 
								struct device_attribute *attr, char* buf) 
{
	int index = to_sensor_dev_attr(attr)->index;
	struct i2c_client *client = to_i2c_client(dev);
    u8 addr = (u8)index;

    if (addr > ADXL345_ADDR_MAX) {
        ADX_ERR("invalid address: 0x%02X\n", addr);
        return 0;
    } else {
        return hwmsen_show_reg(client, addr, buf, PAGE_SIZE);
    }
}
/*----------------------------------------------------------------------------*/
static ssize_t adxl345_store_reg(struct device *dev, 
								 struct device_attribute *attr, 
								 const char* buf, size_t count) 
{   /*if the function doesn't return count, the funtion will be called again to consume buffer*/
	int err, index = to_sensor_dev_attr(attr)->index;
	struct i2c_client *client = to_i2c_client(dev);
    struct adxl345_object *obj = i2c_get_clientdata(client);    
    u8 addr = (u8)index;
    ssize_t cnt;

    if (addr > ADXL345_ADDR_MAX) {
        ADX_ERR("invalid address: 0x%02X\n", addr);
        return count;
    } else {
        cnt = hwmsen_store_reg(client, addr, buf, PAGE_SIZE);
        if (addr == ADXL345_REG_DATA_FORMAT) {
            if ((err = adxl345_set_data_resolution(obj))) 
                ADX_ERR("fail to set data resolution: %d\n", err);
        }
        return cnt;        
    }    
}
/*----------------------------------------------------------------------------*/
static ssize_t adxl345_show_offset(struct device *dev, 
								struct device_attribute *attr, char* buf)
{
    int idx, decimal[ADXL345_AXES_NUM], fraction[ADXL345_AXES_NUM];    
    struct i2c_client *client = to_i2c_client(dev);
    struct adxl345_object *obj = i2c_get_clientdata(client);
    int err;
    if ((err = adxl345_read_offset(obj, obj->offset)))
        return -EINVAL;

    for (idx = 0; idx < ADXL345_AXES_NUM; idx++) {
        twocomp_2_fraction(obj->offset[idx], 
                           adxl345_offset_resolution.scalefactor.whole, 
                           adxl345_offset_resolution.scalefactor.fraction, 
                           &decimal[idx], &fraction[idx]);
    }
    
	return snprintf(buf, PAGE_SIZE, "offs: (%5d.%1d, %5d.%1d, %5d.%1d)\n", 
                    decimal[ADXL345_AXIS_X], fraction[ADXL345_AXIS_X],
                    decimal[ADXL345_AXIS_Y], fraction[ADXL345_AXIS_Y],
                    decimal[ADXL345_AXIS_Z], fraction[ADXL345_AXIS_Z]);
}
/*----------------------------------------------------------------------------*/
static ssize_t adxl345_show_data(struct device *dev, 
								struct device_attribute *attr, char* buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct adxl345_object *obj = i2c_get_clientdata(client);
    int err;
    if ((err = adxl345_read_data(obj, obj->data)))
        return -EINVAL;
    
	return snprintf(buf, PAGE_SIZE, "data: (%5d, %5d, %5d)\n", 
                    obj->data[ADXL345_AXIS_X], obj->data[ADXL345_AXIS_Y],
                    obj->data[ADXL345_AXIS_Z]);

}
/*----------------------------------------------------------------------------*/
static ssize_t adxl345_show_trace(struct device *dev, 
                                  struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct adxl345_object *obj = i2c_get_clientdata(client);
    
	return snprintf(buf, PAGE_SIZE, "0x%08X\n", atomic_read(&obj->trace));
}
/*----------------------------------------------------------------------------*/
static ssize_t adxl345_store_trace(struct device* dev, 
                                   struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
    struct adxl345_object *obj = i2c_get_clientdata(client);    
    int trc;

    if (1 == sscanf(buf, "0x%x\n", &trc)) 
        atomic_set(&obj->trace, trc);
    else
        ADX_ERR("set trace level fail!!\n");
    return count;
}
int adxl345_enable(struct i2c_client *client, u8 enable) 
{
    int err;
    u8 cur, nxt;

	if ((err = hwmsen_read_byte(client, ADXL345_REG_POWER_CTL, &cur))) {
		ADX_ERR("write data format fail!!\n");
		return err;
	}

    if (enable)
        nxt = cur | (ADXL345_MEASURE);
    else
        nxt = cur & (~ADXL345_MEASURE);

    if (cur ^ nxt) {    /*update POWER_CTL if changed*/    
        if ((err = hwmsen_write_byte(client, ADXL345_REG_POWER_CTL, nxt))) {
            ADX_ERR("write power control fail!!\n");
            return err;
        }
    }
    return 0;    
}
/*----------------------------------------------------------------------------*/
int adxl345_activate(void* self, u8 enable) 
{
    struct adxl345_object* obj = (struct adxl345_object*)self;
    struct i2c_client *client = &obj->client;

    ADX_LOG("%s (%d)\n", __func__, enable);
    if (!obj)
        return -EINVAL;

    if (enable)
        atomic_set(&obj->enable, 1);
    else
        atomic_set(&obj->enable, 0);

    return adxl345_enable(client, enable);
}
/*----------------------------------------------------------------------------*/
int adxl345_get_data(void* self, struct hwmsen_data *data) 
{
    struct adxl345_object* obj = (struct adxl345_object*)self;
    int idx, err;
    s16 output[ADXL345_AXES_NUM];

    if (!obj || !data) {
        ADX_ERR("null pointer");
        return -EINVAL;
    }

    if (!atomic_read(&obj->enable) || atomic_read(&obj->suspend)) {
        ADX_ERR("the sensor is not activated: %d, %d\n", 
                atomic_read(&obj->enable), atomic_read(&obj->suspend));
        return -EINVAL;
    }
        
    if ((err = adxl345_read_data(obj, obj->data))) {
        ADX_ERR("read data failed!!");
        return -EINVAL;
    }

    if ((err = adxl345_to_standard_format(obj, obj->data, output))) {
        ADX_ERR("convert to standard format fail:%d\n",err);
        return -EINVAL;
    }

    data->num = 3;
    for (idx = 0; idx < data->num; idx++)
        data->raw[idx] = (int)output[idx];

    if (atomic_read(&obj->trace) & ADXL345_TRC_GET_DATA) {
        ADX_LOG("%d (0x%08X, 0x%08X, 0x%08X) -> (%5d, %5d, %5d)\n", obj->reso->sensitivity,
                obj->data[ADXL345_AXIS_X], obj->data[ADXL345_AXIS_Y], obj->data[ADXL345_AXIS_Z],
                data->raw[HWM_EVT_ACC_X], data->raw[HWM_EVT_ACC_Y], data->raw[HWM_EVT_ACC_Z]);
    }
    return 0;
}
/*----------------------------------------------------------------------------*/
int adxl345_set_cali(void* self, struct hwmsen_data *data)
{
    struct adxl345_object* obj = (struct adxl345_object*)self;
    
    ADX_FUN();
    if (!obj || ! data) {
        ADX_ERR("null ptr!!\n");
        return -EINVAL;
    } else {
        s16 cali[ADXL345_AXES_NUM];
        cali[HWM_EVT_ACC_X] = data->raw[HWM_EVT_ACC_X];
        cali[HWM_EVT_ACC_Y] = data->raw[HWM_EVT_ACC_Y];
        cali[HWM_EVT_ACC_Z] = data->raw[HWM_EVT_ACC_Z];
        return adxl345_write_calibration(obj, cali);
    }        
}
/*----------------------------------------------------------------------------*/
/* get calibration data with standard format                                  */  
/*----------------------------------------------------------------------------*/
int adxl345_get_cali(void* self, struct hwmsen_data *data)
{
    struct adxl345_object* obj = (struct adxl345_object*)self;
    int err;
    int tmp[ADXL345_AXES_NUM];
    
    ADX_FUN();
    if (!obj || ! data) {
        ADX_ERR("null ptr!!\n");
        return -EINVAL;
    } else if ((err = hwmsen_read_block(&obj->client, ADXL345_REG_OFSX, obj->offset, ADXL345_AXES_NUM)))  {
        ADX_ERR("read offset: %d\n", err);
        return -err;
    } else {
        int lsb = adxl345_offset_resolution.sensitivity;    
        tmp[ADXL345_AXIS_X] = (obj->cvt.sign[ADXL345_AXIS_X]*obj->cnf.sensitivity[ADXL345_AXIS_X]*(obj->offset[ADXL345_AXIS_X]))/lsb;
        tmp[ADXL345_AXIS_Y] = (obj->cvt.sign[ADXL345_AXIS_Y]*obj->cnf.sensitivity[ADXL345_AXIS_Y]*(obj->offset[ADXL345_AXIS_Y]))/lsb;
        tmp[ADXL345_AXIS_Z] = (obj->cvt.sign[ADXL345_AXIS_Z]*obj->cnf.sensitivity[ADXL345_AXIS_Z]*(obj->offset[ADXL345_AXIS_Z]))/lsb;

        data->raw[obj->cvt.map[ADXL345_AXIS_X]] = tmp[ADXL345_AXIS_X] + obj->cali_sw[ADXL345_AXIS_X];
        data->raw[obj->cvt.map[ADXL345_AXIS_Y]] = tmp[ADXL345_AXIS_Y] + obj->cali_sw[ADXL345_AXIS_Y];
        data->raw[obj->cvt.map[ADXL345_AXIS_Z]] = tmp[ADXL345_AXIS_Z] + obj->cali_sw[ADXL345_AXIS_Z];
        data->num = 3;
        return 0;
    }    
}
/*----------------------------------------------------------------------------*/
int adxl345_get_prop(void *self, struct hwmsen_prop *prop)
{
    struct adxl345_object* obj = (struct adxl345_object*)self;
    
    ADX_FUN();
    if (!obj) {
        ADX_ERR("null pointer");
        return -EINVAL;
    }   

    prop->evt_max[obj->cvt.map[ADXL345_AXIS_X]] =  4095 * (obj->cnf.sensitivity[ADXL345_AXIS_X]/256);
    prop->evt_min[obj->cvt.map[ADXL345_AXIS_X]] = -4096 * (obj->cnf.sensitivity[ADXL345_AXIS_X]/256);
    prop->evt_max[obj->cvt.map[ADXL345_AXIS_Y]] =  4095 * (obj->cnf.sensitivity[ADXL345_AXIS_Y]/256);
    prop->evt_min[obj->cvt.map[ADXL345_AXIS_Y]] = -4096 * (obj->cnf.sensitivity[ADXL345_AXIS_Y]/256);
    prop->evt_max[obj->cvt.map[ADXL345_AXIS_Z]] =  4095 * (obj->cnf.sensitivity[ADXL345_AXIS_Y]/256);
    prop->evt_min[obj->cvt.map[ADXL345_AXIS_Z]] = -4096 * (obj->cnf.sensitivity[ADXL345_AXIS_Y]/256);
    prop->num = 3;
    return 0;
}
static unsigned short normal_i2c[] = {ADXL345_WR_SLAVE_ADDR, I2C_CLIENT_END};
static unsigned short ignore = I2C_CLIENT_END;
static struct i2c_client_address_data adxl345_addr_data = {
	.normal_i2c = normal_i2c,
	.probe  = &ignore,
	.ignore = &ignore,
};
/*----------------------------------------------------------------------------*/
static struct i2c_driver adxl345_driver = {
	.attach_adapter = adxl345_attach_adapter,
	.detach_client  = adxl345_detach_client,
#if !defined(CONFIG_HAS_EARLYSUSPEND)	
	.suspend        = adxl345_suspend,
	.resume         = adxl345_resume,
#endif 	
	.driver = {
		.name = ADX_DEV_NAME,
	},
};
/*----------------------------------------------------------------------------*/
static int adxl345_init_client(struct i2c_client* client)
{
	/* 1. Power On/Off is done by I2C driver
	 * 2. GPIO configuration is done in board-phone.c
	 */
    struct adxl345_object *obj = i2c_get_clientdata(client);    	 
	int err;

	/*configure device*/
	if ((err = hwmsen_write_byte(client, ADXL345_REG_DATA_FORMAT, ADXL345_DATA_FORMAT_DEFAULT))) {
		ADX_ERR("write data format fail!!\n");
		return err;
	}
    /*default is standby mode*/
    if ((err = hwmsen_write_byte(client, ADXL345_REG_POWER_CTL, ADXL345_POWER_CTL_DEFAULT))) {
        ADX_ERR("write power control fail!!\n");
        return err;
    }
    if ((err = adxl345_set_data_resolution(obj))) {
        ADX_ERR("set data resolution fail!!\n");
        return err;
    }    
    
    if ((err = adxl345_reset_offset(obj))) {
        ADX_ERR("reset offset fail!!\n");
        return err;
    }
    /*write default calibration data*/
    if ((err = adxl345_write_calibration(obj, obj->hw->offset))) {
        ADX_ERR("write calibration fail!!\n");
        return err;
    }
	return 0;
}
/*----------------------------------------------------------------------------*/
static int adxl345_detect(struct i2c_adapter* adapter, int address, int kind)
{
	int err = 0;
	struct i2c_client *client;
	struct adxl345_object *obj;
    struct hwmsen_object sobj;
	u8 dev_id;
	
	ADX_FUN();

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		goto exit;

	if (!(obj = kmalloc(sizeof(*obj), GFP_KERNEL))){
		err = -ENOMEM;
		goto exit;
	}

	memset(obj, 0 ,sizeof(*obj));

    obj->hw = &adxl345_platform_data;
    if ((err = hwmsen_get_convert(obj->hw->direction, &obj->cvt))) {
        ADX_ERR("invalid direction: %d\n", obj->hw->direction);
        goto exit;
    }
    if ((err = hwmsen_get_conf(HWM_SENSOR_ACCELERATION, &obj->cnf))) {
        ADX_ERR("get conf fail: %d\n", err);
        goto exit;
    }    
    atomic_set(&obj->enable, 0);
    atomic_set(&obj->suspend, 0);
    atomic_set(&obj->trace, 0);   /*disable all log by default*/
    //INIT_WORK(&obj->cali_work, adxl345_calibration_work);    
	client = &obj->client;
	client->addr = address;
	client->adapter = adapter;
	client->driver = &adxl345_driver;
	client->flags = 0;
	strncpy(client->name, ADX_DEV_NAME , I2C_NAME_SIZE);
	i2c_set_clientdata(client, obj);

	if ((err = i2c_attach_client(client)))
		goto exit_kfree;

	/*check device ID*/
	if ((err = hwmsen_read_byte(client, ADXL345_REG_DEVID, &dev_id))) {
		ADX_ERR("read device id fail!!\n");
		return err;
	}

	if (dev_id != 0xE5) {
		ADX_ERR("device id 0x%X not match!!\n", (unsigned int)dev_id);
		return -ENODEV; /*no such device*/
	} else {
		ADX_LOG("device is found, driver version: %s\n", ADXL345_DRV_VERSION);
	}

	if ((err = adxl345_init_client(client)))
		goto exit_kfree;
        
	if ((err = sysfs_create_group(&client->dev.kobj, &adxl345_group))) {
		ADX_ERR("sysfs_create_group error = %d\n", err);
		goto exit_kfree;
	}

    sobj.self = obj;
    sobj.activate = adxl345_activate;
    sobj.get_prop = adxl345_get_prop;
    sobj.get_data = adxl345_get_data;
    sobj.set_cali = adxl345_set_cali;
    sobj.get_cali = adxl345_get_cali;
    if ((err = hwmsen_attach(HWM_SENSOR_ACCELERATION, &sobj))) {
        ADX_ERR("attach fail = %d\n", err);
        goto exit_kfree;
    }
    
#if defined(CONFIG_HAS_EARLYSUSPEND)
    obj->early_drv.level    = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1,
    obj->early_drv.suspend  = adxl345_early_suspend,
    obj->early_drv.resume   = adxl345_late_resume,    
    register_early_suspend(&obj->early_drv);
#endif        
	return 0;

exit_kfree:
	kfree(obj);
exit:
	return err;
}
/*----------------------------------------------------------------------------*/
#if !defined(CONFIG_HAS_EARLYSUSPEND)
/*----------------------------------------------------------------------------*/
static int adxl345_suspend(struct i2c_client *client, pm_message_t msg) 
{
    struct adxl345_object *obj = i2c_get_clientdata(client);    
    int err;
    ADX_FUN();    

    if (msg.event == PM_EVENT_SUSPEND) {   
        if (!obj) {
            ADX_ERR("null pointer!!\n");
            return -EINVAL;
        }
        if ((err = hwmsen_write_byte(client, ADXL345_REG_POWER_CTL, 0x00))) {
            ADX_ERR("write power control fail!!\n");
            return err;
        }        
        atomic_set(&obj->suspend, 1);
        adxl345_power(0);
    }
    return 0;
}
/*----------------------------------------------------------------------------*/
static int adxl345_resume(struct i2c_client *client)
{
    struct adxl345_object *obj = i2c_get_clientdata(client);        
    int err;
    ADX_FUN();

    if (!obj) {
        ADX_ERR("null pointer!!\n");
        return -EINVAL;
    }
    
    adxl345_power(1);
    if ((err = adxl345_init_client(client))) {
        ADX_ERR("initialize client fail!!\n");
        return err;        
    }

    atomic_set(&obj->suspend, 0);

    if (atomic_read(&obj->enable)) 
        return adxl345_enable(client, 1);
    
    return 0;
}
/*----------------------------------------------------------------------------*/
#else /*CONFIG_HAS_EARLY_SUSPEND is defined*/
/*----------------------------------------------------------------------------*/
static void adxl345_early_suspend(struct early_suspend *h) 
{
    struct adxl345_object *obj = container_of(h, struct adxl345_object, early_drv);   
    int err;
    ADX_FUN();    

    if (!obj) {
        ADX_ERR("null pointer!!\n");
        return;
    }
    if ((err = hwmsen_write_byte(&obj->client, ADXL345_REG_POWER_CTL, 0x00))) {
        ADX_ERR("write power control fail!!\n");
        return;
    }        
    atomic_set(&obj->suspend, 1);
    adxl345_power(0);
}
/*----------------------------------------------------------------------------*/
static void adxl345_late_resume(struct early_suspend *h)
{
    struct adxl345_object *obj = container_of(h, struct adxl345_object, early_drv);         
    int err;
    ADX_FUN();

    if (!obj) {
        ADX_ERR("null pointer!!\n");
        return;
    }
    
    adxl345_power(1);
    if ((err = adxl345_init_client(&obj->client))) {
        ADX_ERR("initialize client fail!!\n");
        return;        
    }

    atomic_set(&obj->suspend, 0);

    if (atomic_read(&obj->enable)) 
        adxl345_enable(&obj->client, 1);
    
}
/*----------------------------------------------------------------------------*/
#endif /*CONFIG_HAS_EARLYSUSPEND*/
/*----------------------------------------------------------------------------*/
static int adxl345_attach_adapter(struct i2c_adapter *adapter)
{
 	if (adapter->id == adxl345_platform_data.i2c_num)
		return i2c_probe(adapter, &adxl345_addr_data, adxl345_detect);
	return -1;	
}
/*----------------------------------------------------------------------------*/
static int adxl345_detach_client(struct i2c_client *client)
{
	int err;
	struct adxl345_object *obj = i2c_get_clientdata(client);

	ADX_FUN();

    err = hwmsen_detach(HWM_SENSOR_ACCELERATION);
    if (err) 
        ADX_ERR("detach fail:%d\n", err);
	err = i2c_detach_client(client);
	if (err){
		ADX_ERR("Client deregistration failed, client not detached.\n");
		return err;
	}
	sysfs_remove_group(&client->dev.kobj, &adxl345_group);
	i2c_unregister_device(client);	
	kfree(obj);
	return 0;
}
/*----------------------------------------------------------------------------*/
static int adxl345_probe(struct platform_device *pdev) 
{
    struct gsensor_hardware *hw = (struct gsensor_hardware*)pdev->dev.platform_data;   
    ADX_FUN();    
    if (hw)
        memcpy(&adxl345_platform_data, hw, sizeof(*hw));

    adxl345_power(1);    
    if (i2c_add_driver(&adxl345_driver)) {
        ADX_ERR("add driver error\n");
        return -1;
    } 
    return 0;
}
/*----------------------------------------------------------------------------*/
static int adxl345_remove(struct platform_device *pdev)
{
    ADX_FUN();    
    adxl345_power(0);    
    i2c_del_driver(&adxl345_driver);
    return 0;
}
/*----------------------------------------------------------------------------*/
static struct platform_driver adxl345_gsensor_driver = {
    .probe      = adxl345_probe,
    .remove     = adxl345_remove,    
    .driver     = {
        .name  = "adxl345",
        .owner = THIS_MODULE,
     }
};
/*----------------------------------------------------------------------------*/
static int __init adxl345_init(void)
{
    ADX_FUN();
    if (platform_driver_register(&adxl345_gsensor_driver)) {
        ADX_ERR("failed to register driver");
        return -ENODEV;
    }
    return 0;    
}
/*----------------------------------------------------------------------------*/
static void __exit adxl345_exit(void)
{
    ADX_FUN();
    platform_driver_unregister(&adxl345_gsensor_driver);
}
/*----------------------------------------------------------------------------*/
module_init(adxl345_init);
module_exit(adxl345_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ADXL345 I2C driver");
MODULE_AUTHOR("MingHsien Hsieh<minghsien.hsieh@mediatek.com");
