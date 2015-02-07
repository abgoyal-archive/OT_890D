

#ifndef MT6516_SMART_BATTERY_H
#define MT6516_SMART_BATTERY_H

#include <linux/ioctl.h>
#include <mach/mt6516_typedefs.h>
#include <mach/mt6516_reg_base.h>

#define PRE_CHARGE_VOLTAGE                  3200
#define SYSTEM_OFF_VOLTAGE                  3400  
#define CONSTANT_CURRENT_CHARGE_VOLTAGE     4100  
#define CONSTANT_VOLTAGE_CHARGE_VOLTAGE     4200  
#define CV_DROPDOWN_VOLTAGE                 4000
#define CHARGER_THRESH_HOLD                 4300
#define BATTERY_UVLO_VOLTAGE                2700

#define MAX_CHARGING_TIME                   8*60*60 	// 8hr
#define MAX_PreCC_CHARGING_TIME         	1*30*60  	// 0.5hr
#define MAX_CV_CHARGING_TIME              	3*60*60 	// 3hr
#define MUTEX_TIMEOUT                       5000
#define BAT_TASK_PERIOD                     10 			// 10sec
#define MAX_CV_CHARGING_CURRENT       		100 		//100mA
#define MAX_CV_LOW_CURRENT_TIME       		1800 		//30mins

#define Battery_Percent_100    100
#define charger_OVER_VOL	    1
#define BATTERY_UNDER_VOL		2
#define BATTERY_OVER_TEMP		3
#define ADC_SAMPLE_TIMES        5

typedef unsigned int       WORD;
typedef enum {
    CHARGER_UNKNOWN = 0,
    STANDARD_HOST,          // USB : 450mA
    CHARGING_HOST,
    NONSTANDARD_CHARGER,    // AC : 450mA~1A 
    STANDARD_CHARGER,       // AC : ~1A
} CHARGER_TYPE;

typedef enum
{
	USB_SUSPEND = 0,
	USB_UNCONFIGURED,
	USB_CONFIGURED
}usb_state_enum;

#endif

