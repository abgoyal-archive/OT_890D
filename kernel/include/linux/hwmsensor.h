

#ifndef __HWMSENSOR_H__
#define __HWMSENSOR_H__

#include <linux/ioctl.h>
/*----------------------------------------------------------------------------*/
#define HWM_INPUTDEV_NAME               "hwmdata"
#define HWM_SENSOR_DEV_NAME             "hwmsensor"
#define HWM_SENSOR_DEV                  "/dev/hwmsensor"
#define C_MAX_HWMSEN_EVENT_NUM          4 
/*----------------------------------------------------------------------------*/
enum {
    HWM_MODE_DISABLE = 0,
    HWM_MODE_ENABLE  = 1,    
};
/*----------------------------------------------------------------------------*/
typedef enum _HWM_SENSOR {

    HWM_SENSOR_ACCELERATION = 0,
    HWM_SENSOR_MAGNETIC_FIELD,
    HWM_SENSOR_ORIENTATION,
    HWM_SENSOR_GYROSCOPE,
    HWM_SENSOR_LIGHT,
    HWM_SENSOR_PRESSURE,
    HWM_SENSOR_TEMPERATURE,
    HWM_SENSOR_PROXIMITY,

    HWM_SENSOR_MAX,
    C_MAX_HWMSEN_NUM = HWM_SENSOR_MAX,
}HWM_SENSOR;
/*----------------------------------------------------------------------------*/
#define HWM_ID_ACC  (1 << HWM_SENSOR_ACCELERATION)
#define HWM_ID_MAG  (1 << HWM_SENSOR_MAGNETIC_FIELD)
#define HWM_ID_ORI  (1 << HWM_SENSOR_ORIENTATION)
#define HWM_ID_GYR  (1 << HWM_SENSOR_GYROSCOPE)
#define HWM_ID_LIG  (1 << HWM_SENSOR_LIGHT)
#define HWM_ID_PRE  (1 << HWM_SENSOR_PRESSURE)
#define HWM_ID_TEM  (1 << HWM_SENSOR_TEMPERATURE)
#define HWM_ID_PRO  (1 << HWM_SENSOR_PROXIMITY)
/*----------------------------------------------------------------------------*/
struct hwmsen_event {
    uint32_t code;
    int min;
    int max;
    uint32_t sensitivity; 
};
/*----------------------------------------------------------------------------*/
struct hwmsen_info {
    int num;
    struct hwmsen_event evt[C_MAX_HWMSEN_EVENT_NUM];
};
/*----------------------------------------------------------------------------*/
struct hwmsen_data {
    int   raw[C_MAX_HWMSEN_EVENT_NUM];  
    int   num;
};
/*----------------------------------------------------------------------------*/
struct hwmsen_spec {
    struct hwmsen_info info[C_MAX_HWMSEN_NUM];
    uint32_t avail;  /*bit mask of HWM_ID_XXX*/
};
/*----------------------------------------------------------------------------*/
/* Sensor event index                                                         */
/*----------------------------------------------------------------------------*/
/*ACCELEROMETER*/
#define HWM_EVT_ACC_X          0
#define HWM_EVT_ACC_Y          1
#define HWM_EVT_ACC_Z          2
/*MAGNETIC_FIELD*/
#define HWM_EVT_MAG_X          0
#define HWM_EVT_MAG_Y          1
#define HWM_EVT_MAG_Z          2
/*ORIENTATION*/
#define HWM_EVT_ORI_AZIMUTH    0
#define HWM_EVT_ORI_PITCH      1
#define HWM_EVT_ORI_ROLL       2
/*LIGHT*/
#define HWM_EVT_LIGHT          0
/*PROXIMITY*/
#define HWM_EVT_DISTANCE       0
/*----------------------------------------------------------------------------*/
#define HWM_IOC_MAGIC           0x91
/*ACCELEROMETER*/
#define HWM_IOCG_ACC_MODE       _IOR(HWM_IOC_MAGIC, 0x01, uint32_t)  
#define HWM_IOCS_ACC_MODE       _IOW(HWM_IOC_MAGIC, 0x02, uint32_t)  
/*MAGNETIC_FIELD*/
#define HWM_IOCG_MAG_MODE       _IOR(HWM_IOC_MAGIC, 0x03, uint32_t)  
#define HWM_IOCS_MAG_MODE       _IOW(HWM_IOC_MAGIC, 0x04, uint32_t)
/*ORIENTATION*/
#define HWM_IOCG_ORI_MODE       _IOR(HWM_IOC_MAGIC, 0x05, uint32_t)
#define HWM_IOCS_ORI_MODE       _IOW(HWM_IOC_MAGIC, 0x06, uint32_t)
/*GYROSCOPE*/
#define HWM_IOCG_GYR_MODE       _IOR(HWM_IOC_MAGIC, 0x07, uint32_t)
#define HWM_IOCS_GYR_MODE       _IOW(HWM_IOC_MAGIC, 0x08, uint32_t)
/*LIGHT*/
#define HWM_IOCG_LIG_MODE       _IOR(HWM_IOC_MAGIC, 0x09, uint32_t)
#define HWM_IOCS_LIG_MODE       _IOW(HWM_IOC_MAGIC, 0x0A, uint32_t)
/*PRESSURE*/
#define HWM_IOCG_PRE_MODE       _IOR(HWM_IOC_MAGIC, 0x0B, uint32_t)
#define HWM_IOCS_PRE_MODE       _IOW(HWM_IOC_MAGIC, 0x0C, uint32_t)
/*TEMPERATURE*/
#define HWM_IOCG_TEM_MODE       _IOR(HWM_IOC_MAGIC, 0x0D, uint32_t)
#define HWM_IOCS_TEM_MODE       _IOW(HWM_IOC_MAGIC, 0x0E, uint32_t)
/*PROXIMITY*/
#define HWM_IOCG_PRO_MODE       _IOR(HWM_IOC_MAGIC, 0x0F, uint32_t)
#define HWM_IOCS_PRO_MODE       _IOW(HWM_IOC_MAGIC, 0x10, uint32_t)

/*set delay*/
#define HWM_IOCG_DELAY          _IOR(HWM_IOC_MAGIC, 0x11, uint32_t)
#define HWM_IOCS_DELAY          _IOW(HWM_IOC_MAGIC, 0x12, uint32_t)

/*wake up*/
#define HWM_IOCS_WAKE           _IOW(HWM_IOC_MAGIC, 0x13, uint32_t)

/*Get sensor spec*/
#define HWM_IOCG_SPEC           _IOW(HWM_IOC_MAGIC, 0x14, struct hwmsen_spec)

/*accelerometer*/
#define HWM_IOCG_ACC_CALI       _IOR(HWM_IOC_MAGIC, 0x20, struct hwmsen_data)   /*!< get calibration */
#define HWM_IOCS_ACC_CALI       _IOW(HWM_IOC_MAGIC, 0x21, struct hwmsen_data)   /*!< set calibration */
#define HWM_IOCS_ACC_CALI_CLR   _IOW(HWM_IOC_MAGIC, 0x22, uint32_t)             /*!< clear calibration */
#define HWM_IOCG_ACC_RAW        _IOR(HWM_IOC_MAGIC, 0x23, struct hwmsen_data)   /*!< get raw data*/

/*magnetometer*/
#define HWM_IOCG_MAG_RAW        _IOR(HWM_IOC_MAGIC, 0x30, struct hwmsen_data)   /*!< get raw data */

/*proximity*/
#define HWM_IOCG_PRO_DATA       _IOR(HWM_IOC_MAGIC, 0x40, struct hwmsen_data)   /*!< get converted data */
#define HWM_IOCG_PRO_RAW        _IOR(HWM_IOC_MAGIC, 0x41, struct hwmsen_data)   /*!< get raw data */

/*light*/
#define HWM_IOCG_LIG_DATA       _IOR(HWM_IOC_MAGIC, 0x50, struct hwmsen_data)   /*!< get converted data */
#define HWM_IOCG_LIG_RAW        _IOR(HWM_IOC_MAGIC, 0x51, struct hwmsen_data)   /*!< get raw data */

/*----------------------------------------------------------------------------*/

#endif // __HWMSENSOR_H__
