

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/earlysuspend.h>
#include <linux/platform_device.h>
#include <asm/atomic.h>
#include <mach/mt6516_devs.h>
#include <mach/mt6516_typedefs.h>
#include <mach/mt6516_gpio.h>
#include <mach/mt6516_pll.h>

#include <cust_acc.h>
#include <linux/hwmsensor.h>
#include "adxl345.h"
#include "hwmsen_helper.h"
/*----------------------------------------------------------------------------*/
#define I2C_DRIVERID_ADXL345 345
/*----------------------------------------------------------------------------*/
#define DEBUG 1
/*----------------------------------------------------------------------------*/
#define CONFIG_ADXL345_LOWPASS   /*apply low pass filter on output*/       
/*----------------------------------------------------------------------------*/
#define ADXL345_AXIS_X          0
#define ADXL345_AXIS_Y          1
#define ADXL345_AXIS_Z          2
#define ADXL345_AXES_NUM        3
#define ADXL345_DATA_LEN        6
/*----------------------------------------------------------------------------*/
static struct i2c_client *adxl345_i2c_client = NULL;
/*----------------------------------------------------------------------------*/
/* Addresses to scan */
static unsigned short normal_i2c[] = { ADXL345_I2C_SLAVE_ADDR, I2C_CLIENT_END };
/*----------------------------------------------------------------------------*/
/* Insmod parameters */
I2C_CLIENT_INSMOD_1(adxl345);
/*----------------------------------------------------------------------------*/
static int adxl345_i2c_attach_adapter(struct i2c_adapter *adapter);
static int adxl345_i2c_detect(struct i2c_adapter *adapter, int address, int kind);
static int adxl345_i2c_detach_client(struct i2c_client *client);
/*----------------------------------------------------------------------------*/
typedef enum {
    ADX_TRC_FILTER  = 0x01,
    ADX_TRC_RAWDATA = 0x02,
    ADX_TRC_IOCTL   = 0x04,
} ADX_TRC;
/*----------------------------------------------------------------------------*/
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
#define C_MAX_FIR_LENGTH (32)
/*----------------------------------------------------------------------------*/
struct data_filter {
    s16 raw[C_MAX_FIR_LENGTH][ADXL345_AXES_NUM];
    int sum[ADXL345_AXES_NUM];
    int num;
    int idx;
};
/*----------------------------------------------------------------------------*/
struct adxl345_i2c_data {
    struct i2c_client client;
    struct acc_hw *hw;
    struct hwmsen_convert   cvt;
    
    /*misc*/
    struct data_resolution *reso;
    atomic_t                trace;
    atomic_t                suspend;
    atomic_t                selftest;
    s16                     cali_sw[ADXL345_AXES_NUM+1];

    /*data*/
    s8                      offset[ADXL345_AXES_NUM+1];  /*+1: for 4-byte alignment*/
    s16                     data[ADXL345_AXES_NUM+1];

#if defined(CONFIG_ADXL345_LOWPASS)
    atomic_t                firlen;
    atomic_t                fir_en;
    struct data_filter      fir;
#endif 
    /*early suspend*/
#if defined(CONFIG_HAS_EARLYSUSPEND)
    struct early_suspend    early_drv;
#endif     
};
/*----------------------------------------------------------------------------*/
static struct i2c_driver adxl345_i2c_driver = {
    .driver = {
        .owner          = THIS_MODULE,
        .name           = "adxl345 driver",
    },
    .attach_adapter     = adxl345_i2c_attach_adapter,
    .detach_client      = adxl345_i2c_detach_client,
#if !defined(CONFIG_HAS_EARLYSUSPEND)    
    .suspend            = adxl345_suspend,
    .resume             = adxl345_resume,
#endif         
    .id                 = I2C_DRIVERID_ADXL345,
};
/*----------------------------------------------------------------------------*/
#define ADX_TAG                  "[ADXL345] "
#define ADX_FUN(f)               printk(KERN_INFO ADX_TAG"%s\n", __FUNCTION__)
#define ADX_ERR(fmt, args...)    printk(KERN_ERR ADX_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define ADX_LOG(fmt, args...)    printk(KERN_INFO ADX_TAG fmt, ##args)
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
static void ADXL345_power(struct acc_hw *hw, unsigned int on) 
{
    static unsigned int power_on = 0;

    if (hw->power_id != MT6516_POWER_NONE) {        
        ADX_LOG("power %s\n", on ? "on" : "off");
        if (power_on == on) {
            ADX_LOG("ignore power control: %d\n", on);
        } else if (on) {
            if (!hwPowerOn(hw->power_id, hw->power_vol, "ADXL345")) 
                ADX_ERR("power on fails!!\n");
        } else {
            if (!hwPowerDown(hw->power_id, "ADXL345")) 
                ADX_ERR("power off fail!!\n");   
        }
    }
    power_on = on;    
}
/*----------------------------------------------------------------------------*/
static int ADXL345_SetDataResolution(struct adxl345_i2c_data *obj)
{
    int err;
    u8  dat, reso;
    
    if ((err = hwmsen_read_byte(&obj->client, ADXL345_REG_DATA_FORMAT, &dat))) {
        ADX_ERR("write data format fail!!\n");
        return err;
    }

    /*the data_reso is combined by 3 bits: {FULL_RES, DATA_RANGE}*/
    reso  = (dat & ADXL345_FULL_RES) ? (0x04) : (0x00);
    reso |= (dat & ADXL345_RANGE_16G); 

    if (reso < sizeof(adxl345_data_resolution)/sizeof(adxl345_data_resolution[0])) {        
        obj->reso = &adxl345_data_resolution[reso];
        return 0;
    } else {
        return -EINVAL;
    }
}
/*----------------------------------------------------------------------------*/
static int ADXL345_ReadData(struct i2c_client *client, s16 data[ADXL345_AXES_NUM])
{
    struct adxl345_i2c_data *priv = i2c_get_clientdata(client);        
    u8 addr = ADXL345_REG_DATAX0;
    u8 buf[ADXL345_DATA_LEN] = {0};
    int err = 0;

    if (!client) {
        err = -EINVAL;
    } else if ((err = hwmsen_read_block(client, addr, buf, 0x06))) {
        ADX_ERR("error: %d\n", err);
    } else {
        data[ADXL345_AXIS_X] = (s16)((buf[ADXL345_AXIS_X*2]) |
                                     (buf[ADXL345_AXIS_X*2+1] << 8));
        data[ADXL345_AXIS_Y] = (s16)((buf[ADXL345_AXIS_Y*2]) |
                                     (buf[ADXL345_AXIS_Y*2+1] << 8));
        data[ADXL345_AXIS_Z] = (s16)((buf[ADXL345_AXIS_Z*2]) |
                                     (buf[ADXL345_AXIS_Z*2+1] << 8));
        data[ADXL345_AXIS_X] += priv->cali_sw[ADXL345_AXIS_X];
        data[ADXL345_AXIS_Y] += priv->cali_sw[ADXL345_AXIS_Y];
        data[ADXL345_AXIS_Z] += priv->cali_sw[ADXL345_AXIS_Z];
        
        if (atomic_read(&priv->trace) & ADX_TRC_RAWDATA) {
            ADX_LOG("[%08X %08X %08X] => [%5d %5d %5d]\n", data[ADXL345_AXIS_X], data[ADXL345_AXIS_Y], data[ADXL345_AXIS_Z],
                                                           data[ADXL345_AXIS_X], data[ADXL345_AXIS_Y], data[ADXL345_AXIS_Z]);
        }
#if defined(CONFIG_ADXL345_LOWPASS)
        if (atomic_read(&priv->fir_en) && !atomic_read(&priv->suspend)) {
            int idx, firlen = atomic_read(&priv->firlen);   
            if (priv->fir.num < firlen) {                
               priv->fir.raw[priv->fir.num][ADXL345_AXIS_X] = data[ADXL345_AXIS_X];
               priv->fir.raw[priv->fir.num][ADXL345_AXIS_Y] = data[ADXL345_AXIS_Y];
               priv->fir.raw[priv->fir.num][ADXL345_AXIS_Z] = data[ADXL345_AXIS_Z];
               priv->fir.sum[ADXL345_AXIS_X] += data[ADXL345_AXIS_X];
               priv->fir.sum[ADXL345_AXIS_Y] += data[ADXL345_AXIS_Y];
               priv->fir.sum[ADXL345_AXIS_Z] += data[ADXL345_AXIS_Z];
               if (atomic_read(&priv->trace) & ADX_TRC_FILTER) {
                    ADX_LOG("add [%2d] [%5d %5d %5d] => [%5d %5d %5d]\n", priv->fir.num,
                        priv->fir.raw[priv->fir.num][ADXL345_AXIS_X], priv->fir.raw[priv->fir.num][ADXL345_AXIS_Y], priv->fir.raw[priv->fir.num][ADXL345_AXIS_Z],
                        priv->fir.sum[ADXL345_AXIS_X], priv->fir.sum[ADXL345_AXIS_Y], priv->fir.sum[ADXL345_AXIS_Z]);
               }
               priv->fir.num++;
               priv->fir.idx++;
            } else {
               idx = priv->fir.idx % firlen;
               priv->fir.sum[ADXL345_AXIS_X] -= priv->fir.raw[idx][ADXL345_AXIS_X];
               priv->fir.sum[ADXL345_AXIS_Y] -= priv->fir.raw[idx][ADXL345_AXIS_Y];
               priv->fir.sum[ADXL345_AXIS_Z] -= priv->fir.raw[idx][ADXL345_AXIS_Z];
               priv->fir.raw[idx][ADXL345_AXIS_X] = data[ADXL345_AXIS_X];
               priv->fir.raw[idx][ADXL345_AXIS_Y] = data[ADXL345_AXIS_Y];
               priv->fir.raw[idx][ADXL345_AXIS_Z] = data[ADXL345_AXIS_Z];
               priv->fir.sum[ADXL345_AXIS_X] += data[ADXL345_AXIS_X];
               priv->fir.sum[ADXL345_AXIS_Y] += data[ADXL345_AXIS_Y];
               priv->fir.sum[ADXL345_AXIS_Z] += data[ADXL345_AXIS_Z];
               priv->fir.idx++;
               data[ADXL345_AXIS_X] = priv->fir.sum[ADXL345_AXIS_X]/firlen;
               data[ADXL345_AXIS_Y] = priv->fir.sum[ADXL345_AXIS_Y]/firlen;
               data[ADXL345_AXIS_Z] = priv->fir.sum[ADXL345_AXIS_Z]/firlen;
               if (atomic_read(&priv->trace) & ADX_TRC_FILTER) {
                    ADX_LOG("add [%2d] [%5d %5d %5d] => [%5d %5d %5d] : [%5d %5d %5d]\n", idx,
                        priv->fir.raw[idx][ADXL345_AXIS_X], priv->fir.raw[idx][ADXL345_AXIS_Y], priv->fir.raw[idx][ADXL345_AXIS_Z],
                        priv->fir.sum[ADXL345_AXIS_X], priv->fir.sum[ADXL345_AXIS_Y], priv->fir.sum[ADXL345_AXIS_Z],
                        data[ADXL345_AXIS_X], data[ADXL345_AXIS_Y], data[ADXL345_AXIS_Z]);
               }
            }
        }
#endif         
    }
    return err;
}
/*----------------------------------------------------------------------------*/
static int ADXL345_ReadOffset(struct i2c_client *client, s8 ofs[ADXL345_AXES_NUM])
{    
    int err;

    if ((err = hwmsen_read_block(client, ADXL345_REG_OFSX, ofs, ADXL345_AXES_NUM))) 
        ADX_ERR("error: %d\n", err);

    return err;    
}
/*----------------------------------------------------------------------------*/
static int ADXL345_ResetCalibration(struct i2c_client *client)
{
    struct adxl345_i2c_data *obj = i2c_get_clientdata(client);
    s8 ofs[ADXL345_AXES_NUM] = {0x00, 0x00, 0x00};
    int err;

    if ((err = hwmsen_write_block(client, ADXL345_REG_OFSX, ofs, ADXL345_AXES_NUM))) 
        ADX_ERR("error: %d\n", err);
    memset(obj->cali_sw, 0x00, sizeof(obj->cali_sw));
    return err;    
}
/*----------------------------------------------------------------------------*/
static int ADXL345_ReadCalibration(struct i2c_client *client, int dat[ADXL345_AXES_NUM])
{
    struct adxl345_i2c_data *obj = i2c_get_clientdata(client);
    int err;
    int mul;

    if ((err = ADXL345_ReadOffset(client, obj->offset))) {
        ADX_ERR("read offset fail, %d\n", err);
        return err;
    }    
    
    mul = obj->reso->sensitivity/adxl345_offset_resolution.sensitivity;

    dat[obj->cvt.map[ADXL345_AXIS_X]] = obj->cvt.sign[ADXL345_AXIS_X]*(obj->offset[ADXL345_AXIS_X]*mul + obj->cali_sw[ADXL345_AXIS_X]);
    dat[obj->cvt.map[ADXL345_AXIS_Y]] = obj->cvt.sign[ADXL345_AXIS_Y]*(obj->offset[ADXL345_AXIS_Y]*mul + obj->cali_sw[ADXL345_AXIS_Y]);
    dat[obj->cvt.map[ADXL345_AXIS_Z]] = obj->cvt.sign[ADXL345_AXIS_Z]*(obj->offset[ADXL345_AXIS_Z]*mul + obj->cali_sw[ADXL345_AXIS_Z]);                        
                                       
    return 0;
}
/*----------------------------------------------------------------------------*/
static int ADXL345_ReadCalibrationEx(struct i2c_client *client, int act[ADXL345_AXES_NUM], int raw[ADXL345_AXES_NUM])
{   /*raw: the raw calibration data; act: the actual calibration data*/
    struct adxl345_i2c_data *obj = i2c_get_clientdata(client);
    int err;
    int mul;

    if ((err = ADXL345_ReadOffset(client, obj->offset))) {
        ADX_ERR("read offset fail, %d\n", err);
        return err;
    }    
    
    mul = obj->reso->sensitivity/adxl345_offset_resolution.sensitivity;
    raw[ADXL345_AXIS_X] = obj->offset[ADXL345_AXIS_X]*mul + obj->cali_sw[ADXL345_AXIS_X];
    raw[ADXL345_AXIS_Y] = obj->offset[ADXL345_AXIS_Y]*mul + obj->cali_sw[ADXL345_AXIS_Y];
    raw[ADXL345_AXIS_Z] = obj->offset[ADXL345_AXIS_Z]*mul + obj->cali_sw[ADXL345_AXIS_Z];
    
    act[obj->cvt.map[ADXL345_AXIS_X]] = obj->cvt.sign[ADXL345_AXIS_X]*raw[ADXL345_AXIS_X];
    act[obj->cvt.map[ADXL345_AXIS_Y]] = obj->cvt.sign[ADXL345_AXIS_Y]*raw[ADXL345_AXIS_Y];
    act[obj->cvt.map[ADXL345_AXIS_Z]] = obj->cvt.sign[ADXL345_AXIS_Z]*raw[ADXL345_AXIS_Z];                        
                                       
    return 0;
}
/*----------------------------------------------------------------------------*/
static int ADXL345_WriteCalibration(struct i2c_client *client, int dat[ADXL345_AXES_NUM])
{
    struct adxl345_i2c_data *obj = i2c_get_clientdata(client);
    int err;
    int cali[ADXL345_AXES_NUM], raw[ADXL345_AXES_NUM];
    int lsb = adxl345_offset_resolution.sensitivity;
    int divisor = obj->reso->sensitivity/lsb;

    if ((err = ADXL345_ReadCalibrationEx(client, cali, raw))) { /*offset will be updated in obj->offset*/
        ADX_ERR("read offset fail, %d\n", err);
        return err;
    }
    
    ADX_LOG("OLDOFF: (%+3d %+3d %+3d): (%+3d %+3d %+3d) / (%+3d %+3d %+3d)\n", 
            raw[ADXL345_AXIS_X], raw[ADXL345_AXIS_Y], raw[ADXL345_AXIS_Z],
            obj->offset[ADXL345_AXIS_X], obj->offset[ADXL345_AXIS_Y], obj->offset[ADXL345_AXIS_Z],
            obj->cali_sw[ADXL345_AXIS_X], obj->cali_sw[ADXL345_AXIS_Y], obj->cali_sw[ADXL345_AXIS_Z]);
    
    /*calculate the real offset expected by caller*/
    cali[ADXL345_AXIS_X] += dat[ADXL345_AXIS_X];
    cali[ADXL345_AXIS_Y] += dat[ADXL345_AXIS_Y];
    cali[ADXL345_AXIS_Z] += dat[ADXL345_AXIS_Z];

    ADX_LOG("UPDATE: (%+3d %+3d %+3d)\n", 
            dat[ADXL345_AXIS_X], dat[ADXL345_AXIS_Y], dat[ADXL345_AXIS_Z]);
    
    obj->offset[ADXL345_AXIS_X] = (s8)(obj->cvt.sign[ADXL345_AXIS_X]*(cali[obj->cvt.map[ADXL345_AXIS_X]])/(divisor));
    obj->offset[ADXL345_AXIS_Y] = (s8)(obj->cvt.sign[ADXL345_AXIS_Y]*(cali[obj->cvt.map[ADXL345_AXIS_Y]])/(divisor));
    obj->offset[ADXL345_AXIS_Z] = (s8)(obj->cvt.sign[ADXL345_AXIS_Z]*(cali[obj->cvt.map[ADXL345_AXIS_Z]])/(divisor));
    /*convert software calibration using standard calibration*/
    obj->cali_sw[ADXL345_AXIS_X] = obj->cvt.sign[ADXL345_AXIS_X]*(cali[obj->cvt.map[ADXL345_AXIS_X]])%(divisor);
    obj->cali_sw[ADXL345_AXIS_Y] = obj->cvt.sign[ADXL345_AXIS_Y]*(cali[obj->cvt.map[ADXL345_AXIS_Y]])%(divisor);
    obj->cali_sw[ADXL345_AXIS_Z] = obj->cvt.sign[ADXL345_AXIS_Z]*(cali[obj->cvt.map[ADXL345_AXIS_Z]])%(divisor);
    
    ADX_LOG("NEWOFF: (%+3d %+3d %+3d): (%+3d %+3d %+3d) / (%+3d %+3d %+3d)\n", 
            obj->offset[ADXL345_AXIS_X]*divisor + obj->cali_sw[ADXL345_AXIS_X], 
            obj->offset[ADXL345_AXIS_Y]*divisor + obj->cali_sw[ADXL345_AXIS_Y], 
            obj->offset[ADXL345_AXIS_Z]*divisor + obj->cali_sw[ADXL345_AXIS_Z], 
            obj->offset[ADXL345_AXIS_X], obj->offset[ADXL345_AXIS_Y], obj->offset[ADXL345_AXIS_Z],
            obj->cali_sw[ADXL345_AXIS_X], obj->cali_sw[ADXL345_AXIS_Y], obj->cali_sw[ADXL345_AXIS_Z]);

    if ((err = hwmsen_write_block(&obj->client, ADXL345_REG_OFSX, obj->offset, ADXL345_AXES_NUM))) {
        ADX_ERR("write offset fail: %d\n", err);
        return err;
    }

    return err;
}
/*----------------------------------------------------------------------------*/
static int ADXL345_CheckDeviceID(struct i2c_client *client)
{
    u8 databuf[10];    
    int res = 0;
    
    memset(databuf, 0, sizeof(u8)*10);    
    databuf[0] = ADXL345_REG_DEVID;    
    
    res = i2c_master_send(client, databuf, 0x1);
    if (res<=0)
        goto exit_ADXL345_CheckDeviceID;
    udelay(500);

    databuf[0] = 0x0;        
    res = i2c_master_recv(client, databuf, 0x01);
    if (res<=0)
        goto exit_ADXL345_CheckDeviceID;

    if(databuf[0]!=ADXL345_FIXED_DEVID )
        return ADXL345_ERR_IDENTIFICATION;

exit_ADXL345_CheckDeviceID:
    if (res<=0)
        return ADXL345_ERR_I2C;
    return ADXL345_SUCCESS;
}
/*----------------------------------------------------------------------------*/
static int ADXL345_SetPowerMode(struct i2c_client *client, u8 powermode)
{
    u8 databuf[10];    
    int res = 0;
    
    memset(databuf, 0, sizeof(u8)*10);    
    databuf[0] = ADXL345_REG_POWER_CTL;    
    databuf[1] = powermode;
    
    res = i2c_master_send(client, databuf, 0x2);

    if (res<=0)
        return ADXL345_ERR_I2C;
    return ADXL345_SUCCESS;    
}
/*----------------------------------------------------------------------------*/
static int ADXL345_SetDataFormat(struct i2c_client *client, u8 dataformat)
{
    struct adxl345_i2c_data *obj = i2c_get_clientdata(client);
    u8 databuf[10];    
    int res = 0;
    
    memset(databuf, 0, sizeof(u8)*10);    
    databuf[0] = ADXL345_REG_DATA_FORMAT;    
    databuf[1] = dataformat;
    
    res = i2c_master_send(client, databuf, 0x2);

    if (res<=0)
        return ADXL345_ERR_I2C;
        
    return ADXL345_SetDataResolution(obj);    
}
/*----------------------------------------------------------------------------*/
static int ADXL345_SetBWRate(struct i2c_client *client, u8 bwrate)
{
    u8 databuf[10];    
    int res = 0;
    
    memset(databuf, 0, sizeof(u8)*10);    
    databuf[0] = ADXL345_REG_BW_RATE;    
    databuf[1] = bwrate;
    
    res = i2c_master_send(client, databuf, 0x2);

    if (res<=0)
        return ADXL345_ERR_I2C;
    return ADXL345_SUCCESS;    
}
/*----------------------------------------------------------------------------*/
static int ADXL345_SetIntEnable(struct i2c_client *client, u8 intenable)
{
    u8 databuf[10];    
    int res = 0;
    
    memset(databuf, 0, sizeof(u8)*10);    
    databuf[0] = ADXL345_REG_INT_ENABLE;    
    databuf[1] = intenable;
    
    res = i2c_master_send(client, databuf, 0x2);

    if (res<=0)
        return ADXL345_ERR_I2C;
    return ADXL345_SUCCESS;    
}
/*----------------------------------------------------------------------------*/
static int ADXL345_Init(struct i2c_client *client, int reset_cali)
{
    struct adxl345_i2c_data *obj = i2c_get_clientdata(client);
    int res = 0;

    res = ADXL345_CheckDeviceID(client); 
    if (res != ADXL345_SUCCESS)
        return res;
    
    res = ADXL345_SetPowerMode(client, ADXL345_MEASURE_MODE);
    if (res != ADXL345_SUCCESS)
        return res;
        
    res = ADXL345_SetBWRate(client, ADXL345_BW_100HZ);
    if (res != ADXL345_SUCCESS ) //0x2C->BW=100Hz
        return res;

    res = ADXL345_SetDataFormat(client, ADXL345_FULL_RES|ADXL345_RANGE_2G);
    if (res != ADXL345_SUCCESS) //0x2C->BW=100Hz
        return res;
    
    res = ADXL345_SetIntEnable(client, ADXL345_DATA_READY);        
    if (res != ADXL345_SUCCESS)//0x2E->0x80
        return res;

    if (reset_cali) { /*reset calibration only in power on*/
        res = ADXL345_ResetCalibration(client);
        if (res != ADXL345_SUCCESS)
            return res;
    }

#if defined(CONFIG_ADXL345_LOWPASS)
    memset(&obj->fir, 0x00, sizeof(obj->fir));  
#endif 
    return ADXL345_SUCCESS;
}
/*----------------------------------------------------------------------------*/
static int ADXL345_ReadChipInfo(struct i2c_client *client, char *buf, int bufsize)
{
    u8 databuf[10];    
    
    memset(databuf, 0, sizeof(u8)*10);
        
    if ((!buf)||(bufsize<=30))
        return -1;
    if (!client)
    {
        *buf = 0;
        return -2;
    }
    
    sprintf(buf, "ADXL345 Chip");
    return 0;
}
/*----------------------------------------------------------------------------*/
static int ADXL345_ReadSensorData(struct i2c_client *client, char *buf, int bufsize)
{
    struct adxl345_i2c_data *obj = (struct adxl345_i2c_data*)i2c_get_clientdata(client);
    u8 databuf[20];
    int acc[ADXL345_AXES_NUM];
    int res = 0;
    memset(databuf, 0, sizeof(u8)*10);
        
    if ((!buf)||(bufsize<=80))
        return -1;
    if (!client)
    {
        *buf = 0;
        return -2;
    }
    
    if ((res = ADXL345_ReadData(client, obj->data))) {        
        ADX_ERR("I2C error: ret value=%d", res);
        return -3;
    } else {
        /*remap coordinate*/
        acc[obj->cvt.map[ADXL345_AXIS_X]] = obj->cvt.sign[ADXL345_AXIS_X]*obj->data[ADXL345_AXIS_X];
        acc[obj->cvt.map[ADXL345_AXIS_Y]] = obj->cvt.sign[ADXL345_AXIS_Y]*obj->data[ADXL345_AXIS_Y];
        acc[obj->cvt.map[ADXL345_AXIS_Z]] = obj->cvt.sign[ADXL345_AXIS_Z]*obj->data[ADXL345_AXIS_Z];
        
        sprintf(buf, "%04x %04x %04x", acc[ADXL345_AXIS_X], acc[ADXL345_AXIS_Y], acc[ADXL345_AXIS_Z]);
        if (atomic_read(&obj->trace) & ADX_TRC_IOCTL)
            ADX_LOG("%s\n", buf);
    }
    return 0;
}
/*----------------------------------------------------------------------------*/
static int ADXL345_ReadRawData(struct i2c_client *client, struct hwmsen_data *buf)
{
    struct adxl345_i2c_data *obj = (struct adxl345_i2c_data*)i2c_get_clientdata(client);
    int res = 0;
        
    if (!buf || !client)
        return EINVAL;
    
    if ((res = ADXL345_ReadData(client, obj->data))) {        
        ADX_ERR("I2C error: ret value=%d", res);
        return EIO;
    } else {
        buf->raw[HWM_EVT_ACC_X] = obj->data[ADXL345_AXIS_X];
        buf->raw[HWM_EVT_ACC_Y] = obj->data[ADXL345_AXIS_Y];
        buf->raw[HWM_EVT_ACC_Z] = obj->data[ADXL345_AXIS_Z];  
        buf->num = 3;
    }
    return 0;
}
/*----------------------------------------------------------------------------*/
static int ADXL345_InitSelfTest(struct i2c_client *client)
{
    int res = 0;
    u8  data;
        
    res = ADXL345_SetBWRate(client, ADXL345_BW_100HZ);
    if (res != ADXL345_SUCCESS ) //0x2C->BW=100Hz
        return res;

    res = hwmsen_read_byte(client, ADXL345_REG_DATA_FORMAT, &data);
    if (res != ADXL345_SUCCESS)
        return res;
    
    res = ADXL345_SetDataFormat(client, ADXL345_SELF_TEST|data);
    if (res != ADXL345_SUCCESS) //0x2C->BW=100Hz
        return res;
    
    return ADXL345_SUCCESS;
}
/*----------------------------------------------------------------------------*/
static int ADXL345_JudgeTestResult(struct i2c_client *client, s32 prv[ADXL345_AXES_NUM], s32 nxt[ADXL345_AXES_NUM])
{
    struct criteria {
        int min;
        int max;
    };
    struct criteria self[4][3] = {
        {{50, 540}, {-540, -50}, {75, 875}},
        {{25, 270}, {-270, -25}, {38, 438}},
        {{12, 135}, {-135, -12}, {19, 219}},            
        {{ 6,  67}, {-67,  -6},  {10, 110}},            
    };
    struct criteria (*ptr)[3] = NULL;
    u8 format;
    int res;
    if ((res = hwmsen_read_byte(client, ADXL345_REG_DATA_FORMAT, &format)))
        return res;
    if ((format & ADXL345_FULL_RES))
        ptr = &self[0];
    else if ((format & ADXL345_RANGE_4G))
        ptr = &self[1];
    else if ((format & ADXL345_RANGE_8G))
        ptr = &self[2];
    else if ((format & ADXL345_RANGE_16G))
        ptr = &self[3];

    if (!ptr) {
        ADX_ERR("null pointer\n");
        return -EINVAL;
    }

    if (((nxt[ADXL345_AXIS_X] - prv[ADXL345_AXIS_X]) > (*ptr)[ADXL345_AXIS_X].max) ||
        ((nxt[ADXL345_AXIS_X] - prv[ADXL345_AXIS_X]) < (*ptr)[ADXL345_AXIS_X].min)) {
        ADX_ERR("X is over range\n");
        res = -EINVAL;
    }
    if (((nxt[ADXL345_AXIS_Y] - prv[ADXL345_AXIS_Y]) > (*ptr)[ADXL345_AXIS_Y].max) ||
        ((nxt[ADXL345_AXIS_Y] - prv[ADXL345_AXIS_Y]) < (*ptr)[ADXL345_AXIS_Y].min)) {
        ADX_ERR("Y is over range\n");
        res = -EINVAL;
    }
    if (((nxt[ADXL345_AXIS_Z] - prv[ADXL345_AXIS_Z]) > (*ptr)[ADXL345_AXIS_Z].max) ||
        ((nxt[ADXL345_AXIS_Z] - prv[ADXL345_AXIS_Z]) < (*ptr)[ADXL345_AXIS_Z].min)) {
        ADX_ERR("Z is over range\n");
        res = -EINVAL;
    }
    return res;
}
/*----------------------------------------------------------------------------*/
static ssize_t show_chipinfo_value(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    char strbuf[ADXL345_BUFSIZE];
    ADXL345_ReadChipInfo(client, strbuf, ADXL345_BUFSIZE);
    return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);        
}
/*----------------------------------------------------------------------------*/
static ssize_t show_sensordata_value(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    char strbuf[ADXL345_BUFSIZE];
    ADXL345_ReadSensorData(client, strbuf, ADXL345_BUFSIZE);
    return snprintf(buf, PAGE_SIZE, "%s\n", strbuf);            
}
/*----------------------------------------------------------------------------*/
static ssize_t show_cali_value(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct adxl345_i2c_data *obj = i2c_get_clientdata(client);
    int err, len = 0, mul;
    int tmp[ADXL345_AXES_NUM];

    if ((err = ADXL345_ReadOffset(client, obj->offset))) {
        return -EINVAL;
    } else if ((err = ADXL345_ReadCalibration(client, tmp))) {
        return -EINVAL;
    } else {    
        mul = obj->reso->sensitivity/adxl345_offset_resolution.sensitivity;
        len += snprintf(buf+len, PAGE_SIZE-len, "[HW ][%d] (%+3d, %+3d, %+3d) : (0x%02X, 0x%02X, 0x%02X)\n", mul,                        
                        obj->offset[ADXL345_AXIS_X], obj->offset[ADXL345_AXIS_Y], obj->offset[ADXL345_AXIS_Z],
                        obj->offset[ADXL345_AXIS_X], obj->offset[ADXL345_AXIS_Y], obj->offset[ADXL345_AXIS_Z]);
        len += snprintf(buf+len, PAGE_SIZE-len, "[SW ][%d] (%+3d, %+3d, %+3d)\n", 1, 
                        obj->cali_sw[ADXL345_AXIS_X], obj->cali_sw[ADXL345_AXIS_Y], obj->cali_sw[ADXL345_AXIS_Z]);                        

        len += snprintf(buf+len, PAGE_SIZE-len, "[ALL]    (%+3d, %+3d, %+3d) : (%+3d, %+3d, %+3d)\n", 
                        obj->offset[ADXL345_AXIS_X]*mul + obj->cali_sw[ADXL345_AXIS_X],
                        obj->offset[ADXL345_AXIS_Y]*mul + obj->cali_sw[ADXL345_AXIS_Y],
                        obj->offset[ADXL345_AXIS_Z]*mul + obj->cali_sw[ADXL345_AXIS_Z],
                        tmp[ADXL345_AXIS_X], tmp[ADXL345_AXIS_Y], tmp[ADXL345_AXIS_Z]);                        
        return len;
    }
}
/*----------------------------------------------------------------------------*/
static ssize_t store_cali_value(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);  
    int err, x, y, z;
    int dat[ADXL345_AXES_NUM];
    
    if (!strncmp(buf, "rst", 3)) {
        if ((err = ADXL345_ResetCalibration(client))) 
            ADX_ERR("reset offset err = %d\n", err);
    } else if (3 == sscanf(buf, "0x%02X 0x%02X 0x%02X", &x, &y, &z)) {
        dat[ADXL345_AXIS_X] = x;
        dat[ADXL345_AXIS_Y] = y;
        dat[ADXL345_AXIS_Z] = z;
        if ((err = ADXL345_WriteCalibration(client, dat)))
            ADX_ERR("write calibration err = %d\n", err);        
    } else {
        ADX_ERR("invalid format\n");
    }
    return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t show_self_value(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct adxl345_i2c_data *obj = i2c_get_clientdata(client);

    return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&obj->selftest));
}
/*----------------------------------------------------------------------------*/
static ssize_t store_self_value(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{   /*write anything to this register will trigger the process*/
    struct item{
        s16 raw[ADXL345_AXES_NUM];
    };
    struct i2c_client *client = to_i2c_client(dev);  
    int idx, res, num;
    struct item *prv = NULL, *nxt = NULL;
    s32 avg_prv[ADXL345_AXES_NUM] = {0, 0, 0};
    s32 avg_nxt[ADXL345_AXES_NUM] = {0, 0, 0};


    if (1 != sscanf(buf, "%d", &num)) {
        ADX_ERR("parse number fail\n");
        return count;
    } else if (num == 0) {
        ADX_ERR("invalid data count\n");
        return count;
    }

    prv = kzalloc(sizeof(*prv) * num, GFP_KERNEL);
    nxt = kzalloc(sizeof(*nxt) * num, GFP_KERNEL);
    if (!prv || !nxt)
        goto exit;

    ADX_LOG("NORMAL:\n");
    for (idx = 0; idx < num; idx++) {
        if ((res = ADXL345_ReadData(client, prv[idx].raw))) {            
            ADX_ERR("read data fail: %d\n", res);
            goto exit;
        }
        avg_prv[ADXL345_AXIS_X] += prv[idx].raw[ADXL345_AXIS_X];
        avg_prv[ADXL345_AXIS_Y] += prv[idx].raw[ADXL345_AXIS_Y];
        avg_prv[ADXL345_AXIS_Z] += prv[idx].raw[ADXL345_AXIS_Z];        
        ADX_LOG("[%5d %5d %5d]\n", prv[idx].raw[ADXL345_AXIS_X], prv[idx].raw[ADXL345_AXIS_Y], prv[idx].raw[ADXL345_AXIS_Z]);
    }
    avg_prv[ADXL345_AXIS_X] /= num;
    avg_prv[ADXL345_AXIS_Y] /= num;
    avg_prv[ADXL345_AXIS_Z] /= num;    
    
    /*initial setting for self test*/
    ADXL345_InitSelfTest(client);
    ADX_LOG("SELFTEST:\n");    
    for (idx = 0; idx < num; idx++) {
        if ((res = ADXL345_ReadData(client, nxt[idx].raw))) {            
            ADX_ERR("read data fail: %d\n", res);
            goto exit;
        }
        avg_nxt[ADXL345_AXIS_X] += nxt[idx].raw[ADXL345_AXIS_X];
        avg_nxt[ADXL345_AXIS_Y] += nxt[idx].raw[ADXL345_AXIS_Y];
        avg_nxt[ADXL345_AXIS_Z] += nxt[idx].raw[ADXL345_AXIS_Z];        
        ADX_LOG("[%5d %5d %5d]\n", nxt[idx].raw[ADXL345_AXIS_X], nxt[idx].raw[ADXL345_AXIS_Y], nxt[idx].raw[ADXL345_AXIS_Z]);
    }
    avg_nxt[ADXL345_AXIS_X] /= num;
    avg_nxt[ADXL345_AXIS_Y] /= num;
    avg_nxt[ADXL345_AXIS_Z] /= num;    
    
    ADX_LOG("X: %5d - %5d = %5d \n", avg_nxt[ADXL345_AXIS_X], avg_prv[ADXL345_AXIS_X], avg_nxt[ADXL345_AXIS_X] - avg_prv[ADXL345_AXIS_X]);
    ADX_LOG("Y: %5d - %5d = %5d \n", avg_nxt[ADXL345_AXIS_Y], avg_prv[ADXL345_AXIS_Y], avg_nxt[ADXL345_AXIS_Y] - avg_prv[ADXL345_AXIS_Y]);
    ADX_LOG("Z: %5d - %5d = %5d \n", avg_nxt[ADXL345_AXIS_Z], avg_prv[ADXL345_AXIS_Z], avg_nxt[ADXL345_AXIS_Z] - avg_prv[ADXL345_AXIS_Z]); 

    if (!ADXL345_JudgeTestResult(client, avg_prv, avg_nxt))
        ADX_LOG("SELFTEST : PASS\n");
    else
        ADX_LOG("SELFTEST : FAIL\n");
exit:
    /*restore the setting*/    
    ADXL345_Init(client, 0);
    kfree(prv);
    kfree(nxt);
    return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t show_selftest_value(struct device* dev, 
                             struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct adxl345_i2c_data *obj = i2c_get_clientdata(client);

    return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&obj->selftest));
}
/*----------------------------------------------------------------------------*/
static ssize_t store_selftest_value(struct device* dev, struct device_attribute *attr,
                              const char *buf, size_t count)
{
    struct adxl345_i2c_data *obj;
    int tmp;
    
    if (!dev) {
        ADX_ERR("dev is null!!\n");
        return 0;
    } else if (!(obj = (struct adxl345_i2c_data*)dev_get_drvdata(dev))) {
        ADX_ERR("drv data is null!!\n");
        return 0;
    }
    if (1 == sscanf(buf, "%d", &tmp)) {        
        if (atomic_read(&obj->selftest) && !tmp) {
            /*enable -> disable*/
            ADXL345_Init(&obj->client, 0);
        } else if (!atomic_read(&obj->selftest) && tmp) {
            /*disable -> enable*/
            ADXL345_InitSelfTest(&obj->client);            
        }
        ADX_LOG("selftest: %d => %d\n", atomic_read(&obj->selftest), tmp);
        atomic_set(&obj->selftest, tmp); 
    } else { 
        ADX_ERR("invalid content: '%s', length = %d\n", buf, count);   
    }
    return count;    
}
/*----------------------------------------------------------------------------*/
static ssize_t show_firlen_value(struct device *dev, struct device_attribute *attr, char *buf)
{
#if defined(CONFIG_ADXL345_LOWPASS)
    struct i2c_client *client = to_i2c_client(dev);
    struct adxl345_i2c_data *obj = i2c_get_clientdata(client);
    if (atomic_read(&obj->firlen)) {
        int idx, len = atomic_read(&obj->firlen);
        ADX_LOG("len = %2d, idx = %2d\n", obj->fir.num, obj->fir.idx);
        for (idx = 0; idx < len; idx++)
            ADX_LOG("[%5d %5d %5d]\n", obj->fir.raw[idx][ADXL345_AXIS_X], obj->fir.raw[idx][ADXL345_AXIS_Y], obj->fir.raw[idx][ADXL345_AXIS_Z]);
        ADX_LOG("sum = [%5d %5d %5d]\n", obj->fir.sum[ADXL345_AXIS_X], obj->fir.sum[ADXL345_AXIS_Y], obj->fir.sum[ADXL345_AXIS_Z]);
        ADX_LOG("avg = [%5d %5d %5d]\n", obj->fir.sum[ADXL345_AXIS_X]/len, obj->fir.sum[ADXL345_AXIS_Y]/len, obj->fir.sum[ADXL345_AXIS_Z]/len);
    }
    return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&obj->firlen));
#else
    return snprintf(buf, PAGE_SIZE, "not support\n");
#endif
}
/*----------------------------------------------------------------------------*/
static ssize_t store_firlen_value(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
#if defined(CONFIG_ADXL345_LOWPASS)
    struct i2c_client *client = to_i2c_client(dev);  
    struct adxl345_i2c_data *obj = i2c_get_clientdata(client);
    int firlen;
    
    if (1 != sscanf(buf, "%d", &firlen)) {
        ADX_ERR("invallid format\n");
    } else if (firlen > C_MAX_FIR_LENGTH) {
        ADX_ERR("exceeds maximum filter length\n");
    } else { 
        atomic_set(&obj->firlen, firlen);
        if (!firlen) {
            atomic_set(&obj->fir_en, 0);
        } else {
            memset(&obj->fir, 0x00, sizeof(obj->fir));
            atomic_set(&obj->fir_en, 1);
        }
    }
#endif    
    return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t show_trace_value(struct device* dev, 
                             struct device_attribute *attr, char *buf)
{
    ssize_t res;
    struct adxl345_i2c_data *obj;
    if (!dev) {
        ADX_ERR("dev is null!!\n");
        return 0;
    } else if (!(obj = (struct adxl345_i2c_data*)dev_get_drvdata(dev))) {
        ADX_ERR("drv data is null!!\n");
        return 0;
    }
    res = snprintf(buf, PAGE_SIZE, "0x%04X\n", atomic_read(&obj->trace));     
    return res;    
}
/*----------------------------------------------------------------------------*/
static ssize_t store_trace_value(struct device* dev, struct device_attribute *attr,
                              const char *buf, size_t count)
{
    struct adxl345_i2c_data *obj;
    int trace;
    if (!dev) {
        ADX_ERR("dev is null!!\n");
        return 0;
    } else if (!(obj = (struct adxl345_i2c_data*)dev_get_drvdata(dev))) {
        ADX_ERR("drv data is null!!\n");
        return 0;
    }
    if (1 == sscanf(buf, "0x%x", &trace)) 
        atomic_set(&obj->trace, trace);
    else 
        ADX_ERR("invalid content: '%s', length = %d\n", buf, count);
    return count;    
}
/*----------------------------------------------------------------------------*/
static ssize_t show_status_value(struct device* dev, 
                             struct device_attribute *attr, char *buf)
{
    ssize_t len = 0;    
    struct adxl345_i2c_data *obj;
    if (!dev) {
        ADX_ERR("dev is null!!\n");
        return 0;
    } else if (!(obj = (struct adxl345_i2c_data*)dev_get_drvdata(dev))) {
        ADX_ERR("drv data is null!!\n");
        return 0;
    }
    if (obj->hw) {
        len += snprintf(buf+len, PAGE_SIZE-len, "CUST: %d %d (%d %d)\n", 
                        obj->hw->i2c_num, obj->hw->direction, obj->hw->power_id, obj->hw->power_vol);   
    } else {
        len += snprintf(buf+len, PAGE_SIZE-len, "CUST: NULL\n");
    }
    return len;    
}
/*----------------------------------------------------------------------------*/
static DEVICE_ATTR(chipinfo,             S_IRUGO, show_chipinfo_value,      NULL);
static DEVICE_ATTR(sensordata,           S_IRUGO, show_sensordata_value,    NULL);
static DEVICE_ATTR(cali,       S_IWUSR | S_IRUGO, show_cali_value,          store_cali_value);
static DEVICE_ATTR(self,       S_IWUSR | S_IRUGO, show_self_value,          store_self_value);
static DEVICE_ATTR(selftest,   S_IWUSR | S_IRUGO, show_selftest_value,      store_selftest_value);
static DEVICE_ATTR(firlen,     S_IWUSR | S_IRUGO, show_firlen_value,        store_firlen_value);
static DEVICE_ATTR(trace,      S_IWUSR | S_IRUGO, show_trace_value,         store_trace_value);
static DEVICE_ATTR(status,               S_IRUGO, show_status_value,        NULL);
/*----------------------------------------------------------------------------*/
static struct device_attribute *adxl345_attr_list[] = {
    &dev_attr_chipinfo,     /*chip information*/
    &dev_attr_sensordata,   /*dump sensor data*/
    &dev_attr_cali,         /*show calibration data*/
    &dev_attr_self,         /*self test demo*/
    &dev_attr_selftest,     /*self control: 0: disable, 1: enable*/
    &dev_attr_firlen,       /*filter length: 0: disable, others: enable*/
    &dev_attr_trace,        /*trace log*/
    &dev_attr_status,        
};
/*----------------------------------------------------------------------------*/
static int adxl345_create_attr(struct device *dev) 
{
    int idx, err = 0;
    int num = (int)(sizeof(adxl345_attr_list)/sizeof(adxl345_attr_list[0]));
    if (!dev)
        return -EINVAL;

    for (idx = 0; idx < num; idx++) {
        if ((err = device_create_file(dev, adxl345_attr_list[idx]))) {            
            ADX_ERR("device_create_file (%s) = %d\n", adxl345_attr_list[idx]->attr.name, err);        
            break;
        }
    }    
    return err;
}
/*----------------------------------------------------------------------------*/
static int adxl345_delete_attr(struct device *dev)
{
    int idx ,err = 0;
    int num = (int)(sizeof(adxl345_attr_list)/sizeof(adxl345_attr_list[0]));
    
    if (!dev)
        return -EINVAL;

    for (idx = 0; idx < num; idx++) 
        device_remove_file(dev, adxl345_attr_list[idx]);

    return err;
}
static int adxl345_open(struct inode *inode, struct file *file)
{
    file->private_data = adxl345_i2c_client;
    
    if (!file->private_data) {
        ADX_ERR("null pointer!!\n");
        return -EINVAL;
    }
    return nonseekable_open(inode, file);
}
/*----------------------------------------------------------------------------*/
static int adxl345_release(struct inode *inode, struct file *file)
{
    file->private_data = NULL;
    return 0;
}
/*----------------------------------------------------------------------------*/
static int adxl345_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
       unsigned long arg)
{
    struct i2c_client *client = (struct i2c_client*)file->private_data;
    struct adxl345_i2c_data *obj = (struct adxl345_i2c_data*)i2c_get_clientdata(client);    
    char strbuf[ADXL345_BUFSIZE];
    void __user *data;
    int err = 0;
    struct hwmsen_data dat;
    int cali[3];

	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	if (err) {
        ADX_ERR("access error: %08X, (%2d, %2d)\n", cmd, _IOC_DIR(cmd), _IOC_SIZE(cmd));
        return -EFAULT;
	}
    
    switch (cmd) {
        case ADXL345_IOCTL_INIT:
            ADXL345_Init(client, 0);            
            break;

        case ADXL345_IOCTL_READ_CHIPINFO:
            data = (void __user *) arg;
            if (data == NULL) {
                err = -EINVAL;
                break;    
            }
            ADXL345_ReadChipInfo(client, strbuf, ADXL345_BUFSIZE);
            if (copy_to_user(data, strbuf, strlen(strbuf)+1)) {
                err = -EFAULT;
                break;
            }                
            break;    
            
        case ADXL345_IOCTL_READ_SENSORDATA:
            data = (void __user *) arg;
            if (data == NULL) {
                err = -EINVAL;
                break;    
            }
            ADXL345_ReadSensorData(client, strbuf, ADXL345_BUFSIZE);
            if (copy_to_user(data, strbuf, strlen(strbuf)+1)) {
                err = -EFAULT;
                break;    
            }                
            break;    
            
        case HWM_IOCG_ACC_RAW:
            data = (void __user *) arg;
            if (data == NULL) {
                err = -EINVAL;
                break;    
            }
            ADXL345_ReadRawData(client, &dat);
            if (copy_to_user(data, &dat, sizeof(dat))) {
                err = -EFAULT;
                break;    
            }                
            break;    
        
        case HWM_IOCS_ACC_CALI:
            data = (void __user*)arg;
            if (data == NULL) {
                err = -EINVAL;
                break;    
            }
            if (copy_from_user(&dat, data, sizeof(dat))) {
                err = -EFAULT;
                break;    
            }
            if (atomic_read(&obj->suspend)) {
                ADX_ERR("Perform calibration in suspend state!!\n");
                err = -EINVAL;
            } else {
                cali[ADXL345_AXIS_X] = dat.raw[HWM_EVT_ACC_X];
                cali[ADXL345_AXIS_Y] = dat.raw[HWM_EVT_ACC_Y];
                cali[ADXL345_AXIS_Z] = dat.raw[HWM_EVT_ACC_Z];            
                err = ADXL345_WriteCalibration(client, cali);            
            }
            break;

        case HWM_IOCS_ACC_CALI_CLR:
            err = ADXL345_ResetCalibration(client);
            break;
    
        case HWM_IOCG_ACC_CALI:
            data = (void __user*)arg;
            if (data == NULL) {
                err = -EINVAL;
                break;    
            }
            if ((err = ADXL345_ReadCalibration(client, cali)))
                break;
            dat.raw[HWM_EVT_ACC_X] = cali[ADXL345_AXIS_X];
            dat.raw[HWM_EVT_ACC_Y] = cali[ADXL345_AXIS_Y];
            dat.raw[HWM_EVT_ACC_Z] = cali[ADXL345_AXIS_Z];
            if (copy_to_user(data, &dat, sizeof(dat))) {
                err = -EFAULT;
                break;
            }       
            break;                            
            
        default:
            ADX_ERR("unknown IOCTL: 0x%08x\n", cmd);
            err = -ENOIOCTLCMD;
            break;
    }
    
    return err;    
}
/*----------------------------------------------------------------------------*/
static struct file_operations adxl345_fops = {
    .owner = THIS_MODULE,
    .open = adxl345_open,
    .release = adxl345_release,
    .ioctl = adxl345_ioctl,
};
/*----------------------------------------------------------------------------*/
static struct miscdevice adxl345_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "adxl345",
    .fops = &adxl345_fops,
};
/*----------------------------------------------------------------------------*/
#if !defined(CONFIG_HAS_EARLYSUSPEND)
/*----------------------------------------------------------------------------*/
static int adxl345_suspend(struct i2c_client *client, pm_message_t msg) 
{
    struct adxl345_i2c_data *obj = i2c_get_clientdata(client);    
    int err;
    ADX_FUN();    

    if (msg.event == PM_EVENT_SUSPEND) {   
        if (!obj) {
            ADX_ERR("null pointer!!\n");
            return -EINVAL;
        }
        atomic_set(&obj->suspend, 1);
        if ((err = hwmsen_write_byte(client, ADXL345_REG_POWER_CTL, 0x00))) {
            ADX_ERR("write power control fail!!\n");
            return err;
        }        
        ADXL345_power(obj->hw, 0);
    }
    return 0;
}
/*----------------------------------------------------------------------------*/
static int adxl345_resume(struct i2c_client *client)
{
    struct adxl345_i2c_data *obj = i2c_get_clientdata(client);        
    int err;
    ADX_FUN();

    if (!obj) {
        ADX_ERR("null pointer!!\n");
        return -EINVAL;
    }
    
    ADXL345_power(obj->hw, 1);
    if ((err = ADXL345_Init(client, 0))) {
        ADX_ERR("initialize client fail!!\n");
        return err;        
    }
    atomic_set(&obj->suspend, 0);

    return 0;
}
/*----------------------------------------------------------------------------*/
#else /*CONFIG_HAS_EARLY_SUSPEND is defined*/
/*----------------------------------------------------------------------------*/
static void adxl345_early_suspend(struct early_suspend *h) 
{
    struct adxl345_i2c_data *obj = container_of(h, struct adxl345_i2c_data, early_drv);   
    int err;
    ADX_FUN();    

    if (!obj) {
        ADX_ERR("null pointer!!\n");
        return;
    }
    atomic_set(&obj->suspend, 1);    
    if ((err = hwmsen_write_byte(&obj->client, ADXL345_REG_POWER_CTL, 0x00))) {
        ADX_ERR("write power control fail!!\n");
        return;
    }        
    ADXL345_power(obj->hw, 0);
}
/*----------------------------------------------------------------------------*/
static void adxl345_late_resume(struct early_suspend *h)
{
    struct adxl345_i2c_data *obj = container_of(h, struct adxl345_i2c_data, early_drv);         
    int err;
    ADX_FUN();

    if (!obj) {
        ADX_ERR("null pointer!!\n");
        return;
    }
    
    ADXL345_power(obj->hw, 1);
    if ((err = ADXL345_Init(&obj->client, 0))) {
        ADX_ERR("initialize client fail!!\n");
        return;        
    }
    atomic_set(&obj->suspend, 0);    
}
/*----------------------------------------------------------------------------*/
#endif /*CONFIG_HAS_EARLYSUSPEND*/
/*----------------------------------------------------------------------------*/
static int adxl345_i2c_attach_adapter(struct i2c_adapter *adapter)
{   
    struct acc_hw *hw = get_cust_acc_hw();
    if (adapter->id == hw->i2c_num)         
        return i2c_probe(adapter, &addr_data, adxl345_i2c_detect);
   
    return -1;
}
/*----------------------------------------------------------------------------*/
static int adxl345_i2c_detect(struct i2c_adapter *adapter, int address, int kind)
{
    struct i2c_client *new_client;
    struct adxl345_i2c_data *obj;
    int err = 0;

    if (!(obj = kzalloc(sizeof(*obj), GFP_KERNEL))) {
        err = -ENOMEM;
        goto exit;
    }
    memset(obj, 0, sizeof(*obj));

    obj->hw = get_cust_acc_hw();
    if ((err = hwmsen_get_convert(obj->hw->direction, &obj->cvt))) {
        ADX_ERR("invalid direction: %d\n", obj->hw->direction);
        goto exit;
    }
     
    new_client = &obj->client;
    i2c_set_clientdata(new_client,obj);
    new_client->addr = address;
    new_client->adapter = adapter;
    new_client->driver = &adxl345_i2c_driver;
    new_client->flags = 0;
    atomic_set(&obj->trace, 0);
    atomic_set(&obj->suspend, 0);
#if defined(CONFIG_ADXL345_LOWPASS)
    if (obj->hw->firlen > C_MAX_FIR_LENGTH)
        atomic_set(&obj->firlen, C_MAX_FIR_LENGTH);
    else
        atomic_set(&obj->firlen, obj->hw->firlen);
    if (atomic_read(&obj->firlen) > 0)
        atomic_set(&obj->fir_en, 1);
#endif 

    strlcpy(new_client->name, "adxl345_i2c", I2C_NAME_SIZE);
    adxl345_i2c_client = new_client;
        
    if ((err = i2c_attach_client(new_client)))
        goto exit_kfree;

    if ((err = ADXL345_Init(new_client, 1))) 
        goto exit_init_failed;

    if ((err = misc_register(&adxl345_device))) {
        ADX_ERR("adxl345_device register failed\n");
        goto exit_misc_device_register_failed;
    }

    if ((err = adxl345_create_attr(&new_client->dev))) {
        ADX_ERR("create attribute err = %d\n", err);
        goto exit_create_attr_failed;
    }


#if defined(CONFIG_HAS_EARLYSUSPEND)
    obj->early_drv.level    = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1,
    obj->early_drv.suspend  = adxl345_early_suspend,
    obj->early_drv.resume   = adxl345_late_resume,    
    register_early_suspend(&obj->early_drv);
#endif 

    ADX_LOG("%s: OK\n", __func__);    
    return 0;

exit_create_attr_failed:
    misc_deregister(&adxl345_device);
exit_misc_device_register_failed:
exit_init_failed:
    i2c_detach_client(new_client);
exit_kfree:
    kfree(obj);
exit:
    ADX_ERR("%s: err = %d\n", __func__, err);        
    return err;
}
/*----------------------------------------------------------------------------*/
static int adxl345_i2c_detach_client(struct i2c_client *client)
{
    int err;

    if ((err = i2c_detach_client(client))) 
        ADX_ERR("i2c_detach_client:%d\n", err);
    if ((err = adxl345_delete_attr(&client->dev))) 
        ADX_ERR("adxl345_delete_attr fail: %d\n", err);
    if ((err = misc_deregister(&adxl345_device)))
        ADX_ERR("misc_deregister fail: %d\n", err);    

    adxl345_i2c_client = NULL;
    kfree(i2c_get_clientdata(client));
    return 0;
}
/*----------------------------------------------------------------------------*/
static int adxl345_probe(struct platform_device *pdev) 
{
    struct acc_hw *hw = get_cust_acc_hw();

    ADXL345_power(hw, 1);    
    if (i2c_add_driver(&adxl345_i2c_driver)) {
        ADX_ERR("add driver error\n");
        return -1;
    } 
    return 0;
}
/*----------------------------------------------------------------------------*/
static int adxl345_remove(struct platform_device *pdev)
{
    struct acc_hw *hw = get_cust_acc_hw();

    ADX_FUN();    
    ADXL345_power(hw, 0);    
    i2c_del_driver(&adxl345_i2c_driver);
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
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("Kyle K.Y. Chen");
MODULE_DESCRIPTION("ADXL345 accelerometer driver");
MODULE_LICENSE("GPL");
