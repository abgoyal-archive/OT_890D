

#ifndef __INCLUDE_MT3351_BATTERY_H
#define __INCLUDE_MT3351_BATTERY_H


#include <linux/ioctl.h>
#include <mach/mt3351_typedefs.h>
#include <mach/mt3351_auxadc_hw.h>
#include <mach/mt3351_auxadc_sw.h>
#include <mach/mt3351_reg_base.h>
#include <mach/mt3351_pdn_sw.h>
#include <mach/mt3351_pmu_hw.h>
#include <mach/mt3351_pmu_sw.h>


#define BATTERY_DEVNAME				"battery"
#define BATTERY_MAJOR				241
#define PARAM_COUNT 71
#define ADC_SAMPLE_TIMES 5

typedef struct {
	UINT16 u16BatteryVoltage;		/* battery voltage */
	UINT16 u16ChargeCurrent;		/* charge current */
	UINT8 u8ChargeStatus;
	UINT8 u8VoltageSource;
} BATTERY_STATUS_IOCTL;

#define CHARGE_STATE_ACPWR_OFF		0		/* AC power is off   */
#define CHARGE_STATE_FAULT		1		/* Charge fault      */
#define CHARGE_STATE_COMPLETE		2		/* Charging complete */
#define CHARGE_STATE_FAST_CHARGING	3		/* Fast charging     */
#define CHARGE_STATE_TOPPINGUP		4		/* Topping up        */

#define VOLTAGE_SOURCE_BATTERY		0		/* Standard battery  */
#define VOLTAGE_SOURCE_BATTERY_PACK	1		/* AA batter pack    */

#define BATTERY_DRIVER_MAGIC	'B' /* Battery driver magic number */
#define IOR_BATTERY_STATUS	_IOR(BATTERY_DRIVER_MAGIC, 3, BATTERY_STATUS)
#define IO_ENABLE_CHARGING	_IO(BATTERY_DRIVER_MAGIC, 4)
#define IO_DISABLE_CHARGING	_IO(BATTERY_DRIVER_MAGIC, 5)
#define IO_DUMP_POWER	_IO(BATTERY_DRIVER_MAGIC, 6)

typedef unsigned int       WORD;

//#define BATTERY_NTC_TYPE_A         1          //TSM0A103F34D1RZ
#define BATTERY_NTC_TYPE_B         1            //ERTJ1VR103J

#define ADC_DEFAULT_SLOPE       1
#define ADC_DEFAULT_OFFSET      0
#define ADC_CALI_RANGE          30
//#define ADC_CALI_DIVISOR        10000
#define ADC_CALI_DIVISOR        1000000


#define	BAT_STATUS_OK		0
#define	BAT_STATUS_FAIL		1
#define RTC_DEBOUNCE_OK         0x80
#define ADC_SAMPLE_TIMES        5
#define MAX_CHARGE_TEMPERATURE  50
#define MIN_CHARGE_TEMPERATURE  0
#define ERR_CHARGE_TEMPERATURE  0xFF


#define PRE_CHARGE_VOLTAGE                  3200
#define SYSTEM_OFF_VOLTAGE                  3450  
#define CONSTANT_CURRENT_CHARGE_VOLTAGE     4100  
#define CONSTANT_VOLTAGE_CHARGE_VOLTAGE     4200  
#define RECHARGING_VOLTAGE                  4000  
#define CV_DROPDOWN_VOLTAGE                 4000
#define CHARGER_THRESH_HOLD                 4300
#define BATTERY_UVLO_VOLTAGE                2700

#define MAX_CHARGING_TIME                    8*60*60 // 8hr
#define MAX_PreCC_CHARGING_TIME         1*30*60  // 0.5hr
#define MAX_CV_CHARGING_TIME              3*60*60 // 3hr
#define MUTEX_TIMEOUT                            5000
#define BAT_TASK_PERIOD                         10 // 10sec
#define MAX_CV_CHARGING_CURRENT       50 //50mA

#define BatteryLevelCount       6
#define BatteryLevel_0_Percent 15
#define BatteryLevel_1_Percent 30
#define BatteryLevel_2_Percent 50
#define BatteryLevel_3_Percent 60
#define BatteryLevel_4_Percent 80
#define BatteryLevel_5_Percent 95
#define Battery_Percent_100    100

#define BatteryLevel_0_Percent_VOLTAGE 3550 
#define BatteryLevel_1_Percent_VOLTAGE 3670
#define BatteryLevel_2_Percent_VOLTAGE 3700
#define BatteryLevel_3_Percent_VOLTAGE 3750
#define BatteryLevel_4_Percent_VOLTAGE 3850
#define BatteryLevel_5_Percent_VOLTAGE 3990


#define charger_OVER_VOL	    1
#define BATTERY_UNDER_VOL		2
#define BATTERY_OVER_TEMP		3


typedef enum {
    CHARGER_UNKNOWN = 0,
    STANDARD_HOST,          // 500mA
    CHARGING_HOST,
    NONSTANDARD_CHARGER,    // 500mA or 1A not sure
    STANDARD_CHARGER,       // 1A
} CHARGER_TYPE;


/* Register definition */

#define EFUSEC_CON              ((volatile P_U32)(EFUSE_BASE+0x0000))
#define EFUSEC_WDATA0       ((volatile P_U32)(EFUSE_BASE+0x0010))
#define EFUSEC_WDATA1       ((volatile P_U32)(EFUSE_BASE+0x0014))
#define EFUSEC_WDATA2       ((volatile P_U32)(EFUSE_BASE+0x0018))
#define EFUSEC_WDATA3       ((volatile P_U32)(EFUSE_BASE+0x001C))
#define EFUSEC_D0                ((volatile P_U32)(EFUSE_BASE+0x0020))
#define EFUSEC_D1                ((volatile P_U32)(EFUSE_BASE+0x0024))
#define EFUSEC_D2                ((volatile P_U32)(EFUSE_BASE+0x0028))
#define EFUSEC_D3                ((volatile P_U32)(EFUSE_BASE+0x002C))
#define EFUSEC_PGMCTR       ((volatile P_U32)(EFUSE_BASE+0x0030))

/* EFUSEC_CON */
#define EFUSEC_CON_VLD                      (0x0001)
#define EFUSEC_CON_BUSY                    (0x0002)
#define EFUSEC_CON_READ                    (0x0004)
#define EFUSEC_CON_PGM                      (0x0008)
#define EFUSEC_CON_F52M_EN              (0x0010)
#define EFUSEC_CON_PGM_UPDATAE     (0x0020)

void BAT_thread(UINT16);

#endif /* __INCLUDE_MT3351_BATTERY_H */


