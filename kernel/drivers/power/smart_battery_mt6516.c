

#include <linux/autoconf.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/wait.h>
#include <linux/power_supply.h>
#include <mach/mt6516_typedefs.h>
#include <mach/mt6516_gpt_sw.h>
#include <mach/mt6516_smart_battery.h>
#include <mach/mt6516_auxadc_sw.h>
#include <mach/mt6516_auxadc_hw.h>
#include <linux/kthread.h>

#include "pmic6326_sw.h"        /* will delete */

#include <linux/wakelock.h> 
#include <mach/mt6516_pll.h>
#include <mach/mt65xx_leds.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include "../usb/gadget/mt6516_udc.h"

#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>

#include <asm/uaccess.h>

#include <linux/interrupt.h>
#include <mach/irqs.h>

//#include <mach/cust_battery.h>
#include <cust_battery.h>
#include <cust_adc.h>

#include <linux/module.h>
#include <linux/proc_fs.h>

//#define CONFIG_DEBUG_MSG			/*enable for print log when charging*/	
//#define CONFIG_DEBUG_MSG_NO_BQ27500

extern UINT8 g_usb_power_saving;

static DEFINE_MUTEX(bat_mutex);

int GetOneChannelValue(int dwChannel, int deCount);

int g_usb_state = USB_UNCONFIGURED;
int g_temp_CC_value = 0;
int g_soft_start_delay = 1;

int g_BAT_TemperatureR = 0;
int g_TempBattVoltage = 0;
int g_InstatVolt = 0;
int g_BatteryAverageCurrent = 0;
int g_BAT_BatterySenseVoltage = 0;
int g_BAT_ISenseVoltage = 0;
int g_BAT_ChargerVoltage = 0;

int g_chr_event = 0;
int bat_volt_cp_flag = 0;
int bat_volt_check_point = 0;
int Enable_BATDRV_LOG = 0;

/*#if defined(CONFIG_BATTERY_E1000)*/
/*int g_BatTempProtectEn = 0; //0:temperature measuring default off*/
/*#else*/
int g_BatTempProtectEn = 1; /*1:temperature measuring default on*/
/*#endif*/

#define ADC_CALI_DEVNAME "MT6516-adc_cali"
#define TEST_ADC_CALI_PRINT 0
#define SET_ADC_CALI_Slop 1
#define SET_ADC_CALI_Offset 2
#define SET_ADC_CALI_Cal 3
#define ADC_CHANNEL_READ 4
#define BAT_STATUS_READ 5
#define Set_Charger_Current 6

static struct class *adc_cali_class = NULL;
static int adc_cali_major = 0;
static dev_t adc_cali_devno;
static struct cdev *adc_cali_cdev;

int adc_cali_slop[9] = {1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000};
int adc_cali_offset[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
int adc_cali_cal[1] = {0};

int adc_in_data[2] = {1,1};
int adc_out_data[2] = {1,1};

int battery_in_data[1] = {0};
int battery_out_data[1] = {0};    

int charging_level_data[1] = {0};

BOOL g_ADC_Cali = FALSE;
BOOL g_ftm_battery_flag = FALSE;

#define ADC_SET_BITS(BS,REG)       ((*(volatile u32*)(REG)) |= (u32)(BS))
#define ADC_CLR_BITS(BS,REG)       ((*(volatile u32*)(REG)) &= ~((u32)(BS)))

inline static void mt6516_ADC_power_up(void)
{
    #if 0
    //2009/05/24: MT6516, the ADC power on is controlled by APMCUSYS_PDN_CLR0
    //            Bit 26: I2C3
    #define PDN_CLR0 (0xF0039340)   //80039340h
    u32 pwrbit = 26;
    unsigned int poweron = 1 << pwrbit;
    ADC_SET_BITS(poweron, PDN_CLR0);
    #else
    hwEnableClock(MT6516_PDN_PERI_ADC,"Battery");    
    #endif
}

inline static void mt6516_ADC_power_down(void)
{
    #if 0
    //2009/05/24: MT6516, the ADC power down is controlled by APMCUSYS_PDN_SET0
    //            Bit 26: I2C3
    #define PDN_SET0 (0xF0039320)   //80039320h
    u32 pwrbit = 26;
    unsigned int poweroff = 1 << pwrbit;
    ADC_SET_BITS(poweroff, PDN_SET0);
    #else    
    hwDisableClock(MT6516_PDN_PERI_ADC,"Battery");
    #endif
    
}

#define mt6516_smart_battery_SLAVE_ADDR_WRITE    0xAA

/* Addresses to scan */
static unsigned short normal_i2c[] = { mt6516_smart_battery_SLAVE_ADDR_WRITE,  I2C_CLIENT_END };
static unsigned short ignore = I2C_CLIENT_END;

static int mt6516_smart_battery_attach_adapter(struct i2c_adapter *adapter);
static int mt6516_smart_battery_detect(struct i2c_adapter *adapter, int address, int kind);
static int mt6516_smart_battery_detach_client(struct i2c_client *client);

static struct i2c_client_address_data addr_data = {
    .normal_i2c = normal_i2c,
    .probe    = &ignore,
    .ignore    = &ignore,
};

static struct i2c_client *new_client = NULL;

/* This is the driver that will be inserted */
static struct i2c_driver mt6516_smart_battery_driver = {       
    .attach_adapter    = mt6516_smart_battery_attach_adapter,
    .detach_client    = mt6516_smart_battery_detach_client,
    .driver =     {
        .name            = "smart_battery",
    },
};

static int bat_thread_timeout = 0;
static DECLARE_WAIT_QUEUE_HEAD(bat_thread_wq);

void wake_up_bat (void)
{
    bat_thread_timeout = 1;
    wake_up(&bat_thread_wq);
}
EXPORT_SYMBOL(wake_up_bat);

extern kal_bool pmic_ovp_status(void);
extern kal_bool pmic_chrdet_status(void);
extern kal_bool pmic_bat_on_status(void);
extern kal_bool pmic_cv_status(void);
extern void pmic_chr_current(chr_chr_current_enum curr);
extern void pmic_wdt_enable(kal_bool enable);
extern void pmic_wdt_timeout(wdt_timout_enum sel);
extern void pmic_chr_chr_enable(kal_bool enable);
extern void pmic_adc_vbat_enable(kal_bool enable);
extern void pmic_adc_isense_enable(kal_bool enable);
extern void pmic6326_kick_charger_wdt(void);
extern kal_uint8 pmic_int_status_3(void);
extern void kernel_power_off(void);

typedef struct 
{
    BOOL    bat_exist;
    BOOL    bat_full;  
    BOOL    bat_low;  
    UINT16  bat_charging_state;
    UINT16  bat_vol;            //BATSENSE
    BOOL    charger_exist;   
    UINT16  pre_charging_current;
    UINT16  charging_current;
    UINT16  charger_vol;        //CHARIN
    UINT8   charger_protect_status; 
    UINT16  ISENSE;                //ISENSE
    UINT16  ICharging;
    INT16   temperature;
    UINT32  total_charging_time;
    UINT32  CV_charging_time;
    UINT32  PRE_charging_time;
    UINT32  charger_type ;
    UINT32  PWR_SRC;
    UINT32  SOC;
    UINT32  ADC_BAT_SENSE;
    UINT32  ADC_I_SENSE;
    UINT32  ADC_Current_RSENSE;   
    UINT32  System_Current;   
    UINT32  CV_low_current_time;
} PMU_ChargerStruct;

typedef enum 
{
    PMU_STATUS_OK = 0,
    PMU_STATUS_FAIL = 1,
}PMU_STATUS;

#define  CHR_TICKLE                     0x1000
#define  CHR_PRE                        0x1001
#define  CHR_CC                         0x1002 
#define  CHR_CV                         0x1003 
#define  CHR_BATFULL                    0x1004 
#define  CHR_ERROR                      0x1005 

BOOL times_check = 1;
BOOL g_bat_timeout_happen = FALSE; 
INT16 PrevBatteryTemperature = 100;
BOOL g_bat_full_user_view = FALSE; /* 20091028 for ANdroid UI : keep soc=100 afer charging full, till charger out */ 
struct wake_lock battery_suspend_lock; /* 20091111, Kelvin add for standard charger wakelock */
struct wake_lock low_battery_suspend_lock; /* 20100317, add for low battery wakelock */

static CHARGER_TYPE CHR_Type_num = CHARGER_UNKNOWN;
PMU_ChargerStruct BMT_status;
BOOL g_Battery_Fail = FALSE;

static unsigned short batteryVoltageBuffer[BATTERY_AVERAGE_SIZE];
static unsigned short batteryCurrentBuffer[BATTERY_AVERAGE_SIZE];
static unsigned short batterySOCBuffer[BATTERY_AVERAGE_SIZE];
static int batteryIndex = 0;
static int batteryVoltageSum = 0;
static int batteryCurrentSum = 0;
static int batterySOCSum = 0; 
BOOL batteryBufferFirst = FALSE;

#define g_free_bat_temp 1000 		// 1 s
#define CHARGING_FULL_CURRENT 120	// 120 mA 
int g_HW_Charging_Done = 0;
int g_Charging_Over_Time = 0;

#define VBAT_CV_CHECK_POINT 4100	// 4.1V
int g_CV_CHECK_POINT_EN = 0;
int g_CV_CHECK_POINT_Done = 0;

struct mt6516_ac_data {
    struct power_supply psy;
    int AC_ONLINE;    
};

struct mt6516_usb_data {
    struct power_supply psy;
    int USB_ONLINE;    
};

struct mt6516_battery_data {
    struct power_supply psy;
    int BAT_STATUS;
    int BAT_HEALTH;
    int BAT_PRESENT;
    int BAT_TECHNOLOGY;
    int BAT_CAPACITY;
    /* 20090603 James Lo add */
    int BAT_batt_vol;
    int BAT_batt_temp;
	/* 20100405 Add for EM */
	int BAT_TemperatureR;
	int BAT_TempBattVoltage;
	int BAT_InstatVolt;
	int BAT_BatteryAverageCurrent;
	int BAT_BatterySenseVoltage;
	int BAT_ISenseVoltage;
	int BAT_ChargerVoltage;
};

static enum power_supply_property mt6516_ac_props[] = {
    POWER_SUPPLY_PROP_ONLINE,
};

static enum power_supply_property mt6516_usb_props[] = {
    POWER_SUPPLY_PROP_ONLINE,
};

static enum power_supply_property mt6516_battery_props[] = {
    POWER_SUPPLY_PROP_STATUS,
    POWER_SUPPLY_PROP_HEALTH,
    POWER_SUPPLY_PROP_PRESENT,
    POWER_SUPPLY_PROP_TECHNOLOGY,
    POWER_SUPPLY_PROP_CAPACITY,
    /* 20090603 James Lo add */
    POWER_SUPPLY_PROP_batt_vol,
    POWER_SUPPLY_PROP_batt_temp,    
    /* 20100405 Add for EM */
	POWER_SUPPLY_PROP_TemperatureR,
	POWER_SUPPLY_PROP_TempBattVoltage,
	POWER_SUPPLY_PROP_InstatVolt,
	POWER_SUPPLY_PROP_BatteryAverageCurrent,
	POWER_SUPPLY_PROP_BatterySenseVoltage,
	POWER_SUPPLY_PROP_ISenseVoltage,
	POWER_SUPPLY_PROP_ChargerVoltage,
};

static int mt6516_ac_get_property(struct power_supply *psy,
            enum power_supply_property psp,
            union power_supply_propval *val)
{
    int ret = 0;
    struct mt6516_ac_data *data = container_of(psy, struct mt6516_ac_data, psy);    

    switch (psp) {
    case POWER_SUPPLY_PROP_ONLINE:                           
        val->intval = data->AC_ONLINE;
        break;
    default:
        ret = -EINVAL;
        break;
    }
    return ret;
}

static int mt6516_usb_get_property(struct power_supply *psy,
            enum power_supply_property psp,
            union power_supply_propval *val)
{
    int ret = 0;
    struct mt6516_usb_data *data = container_of(psy, struct mt6516_usb_data, psy);    

    switch (psp) {
    case POWER_SUPPLY_PROP_ONLINE:                           
        val->intval = data->USB_ONLINE;
        break;
    default:
        ret = -EINVAL;
        break;
    }
    return ret;
}

static int mt6516_battery_get_property(struct power_supply *psy,
                 enum power_supply_property psp,
                 union power_supply_propval *val)
{
    int ret = 0;     
    struct mt6516_battery_data *data = container_of(psy, struct mt6516_battery_data, psy);
    
    switch (psp) {
    case POWER_SUPPLY_PROP_STATUS:
        val->intval = data->BAT_STATUS;
        break;
    case POWER_SUPPLY_PROP_HEALTH:
        val->intval = data->BAT_HEALTH;
        break;
    case POWER_SUPPLY_PROP_PRESENT:
        val->intval = data->BAT_PRESENT;
        break;
    case POWER_SUPPLY_PROP_TECHNOLOGY:
        val->intval = data->BAT_TECHNOLOGY;
        break;
    case POWER_SUPPLY_PROP_CAPACITY:
        val->intval = data->BAT_CAPACITY;
        break;        
    case POWER_SUPPLY_PROP_batt_vol:
        val->intval = data->BAT_batt_vol;
        break;
    case POWER_SUPPLY_PROP_batt_temp:
        val->intval = data->BAT_batt_temp;
        break;
	case POWER_SUPPLY_PROP_TemperatureR:
		val->intval = data->BAT_TemperatureR;
		break;		
	case POWER_SUPPLY_PROP_TempBattVoltage:		
		val->intval = data->BAT_TempBattVoltage;
		break;		
	case POWER_SUPPLY_PROP_InstatVolt:
		val->intval = data->BAT_InstatVolt;
		break;		
	case POWER_SUPPLY_PROP_BatteryAverageCurrent:
		val->intval = data->BAT_BatteryAverageCurrent;
		break;		
	case POWER_SUPPLY_PROP_BatterySenseVoltage:
		val->intval = data->BAT_BatterySenseVoltage;
		break;		
	case POWER_SUPPLY_PROP_ISenseVoltage:
		val->intval = data->BAT_ISenseVoltage;
		break;		
	case POWER_SUPPLY_PROP_ChargerVoltage:
		val->intval = data->BAT_ChargerVoltage;
		break;
 
    default:
        ret = -EINVAL;
        break;
    }

    return ret;
}

/* mt6516_ac_data initialization */
static struct mt6516_ac_data mt6516_ac_main = {
    .psy = {
        .name = "ac",
        .type = POWER_SUPPLY_TYPE_MAINS,
        .properties = mt6516_ac_props,
        .num_properties = ARRAY_SIZE(mt6516_ac_props),
        .get_property = mt6516_ac_get_property,                
    },
    .AC_ONLINE = 0,
};

/* mt6516_usb_data initialization */
static struct mt6516_usb_data mt6516_usb_main = {
    .psy = {
        .name = "usb",
        .type = POWER_SUPPLY_TYPE_USB,
        .properties = mt6516_usb_props,
        .num_properties = ARRAY_SIZE(mt6516_usb_props),
        .get_property = mt6516_usb_get_property,                
    },
    .USB_ONLINE = 0,
};

/* mt6516_battery_data initialization */
static struct mt6516_battery_data mt6516_battery_main = {
    .psy = {
        .name = "battery",
        .type = POWER_SUPPLY_TYPE_BATTERY,
        .properties = mt6516_battery_props,
        .num_properties = ARRAY_SIZE(mt6516_battery_props),
        .get_property = mt6516_battery_get_property,                
    },
    .BAT_STATUS = POWER_SUPPLY_STATUS_UNKNOWN,    
    .BAT_HEALTH = POWER_SUPPLY_HEALTH_UNKNOWN,
    .BAT_PRESENT = 0,
    .BAT_TECHNOLOGY = POWER_SUPPLY_TECHNOLOGY_UNKNOWN,
    .BAT_CAPACITY = 0,
    .BAT_batt_vol = 0,
    .BAT_batt_temp = 0,
};

static void mt6516_ac_update(struct mt6516_ac_data *ac_data)
{
    struct power_supply *ac_psy = &ac_data->psy;

    if( pmic_chrdet_status() ) 
    {        
        if ( (BMT_status.charger_type == NONSTANDARD_CHARGER) || (BMT_status.charger_type == STANDARD_CHARGER) )
        {
            ac_data->AC_ONLINE = 1;        
            ac_psy->type = POWER_SUPPLY_TYPE_MAINS;
        }
    }
    else
    {
        ac_data->AC_ONLINE = 0;        
    }

    power_supply_changed(ac_psy);    
}

static void mt6516_usb_update(struct mt6516_usb_data *usb_data)
{
    struct power_supply *usb_psy = &usb_data->psy;

    if( pmic_chrdet_status() )    
    {
        if (BMT_status.charger_type == STANDARD_HOST)
        {
            usb_data->USB_ONLINE = 1;            
            usb_psy->type = POWER_SUPPLY_TYPE_USB;            
        }
    }
    else
    {
        usb_data->USB_ONLINE = 0;
    }
 
    power_supply_changed(usb_psy); 
}

static void mt6516_battery_update(struct mt6516_battery_data *bat_data)
{
    struct power_supply *bat_psy = &bat_data->psy;
	int i;

    /* Update : BAT_STATUS, BAT_HEALTH, BAT_PRESENT, BAT_TECHNOLOGY, BAT_CAPACITY, BAT_VOLTAGE_NOW, BAT_TEMP */  

    bat_data->BAT_TECHNOLOGY = POWER_SUPPLY_TECHNOLOGY_LION;
    bat_data->BAT_HEALTH = POWER_SUPPLY_HEALTH_GOOD;
    bat_data->BAT_batt_vol = BMT_status.bat_vol;
    bat_data->BAT_batt_temp= BMT_status.temperature * 10;

    if (BMT_status.bat_exist)
        bat_data->BAT_PRESENT = 1;
    else
        bat_data->BAT_PRESENT = 0;

    /* Charger and Battery Exist */
    if( pmic_chrdet_status() && (!g_Battery_Fail) )
    {        
        if ( BMT_status.bat_exist )                
        {            
            /* Battery Full */
            if ( (BMT_status.bat_vol >= RECHARGING_VOLTAGE) && (BMT_status.bat_full == TRUE) )
            {
                bat_data->BAT_STATUS = POWER_SUPPLY_STATUS_FULL;
                bat_data->BAT_CAPACITY = Battery_Percent_100;
				
				/* For user view */
				for (i=0; i<BATTERY_AVERAGE_SIZE; i++) {
					batterySOCBuffer[i] = 100; 
					batterySOCSum = 100 * BATTERY_AVERAGE_SIZE; /* for user view */
				}
				bat_volt_check_point = 100;
            }
            /* battery charging */
            else 
            {
                /* 20091028, do re-charging for keep battery soc */
                if (g_bat_full_user_view) 
                {
                    bat_data->BAT_STATUS = POWER_SUPPLY_STATUS_FULL;
                    bat_data->BAT_CAPACITY = Battery_Percent_100;

					/* For user view */
					for (i=0; i<BATTERY_AVERAGE_SIZE; i++) {
						batterySOCBuffer[i] = 100; 
						batterySOCSum = 100 * BATTERY_AVERAGE_SIZE; /* for user view */
					}
					bat_volt_check_point = 100;
                }
                else
                {
                    bat_data->BAT_STATUS = POWER_SUPPLY_STATUS_CHARGING;
                    //bat_data->BAT_CAPACITY = BMT_status.SOC;
                    
                    /* SOC only UP when charging */
                    if ( BMT_status.SOC > bat_volt_check_point ) {						
						bat_volt_check_point = BMT_status.SOC;
                    } 
					bat_data->BAT_CAPACITY = bat_volt_check_point;
                }
            }
        }
        /* No Battery, Only Charger */
        else
        {
            bat_data->BAT_STATUS = POWER_SUPPLY_STATUS_UNKNOWN;
            bat_data->BAT_CAPACITY = 0;
        }
        
    }
    /* Only Battery */
    else
    {
        bat_data->BAT_STATUS = POWER_SUPPLY_STATUS_NOT_CHARGING;

        /* If VBAT < CLV, then shutdown */
        if (BMT_status.bat_vol <= SYSTEM_OFF_VOLTAGE)
        {            
            printk(  "[BAT BATTERY] VBAT < %d mV : Android will Power Off System !!\r\n", SYSTEM_OFF_VOLTAGE);                              
            bat_data->BAT_CAPACITY = 0;
            
            wake_lock(&low_battery_suspend_lock);/* 20100317 for suspend low battery */
        }
        else
        {    
            //bat_data->BAT_CAPACITY = BMT_status.SOC;

			/* SOC only Done when dis-charging */
            if ( BMT_status.SOC < bat_volt_check_point ) {
				bat_volt_check_point = BMT_status.SOC;
            }
			bat_data->BAT_CAPACITY = bat_volt_check_point;

            wake_unlock(&low_battery_suspend_lock); /* 20100317 for suspend low battery */
        }
    }    

	/* Update for EM */
	bat_data->BAT_TemperatureR=g_BAT_TemperatureR;
	bat_data->BAT_TempBattVoltage=g_TempBattVoltage;
	bat_data->BAT_InstatVolt=g_InstatVolt;
	bat_data->BAT_BatteryAverageCurrent=g_BatteryAverageCurrent;
	bat_data->BAT_BatterySenseVoltage=g_BAT_BatterySenseVoltage;
	bat_data->BAT_ISenseVoltage=g_BAT_ISenseVoltage;
	bat_data->BAT_ChargerVoltage=g_BAT_ChargerVoltage;
	
    power_supply_changed(bat_psy);    
}

#if defined(CONFIG_SMART_BATTERY_IC_SOLUTION)
static ssize_t mt6516_smart_battery_read_temperature(void)
{
    char  cmd_buf[2]={0x06,0x07};
    char  readData[2] = {0xff, 0xff};
    int  ret=0;
    u16  temperature = 0;

    ret = i2c_master_send(new_client, &cmd_buf[0], 1);
    if (ret < 0) {
        printk("sends command error!! \n");
        return 0;
    }
    ret = i2c_master_recv(new_client, readData, 2);
    if (ret < 0) {
        printk("reads data error!! \n");
        return 0;
    }

    #if 0
    printk(" mt6516_smart_battery_detect : cmd %x . read data : MSB=0x%x , LSB=0x%x!!\n ", \
        cmd_buf[0], readData[1], readData[0]);
    #endif

    temperature |= readData[1] << 8;
    temperature |= readData[0];
    
    BMT_status.temperature = ( temperature / 10 ) - 273;        

    return 0;
}

static ssize_t mt6516_smart_battery_read_vbat(void)
{
    char  cmd_buf[2]={0x08,0x09};
    char  readData[2] = {0xff, 0xff};
    int  ret=0;
    u16  vbat = 0;

    ret = i2c_master_send(new_client, &cmd_buf[0], 1);
    if (ret < 0) {
        printk("sends command error!! \n");
        return 0;
    }
    ret = i2c_master_recv(new_client, readData, 2);
    if (ret < 0) {
        printk("reads data error!! \n");
        return 0;
    }

    #if 0
    printk(" mt6516_smart_battery_detect : cmd %x . read data : MSB=0x%x , LSB=0x%x!!\n ", \
        cmd_buf[0], readData[1], readData[0]);
    #endif

    vbat |= readData[1] << 8;
    vbat |= readData[0];
     
    BMT_status.bat_vol = vbat;        

    return 0;
}

static ssize_t mt6516_smart_battery_read_flag(void)
{
    char  cmd_buf[2]={0x0a,0x0b};
    char  readData[2] = {0xff, 0xff};
    int  ret=0;
    u16  sb_flags = 0;

    ret = i2c_master_send(new_client, &cmd_buf[0], 1);
    if (ret < 0) {
        printk("sends command error!! \n");
        return 0;
    }
    ret = i2c_master_recv(new_client, readData, 2);
    if (ret < 0) {
        printk("reads data error!! \n");
        return 0;
    }

    #if 0
    printk(" mt6516_smart_battery_detect : cmd %x . read data : MSB=0x%x , LSB=0x%x!!\n ", \
        cmd_buf[0], readData[1], readData[0]);
    #endif

    sb_flags |= readData[1] << 8;
    sb_flags |= readData[0];
    
    printk("smart_battery_read_flag : %x\n", sb_flags);

    return 0;
}

static ssize_t mt6516_smart_battery_read_AI(void)
{
    char  cmd_buf[2]={0x14,0x15};
    char  readData[2] = {0xff, 0xff};
    int  ret=0;
    u16  AI = 0;

    ret = i2c_master_send(new_client, &cmd_buf[0], 1);
    if (ret < 0) {
        printk("sends command error!! \n");
        return 0;
    }
    ret = i2c_master_recv(new_client, readData, 2);
    if (ret < 0) {
        printk("reads data error!! \n");
        return 0;
    }

    #if 0
    printk(" mt6516_smart_battery_detect : cmd %x . read data : MSB=0x%x , LSB=0x%x!!\n ", \
        cmd_buf[0], readData[1], readData[0]);
    #endif

    AI |= readData[1] << 8;
    AI |= readData[0];

    if ( AI >= 1000 ) 
    {
        /* error value */
        BMT_status.ICharging= 0;        
    }
    else
    {
        BMT_status.ICharging= AI;        
    }
    
    return 0;
}

static ssize_t mt6516_smart_battery_read_SOC(void)
{
    char  cmd_buf[2]={0x2c,0x2d};
    char  readData[2] = {0xff, 0xff};
    int  ret=0;
    u16  SOC = 0;

    ret = i2c_master_send(new_client, &cmd_buf[0], 1);
    if (ret < 0) {
        printk("sends command error!! \n");
        return 0;
    }
    ret = i2c_master_recv(new_client, readData, 2);
    if (ret < 0) {
        printk("reads data error!! \n");
        return 0;
    }

    #if 0
    printk(" mt6516_smart_battery_detect : cmd %x . read data : MSB=0x%x , LSB=0x%x!!\n ", \
        cmd_buf[0], readData[1], readData[0]);
    #endif

    SOC |= readData[1] << 8;
    SOC |= readData[0];
    
    BMT_status.SOC = SOC;        

    if( BMT_status.SOC == 100 )
    {
        BMT_status.bat_full = TRUE;
    }

    return 0;
}
#endif

#if defined(CONFIG_ADC_SOLUTION)

typedef struct{
    UINT32 BatteryTemp;
    UINT32 TemperatureR;
}BATT_TEMPERATURE;

/* convert register to temperature  */
INT16 BattThermistorConverTemp(UINT32 Res)
{
    int i=0;
    UINT32 RES1=0,RES2=0;
    INT16 TBatt_Value=-200,TMP1=0,TMP2=0;

    BATT_TEMPERATURE Batt_Temperature_Table[] = {
        {-20,68237},
        {-15,53650},
        {-10,42506},
        { -5,33892},
        {  0,27219},
        {  5,22021},
        { 10,17926},
        { 15,14674},
        { 20,12081},
        { 25,10000},
        { 30,8315},
        { 35,6948},
        { 40,5834},
        { 45,4917},
        { 50,4161},
        { 55,3535},
        { 60,3014}
    };
    
    if(Res>=Batt_Temperature_Table[0].TemperatureR)
    {
        #ifdef CONFIG_DEBUG_MSG_NO_BQ27500
        printk("Res>=%d\n", Batt_Temperature_Table[0].TemperatureR);
        #endif
        TBatt_Value = -20;
    }
    else if(Res<=Batt_Temperature_Table[16].TemperatureR)
    {
        #ifdef CONFIG_DEBUG_MSG_NO_BQ27500
        printk("Res<=%d\n", Batt_Temperature_Table[16].TemperatureR);
        #endif
        TBatt_Value = 60;
    }
    else
    {
        RES1=Batt_Temperature_Table[0].TemperatureR;
        TMP1=Batt_Temperature_Table[0].BatteryTemp;

        for(i=0;i<=16;i++)
        {
            if(Res>=Batt_Temperature_Table[i].TemperatureR)
            {
                RES2=Batt_Temperature_Table[i].TemperatureR;
                TMP2=Batt_Temperature_Table[i].BatteryTemp;
                break;
            }
            else
            {
                RES1=Batt_Temperature_Table[i].TemperatureR;
                TMP1=Batt_Temperature_Table[i].BatteryTemp;
            }
        }
        
        TBatt_Value = (((Res-RES2)*TMP1)+((RES1-Res)*TMP2))/(RES1-RES2);
    }

    #ifdef CONFIG_DEBUG_MSG_NO_BQ27500
    printk("BattThermistorConverTemp() : TBatt_Value = %d\n",TBatt_Value);
    #endif

    return TBatt_Value;    
}

/* convert ADC_bat_temp_volt to register */
INT16 BattVoltToTemp(UINT32 dwVolt)
{
    UINT32 TRes;
    UINT32 dwVCriBat = (TBAT_OVER_CRITICAL_LOW*RBAT_PULL_UP_VOLT)/(TBAT_OVER_CRITICAL_LOW+RBAT_PULL_UP_R); //~2000mV
    INT16 sBaTTMP = -100;

    if(dwVolt > dwVCriBat)
        TRes = TBAT_OVER_CRITICAL_LOW;
    else
        TRes = (RBAT_PULL_UP_R*dwVolt)/(RBAT_PULL_UP_VOLT-dwVolt);        

	g_BAT_TemperatureR = TRes;

    /* convert register to temperature */
    sBaTTMP = BattThermistorConverTemp(TRes);

    #ifdef CONFIG_DEBUG_MSG_NO_BQ27500
    printk("BattVoltToTemp() : dwVolt = %d\n", dwVolt);
    printk("BattVoltToTemp() : TRes = %d\n", TRes);
    printk("BattVoltToTemp() : sBaTTMP = %d\n", sBaTTMP);
    #endif
    
    return sBaTTMP;
}
#endif /* CONFIG_ADC_SOLUTION */

#if defined(CONFIG_ADC_SOLUTION)
UINT32 BattVoltToPercent(UINT16 dwVoltage)
{
    UINT32 m=0;
    UINT32 VBAT1=0,VBAT2=0;
    UINT32 bPercntResult=0,bPercnt1=0,bPercnt2=0;

#if ENABLE_DEBUG_PRINTK
    printk("###### 100 <-> voltage : %d ######\r\n", Batt_VoltToPercent_Table[10].BattVolt);
#endif    

    if(dwVoltage<=Batt_VoltToPercent_Table[0].BattVolt)
    {
        bPercntResult = Batt_VoltToPercent_Table[0].BattPercent;
        return bPercntResult;
    }
    else if (dwVoltage>=Batt_VoltToPercent_Table[10].BattVolt)
    {
        bPercntResult = Batt_VoltToPercent_Table[10].BattPercent;
        return bPercntResult;
    }
    else
    {        
        VBAT1 = Batt_VoltToPercent_Table[0].BattVolt;
        bPercnt1 = Batt_VoltToPercent_Table[0].BattPercent;
        for(m=1;m<=10;m++)
        {
            if(dwVoltage<=Batt_VoltToPercent_Table[m].BattVolt)
            {
                VBAT2 = Batt_VoltToPercent_Table[m].BattVolt;
                bPercnt2 = Batt_VoltToPercent_Table[m].BattPercent;
                break;
            }
            else
            {
                VBAT1 = Batt_VoltToPercent_Table[m].BattVolt;
                bPercnt1 = Batt_VoltToPercent_Table[m].BattPercent;    
            }
        }
    }
    
    bPercntResult = ( ((dwVoltage-VBAT1)*bPercnt2)+((VBAT2-dwVoltage)*bPercnt1) ) / (VBAT2-VBAT1);    

    return bPercntResult;
    
}
#endif /* CONFIG_ADC_SOLUTION */

void BAT_GetVoltage(void)
{ 
    UINT32 u4Sample_times = 0;
    UINT32 dat = 0;
    UINT32 u4channel[9] = {0,0,0,0,0,0,0,0,0};
    UINT32 bat_temperature_volt = 0;

#if defined(CONFIG_BATTERY_B56QN)    
    /* Using smart battery  */
    mt6516_smart_battery_read_vbat();
    mt6516_smart_battery_read_temperature();    
    mt6516_smart_battery_read_SOC();
    mt6516_smart_battery_read_AI();      
    //mt6516_smart_battery_read_flag();
#endif
    
    /* Enable ADC power bit */    
    mt6516_ADC_power_up();

    /* Initialize ADC control register */
    DRV_WriteReg(AUXADC_CON0, 0);
    DRV_WriteReg(AUXADC_CON1, 0);    
    DRV_WriteReg(AUXADC_CON2, 0);    
    DRV_WriteReg(AUXADC_CON3, 0);   
    
    do
    {
        pmic_adc_vbat_enable(KAL_TRUE);
        pmic_adc_isense_enable(KAL_TRUE); 

        DRV_WriteReg(AUXADC_CON1, 0);        
        DRV_WriteReg(AUXADC_CON1, 0x1FF);
         
        DVT_DELAYMACRO(1000);

        /* Polling until bit STA = 0 */
        while (0 != (0x01 & DRV_Reg(AUXADC_CON3)));          
       
        dat = DRV_Reg(AUXADC_DAT0);        
        u4channel[0] += dat;   
        dat = DRV_Reg(AUXADC_DAT1);        
        u4channel[1] += dat;   
        dat = DRV_Reg(AUXADC_DAT2);        
        u4channel[2] += dat;   
        dat = DRV_Reg(AUXADC_DAT3);        
        u4channel[3] += dat;   
        dat = DRV_Reg(AUXADC_DAT4);
        u4channel[4] += dat;
        dat = DRV_Reg(AUXADC_DAT5);
        u4channel[5] += dat;
        dat = DRV_Reg(AUXADC_DAT6);
        u4channel[6] += dat;  
        dat = DRV_Reg(AUXADC_DAT7);
        u4channel[7] += dat;
        dat = DRV_Reg(AUXADC_DAT8);
        u4channel[8] += dat;    

        u4Sample_times++;
    }
    while (u4Sample_times < ADC_SAMPLE_TIMES);

    #if 0
    printk("BAT_GetVoltage : channel_0 = %d \n", u4channel[0] );
    printk("BAT_GetVoltage : channel_1 = %d \n", u4channel[1] );
    printk("BAT_GetVoltage : channel_2 = %d \n", u4channel[2] );
    printk("BAT_GetVoltage : channel_3 = %d \n", u4channel[3] );
    printk("BAT_GetVoltage : channel_4 = %d \n", u4channel[4] );
    printk("BAT_GetVoltage : channel_5 = %d \n", u4channel[5] );
    printk("BAT_GetVoltage : channel_6 = %d \n", u4channel[6] );
    printk("BAT_GetVoltage : channel_7 = %d \n", u4channel[7] );
    printk("BAT_GetVoltage : channel_8 = %d \n", u4channel[8] );
    #endif

    /* Value averaging  */ 
    u4channel[0] = u4channel[0]/ADC_SAMPLE_TIMES;
    u4channel[1] = u4channel[1]/ADC_SAMPLE_TIMES;
    u4channel[2] = u4channel[2]/ADC_SAMPLE_TIMES;
    u4channel[3] = u4channel[3]/ADC_SAMPLE_TIMES;
    u4channel[4] = u4channel[4]/ADC_SAMPLE_TIMES;
    u4channel[5] = u4channel[5]/ADC_SAMPLE_TIMES;
    u4channel[6] = u4channel[6]/ADC_SAMPLE_TIMES;
    u4channel[7] = u4channel[7]/ADC_SAMPLE_TIMES;
    u4channel[8] = u4channel[8]/ADC_SAMPLE_TIMES;

	if (g_chr_event == 0) {    	
		BMT_status.ADC_BAT_SENSE = 
    	    ( ( u4channel[AUXADC_BATTERY_VOLTAGE_CHANNEL]*2800/1024 )*R_BAT_SENSE );			
	} else {
		/* Just charger in/out event */
		g_chr_event = 0;		
		
		//20100513
		BMT_status.ADC_BAT_SENSE = 
			( ( u4channel[AUXADC_REF_CURRENT_CHANNEL]*2800/1024 )*R_I_SENSE );// same as I_sense
	}
    //BMT_status.ADC_BAT_SENSE = 
    //    ( ( u4channel[AUXADC_BATTERY_VOLTAGE_CHANNEL]*2800/1024 )*R_BAT_SENSE );	
    BMT_status.ADC_I_SENSE   = 
        ( ( u4channel[AUXADC_REF_CURRENT_CHANNEL]*2800/1024 )*R_I_SENSE );
    BMT_status.charger_vol   = 
        ( ( u4channel[AUXADC_CHARGER_VOLTAGE_CHANNEL]*2800/1024 )*R_CHARGER_SENSE );
    #if defined(AUXADC_TEMPERATURE_CHANNEL)
    bat_temperature_volt     = 
        ( u4channel[AUXADC_TEMPERATURE_CHANNEL]*2800/1024 ); 
    #endif

    #if defined(CONFIG_BATTERY_B61UN)
    BMT_status.temperature = BattVoltToTemp(bat_temperature_volt);        
    BMT_status.bat_vol = BMT_status.ADC_BAT_SENSE;

    //if(pmic_chrdet_status())
    if(BMT_status.ADC_I_SENSE > BMT_status.ADC_BAT_SENSE)
        BMT_status.ICharging = (BMT_status.ADC_I_SENSE - BMT_status.ADC_BAT_SENSE)*10/R_CURRENT_SENSE;
    else
        BMT_status.ICharging = 0;    
    #endif

    #if defined(CONFIG_BATTERY_BLP509) || defined(CONFIG_BATTERY_E1000)
    /* V1 can not get temperature info from ADC */
    BMT_status.temperature = 25;         
    BMT_status.bat_vol = BMT_status.ADC_BAT_SENSE;

	if ( g_BatTempProtectEn == 1 ) {
		/* jrd use ADC channel 2 for temperature */
		BMT_status.temperature = BattVoltToTemp(bat_temperature_volt);        		
	}

    /* Data Calibration  */    
    if (g_ADC_Cali) {
   #if ENABLE_DEBUG_PRINTK
        printk("Before Cal : %d(B) , %d(I) \r\n", BMT_status.ADC_BAT_SENSE, BMT_status.ADC_I_SENSE);
   #endif
        BMT_status.ADC_I_SENSE = ((BMT_status.ADC_I_SENSE * (*(adc_cali_slop+0)))+(*(adc_cali_offset+0)))/1000;
        BMT_status.ADC_BAT_SENSE = ((BMT_status.ADC_BAT_SENSE * (*(adc_cali_slop+1)))+(*(adc_cali_offset+1)))/1000;
   #if ENABLE_DEBUG_PRINTK
        printk("After Cal : %d(B) , %d(I) \r\n", BMT_status.ADC_BAT_SENSE, BMT_status.ADC_I_SENSE);
   #endif	
    }
    
    //if(pmic_chrdet_status())
	if(BMT_status.ADC_I_SENSE > BMT_status.ADC_BAT_SENSE)
        BMT_status.ICharging = (BMT_status.ADC_I_SENSE - BMT_status.ADC_BAT_SENSE)*10/R_CURRENT_SENSE;
    else
        BMT_status.ICharging = 0;    
    #endif

    /* Get a valid battery voltage */    
    if( BMT_status.bat_vol == 0 )
        BMT_status.bat_vol = BMT_status.ADC_BAT_SENSE;

	#if 0
    /* Charging current  = (VIsense-VBAT) / 0.2 Ohm */
    if(BMT_status.ADC_I_SENSE > BMT_status.ADC_BAT_SENSE)
        BMT_status.ADC_Current_RSENSE = (BMT_status.ADC_I_SENSE - BMT_status.ADC_BAT_SENSE)*10/2;    
    else
        BMT_status.ADC_Current_RSENSE = 0;

    /* Try to get the System_Current when no charging*/
    if( ( pmic_chrdet_status() ) && !BMT_status.bat_full)
        BMT_status.System_Current = BMT_status.ADC_Current_RSENSE - BMT_status.ICharging;    
    else
        BMT_status.System_Current = 0;
	#endif	

    //#ifdef CONFIG_DEBUG_MSG
    if (Enable_BATDRV_LOG == 1) {
    	printk("[BATTERY:ADC] VCHR:%d BAT_SENSE:%d I_SENSE:%d Current:%d CAL:%d\n", BMT_status.charger_vol,
            BMT_status.ADC_BAT_SENSE, BMT_status.ADC_I_SENSE, BMT_status.ICharging, g_ADC_Cali );
    }
    //#endif

	g_InstatVolt = u4channel[AUXADC_BATTERY_VOLTAGE_CHANNEL];
	g_BatteryAverageCurrent = BMT_status.ICharging;
	g_BAT_BatterySenseVoltage = BMT_status.ADC_BAT_SENSE;
	g_BAT_ISenseVoltage = BMT_status.ADC_I_SENSE;
	g_BAT_ChargerVoltage = BMT_status.charger_vol;
	
    /* Disable ADC power bit */    
    //mt6516_ADC_power_down();
   
}

UINT32 BAT_CheckPMUStatusReg(void)
{
	/* battery is the only power source, no battery = can not run system */
#if 0	
    /* battery status, 1: remove */
    if( pmic_bat_on_status() )
    {
        /* Reset g_Battery_Fail value when remove battery */
        g_Battery_Fail = FALSE;           
        BMT_status.bat_exist = FALSE;

        printk(  "[BAT ERROR] battery remove !!\n\r");
        return PMU_STATUS_FAIL;        
    }
    else
    {
        BMT_status.bat_exist = TRUE;
    }
#endif	
   
    /* MT6326 power source = battery only */
    if( pmic_chrdet_status() )
    {
        BMT_status.charger_exist = TRUE;

        if( pmic_cv_status() )
        {
            BMT_status.bat_charging_state = CHR_CV;
        }
        else
        {
        	// 20100730
            //if ( BMT_status.bat_full )
            if( g_HW_Charging_Done == 1 )
            {
                 BMT_status.bat_charging_state = CHR_BATFULL;
            }
            else
            {
            	if( g_CV_CHECK_POINT_Done == 0 )
            	{
                 BMT_status.bat_charging_state = CHR_CC;
         }       
				else
				{
					BMT_status.bat_charging_state = CHR_CV;
				}
            }
         }       
    }
    else
    {   
        BMT_status.charger_exist = FALSE;
        BMT_status.total_charging_time = 0;
        BMT_status.CV_charging_time = 0;
        BMT_status.PRE_charging_time = 0;
        BMT_status.bat_charging_state = CHR_ERROR;        
        return PMU_STATUS_FAIL;
    }    

    /* charger OV protect */
    if( pmic_ovp_status() )
    {
        BMT_status.charger_protect_status = charger_OVER_VOL;
        BMT_status.bat_charging_state = CHR_ERROR;

        printk(  "[BAT ERROR] Charger Over Voltage !!\n\r");
        return PMU_STATUS_FAIL;        
    }

    return PMU_STATUS_OK;        
}    

void Show_ChargingCurrent(void)
{
	int tmp_bat_sense = 0;
	int tmp_i_sense = 0;
	int tmp_charging_current = 0;

	tmp_bat_sense = GetOneChannelValue(AUXADC_BATTERY_VOLTAGE_CHANNEL, 5) / 5;
	tmp_i_sense = GetOneChannelValue(AUXADC_REF_CURRENT_CHANNEL, 5) / 5;
	tmp_charging_current = (tmp_i_sense - tmp_bat_sense)*10/R_CURRENT_SENSE;

	printk("Show_ChargingCurrent:%d\n", tmp_charging_current);
}

int BAT_ChargingCurrent_softStart(int temp_CC_value)
{
	if( temp_CC_value == Cust_CC_0MA )
	{
		pmic_chr_chr_enable(KAL_FALSE);
		return 1;
	}
	
	if( temp_CC_value >= Cust_CC_90MA ) 
	{
		pmic_chr_current(Cust_CC_90MA);
		pmic_chr_chr_enable(KAL_TRUE);	
		msleep(g_soft_start_delay);				
	  #if ENABLE_DEBUG_PRINTK
		printk("BAT_ChargingCurrent_softStart:%d on %d\n",Cust_CC_90MA,temp_CC_value);		
		Show_ChargingCurrent();
	  #endif
	}
	if( temp_CC_value >= Cust_CC_150MA ) 
	{
		pmic_chr_current(Cust_CC_150MA);
		pmic_chr_chr_enable(KAL_TRUE);	
		msleep(g_soft_start_delay);		
	  #if ENABLE_DEBUG_PRINTK
		printk("BAT_ChargingCurrent_softStart:%d on %d\n",Cust_CC_150MA,temp_CC_value);
		Show_ChargingCurrent();
	  #endif
	}
	if( temp_CC_value >= Cust_CC_225MA ) 
	{
		pmic_chr_current(Cust_CC_225MA);
		pmic_chr_chr_enable(KAL_TRUE);	
		msleep(g_soft_start_delay);
	  #if ENABLE_DEBUG_PRINTK
		printk("BAT_ChargingCurrent_softStart:%d on %d\n",Cust_CC_225MA,temp_CC_value);
		Show_ChargingCurrent();
	  #endif
	}
	if( temp_CC_value >= Cust_CC_300MA ) 
	{
		pmic_chr_current(Cust_CC_300MA);
		pmic_chr_chr_enable(KAL_TRUE);	
		msleep(g_soft_start_delay);
	  #if ENABLE_DEBUG_PRINTK
		printk("BAT_ChargingCurrent_softStart:%d on %d\n",Cust_CC_300MA,temp_CC_value);
		Show_ChargingCurrent();
	  #endif
	}
	if( temp_CC_value >= Cust_CC_450MA ) 
	{
		pmic_chr_current(Cust_CC_450MA);
		pmic_chr_chr_enable(KAL_TRUE);	
		msleep(g_soft_start_delay);
	  #if ENABLE_DEBUG_PRINTK
		printk("BAT_ChargingCurrent_softStart:%d on %d\n",Cust_CC_450MA,temp_CC_value);
		Show_ChargingCurrent();
	  #endif
	}
	if( temp_CC_value >= Cust_CC_650MA ) 
	{
		pmic_chr_current(Cust_CC_650MA);
		pmic_chr_chr_enable(KAL_TRUE);	
		msleep(g_soft_start_delay);
	  #if ENABLE_DEBUG_PRINTK
		printk("BAT_ChargingCurrent_softStart:%d on %d\n",Cust_CC_650MA,temp_CC_value);
		Show_ChargingCurrent();
	  #endif
	}
	if( temp_CC_value >= Cust_CC_800MA ) 
	{
		pmic_chr_current(Cust_CC_800MA);
		pmic_chr_chr_enable(KAL_TRUE);	
		msleep(g_soft_start_delay);
	  #if ENABLE_DEBUG_PRINTK
		printk("BAT_ChargingCurrent_softStart:%d on %d\n",Cust_CC_800MA,temp_CC_value);
		Show_ChargingCurrent();
	  #endif
	}

	return 0;	
}

void bat_show_time(int now_soc)
{
    struct timeval now;
    
    if ( (BMT_status.SOC % 2) == 0 ) { 
        if (times_check == 1) {         
            times_check = 0;    
            do_gettimeofday(&now);
            printk("SOC=%d, %lu, %lu\n", now_soc, now.tv_sec, now.tv_usec);
        }    
    }
    else {
        times_check = 1;    
    }
}

int getVoltFlag = 0; 

UINT32 BAT_CheckBatteryStatus(void)
{
    UINT16 BAT_status = PMU_STATUS_OK;
    int i = 0;

	/* 1. Get Battery Information */
    BAT_GetVoltage();

#if defined(CONFIG_ADC_SOLUTION)

if (g_CV_CHECK_POINT_EN != 1)
{
	/*Fine Tune VBAT when Charging: start*/
	// 20100731
	//if( pmic_chrdet_status() && (BMT_status.bat_full == FALSE) ) {
	if( pmic_chrdet_status() && (g_HW_Charging_Done == 0) ) {
		// In charging
		pmic_chr_chr_enable(KAL_FALSE);
		getVoltFlag = 1;
	  #if ENABLE_DEBUG_PRINTK	
		printk("s");		
		Show_ChargingCurrent();		

		printk("[Battery:Method2] Enable ON-OFF charging\r\n");
	  #endif		

		//disable charging for free bat temperature
		msleep(g_free_bat_temp);
	}
	
	BMT_status.bat_vol = GetOneChannelValue(AUXADC_BATTERY_VOLTAGE_CHANNEL, 5) / 5;
		
	// 20100813
	//if ( BMT_status.bat_vol > VBAT_CV_CHECK_POINT )
	//{
	//	g_CV_CHECK_POINT_EN = 1;
	//}
		
	if ( getVoltFlag == 1 ){
		
		// 20100731
		if ( BMT_status.bat_charging_state == CHR_CC )
		{
		/* charging current soft start */
		if ( BMT_status.charger_type == STANDARD_HOST ) 
		{
			#if defined(CONFIG_USB_IF)
			BAT_ChargingCurrent_softStart(g_temp_CC_value);
			#else
			BAT_ChargingCurrent_softStart(USB_CHARGER_CURRENT);
			#endif		
		} 
		else if (BMT_status.charger_type == NONSTANDARD_CHARGER) 
		{
			BAT_ChargingCurrent_softStart(USB_CHARGER_CURRENT);
		} 
		else if (BMT_status.charger_type == STANDARD_CHARGER) 
		{
			BAT_ChargingCurrent_softStart(AC_CHARGER_CURRENT);
		} 
		else 
		{
			BAT_ChargingCurrent_softStart(CHR_CURRENT_450MA);
		}
		}
	
		pmic_chr_chr_enable(KAL_TRUE);
		getVoltFlag = 0;
		//printk("&");
	}	
	/*Fine Tune VBAT when Charging: end*/
}
else
{
	#if ENABLE_DEBUG_PRINTK
		printk("[Battery:Method2] Disable ON-OFF charging\r\n");
	#endif
	if( pmic_cv_status() && ( g_CV_CHECK_POINT_Done == 0 ) )
	{
#if 0 
		pmic_chr_chr_enable(KAL_FALSE);
		printk("[Battery] Decrease one current level\r\n");
		
		if ( BMT_status.charger_type == STANDARD_HOST )
		{
			pmic_chr_current(CHR_CURRENT_300MA);
		}
		else if (BMT_status.charger_type == NONSTANDARD_CHARGER)
		{
			pmic_chr_current(CHR_CURRENT_300MA);
		}
		else if (BMT_status.charger_type == STANDARD_CHARGER)
		{
			pmic_chr_current(CHR_CURRENT_450MA);
		}
		else
		{
			pmic_chr_current(CHR_CURRENT_300MA);
		}
#endif

		g_CV_CHECK_POINT_Done = 1;
	}
}

	g_TempBattVoltage = BMT_status.bat_vol;

    BMT_status.SOC = BattVoltToPercent(BMT_status.bat_vol);
	// 20100816
    //if( BMT_status.SOC == 100 )
    //    BMT_status.bat_full = TRUE;   

	/* User smooth View when discharging*/
	if( pmic_chrdet_status() ) {
		// Do nothing
	} else {
		// No charger
		if (BMT_status.bat_vol >= RECHARGING_VOLTAGE) {
		  #if ENABLE_DEBUG_PRINTK
			printk("Discharging:user view-------\r\n");	
		  #endif
			BMT_status.SOC = 100;	
			BMT_status.bat_full = TRUE;
		} 
	}	
#endif

	if (bat_volt_cp_flag == 0) {
		bat_volt_cp_flag = 1;
		bat_volt_check_point = BMT_status.SOC;
	}	

    /**************** Averaging : START ****************/        
    if (!batteryBufferFirst)
    {
        batteryBufferFirst = TRUE;
        
        for (i=0; i<BATTERY_AVERAGE_SIZE; i++) {
            batteryVoltageBuffer[i] = BMT_status.bat_vol;            
            batteryCurrentBuffer[i] = BMT_status.ICharging;            
            batterySOCBuffer[i] = BMT_status.SOC; /* for user view */
        }

        batteryVoltageSum = BMT_status.bat_vol * BATTERY_AVERAGE_SIZE;
        batteryCurrentSum = BMT_status.ICharging * BATTERY_AVERAGE_SIZE;        
        batterySOCSum = BMT_status.SOC * BATTERY_AVERAGE_SIZE; /* for user view */
    }

    batteryVoltageSum -= batteryVoltageBuffer[batteryIndex];
    batteryVoltageSum += BMT_status.bat_vol;
    batteryVoltageBuffer[batteryIndex] = BMT_status.bat_vol;

    batteryCurrentSum -= batteryCurrentBuffer[batteryIndex];
    batteryCurrentSum += BMT_status.ICharging;
    batteryCurrentBuffer[batteryIndex] = BMT_status.ICharging;

    /* 20091028 for user view */
    if (BMT_status.bat_full)
        BMT_status.SOC = 100;
    if (g_bat_full_user_view)
        BMT_status.SOC = 100;    
    batterySOCSum -= batterySOCBuffer[batteryIndex];
    batterySOCSum += BMT_status.SOC;
    batterySOCBuffer[batteryIndex] = BMT_status.SOC;
    
    BMT_status.bat_vol = batteryVoltageSum / BATTERY_AVERAGE_SIZE;
    BMT_status.ICharging = batteryCurrentSum / BATTERY_AVERAGE_SIZE;    
    BMT_status.SOC = batterySOCSum / BATTERY_AVERAGE_SIZE; /* for user view */

    batteryIndex++;
    if (batteryIndex >= BATTERY_AVERAGE_SIZE)
        batteryIndex = 0;
    /**************** Averaging : END ****************/

#if defined(CONFIG_BATTERY_B61UN)
    /* first time */
    if(PrevBatteryTemperature == 100) 
    {
        PrevBatteryTemperature = BMT_status.temperature;
        g_bat_timeout_happen = FALSE;
    }
    /* 10s routine */
    else if( g_bat_timeout_happen ) 
    {        
        BMT_status.temperature = ( BMT_status.temperature + PrevBatteryTemperature ) / 2;        
        PrevBatteryTemperature = BMT_status.temperature;
        g_bat_timeout_happen = FALSE;
    }    
    /* CHRIN interrupt */
    else 
    {
        BMT_status.temperature = PrevBatteryTemperature;
        g_bat_timeout_happen = FALSE;        
    }
#endif

	// 20100816
	if( BMT_status.SOC == 100 )
		BMT_status.bat_full = TRUE;	
	
	// 20100816
	if ( BMT_status.bat_vol > VBAT_CV_CHECK_POINT )
	{
		g_CV_CHECK_POINT_EN = 1;
	}

	//#ifdef CONFIG_DEBUG_MSG
	if (Enable_BATDRV_LOG == 1) {
    	printk(  "[BATTERY:AVG] Temp:%d vbat:%d batsen:%d SOC:%d state:%x chrdet:%d chrtype:%d Vchrin:%d Icharging:%d USB_state:%d\r\n", 
       	BMT_status.temperature ,BMT_status.bat_vol, BMT_status.ADC_BAT_SENSE, BMT_status.SOC, BMT_status.bat_charging_state,
       	pmic_chrdet_status(), CHR_Type_num, BMT_status.charger_vol, BMT_status.ICharging, g_usb_state );            
	}
	//#endif

    BAT_status = BAT_CheckPMUStatusReg();

	#if ENABLE_DEBUG_PRINTK	
	printk("%d,%d,%x,%d<>%d,%d,%d,%d,%d\r\n", BMT_status.bat_vol, BMT_status.SOC, 
	BMT_status.bat_charging_state, BMT_status.ICharging, g_BatteryAverageCurrent,
	BMT_status.temperature, g_ADC_Cali, g_usb_state, g_CV_CHECK_POINT_EN);            
	#endif

    if(BAT_status != PMU_STATUS_OK)
        return PMU_STATUS_FAIL;                  

#if (BAT_TEMP_PROTECT_ENABLE == 1)
	#if ENABLE_DEBUG_PRINTK	
	printk(  "[BATTERY] BAT_TEMP_PROTECT_ENABLE!!\r\n");
	#endif

    if ((BMT_status.temperature >= MAX_CHARGE_TEMPERATURE) || 
        (BMT_status.temperature <= MIN_CHARGE_TEMPERATURE) || 
        (BMT_status.temperature == ERR_CHARGE_TEMPERATURE))
    {
        printk(  "[BAT ERROR] Battery Over / Under Temperature or NTC fail !!\n\r");                
        return PMU_STATUS_FAIL;       
    }		
#endif		    

    if(pmic_chrdet_status())
    {
        /* Re-charging */
		// 20100731
        //if((BMT_status.bat_vol < RECHARGING_VOLTAGE) && (BMT_status.bat_full))
        if((BMT_status.bat_vol < RECHARGING_VOLTAGE) && (BMT_status.bat_full) && (g_HW_Charging_Done == 1) )
        {
            //#ifdef CONFIG_DEBUG_MSG
            if (Enable_BATDRV_LOG == 1) {
            	printk(  "[BATTERY] Battery Re-charging !!\n\r");                
            }
            //#endif
            BMT_status.bat_full = FALSE;    
            g_bat_full_user_view = TRUE; // 20091028
            g_HW_Charging_Done = 0;

			g_CV_CHECK_POINT_EN = 0;
			g_CV_CHECK_POINT_Done = 0;
        }

        /* Note : Trickle and Pre-CC 0.5 hour */
        /* But SW can not control on PMIC Trickle and Pre-CC mode */

#if (V_CHARGER_ENABLE == 1)
	#if ENABLE_DEBUG_PRINTK
		printk(  "[BATTERY] V_CHARGER_ENABLE!!\r\n");
	#endif
        if (BMT_status.charger_vol <= V_CHARGER_MIN )
        {
            printk(  "[BAT ERROR]Charger under voltage!!\r\n");                    
            BMT_status.bat_charging_state = CHR_ERROR;
            return PMU_STATUS_FAIL;        
        }

        if ( BMT_status.charger_vol >= V_CHARGER_MAX )
        {
            printk(  "[BAT ERROR]Charger over voltage !!\r\n");                    
            BMT_status.charger_protect_status = charger_OVER_VOL;
            BMT_status.bat_charging_state = CHR_ERROR;
            return PMU_STATUS_FAIL;        
        }
#endif
		
    }
        
    return PMU_STATUS_OK;
}

PMU_STATUS BAT_TrickleConstantCurrentModeAction(void)
{    
    #ifdef CONFIG_DEBUG_MSG
    printk(("[BATTERY] Trickle-Charge mode, timer=%d Trickle=%d!!\n\r"),
                BMT_status.total_charging_time, BMT_status.PRE_charging_time);
    #endif
    
    BMT_status.total_charging_time += BAT_TASK_PERIOD;
    BMT_status.PRE_charging_time += BAT_TASK_PERIOD;

    return PMU_STATUS_OK;
}

PMU_STATUS BAT_PreConstantCurrentModeAction(void)
{
    #ifdef CONFIG_DEBUG_MSG
    printk(  "[BATTERY] Pre-Charge mode, timer=%d Pre=%d!!\n\r",
    BMT_status.total_charging_time, BMT_status.PRE_charging_time);
    #endif

    BMT_status.total_charging_time += BAT_TASK_PERIOD;
    BMT_status.PRE_charging_time += BAT_TASK_PERIOD;                    

    return PMU_STATUS_OK;        
}    

void BAT_SetUSBState(int usb_state_value)
{
	if ( (usb_state_value < USB_SUSPEND) || ((usb_state_value > USB_CONFIGURED))){
		printk("[BATTERY] BAT_SetUSBState Fail! Restore to default value\r\n");	
		usb_state_value = USB_UNCONFIGURED;
	} else {
		printk("[BATTERY] BAT_SetUSBState Success! Set %d\r\n", usb_state_value);	
		g_usb_state = usb_state_value;	
	}
}
EXPORT_SYMBOL(BAT_SetUSBState);

PMU_STATUS BAT_ConstantCurrentModeAction(void)
{
	int temp_CC_value = 0;

    if (Enable_BATDRV_LOG == 1) {
    	printk(  "[BATTERY] CC mode charge, timer=%d !!\n\r",BMT_status.total_charging_time);    
    }

    BMT_status.PRE_charging_time = 0;    
    BMT_status.total_charging_time += BAT_TASK_PERIOD;                    
    
    /* Kick Charging WDT */    
    pmic6326_kick_charger_wdt();
    pmic_wdt_enable(KAL_TRUE);
    pmic_wdt_timeout(WDT_TIMEOUT_32_SEC);      

    if (g_ftm_battery_flag) {
        
        pmic_chr_current(charging_level_data[0]);
        printk("[BATTERY] FTM charging : %d\r\n", charging_level_data[0]);
        
    } else {
    
        if ( BMT_status.charger_type == STANDARD_HOST ) {
                        
#if defined(CONFIG_USB_IF)
	#if ENABLE_DEBUG_PRINTK		
		printk("Support CONFIG_USB_IF !\r\n");
	#endif		
			if (g_usb_state == USB_SUSPEND) 
			{
				if (USB_CHARGER_CURRENT_SUSPEND == Cust_CC_0MA) 
				{
					#if ENABLE_DEBUG_PRINTK
						printk("Disable Charging on USB SUSPEND state !\r\n");
					#endif
					pmic_chr_chr_enable(KAL_FALSE);
					
					temp_CC_value = Cust_CC_0MA;
					g_temp_CC_value = temp_CC_value;
					
					return PMU_STATUS_OK;
				} 
				else 
				{
					pmic_chr_current(USB_CHARGER_CURRENT_SUSPEND);
					temp_CC_value = USB_CHARGER_CURRENT_SUSPEND;
				}
			} 
			else if (g_usb_state == USB_UNCONFIGURED) 
			{
				if (USB_CHARGER_CURRENT_UNCONFIGURED == Cust_CC_0MA) 
				{
					#if ENABLE_DEBUG_PRINTK
						printk("Disable Charging on USB UNCONFIGURED sate !\r\n");
					#endif
					pmic_chr_chr_enable(KAL_FALSE);
					
					temp_CC_value = Cust_CC_0MA;
					g_temp_CC_value = temp_CC_value;
					
					return PMU_STATUS_OK;
				} 
				else 
				{
				#if ENABLE_DEBUG_PRINTK
					printk("Set Charging on USB UNCONFIGURED sate %d !\r\n", USB_CHARGER_CURRENT_UNCONFIGURED);
				#endif
				pmic_chr_current(USB_CHARGER_CURRENT_UNCONFIGURED);	
				temp_CC_value = USB_CHARGER_CURRENT_UNCONFIGURED;
				}
			} 
			else if (g_usb_state == USB_CONFIGURED) 
			{
				if (USB_CHARGER_CURRENT_CONFIGURED == Cust_CC_0MA) 
				{
				#if ENABLE_DEBUG_PRINTK
					printk("Disable Charging on USB CONFIGURED sate !\r\n");
				#endif
					pmic_chr_chr_enable(KAL_FALSE);
					
					temp_CC_value = Cust_CC_0MA;
					g_temp_CC_value = temp_CC_value;
					
					return PMU_STATUS_OK;
				} 
				else 
				{
				#if ENABLE_DEBUG_PRINTK
					printk("Set Charging on USB CONFIGURED sate %d !\r\n", USB_CHARGER_CURRENT_CONFIGURED);
				#endif
				pmic_chr_current(USB_CHARGER_CURRENT_CONFIGURED);	
				temp_CC_value = USB_CHARGER_CURRENT_CONFIGURED;
				}
			} 
			else {
				#if ENABLE_DEBUG_PRINTK
				printk("Disable Charging on USB Unknown sate !\r\n");
				#endif
				pmic_chr_chr_enable(KAL_FALSE);
					
				temp_CC_value = Cust_CC_0MA;
				g_temp_CC_value = temp_CC_value;
					
				return PMU_STATUS_OK;	
			}
			
			if (Enable_BATDRV_LOG == 1) {
            	printk("[BATTERY] STANDARD_HOST CC mode charging : %d\r\n", temp_CC_value);
            }			
#else
			#if ENABLE_DEBUG_PRINTK
			printk("NOT support CONFIG_USB_IF !\r\n");
			#endif
			if (USB_CHARGER_CURRENT == Cust_CC_0MA) 
			{
				printk("Disable Charging when use USB !\r\n");
				pmic_chr_chr_enable(KAL_FALSE);
					
				temp_CC_value = Cust_CC_0MA;
				g_temp_CC_value = temp_CC_value;
								
				return PMU_STATUS_OK;
			}
			else
			{
			pmic_chr_current(USB_CHARGER_CURRENT);
			}
			
			if (Enable_BATDRV_LOG == 1) {
            	printk("[BATTERY] STANDARD_HOST CC mode charging : %d\r\n", USB_CHARGER_CURRENT);
            }
#endif

        } else if (BMT_status.charger_type == NONSTANDARD_CHARGER) {    
            //#ifdef CONFIG_DEBUG_MSG
            if (Enable_BATDRV_LOG == 1) {
            	//printk("[BATTERY] NONSTANDARD_CHARGER CC mode charging : %d\r\n", AC_CHARGER_CURRENT);
            	printk("[BATTERY] NONSTANDARD_CHARGER CC mode charging : %d\r\n", USB_CHARGER_CURRENT); // USB HW limitation
            }
            //#endif
            //pmic_chr_current(AC_CHARGER_CURRENT);
            pmic_chr_current(USB_CHARGER_CURRENT); // USB HW limitation
        } else if (BMT_status.charger_type == STANDARD_CHARGER) {
            //#ifdef CONFIG_DEBUG_MSG
            if (Enable_BATDRV_LOG == 1) {
            	printk("[BATTERY] STANDARD_CHARGER CC mode charging : %d\r\n", AC_CHARGER_CURRENT);
            }
            //#endif
            pmic_chr_current(AC_CHARGER_CURRENT);
        } else {
            //#ifdef CONFIG_DEBUG_MSG
            if (Enable_BATDRV_LOG == 1) {
            	printk("[BATTERY] Default CC mode charging : %d\r\n", CHR_CURRENT_450MA);
            }
            //#endif
            pmic_chr_current(CHR_CURRENT_450MA); 
        }
        
    }

	g_temp_CC_value = temp_CC_value;

    pmic_chr_chr_enable(KAL_TRUE);
    pmic_adc_vbat_enable(KAL_TRUE);
    pmic_adc_isense_enable(KAL_TRUE);        
    
    return PMU_STATUS_OK;        
}    

PMU_STATUS BAT_ConstantVoltageModeAction(void)
{
    //#ifdef CONFIG_DEBUG_MSG   
    if (Enable_BATDRV_LOG == 1) {
    	printk(  "[BATTERY] CV mode charge, timer=%d CV=%d!!\n\r",
    	BMT_status.total_charging_time,BMT_status.CV_charging_time);    
    }
    //#endif
 
    BMT_status.total_charging_time += BAT_TASK_PERIOD;
    BMT_status.CV_charging_time += BAT_TASK_PERIOD;                    
    
    /* Kick Charging WDT */    
    pmic6326_kick_charger_wdt();
    pmic_wdt_enable(KAL_TRUE);
    pmic_wdt_timeout(WDT_TIMEOUT_32_SEC);  
	
    pmic_chr_chr_enable(KAL_TRUE);
    pmic_adc_vbat_enable(KAL_TRUE);
    pmic_adc_isense_enable(KAL_TRUE);        

    return PMU_STATUS_OK;        
}    

PMU_STATUS BAT_BatteryFullAction(void)
{
    #ifdef CONFIG_DEBUG_MSG    
    printk(  "[BATTERY] Battery full !!\n\r");            
    #endif
    
    BMT_status.bat_full = TRUE;
    BMT_status.total_charging_time = 0;
    BMT_status.CV_charging_time = 0;
    BMT_status.PRE_charging_time = 0;
    BMT_status.CV_low_current_time = 0;

	g_HW_Charging_Done = 1;

    /*  Disable charger*/
    pmic_chr_chr_enable(KAL_FALSE);
    
    return PMU_STATUS_OK;
}

PMU_STATUS BAT_ChargingOTAction(void)
{
    #ifdef CONFIG_DEBUG_MSG
    printk(  "[BATTERY] Charging over 8 hr stop !!\n\r");            
    #endif

    BMT_status.bat_full = TRUE;
    BMT_status.total_charging_time = 0;
    BMT_status.CV_charging_time = 0;
    BMT_status.PRE_charging_time = 0;
    BMT_status.CV_low_current_time = 0;

	g_HW_Charging_Done = 1;
	g_Charging_Over_Time = 1;

    /*  Disable charger*/
    pmic_chr_chr_enable(KAL_FALSE);
  
    return PMU_STATUS_OK;
}

PMU_STATUS BAT_BatteryStatusFailAction(void)
{
    #ifdef CONFIG_DEBUG_MSG    
    printk(  "[BATTERY] BAD Battery status... Charging Stop !!\n\r");            
    #endif

    BMT_status.total_charging_time = 0;
    BMT_status.CV_charging_time = 0;
    BMT_status.PRE_charging_time = 0;
    BMT_status.CV_low_current_time = 0;

    /*  Disable charger*/
    pmic_chr_chr_enable(KAL_FALSE);

    return PMU_STATUS_OK;
}

extern void MT6516_EINTIRQUnmask(unsigned int line);

void BAT_thread(void)
{
    int BAT_status = 0;
    kal_uint8 tmp8;
    
    /* PMIC Read Clear for pmic_int_status_3 */
    tmp8 = pmic_int_status_3();
    MT6516_EINTIRQUnmask(0);    

    //#ifdef CONFIG_DEBUG_MSG
	if (Enable_BATDRV_LOG == 1) {
    	printk("[BATTERY] LOG ---------------------------------------------------------------------\n");
	}
    //#endif

    /* charger exist */    
    if( pmic_chrdet_status() )
    {
        #ifdef CONFIG_DEBUG_MSG            
        printk("[BATTERY] charger exist!!\n");    
        #endif
        wake_lock(&battery_suspend_lock);        

        /* Get Charger Type */
        if(g_usb_power_saving){
            CHR_Type_num = mt6516_usb_charger_type_detection();      
        }
        else{
            CHR_Type_num = STANDARD_HOST;
        }
		
        switch(CHR_Type_num)
        {
            case CHARGER_UNKNOWN :                
                #ifdef CONFIG_DEBUG_MSG
                printk("[BATTERY] CHR_Type_num : CHARGER_UNKNOWN\r\n");            
                #endif
                break;
            
            case STANDARD_HOST :
                BMT_status.charger_type = STANDARD_HOST;
                if(g_usb_power_saving)
                mt6516_usb_connect();
                break;
            
            case CHARGING_HOST :
                break;
            
            case NONSTANDARD_CHARGER :
                BMT_status.charger_type = NONSTANDARD_CHARGER;
                break;
            
            case STANDARD_CHARGER :
                BMT_status.charger_type = STANDARD_CHARGER;
                break;
            
            default :
                #ifdef CONFIG_DEBUG_MSG    
                printk("[BATTERY] CHR_Type_num : ERROR\r\n");    
                #endif
                break;
        }        
    }
    /* No Charger */
    else 
    {                
        #ifdef CONFIG_DEBUG_MSG            
        printk("[BATTERY] charger NOT exist!!\n");    
        #endif

        BMT_status.charger_type = CHARGER_UNKNOWN;
        BMT_status.bat_full = FALSE;
        g_bat_full_user_view = FALSE;

		g_HW_Charging_Done = 0;
		g_Charging_Over_Time = 0;

		g_CV_CHECK_POINT_EN = 0;
		g_CV_CHECK_POINT_Done = 0;

        if(g_usb_power_saving)
        mt6516_usb_disconnect();
		#if ENABLE_DEBUG_PRINTK
		printk("[BATTERY] No charger : Set g_usb_state = USB_UNCONFIGURED !\r\n");
		#endif
		g_usb_state = USB_UNCONFIGURED;
		
        wake_unlock(&battery_suspend_lock);
        
    }
    
    /* Check Battery Status */
    BAT_status = BAT_CheckBatteryStatus();
    if( BAT_status == PMU_STATUS_FAIL )
        g_Battery_Fail = TRUE;
    else
        g_Battery_Fail = FALSE;
    
    /* ac information update for Android */
    mt6516_ac_update(&mt6516_ac_main);
    
    /* ac information update for Android */
    mt6516_usb_update(&mt6516_usb_main);
    
    /* battery information update for Android */   
    mt6516_battery_update(&mt6516_battery_main);   
    
    /* No Charger */
    if(BAT_status == PMU_STATUS_FAIL || g_Battery_Fail)    
    {
        BAT_BatteryStatusFailAction();
    }
	
	/* HW charging done, real stop charging */
	else if (g_HW_Charging_Done == 1)
    {        
        BAT_BatteryFullAction();
		
		if (Enable_BATDRV_LOG == 1) {
			printk("[BATTERY] Battery real full. \n");
		}
	}

	else if (g_Charging_Over_Time == 1)
	{
		/*  Disable charger*/
		pmic_chr_chr_enable(KAL_FALSE);
		
		if (Enable_BATDRV_LOG == 1) {
			printk("[BATTERY] Charging Over Time. \n");
    }
	}
	
    /* Battery Not Full and Charger exist : Do Charging */
    else
    {
        /* Charging 8hr */
        if(BMT_status.total_charging_time >= MAX_CHARGING_TIME)
        {
            BAT_ChargingOTAction();
        }

		// 20100730
		/* charging full condition when charging current < CHARGING_FULL_CURRENT mA on CV mode*/
		if ( (BMT_status.bat_charging_state == CHR_CV ) &&
			(BMT_status.ICharging <= CHARGING_FULL_CURRENT) &&
				(g_BatteryAverageCurrent <= CHARGING_FULL_CURRENT)             
             )
		{
		    BAT_BatteryFullAction();				
	            printk("[BATTERY] Battery real full and disable charging on %d mA \n", g_BatteryAverageCurrent); 
        }		

#if 0
        /* Charging 30mins after current < 100mA in CV mode */
        if (( BMT_status.bat_charging_state == CHR_CV ) &&
             (BMT_status.ICharging <= MAX_CV_CHARGING_CURRENT))
        {
            #ifdef CONFIG_DEBUG_MSG            
            printk(  "[BATTERY] Check if Charging 30mins after current < 100mA in CV mode : %d\n\r",  \
                        BMT_status.CV_low_current_time );			    		    
            #endif
            if( BMT_status.CV_low_current_time >=  MAX_CV_LOW_CURRENT_TIME)
            {
                BAT_BatteryFullAction();
            }
            else
            {
                BMT_status.CV_low_current_time += BAT_TASK_PERIOD; 
            }            
        }
        else
        {
            BMT_status.CV_low_current_time = 0;
        }
#endif		
        
        /* CV charging 3hr */
        if (( BMT_status.bat_charging_state == CHR_CV ) &&
             (BMT_status.CV_charging_time >= MAX_CV_CHARGING_TIME))
        {
            BAT_BatteryFullAction();
        }

        /* Charging flow begin */
        switch(BMT_status.bat_charging_state)
        {
            case CHR_TICKLE : 
                /* PMIC HW control */                
                break;    
                
            case CHR_PRE :
                /* PMIC HW control */
                break;    
                
            case CHR_CC :
                BAT_ConstantCurrentModeAction();
                break;    
                
            case CHR_CV :
                BAT_ConstantVoltageModeAction();
                break;

            case CHR_BATFULL:
                break;
        }    

		//#ifdef CONFIG_DEBUG_MSG
        //if(BMT_status.ADC_I_SENSE < BMT_status.bat_vol)
        //    printk("[BAT ERROR] Visen < Vbat:vbat:%dmV isen:%dmV \r\n", BMT_status.bat_vol, BMT_status.ADC_I_SENSE );
		//#endif
    }
   
}

static int bat_thread_kthread(void *x)
{
    /* Run on a process content */  
    while (1) {           
        mutex_lock(&bat_mutex);
        BAT_thread();                      
        mutex_unlock(&bat_mutex);        
        wait_event(bat_thread_wq, bat_thread_timeout);
        bat_thread_timeout=0;
    }

    return 0;
}

void bat_thread_wakeup(UINT16 i)
{
#if defined(CONFIG_BATTERY_B61UN)
    g_bat_timeout_happen = TRUE;
#endif
    bat_thread_timeout = 1;
    wake_up(&bat_thread_wq);
}

void BatThread_XGPTConfig(void)
{    
    GPT_CONFIG config;
    GPT_NUM  gpt_num = GPT1;    
    GPT_CLK_DIV clkDiv = GPT_CLK_DIV_128;

    GPT_Init (gpt_num, bat_thread_wakeup);
    config.num = gpt_num;
    config.mode = GPT_REPEAT;
    config.clkDiv = clkDiv;
    config.u4Timeout = 10*128;
    
    if (GPT_Config(config) == FALSE )
        return;                       
        
    GPT_Start(gpt_num);  

    return ;
}

int GetOneChannelValue(int dwChannel, int deCount)
{
    UINT32 u4Sample_times = 0;
    UINT32 dat = 0;
    UINT32 u4channel_0 = 0;    
    UINT32 u4channel_1 = 0;    
    UINT32 u4channel_2 = 0;    
    UINT32 u4channel_3 = 0;    
    UINT32 u4channel_4 = 0;    
    UINT32 u4channel_5 = 0;    
    UINT32 u4channel_6 = 0;    
    UINT32 u4channel_7 = 0;    
    UINT32 u4channel_8 = 0;    
    UINT32 temp_value = 0;

    /* Enable ADC power bit */    
    mt6516_ADC_power_up();

    /* Initialize ADC control register */
    DRV_WriteReg(AUXADC_CON0, 0);
    DRV_WriteReg(AUXADC_CON1, 0);    
    DRV_WriteReg(AUXADC_CON2, 0);    
    DRV_WriteReg(AUXADC_CON3, 0);   

    do
    {
        pmic_adc_vbat_enable(KAL_TRUE);
        pmic_adc_isense_enable(KAL_TRUE); 

        DRV_WriteReg(AUXADC_CON1, 0);        
        DRV_WriteReg(AUXADC_CON1, 0x1FF);
         
        DVT_DELAYMACRO(1000);

        /* Polling until bit STA = 0 */
        while (0 != (0x01 & DRV_Reg(AUXADC_CON3)));          

        dat = DRV_Reg(AUXADC_DAT0);        
        u4channel_0 += dat;
        dat = DRV_Reg(AUXADC_DAT1);        
        u4channel_1 += dat;   
        dat = DRV_Reg(AUXADC_DAT2);        
        u4channel_2 += dat;   
        dat = DRV_Reg(AUXADC_DAT3);        
        u4channel_3 += dat;   
        dat = DRV_Reg(AUXADC_DAT4);
        u4channel_4 += dat;
        dat = DRV_Reg(AUXADC_DAT5);
        u4channel_5 += dat;
        dat = DRV_Reg(AUXADC_DAT6);
        u4channel_6 += dat;  
        dat = DRV_Reg(AUXADC_DAT7);
        u4channel_7 += dat;
        dat = DRV_Reg(AUXADC_DAT8);
        u4channel_8 += dat;    
        
        u4Sample_times++;
    }
    while (u4Sample_times < deCount);

    #if 0
    printk("BAT_GetVoltage : channel_0 = %d \n", u4channel_0 );
    printk("BAT_GetVoltage : channel_1 = %d \n", u4channel_1 );
    printk("BAT_GetVoltage : channel_2 = %d \n", u4channel_2 );
    printk("BAT_GetVoltage : channel_3 = %d \n", u4channel_3 );
    printk("BAT_GetVoltage : channel_4 = %d \n", u4channel_4 );
    printk("BAT_GetVoltage : channel_5 = %d \n", u4channel_5 );
    printk("BAT_GetVoltage : channel_6 = %d \n", u4channel_6 );
    printk("BAT_GetVoltage : channel_7 = %d \n", u4channel_7 );
    printk("BAT_GetVoltage : channel_8 = %d \n", u4channel_8 );
    #endif
    
    /* Disable ADC power bit */    
    //mt6516_ADC_power_down();

    /* Phone */
    if (dwChannel==0) {
        temp_value = ( ( u4channel_0*2800/1024 )*R_I_SENSE );
        if (g_ADC_Cali) {
            temp_value = ((temp_value * (*(adc_cali_slop+0)))+(*(adc_cali_offset+0)))/1000;
        }
        return temp_value;
       }
    else if (dwChannel==1) {        
        temp_value = ( ( u4channel_1*2800/1024 )*R_BAT_SENSE );
        if (g_ADC_Cali) {
            temp_value = ((temp_value * (*(adc_cali_slop+1)))+(*(adc_cali_offset+1)))/1000;
        }
        return temp_value;
    }
    else if (dwChannel==2) {
        temp_value = ( u4channel_2*2800/1024 ); 
        if (g_ADC_Cali) {
            temp_value = ((temp_value * (*(adc_cali_slop+2)))+(*(adc_cali_offset+2)))/1000;
        }
        return temp_value;
    }
    else if (dwChannel==3) {
        temp_value = ( ( u4channel_3*2800/1024 )*R_CHARGER_SENSE );
        if (g_ADC_Cali) {
            temp_value = ((temp_value * (*(adc_cali_slop+3)))+(*(adc_cali_offset+3)))/1000;
        }
        return temp_value;
    }
    else if (dwChannel==4) {
        temp_value = ( u4channel_4*2800/1024 );
        if (g_ADC_Cali) {
            temp_value = ((temp_value * (*(adc_cali_slop+4)))+(*(adc_cali_offset+4)))/1000;
        }
        return temp_value;
    }
    else if (dwChannel==5) {
        temp_value = ( u4channel_5*2800/1024 );
        if (g_ADC_Cali) {
            temp_value = ((temp_value * (*(adc_cali_slop+5)))+(*(adc_cali_offset+5)))/1000;
        }
        return temp_value;        
    }
    else if (dwChannel==6) {
        temp_value = ( u4channel_6*2800/1024 );  
        if (g_ADC_Cali) {
            temp_value = ((temp_value * (*(adc_cali_slop+6)))+(*(adc_cali_offset+6)))/1000;
        }
        return temp_value;
    }
    else if (dwChannel==7) {
        temp_value = ( u4channel_7*2800/1024 );
        if (g_ADC_Cali) {
            temp_value = ((temp_value * (*(adc_cali_slop+7)))+(*(adc_cali_offset+7)))/1000;
        }
        return temp_value;
    }
    else if (dwChannel==8) {
        temp_value = ( u4channel_8*2800/1024 );
        if (g_ADC_Cali) {
            temp_value = ((temp_value * (*(adc_cali_slop+8)))+(*(adc_cali_offset+8)))/1000;
        }
        return temp_value;
    }
    else{        
        return -1;
    }
}
EXPORT_SYMBOL(GetOneChannelValue);

static int adc_cali_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    int *user_data_addr;
    int *naram_data_addr;
    int i = 0;
    int ret = 0;

    mutex_lock(&bat_mutex);

    switch(cmd)
    {
        case TEST_ADC_CALI_PRINT :
            g_ADC_Cali = FALSE;
            break;
        
        case SET_ADC_CALI_Slop:            
            naram_data_addr = (int *)arg;
            ret = copy_from_user(adc_cali_slop, naram_data_addr, 36);
            g_ADC_Cali = FALSE; /* enable calibration after setting ADC_CALI_Cal */            
            /* Protection */
            for (i=0;i<9;i++) 
            { 
                if ( (*(adc_cali_slop+i) == 0) || (*(adc_cali_slop+i) == 1) ) {
                    *(adc_cali_slop+i) = 1000;
                }
            }
            //for (i=0;i<9;i++) printk("adc_cali_slop[%d] = %d\n",i , *(adc_cali_slop+i));
            //printk("**** MT6516 adc_cali ioctl : SET_ADC_CALI_Slop Done!\n");            
            break;    
            
        case SET_ADC_CALI_Offset:            
            naram_data_addr = (int *)arg;
            ret = copy_from_user(adc_cali_offset, naram_data_addr, 36);
            g_ADC_Cali = FALSE; /* enable calibration after setting ADC_CALI_Cal */
            //for (i=0;i<9;i++) printk("adc_cali_offset[%d] = %d\n",i , *(adc_cali_offset+i));
            //printk("**** MT6516 adc_cali ioctl : SET_ADC_CALI_Offset Done!\n");            
            break;
            
        case SET_ADC_CALI_Cal :            
            naram_data_addr = (int *)arg;
            ret = copy_from_user(adc_cali_cal, naram_data_addr, 4);
            //g_ADC_Cali = TRUE;
            if ( adc_cali_cal[0] == 1 ) {
                g_ADC_Cali = TRUE;
            } else {
                g_ADC_Cali = FALSE;
            }            
            //for (i=0;i<1;i++) printk("adc_cali_cal[%d] = %d\n",i , *(adc_cali_cal+i));
            //printk("**** MT6516 adc_cali ioctl : SET_ADC_CALI_Cal Done!\n");            
            break;    

        case ADC_CHANNEL_READ:            
            g_ADC_Cali = FALSE; /* 20100508 Infinity */
            user_data_addr = (int *)arg;
            ret = copy_from_user(adc_in_data, user_data_addr, 8); /* 2*int = 2*4 */
            /*ChannelNUm, Counts*/
            adc_out_data[0] = GetOneChannelValue(adc_in_data[0], adc_in_data[1]);
            if (adc_out_data[0]<0)
                adc_out_data[1]=1; /* failed */
            else
                adc_out_data[1]=0; /* success */
            ret = copy_to_user(user_data_addr, adc_out_data, 8);
            //printk("**** ioctl : Channel %d * %d times = %d\n", adc_in_data[0], adc_in_data[1], adc_out_data[0]);            
            break;

        case BAT_STATUS_READ:            
            user_data_addr = (int *)arg;
            ret = copy_from_user(battery_in_data, user_data_addr, 4); 
            /* [0] is_CAL */
            if (g_ADC_Cali) {
                battery_out_data[0] = 1;
            } else {
                battery_out_data[0] = 0;
            }
            ret = copy_to_user(user_data_addr, battery_out_data, 4); 
            //printk("**** ioctl : CAL:%d\n", battery_out_data[0]);                        
            break;        

        case Set_Charger_Current: /* For Factory Mode*/
            user_data_addr = (int *)arg;
            ret = copy_from_user(charging_level_data, user_data_addr, 4);
            g_ftm_battery_flag = TRUE;            
            wake_up_bat();
            //printk("**** ioctl : set_Charger_Current:%d\n", charging_level_data[0]);
            break;
          
        default:
            g_ADC_Cali = FALSE;
            break;
    }

    mutex_unlock(&bat_mutex);
    
    return 0;
}

static int adc_cali_open(struct inode *inode, struct file *file)
{ 
   return 0;
}

static int adc_cali_release(struct inode *inode, struct file *file)
{
    return 0;
}

static struct file_operations adc_cali_fops = {
    .owner        = THIS_MODULE,
    .ioctl        = adc_cali_ioctl,
    .open        = adc_cali_open,
    .release    = adc_cali_release,    
};

void AdcVoltageSetTriggerWay(kal_bool bHighTrigger)   //default 0
{
    kal_uint16    regValue;
    regValue = DRV_Reg(AUXADC_DET_VOLT);
   
    if (bHighTrigger)
        regValue |= (1L << AUXADC_VOLT_THRESHOLD_offset);     //make sure the bit is 1
    else 
        regValue &= ~(1L << AUXADC_VOLT_THRESHOLD_offset);     //make sure the bit is 0      
    
    DRV_WriteReg(AUXADC_DET_VOLT, regValue);
}

kal_bool AdcVoltageSetThreshold(kal_int16 nVolt)       //default 0: 0~9 bit
{
    kal_uint16  regValue;

    if (0x03FF < nVolt)     //[Maximum voltage: 0x03FF * 2.8/1024 = 2.8(V)]
        return KAL_FALSE;   //Out of boundary
    
    regValue = DRV_Reg(AUXADC_DET_VOLT);
    regValue &= ~(AUXADC_VOLT_INV_mask);   //clean the bits
    regValue |= nVolt;
    DRV_WriteReg(AUXADC_DET_VOLT, regValue);
    
    return KAL_TRUE;
}

kal_bool AdcVoltageSetChannel(AdcChannelType channelNo)
{
    // Only one channel is allowed to be selected
    if ((NotAdcChannel < channelNo) && (_AdcChannel_MAX >= channelNo))
    {
        DRV_WriteReg(AUXADC_DET_SEL, (int)channelNo);    
        return KAL_TRUE;
    }

    return KAL_FALSE;
}

kal_bool AdcVoltageDebounceTimes(kal_uint16 nTimes) //default 0: 0~13 bit
{
    if (0x3fff < nTimes)   //[Maximum value: 0x3fff ]
        return KAL_FALSE; //This value is invalid!

    DRV_WriteReg(AUXADC_DET_DEBT, nTimes);        
    return KAL_TRUE;    
}

kal_bool AdcVoltageDetectPeriod(kal_uint16 nPeriod) //default 0: 0~13 bit
{
    if (0x3fff < nPeriod) //[Maximum value: 0x3fff / 32k = 0.51196 (sec) = 511.96 (msec)]
        return KAL_FALSE; //This value is invalid!

    DRV_WriteReg(AUXADC_DET_PERIOD, nPeriod);        
    return KAL_TRUE;
}

static irqreturn_t lowbat_irq_handler(int irq, void *dev_id)
{
    //int i;

    printk("lowbat_irq_handler-\r\n");
    
    return IRQ_HANDLED;
}

extern void MT6516_IRQSensitivity(unsigned char code, unsigned char edge);
const kal_uint16 gDefaultDetPeriod     = 249;  // 0.25 sec (Maximum 499)
const kal_uint16 gDefaultDebounceTimes = 10;   // Repeat 10 times

kal_bool ADC_SET_THRESHOLD(kal_bool bLowerTrigger, 
                            kal_uint16 nDeciVoltThreshold/*DeciVoltage*/, 
                               AdcChannelType noChannel, 
                               kal_uint16 pnMiSecDetPeriod, 
                               kal_uint16 pnDetDebounceTimes)
{
    kal_uint16 nRegVoltThreshold, nRegDetPeriod, nRegDetDebounce;

   /* Maximum voltage value 2.8V
       * Maximum period time value 0.499 969sec
       * Maximum debounce times 0x3FFF
       */    

    /* Low voltage detection setting */
    if (bLowerTrigger)
        AdcVoltageSetTriggerWay(KAL_FALSE);
    else
        AdcVoltageSetTriggerWay(KAL_TRUE);

    
    //nRegVoltThreshold = (nDeciVoltThreshold*1024/28);  // nVoltRegValue * 2.8 / 1024 = Volt
    nRegVoltThreshold = ((nDeciVoltThreshold*1024/2)/2800);    
    if (KAL_TRUE != AdcVoltageSetThreshold(nRegVoltThreshold))
        return KAL_FALSE;
    
    if (KAL_TRUE != AdcVoltageSetChannel(noChannel))
        return KAL_FALSE;

    nRegDetDebounce = pnDetDebounceTimes;
    if (!AdcVoltageDebounceTimes(nRegDetDebounce)) 
        return KAL_FALSE;

    /* Register IRQ */
    MT6516_IRQSensitivity(MT6516_LOWBAT_IRQ_LINE, MT6516_EDGE_SENSITIVE);        
    if(request_irq(MT6516_LOWBAT_IRQ_LINE, lowbat_irq_handler, IRQF_DISABLED, "LOWBAT", NULL)){
        printk(KERN_ERR"LOWBAT IRQ LINE NOT AVAILABLE!!\n");
    }

    nRegDetPeriod = pnMiSecDetPeriod;
    if (! AdcVoltageDetectPeriod(nRegDetPeriod*32*1024 / 1000))     // 32K clock
        return KAL_FALSE;   

    #if 0
    /* Dump register */
    printk("AUXADC_DET_VOLT   : %x\r\n",DRV_Reg(AUXADC_DET_VOLT));
    printk("AUXADC_DET_SEL    : %x\r\n",DRV_Reg(AUXADC_DET_SEL));
    printk("AUXADC_DET_PERIOD : %x\r\n",DRV_Reg(AUXADC_DET_PERIOD));
    printk("AUXADC_DET_DEBT   : %x\r\n",DRV_Reg(AUXADC_DET_DEBT));
    #endif

    return KAL_TRUE;
}

static struct proc_dir_entry *proc_entry;
static char proc_bat_data[32];  

ssize_t bat_log_write( struct file *filp, const char __user *buff,
                        unsigned long len, void *data )
{
	if (copy_from_user( &proc_bat_data, buff, len )) {
		printk("bat_log_write error.\n");
		return -EFAULT;
	}

	if (proc_bat_data[0] == '1') {
		printk("enable battery driver log system\n");
		Enable_BATDRV_LOG = 1;
	} else {
		printk("Disable battery driver log system\n");
		Enable_BATDRV_LOG = 0;
	}
	
	return len;
}

int init_proc_log(void)
{
	int ret=0;
	proc_entry = create_proc_entry( "batdrv_log", 0644, NULL );
	
	if (proc_entry == NULL) {
		ret = -ENOMEM;
	  	printk("init_proc_log: Couldn't create proc entry\n");
	} else {
		proc_entry->write_proc = bat_log_write;
		proc_entry->owner = THIS_MODULE;
		printk("init_proc_log loaded.\n");
	}
  
	return ret;
}

static struct proc_dir_entry *proc_entry_battemp;
static char proc_battemp_data[32];	

ssize_t BatTempProtectEn_write( struct file *filp, const char __user *buff,
						unsigned long len, void *data )
{
	if (copy_from_user( &proc_battemp_data, buff, len )) {
		printk("BatTempProtectEn_write error.\n");
		return -EFAULT;
	}

	if (proc_battemp_data[0] == '1') {
		printk("Enable Battery Temperature Protection\n");
		g_BatTempProtectEn= 1;
	} else {
		printk("Disable Battery Temperature Protection\n");
		g_BatTempProtectEn = 1;		/*enable temperature whether app enable or not*/
	}
	
	return len;
}

int init_proc_BatTempProtectEn(void)
{
	int ret=0;
	proc_entry_battemp = create_proc_entry( "BatTempProtectEn", 0644, NULL );
	
	if (proc_entry_battemp == NULL) {
		ret = -ENOMEM;
		printk("init_proc_BatTempProtectEn: Couldn't create proc entry\n");
	} else {
		proc_entry_battemp->write_proc = BatTempProtectEn_write;
		proc_entry_battemp->owner = THIS_MODULE;
		printk("init_proc_BatTempProtectEn loaded.\n");
	}
  
	return ret;
}

/* This function is called by i2c_detect */
int mt6516_smart_battery_detect(struct i2c_adapter *adapter, int address, int kind)
{
    struct class_device *class_dev = NULL;
    int err = 0;
    int ret = 0;
    int i = 0;
    
    /* Integrate with NVRAM */
    ret = alloc_chrdev_region(&adc_cali_devno, 0, 1, ADC_CALI_DEVNAME);
    if (ret) 
        printk("Error: Can't Get Major number for adc_cali \n");
    adc_cali_cdev = cdev_alloc();
    adc_cali_cdev->owner = THIS_MODULE;
    adc_cali_cdev->ops = &adc_cali_fops;
    ret = cdev_add(adc_cali_cdev, adc_cali_devno, 1);
    if(ret)
        printk("adc_cali Error: cdev_add\n");
    adc_cali_major = MAJOR(adc_cali_devno);
    adc_cali_class = class_create(THIS_MODULE, ADC_CALI_DEVNAME);
    class_dev = (struct class_device *)device_create(adc_cali_class, 
                                                    NULL, 
                                                    adc_cali_devno, 
                                                    NULL, 
                                                    ADC_CALI_DEVNAME);
    
    printk(" mt6516_smart_battery_detect !!\n ");

    if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
        goto exit;

    if (!(new_client = kmalloc(sizeof(struct i2c_client), GFP_KERNEL))) {
        err = -ENOMEM;
        goto exit;
    }    
    memset(new_client, 0, sizeof(struct i2c_client));

    new_client->addr = address;
    new_client->adapter = adapter;
    new_client->driver = &mt6516_smart_battery_driver;
    new_client->flags = 0;
    
    strncpy(new_client->name, "smart_battery", I2C_NAME_SIZE);

    if ((err = i2c_attach_client(new_client)))
        goto exit_kfree;

    #if 0
    /* For test in /sys/bus/i2c/devices/0-00aa */ 
    ret = sysfs_create_bin_file(&new_client->dev.kobj, &mt6516_smart_battery_attr_read);
    ret = sysfs_create_bin_file(&new_client->dev.kobj, &mt6516_smart_battery_attr_read_device_type);  
    ret = sysfs_create_bin_file(&new_client->dev.kobj, &mt6516_smart_battery_attr_read_firmware_version); 
    ret = sysfs_create_bin_file(&new_client->dev.kobj, &attr_BATStatusUpdate);  
    #endif

     /* Like Probe Function */
    ret = power_supply_register(&new_client->dev, &mt6516_ac_main.psy);
    if (ret)
    {            
        printk("[MT6516 BAT_probe] power_supply_register AC Fail !!\n");                    
        return ret;
    }             
    printk("[MT6516 BAT_probe] power_supply_register AC Success !!\n");

    ret = power_supply_register(&new_client->dev, &mt6516_usb_main.psy);
    if (ret)
    {            
        printk("[MT6516 BAT_probe] power_supply_register USB Fail !!\n");                    
        return ret;
    }             
    printk("[MT6516 BAT_probe] power_supply_register USB Success !!\n");

    ret = power_supply_register(&new_client->dev, &mt6516_battery_main.psy);
    if (ret)
    {
        printk("[MT6516 BAT_probe] power_supply_register Battery Fail !!\n");
        return ret;
    }
    printk("[MT6516 BAT_probe] power_supply_register Battery Success !!\n");

     /* Initialization BMT Struct */
    for (i=0; i<BATTERY_AVERAGE_SIZE; i++) {
        batteryCurrentBuffer[i] = 0;
        batteryVoltageBuffer[i] = 0; 
    }
    batteryVoltageSum = 0;
    batteryCurrentSum = 0;
        
    BMT_status.bat_exist = 1;       /* phone must have battery */
    BMT_status.charger_exist = 0;     /* for default, no charger */
    BMT_status.bat_vol = 0;
    BMT_status.ICharging = 0;
    BMT_status.temperature = 0;
    BMT_status.charger_vol = 0;
    BMT_status.total_charging_time = 0;
    BMT_status.PRE_charging_time = 0;
    BMT_status.CV_charging_time = 0;    
    BMT_status.CV_low_current_time = 0;

    /* Run Battery Thread Use GPT timer */ 
    BatThread_XGPTConfig();

    #if 0
    /*  Background low battery voltage detection */
    mt6516_ADC_power_up(); /* Do not ADC power down for background detection */
    //if (ADC_SET_THRESHOLD(KAL_TRUE, 21, AdcChannel1, 249, 10))
    if (ADC_SET_THRESHOLD(KAL_TRUE, 3600, AdcChannel1, 249, 10))
        printk("[BAT] ADC_SET_THRESHOLD : success!\r\n");
    else
        printk("[BAT] ADC_SET_THRESHOLD : fail!\r\n");
    #endif
    
     /* battery kernel thread for 10s check and charger in/out event */
    kthread_run(bat_thread_kthread, NULL, "bat_thread_kthread");  

	/*LOG System Set*/
	init_proc_log();

	#if defined(CONFIG_BATTERY_BLP509) || defined(CONFIG_BATTERY_E1000)
	printk( "Support init_proc_BatTempProtectEn\n");
	init_proc_BatTempProtectEn();
	#endif
	
    return 0;

exit_kfree:
    kfree(new_client);
exit:
    return err;
}

static int mt6516_smart_battery_attach_adapter(struct i2c_adapter *adapter)
{
#if defined(CONFIG_BATTERY_B56QN)
    if (adapter->id == 2) {
        printk( "mt6516_smart_battery_attach_adapter on phone : SCL2 / SDA2\n"); 
        return i2c_probe(adapter, &addr_data, mt6516_smart_battery_detect);
    }
    return -1;
#else    
    if (adapter->id == 1) {
        printk( "mt6516_smart_battery_attach_adapter Done\n");     
        return i2c_probe(adapter, &addr_data, mt6516_smart_battery_detect);
    }        
    return -1;
#endif    
}

static int mt6516_smart_battery_detach_client(struct i2c_client *client)
{
    int err;

    err = i2c_detach_client(client);
    if (err) {
        dev_err(&client->dev, "Client deregistration failed, client not detached.\n");
        return err;
    }

    kfree(i2c_get_clientdata(client));
    
    return 0;
}

static int __init mt6516_smart_battery_init(void)
{
     wake_lock_init(&battery_suspend_lock, WAKE_LOCK_SUSPEND, "battery wakelock");
    wake_lock_init(&low_battery_suspend_lock, WAKE_LOCK_SUSPEND, "low battery wakelock");
         
    return i2c_add_driver(&mt6516_smart_battery_driver);
}

static void __exit mt6516_smart_battery_exit(void)
{
    i2c_del_driver(&mt6516_smart_battery_driver);
}

module_init(mt6516_smart_battery_init);
module_exit(mt6516_smart_battery_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MediaTek I2C smart battery Driver");
MODULE_AUTHOR("James Lo<james.lo@mediatek.com>");

