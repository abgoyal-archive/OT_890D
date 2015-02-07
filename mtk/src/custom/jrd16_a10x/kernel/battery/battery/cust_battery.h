
#ifndef _CUST_BAT_H_
#define _CUST_BAT_H_

typedef enum
{
	Cust_CC_50MA = 0,
	Cust_CC_90MA,
	Cust_CC_150MA,
	Cust_CC_225MA,
	Cust_CC_300MA,
	Cust_CC_450MA,
	Cust_CC_650MA,
	Cust_CC_800MA,
	Cust_CC_0MA
}cust_charging_current_enum;

typedef struct{	
	UINT32 BattVolt;
	UINT32 BattPercent;
}VBAT_TO_PERCENT;

//#elif defined(CONFIG_MT6516_E1K_BOARD)
/* Battery Temperature Protection */
#define MAX_CHARGE_TEMPERATURE  45
#define MIN_CHARGE_TEMPERATURE  0
#define ERR_CHARGE_TEMPERATURE  0xFF
/* Recharging Battery Voltage */
#define RECHARGING_VOLTAGE      4050
/* Charging Current Setting */
#define CONFIG_USB_IF 			1
#define USB_CHARGER_CURRENT					Cust_CC_450MA      
#define USB_CHARGER_CURRENT_SUSPEND			Cust_CC_0MA		// def CONFIG_USB_IF
#define USB_CHARGER_CURRENT_UNCONFIGURED	Cust_CC_90MA	// def CONFIG_USB_IF
#define USB_CHARGER_CURRENT_CONFIGURED		Cust_CC_450MA	// def CONFIG_USB_IF
#define AC_CHARGER_CURRENT					Cust_CC_650MA
/* Battery Meter Solution */
#define CONFIG_ADC_SOLUTION 	1
/* Battery Voltage and Percentage Mapping Table */
VBAT_TO_PERCENT Batt_VoltToPercent_Table[] = {
	/*BattVolt,BattPercent*/
	{3400,0},
	{3641,10},
	{3708,20},
	{3741,30},
	{3765,40},
	{3793,50},
	{3836,60},
	{3891,70},
	{3960,80},
	{4044,90},
	{4183,100},
};

/* Precise Tunning */
#define BATTERY_AVERAGE_SIZE 	60
/* Battery Type */
//#define CONFIG_BATTERY_BLP509 	1
#define CONFIG_BATTERY_E1000 	1

/* Common setting */
#define R_CURRENT_SENSE 2				// 0.2 Ohm
#define R_BAT_SENSE 2					// times of voltage
#define R_I_SENSE 2						// times of voltage
#define R_CHARGER_SENSE 5				// times of voltage
#define V_CHARGER_MAX 6000				// 6 V
#define V_CHARGER_MIN 4400				// 4.4 V
#define V_CHARGER_ENABLE 0				// 1:ON , 0:OFF

/* Teperature related setting */
#define RBAT_PULL_UP_R             24000
#define RBAT_PULL_UP_VOLT          2800
#define TBAT_OVER_CRITICAL_LOW     68237
#define BAT_TEMP_PROTECT_ENABLE    1		/*change for print enable temperature protect log*/
#define ENABLE_DEBUG_PRINTK     0               /*add for print log when charging */
#endif /* _CUST_BAT_H_ */ 
