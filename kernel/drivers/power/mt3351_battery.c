

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <mach/hardware.h>
#include <mach/mt3351_battery.h>
#include <mach/mt3351_gpt_sw.h>
#include <mach/mt3351_pmu_hw.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include "../usb/gadget/mt3351_udc.h"

#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/interrupt.h>

#include <linux/platform_device.h>
#include <linux/power_supply.h>

extern PMU_ChargerStruct BMT_status;
extern void MT3351_IRQSensitivity(unsigned char code, unsigned char edge);
extern void MT3351_IRQMask(unsigned int line);
extern void MT3351_IRQUnmask(unsigned int line);
extern void MT3351_IRQClearInt(unsigned int line);

void BAT_thread(UINT16);
extern void kernel_power_off(void);
extern PMU_STATUS PMU_EnableCharger( BOOL);
extern BOOL GPT_Config(GPT_CONFIG);
extern CHARGER_TYPE USB_charger_type_detection(void);

//#define PM_DVFS               1
//#define CONFIG_DEBUG_MSG    1

#define EFUSE_SLOPE_D2_MASK 0xF0000000
#define EFUSE_SLOPE_D3_MASK 0x0000000F
#define EFUSE_OFFSET_MASK 0x0FF80000
#define EFUSE_SLOPE_OFFSET 28
#define EFUSE_OFFSET_OFFSET 19
#define BATTERY_AVERAGE_SIZE 60

WORD u4GetBattCounter = 0;
WORD g_PreChargingCurrent = PRE_CC_50mA;
WORD g_ConstantChargingCurrent = CC_450mA;
WORD g_ConstantChargingCurrent_USB = CC_450mA;
WORD g_ConstantVoltage_vol = CONSTANT_VOLTAGE_CHARGE_VOLTAGE;
WORD g_BatteryLevel_15_vol = BatteryLevel_0_Percent_VOLTAGE;
WORD g_BatteryLevel_30_vol = BatteryLevel_1_Percent_VOLTAGE;
WORD g_BatteryLevel_50_vol = BatteryLevel_2_Percent_VOLTAGE;
WORD g_BatteryLevel_60_vol = BatteryLevel_3_Percent_VOLTAGE;
WORD g_BatteryLevel_80_vol = BatteryLevel_4_Percent_VOLTAGE;
WORD g_BatteryLevel_95_vol = BatteryLevel_5_Percent_VOLTAGE;
WORD g_ADC_CALI_SLOPE = ADC_DEFAULT_SLOPE;
WORD g_ADC_CALI_OFFSET = ADC_DEFAULT_OFFSET;
WORD g_ADC_CALI_OFFSET_ADJUST = 0;  //before using g_ADC_CALI_OFFSET, need to minus g_ADC_CALI_OFFSET_SHIFT first
WORD g_BATTERY_TEMP_PROTECT = TRUE;
WORD u4BatteryCount = 0;
WORD g_BatteryCurrentLevel_OnlyAC = BatteryLevel_0_Percent;
WORD g_BatteryCurrentLevel_OnlyBAT = Battery_Percent_100;

BOOL g_ADC_CALI_VALID = FALSE;

static CHARGER_TYPE CHR_Type_num = CHARGER_UNKNOWN;

UINT32 g_wake_up = FALSE;
EXPORT_SYMBOL(g_wake_up);

int g_MAX_CHARGE_TEMPERATURE = 50;
int g_MIN_CHARGE_TEMPERATURE = 0;

BOOL g_Battery_Fail = FALSE;

static unsigned short batteryVoltageBuffer[BATTERY_AVERAGE_SIZE];
static unsigned short batteryCurrentBuffer[BATTERY_AVERAGE_SIZE];
static int batteryIndex = 0;
static int batteryVoltageSum = 0;
static int batteryCurrentSum = 0;
BOOL batteryBufferFirst = FALSE;

#ifdef BATTERY_NTC_TYPE_A

unsigned short temperature_table[PARAM_COUNT] =
{      660,649,638,627,616,604,593,582,571,560, // -10
	 548,537,526,515,504,493,483,472,461,451, // 0
	 440,430,420,410,400,390,381,371,362,353, // 10
	 344,335,326,318,309,301,293,285,278,270, // 20
	 263,256,249,242,235,229,223,217,211,205, // 30
	 199,194,188,183,178,173,168,164,159,155, // 40
	 150,146,142,138,135,131,127,124,120,117, // 50
	 114 
};

#elif BATTERY_NTC_TYPE_B

unsigned short temperature_table[PARAM_COUNT] =
 {     742,729,717,705,692,679,666,653,640,627, // -10
        613,600,586,572,559,545,532,518,505,491, // 0
        478,465,452,439,426,414,402,390,378,366, // 10
        355,343,332,322,311,301,291,282,272,263, // 20
        254,245,237,229,221,213,206,199,192,185, // 30
        179,172,166,160,155,149,144,139,134,129, // 40
        124,120,116,112,108,104,100,97,93,90,    //50
        87
};
#endif

struct mt3351_ac_data {
    struct power_supply psy;
    int AC_ONLINE;    
};

struct mt3351_battery_data {
    struct power_supply psy;
    int BAT_STATUS;
    int BAT_HEALTH;
    int BAT_PRESENT;
    int BAT_TECHNOLOGY;
    int BAT_CAPACITY;
    /* 20090603 James Lo add */
    int BAT_batt_vol;
    int BAT_batt_temp;	
};

static enum power_supply_property mt3351_ac_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static enum power_supply_property mt3351_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_CAPACITY,
    /* 20090603 James Lo add */
    POWER_SUPPLY_PROP_batt_vol,
    POWER_SUPPLY_PROP_batt_temp,
};

/* temporary variable used between goldfish_battery_probe() and goldfish_battery_open() */
//static struct mt3351_battery_data *battery_data;

static int mt3351_ac_get_property(struct power_supply *psy,
			enum power_supply_property psp,
			union power_supply_propval *val)
{
       int ret = 0;
	struct mt3351_ac_data *data = container_of(psy, struct mt3351_ac_data, psy);	

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:		
	       #ifdef CONFIG_DEBUG_MSG
		//printk("Android wants POWER_SUPPLY_PROP_ONLINE\n");
	       #endif	       
		val->intval = data->AC_ONLINE;
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static int mt3351_battery_get_property(struct power_supply *psy,
				 enum power_supply_property psp,
				 union power_supply_propval *val)
{
       int ret = 0; 	
	struct mt3351_battery_data *data = container_of(psy, struct mt3351_battery_data, psy);
	
	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
              #ifdef CONFIG_DEBUG_MSG
		//printk("Android wants POWER_SUPPLY_PROP_STATUS\n");
              #endif              
		val->intval = data->BAT_STATUS;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
              #ifdef CONFIG_DEBUG_MSG
		//printk("Android wants POWER_SUPPLY_PROP_HEALTH\n");
              #endif
		val->intval = data->BAT_HEALTH;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
	       #ifdef CONFIG_DEBUG_MSG
		//printk("Android wants POWER_SUPPLY_PROP_PRESENT\n");
	       #endif
		val->intval = data->BAT_PRESENT;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
	       #ifdef CONFIG_DEBUG_MSG
		//printk("Android wants POWER_SUPPLY_PROP_TECHNOLOGY\n");
	       #endif
		val->intval = data->BAT_TECHNOLOGY;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
	       #ifdef CONFIG_DEBUG_MSG
		//printk("Android wants POWER_SUPPLY_PROP_CAPACITY\n");		
	       #endif
		val->intval = data->BAT_CAPACITY;
		break;
    case POWER_SUPPLY_PROP_batt_vol:
   #ifdef CONFIG_DEBUG_MSG
           printk("Android wants POWER_SUPPLY_PROP_batt_vol\n");
   #endif
           val->intval = data->BAT_batt_vol;
           break;
    case POWER_SUPPLY_PROP_batt_temp:
   #ifdef CONFIG_DEBUG_MSG  
           printk("Android wants POWER_SUPPLY_PROP_batt_temp\n");
   #endif
           val->intval = data->BAT_batt_temp;
           break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

// mt3351_ac_data initialization
static struct mt3351_ac_data mt3351_ac_main = {
    .psy = {
        .name = "ac",
        .type = POWER_SUPPLY_TYPE_MAINS,
        .properties = mt3351_ac_props,
        .num_properties = ARRAY_SIZE(mt3351_ac_props),
        .get_property = mt3351_ac_get_property,                
    },

    .AC_ONLINE = 0,
};

// mt3351_battery_data initialization
static struct mt3351_battery_data mt3351_battery_main = {
    .psy = {
        .name = "battery",
        .type = POWER_SUPPLY_TYPE_BATTERY,
        .properties = mt3351_battery_props,
        .num_properties = ARRAY_SIZE(mt3351_battery_props),
        .get_property = mt3351_battery_get_property,                
    },

    .BAT_STATUS = POWER_SUPPLY_STATUS_UNKNOWN,    
    .BAT_HEALTH = POWER_SUPPLY_HEALTH_UNKNOWN,
    .BAT_PRESENT = 0,
    .BAT_TECHNOLOGY = POWER_SUPPLY_TECHNOLOGY_UNKNOWN,
    .BAT_CAPACITY = 0,
    .BAT_batt_vol = 0,
    .BAT_batt_temp = 0,
};

static void mt3351_ac_update(struct mt3351_ac_data *ac_data)
{
    struct power_supply *ac_psy = &ac_data->psy;
    volatile UINT32 u4ChrStatus = 0;

    #ifdef CONFIG_DEBUG_MSG
    printk("**** mt3351_ac_update !!\n");
    #endif
    
    u4ChrStatus = PMU_GetChargerStatus();
        
    //if (BMT_status.charger_exist)           			
    if (u4ChrStatus & CHR_DET)    
    {
        ac_data->AC_ONLINE = 1;
        
        CHR_Type_num = USB_charger_type_detection();

        switch(CHR_Type_num)
        {
            case CHARGER_UNKNOWN :
                #ifdef CONFIG_DEBUG_MSG            						
                printk(KERN_ALERT "\n[BAT BATTERY]  AC update : CHARGER_UNKNOWN  !!\n");			
                #endif   
                break;
            
            case STANDARD_HOST :
                BMT_status.charger_type = STANDARD_HOST;
                #ifdef CONFIG_DEBUG_MSG            						
                printk(KERN_ALERT "\n[BAT BATTERY]  AC update : STANDARD_HOST  !!\n");			
                #endif   
                break;
            
            case CHARGING_HOST :
                break;
            
            case NONSTANDARD_CHARGER :
                BMT_status.charger_type = NONSTANDARD_CHARGER;
                #ifdef CONFIG_DEBUG_MSG            						
                printk(KERN_ALERT "\n[BAT BATTERY]  AC update : NONSTANDARD_CHARGER  !!\n");			
                #endif 
                break;
            
            case STANDARD_CHARGER :
                break;
            
            default :
                #ifdef CONFIG_DEBUG_MSG 
                printk(KERN_ALERT "\n[BAT BATTERY]  AC update : ERROR!!\n");	
                #endif
                break;
        }

        if (BMT_status.charger_type == STANDARD_HOST)
        {           
            ac_psy->type = POWER_SUPPLY_TYPE_USB;
            
            #ifdef CONFIG_DEBUG_MSG
            printk("[BAT BATTERY]  AC update :  POWER_SUPPLY_TYPE_USB !!\n");
            #endif
        }
        else
        {
            ac_psy->type = POWER_SUPPLY_TYPE_MAINS;

            #ifdef CONFIG_DEBUG_MSG
            printk("[BAT BATTERY]  AC update :  POWER_SUPPLY_TYPE_MAINS !!\n");
            #endif
        }
    }
    else
    {
        ac_data->AC_ONLINE = 0;
        //ac_psy->type = POWER_SUPPLY_TYPE_BATTERY;
    }

    power_supply_changed(ac_psy);    
}

static void mt3351_battery_update(struct mt3351_battery_data *bat_data)
{
    struct power_supply *bat_psy = &bat_data->psy;
    WORD BatteryCurrentLevel_OnlyAC_tmp;	
    WORD BatteryCurrentLevel_OnlyBAT_tmp;

    #ifdef CONFIG_DEBUG_MSG
    printk("**** mt3351_battery_update !!\n");
    #endif

    //********************************
    // Update : BAT_STATUS
    //              BAT_HEALTH
    //              BAT_PRESENT
    //              BAT_TECHNOLOGY
    //              BAT_CAPACITY
    //********************************

    // TODO
    bat_data->BAT_TECHNOLOGY = POWER_SUPPLY_TECHNOLOGY_LION;
    bat_data->BAT_HEALTH = POWER_SUPPLY_HEALTH_GOOD;
    bat_data->BAT_batt_vol = BMT_status.bat_vol;
    bat_data->BAT_batt_temp= BMT_status.temperature * 10;

    if (BMT_status.bat_exist)
    {
        bat_data->BAT_PRESENT = 1;
    }
    else
    {
        bat_data->BAT_PRESENT = 0;
    }

    //bat_data->BAT_STATUS = POWER_SUPPLY_STATUS_NOT_CHARGING;
    //bat_data->BAT_CAPACITY = 80;

    if (BMT_status.charger_exist) 
    {
        // Charger and Battery Exist
        //if ( (BMT_status.bat_exist) &&
        if ( (!g_Battery_Fail) &&
              (BMT_status.bat_exist) &&
	    	(BMT_status.temperature <= g_MAX_CHARGE_TEMPERATURE) && 
              (BMT_status.temperature >= g_MIN_CHARGE_TEMPERATURE)    )	
        {
            // Avoid fake charging voltage by subtracting offset
            if ( (BMT_status.bat_vol <= 4000) && (BMT_status.bat_vol >= 3600) )
            {
                #ifdef CONFIG_DEBUG_MSG
                printk("** Avoid fake charging voltage by subtracting offset\r\n");		
                #endif
                
                BMT_status.bat_vol -= 150; 
            }     

            // Battery voltage average at BATStatusUpdate
            // Charging current average at BATStatusUpdate 

            // Battery Full
            if ( (BMT_status.bat_vol > BatteryLevel_5_Percent_VOLTAGE) && (BMT_status.bat_full == TRUE) )
            {
                bat_data->BAT_STATUS = POWER_SUPPLY_STATUS_FULL;
                bat_data->BAT_CAPACITY = Battery_Percent_100;
            }
            // battery charging
            else 
            {
                bat_data->BAT_STATUS = POWER_SUPPLY_STATUS_CHARGING;
            
                if ( (BMT_status.bat_vol >= g_BatteryLevel_95_vol) && (BMT_status.bat_full == TRUE) )
                {
                    BatteryCurrentLevel_OnlyAC_tmp = Battery_Percent_100;
                }
                else if ( (BMT_status.bat_vol >= g_BatteryLevel_95_vol) )
                {
                    BatteryCurrentLevel_OnlyAC_tmp = BatteryLevel_5_Percent;
                }
                else if ( (BMT_status.bat_vol < g_BatteryLevel_95_vol) && (BMT_status.bat_vol >= g_BatteryLevel_80_vol) )
                {
                    BatteryCurrentLevel_OnlyAC_tmp = BatteryLevel_5_Percent;
                }
                else if ( (BMT_status.bat_vol < g_BatteryLevel_80_vol) && (BMT_status.bat_vol >= g_BatteryLevel_60_vol) )
                {
                    BatteryCurrentLevel_OnlyAC_tmp = BatteryLevel_4_Percent;
                }
                else if ( (BMT_status.bat_vol < g_BatteryLevel_60_vol) && (BMT_status.bat_vol >= g_BatteryLevel_50_vol) )
                {
                    BatteryCurrentLevel_OnlyAC_tmp = BatteryLevel_3_Percent;
                }
                else if ( (BMT_status.bat_vol < g_BatteryLevel_50_vol) && (BMT_status.bat_vol >= g_BatteryLevel_30_vol) )
                {
                    BatteryCurrentLevel_OnlyAC_tmp = BatteryLevel_2_Percent; 
                }
                else if ( (BMT_status.bat_vol < g_BatteryLevel_30_vol) && (BMT_status.bat_vol >= g_BatteryLevel_15_vol) )
                {
                    BatteryCurrentLevel_OnlyAC_tmp = BatteryLevel_1_Percent;
                }
                else if ( (BMT_status.bat_vol < g_BatteryLevel_15_vol)  )
                {
                    BatteryCurrentLevel_OnlyAC_tmp = BatteryLevel_0_Percent;
                }
                else
                {
                    BatteryCurrentLevel_OnlyAC_tmp = BatteryLevel_0_Percent;
                }

                if (BatteryCurrentLevel_OnlyAC_tmp > g_BatteryCurrentLevel_OnlyAC)
                {
                    // When charging, display BatteryLevel Percent
                    bat_data->BAT_CAPACITY = BatteryCurrentLevel_OnlyAC_tmp;
                    g_BatteryCurrentLevel_OnlyAC= BatteryCurrentLevel_OnlyAC_tmp;
                    g_BatteryCurrentLevel_OnlyBAT= g_BatteryCurrentLevel_OnlyAC;
                }
                else
                {
                    // Do not change BatteryLifePercent
                    bat_data->BAT_CAPACITY = g_BatteryCurrentLevel_OnlyAC;	
                    g_BatteryCurrentLevel_OnlyBAT= g_BatteryCurrentLevel_OnlyAC;
                }		
                
            }
        }
        // No Battery, Only Charger
        else
        {
            bat_data->BAT_STATUS = POWER_SUPPLY_STATUS_UNKNOWN;
            bat_data->BAT_CAPACITY = 0;
        }
        
    }
    // Only Battery 
    else
    {
        bat_data->BAT_STATUS = POWER_SUPPLY_STATUS_NOT_CHARGING;

        // Battery voltage average at BATStatusUpdate
        // Charging current average at BATStatusUpdate 
        
        // Power Off VOLTAGE
        if ( BMT_status.bat_vol < SYSTEM_OFF_VOLTAGE )
        {  
            printk(  "[BAT BATTERY] Update BatInfo : VBAT(%d mv) < %d mV : Power Off System !!\r\n", 
                BMT_status.bat_vol, SYSTEM_OFF_VOLTAGE);					
            kernel_power_off();  
        }
        else
        {
            if (BMT_status.bat_vol >= g_BatteryLevel_95_vol)
            {
                BatteryCurrentLevel_OnlyBAT_tmp = Battery_Percent_100;
            }
            else if ( (BMT_status.bat_vol < g_BatteryLevel_95_vol) && (BMT_status.bat_vol >= g_BatteryLevel_80_vol) )
            {
                BatteryCurrentLevel_OnlyBAT_tmp = BatteryLevel_5_Percent;
            }
            else if ( (BMT_status.bat_vol < g_BatteryLevel_80_vol) && (BMT_status.bat_vol >= g_BatteryLevel_60_vol) )
            {
                BatteryCurrentLevel_OnlyBAT_tmp = BatteryLevel_4_Percent;
            }
	     else if ( (BMT_status.bat_vol < g_BatteryLevel_60_vol) && (BMT_status.bat_vol >= g_BatteryLevel_50_vol) )
	     {
                BatteryCurrentLevel_OnlyBAT_tmp = BatteryLevel_3_Percent;
	     }
	     else if ( (BMT_status.bat_vol < g_BatteryLevel_50_vol) && (BMT_status.bat_vol >= g_BatteryLevel_30_vol) )
	     {
                BatteryCurrentLevel_OnlyBAT_tmp = BatteryLevel_2_Percent; 
	     }
	     else if ( (BMT_status.bat_vol < g_BatteryLevel_30_vol) && (BMT_status.bat_vol >= g_BatteryLevel_15_vol) )
	     {
                BatteryCurrentLevel_OnlyBAT_tmp = BatteryLevel_1_Percent;
	     }
	     else if ( (BMT_status.bat_vol < g_BatteryLevel_15_vol)  )
	     {
                BatteryCurrentLevel_OnlyBAT_tmp = BatteryLevel_0_Percent;
	     }
            else
            {
                BatteryCurrentLevel_OnlyBAT_tmp = BatteryLevel_0_Percent;
            }

            if (BatteryCurrentLevel_OnlyBAT_tmp < g_BatteryCurrentLevel_OnlyBAT)
            {
                bat_data->BAT_CAPACITY = BatteryCurrentLevel_OnlyBAT_tmp;	
                g_BatteryCurrentLevel_OnlyBAT = BatteryCurrentLevel_OnlyBAT_tmp;
                g_BatteryCurrentLevel_OnlyAC = g_BatteryCurrentLevel_OnlyBAT;
            }
            else
            {
                bat_data->BAT_CAPACITY = g_BatteryCurrentLevel_OnlyBAT;	
                g_BatteryCurrentLevel_OnlyAC = g_BatteryCurrentLevel_OnlyBAT;
            }
            
        }
    }    

    power_supply_changed(bat_psy);    
}

void ADC_read_efuse(void)
{
	volatile UINT32 Efuse_d0=0,Efuse_d1=0,Efuse_d2=0,Efuse_d3=0;
	UINT32 ADC_A_buttom = 0;
	UINT32 ADC_A_top = 0;
	UINT32 ADC_A = 0;
	UINT32 ADC_B = 0;	
	UINT32 efuse_slope = 0;  
	UINT32 efuse_offset = 0; 

       //VDD = 1.2V, VCCQ = 3.3V, FSOURCE = Floating or Ground 
       DRV_WriteReg32(EFUSEC_CON, EFUSEC_CON_READ);

       while(DRV_Reg32(EFUSEC_CON) & EFUSEC_CON_BUSY);

       Efuse_d0 = DRV_Reg32(EFUSEC_D0); //31~0
       Efuse_d1 = DRV_Reg32(EFUSEC_D1); //63~32
       Efuse_d2 = DRV_Reg32(EFUSEC_D2); //
       Efuse_d3 = DRV_Reg32(EFUSEC_D3);    
    
#ifdef CONFIG_DEBUG_MSG
       printk("[EFUSE] D0 = %x\n",Efuse_d0);    
       printk("[EFUSE] D1 = %x\n",Efuse_d1);    
       printk("[EFUSE] D2 = %x\n",Efuse_d2);    
       printk("[EFUSE] D3 = %x\n",Efuse_d3);     
#endif
    
   	if((Efuse_d1 & 0x80000000)==0 && (Efuse_d3 & 0x80000000)==0)
   	{	g_ADC_CALI_VALID = FALSE;
		printk("[EFUSE] No calibration information\r\n");       	
   		return ;
   	}
   	else
   	{	
   		g_ADC_CALI_VALID = TRUE;   	
		printk("[EFUSE] Calibration information exists\r\n");       		
   	}

	//a : [99:92]      remap D3[4:0]+ D2[31:28]  => 8 bit
	//b : [91:83]      remap D2[27:19]                => 9 bit
	ADC_A_buttom = (Efuse_d2 & EFUSE_SLOPE_D2_MASK) >> EFUSE_SLOPE_OFFSET; 
	ADC_A_top = (Efuse_d3 & EFUSE_SLOPE_D3_MASK) << 4;                     
	ADC_A = ADC_A_top | ADC_A_buttom;
	
#ifdef CONFIG_DEBUG_MSG
       printk("[EFUSE] ADC_A_buttom = %d\r\n",ADC_A_buttom);    
       printk("[EFUSE] ADC_A_top = %d\r\n",ADC_A_top); 
#endif

	ADC_B = (Efuse_d2 & EFUSE_OFFSET_MASK) >> EFUSE_OFFSET_OFFSET;

#ifdef CONFIG_DEBUG_MSG
       printk("[EFUSE] ADC_A = %d\r\n",ADC_A);    
       printk("[EFUSE] ADC_B = %d\r\n",ADC_B); 		
#endif

	//efuse_slope = ((ADC_A/17) + 90)/100;
	//efuse_offset = ( ((ADC_B)/17 - 15)/1000 ) *1024;
	efuse_slope = ((ADC_A*ADC_CALI_DIVISOR/17) + 90*ADC_CALI_DIVISOR)/100;
	efuse_offset = (((ADC_B*ADC_CALI_DIVISOR/17))/1000) *1024; //ADC_B is needed to calibrated by multiplying 1024

#ifdef CONFIG_DEBUG_MSG
       printk("[EFUSE] efuse_slope (multiply %d) = %d\r\n",ADC_CALI_DIVISOR,efuse_slope);    
       printk("[EFUSE] efuse_offset (add 15/1000, then multiply %d)= %d\r\n",ADC_CALI_DIVISOR,efuse_offset); 	
#endif

	g_ADC_CALI_SLOPE = efuse_slope;
	g_ADC_CALI_OFFSET = efuse_offset;   
	g_ADC_CALI_OFFSET_ADJUST = ((15*ADC_CALI_DIVISOR)/1000)*1024;

#ifdef CONFIG_DEBUG_MSG
       printk("[EFUSE] g_ADC_CALI_SLOPE = %d\r\n",g_ADC_CALI_SLOPE);    
       printk("[EFUSE] g_ADC_CALI_OFFSET = %d\r\n",g_ADC_CALI_OFFSET); 		
       printk("[EFUSE] g_ADC_CALI_OFFSET_ADJUST = %d\r\n",g_ADC_CALI_OFFSET_ADJUST); 		    
#endif

}

UINT32 ADC_Calibration(UINT32 dat)
{	
	// ****************************
	// 0.900 <= a <= 1.050    (slope)
	// -0.015 <= b <= 0.015   (Offset)
	// ****************************
	if(g_ADC_CALI_VALID)
	{		
		return ((dat*ADC_CALI_DIVISOR - (g_ADC_CALI_OFFSET-g_ADC_CALI_OFFSET_ADJUST))/g_ADC_CALI_SLOPE);
	}
	else
	{	
	       //return ((dat*g_ADC_CALI_SLOPE - g_ADC_CALI_OFFSET) / ADC_CALI_DIVISOR);
	       return dat;
	}
}
 
PMU_STATUS BAT_GetVoltage(void)
{ 
    BOOL bTempFix = FALSE;
    UINT32 i = 0, u4RefBatTemp= 0, u4Sample_times = 0;
    UINT32 dat = 0, u4CaliData = 0;
    // channel 0
    UINT32 u4TempData = 0;    
    // channel 1
    UINT32 u4TempRef = 0;    
    // channel 6
    UINT32 u4BatVoltage = 0;    
    // channel 7
    UINT32 u4ISenVoltage = 0;    
    // channel 8
    UINT32 u4ChrVoltage = 0;    
    // stop charger, get more precisely data
    // 20081112, Stop Charging cause Isense lower than Vbat
    hwEnableClock(MT3351_CLOCK_AUXADC);
    DRV_WriteReg(AUXADC_CON0, 0);
    DRV_WriteReg(AUXADC_CON1, 0);    
    DRV_WriteReg(AUXADC_CON2, 0);    
    DRV_WriteReg(AUXADC_CON3, 0);   
    do
    {
        DRV_WriteReg(AUXADC_CON1, 0);   	 
    	 DRV_WriteReg(AUXADC_CON1, 0x1FF);
    	 
    	 DVT_DELAYMACRO(1000);

    	 //Polling until bit STA = 0    	
        while (0 != (0x01 & DRV_Reg(AUXADC_CON3)));  

        // Read if ADC is invalid, skip invalid value
    	 dat = PMU_GetChargerStatus();    
        if (dat & ADC_INVALID)
        {
            #ifdef CONFIG_DEBUG_MSG
            printk(  "ADC_INVALID detected!  Re-sample!\r\n");
            #endif
            continue;
        }        

        // 1. ADC channel 0, temperature
        // We use 4 point resolution, so will devide 10000
        // 2800mV / 1024 = 2.7mv, 2.7*30 = 81mV
        // if ABS larger than this value, we will ignore cali data
        dat  = DRV_Reg(AUXADC_DAT0);        
        u4CaliData = ADC_Calibration(dat);
        if (( abs( dat - u4CaliData) ) < ADC_CALI_RANGE)
        {
            dat = u4CaliData; 
        }        
        u4TempData += dat ;
        
        // 2. ADC channel 1, temperature reference, for 4.3" EVK
    	 dat  = DRV_Reg(AUXADC_DAT1);        
        u4CaliData = ADC_Calibration(dat);  
        if (( abs( dat - u4CaliData) ) < ADC_CALI_RANGE)
        {
            dat = u4CaliData; 
        }        
        u4TempRef += dat ;    

        // 3. ADC channel 6, BAT
        dat = DRV_Reg(AUXADC_DAT6);        
        u4CaliData = ADC_Calibration(dat);  
        if (( abs( dat - u4CaliData) ) < ADC_CALI_RANGE)
        {
            dat = u4CaliData; 
        }                
        u4BatVoltage += dat;   

        // 4. ADC channel 7, ISENSE    		
        dat = DRV_Reg(AUXADC_DAT7);
        u4CaliData = ADC_Calibration(dat);  
        if (( abs( dat - u4CaliData) ) < ADC_CALI_RANGE)
        {
            dat = u4CaliData; 
        }                
        u4ISenVoltage += dat;

        // 5. ADC channel 8, Charger
        dat = DRV_Reg(AUXADC_DAT8);      
        u4CaliData = ADC_Calibration(dat);  
        if (( abs( dat - u4CaliData) ) < ADC_CALI_RANGE)
        {
            dat = u4CaliData; 
        }        
        u4ChrVoltage += dat;  

        u4Sample_times++;
    }
    while (u4Sample_times < ADC_SAMPLE_TIMES);

    u4BatVoltage = u4BatVoltage*2800/1023;   
    u4ISenVoltage = u4ISenVoltage*2800/1023;
    u4ChrVoltage = u4ChrVoltage*2800/1023;    

    BMT_status.bat_vol = (u4BatVoltage/ADC_SAMPLE_TIMES)*2;
    BMT_status.ISENSE = (u4ISenVoltage/ADC_SAMPLE_TIMES)*2;
    BMT_status.charger_vol = (u4ChrVoltage/ADC_SAMPLE_TIMES)*5;
    u4TempData = (u4TempData / ADC_SAMPLE_TIMES) ;
    u4TempRef = (u4TempRef / ADC_SAMPLE_TIMES) ;
    
    for(i = 0; i < PARAM_COUNT ; i++ )
    {
        if( (u4TempData < temperature_table[i] ) && (u4TempData >= temperature_table[i+1]) )
        {            
            BMT_status.temperature = (i - 10); // begin from -10C
            bTempFix = TRUE;
            break;
        }
    }

#ifdef BATTERY_NTC_TYPE_B
    for(i = 0; i < PARAM_COUNT ; i++ )
    {
        if( (u4TempRef < temperature_table[i] ) && (u4TempRef >= temperature_table[i+1]) )
        {            
            u4RefBatTemp = (i - 10); // begin from -10C
            bTempFix = TRUE;
            break;
        }
    }

    #ifdef CONFIG_DEBUG_MSG    
    printk(  "[BAT WARN]Reference NTC ,tBAT = %d, tRef = %d \r\n",BMT_status.temperature,u4RefBatTemp);
    #endif

    #if 0 //TODO : Compare two NTC   
    if (( abs( BMT_status.temperature - u4RefBatTemp) ) > 10 )
    {
        BMT_status.temperature = ERR_CHARGE_TEMPERATURE;
        DEBUGMSG(ZONE_ERROR,(TEXT("[BAT ERROR]NTC compare fail ,tBAT = %d, tRef = %d \r\n"),BMT_status.temperature,u4RefBatTemp));
        
    }
    #endif    

#endif
    
    if (!bTempFix)
    {
        BMT_status.temperature = ERR_CHARGE_TEMPERATURE;
        #ifdef CONFIG_DEBUG_MSG		
        printk(  "[BAT ERROR] Can not fix battery temperature!\r\n");	
        #endif
    }
    
    // Charging current  = (VIsense-VBAT) / 0.2 Ohm
    if(BMT_status.ISENSE > BMT_status.bat_vol)
    {
        BMT_status.ICharging = (BMT_status.ISENSE - BMT_status.bat_vol)*10/2;    
    }
    else
    {
        BMT_status.ICharging = 0;
    }    
    
    hwDisableClock(MT3351_CLOCK_AUXADC);

    return PMU_STATUS_OK;	
}

void BATStatusUpdate(UINT16 data)
{
    volatile UINT32 u4ChrStatus, i;
    u4ChrStatus = PMU_GetChargerStatus();    

    if (u4ChrStatus & CHR_DET)
    {
        #ifdef CONFIG_DEBUG_MSG
        printk("[BAT BATTERY] Charger plug IN \r\n");
        #endif
    
        BMT_status.charger_exist = TRUE;
    }
    else
    {
        #ifdef CONFIG_DEBUG_MSG
        printk("[BAT BATTERY] Charger plug OUT \r\n");
        #endif

        BMT_status.charger_exist = FALSE;
    }

    if (u4ChrStatus & BAT_ONB)
    {
        #ifdef CONFIG_DEBUG_MSG
        printk("[BAT BATTERY] Battery plug OUT \r\n");
        #endif

        // Reset g_Battery_Fail value when remove battery
        g_Battery_Fail = FALSE;
        
        BMT_status.bat_exist = FALSE;

        // SwitchPowerSource AUTO when removing battery        
        if (BMT_status.charger_type == STANDARD_HOST) 
        {
            #ifdef CONFIG_DEBUG_MSG
            printk("[BAT BATTERY] Switch Power Source AUTO !!\r\n");
            #endif
            
            PMU_SwitchPowerSource(PS_SET_AUTO);
        }
    }
    else
    {
        #ifdef CONFIG_DEBUG_MSG
        printk("[BAT BATTERY] Battery plug IN \r\n");
        #endif

        BMT_status.bat_exist = TRUE;
    }

    // 1. Get temperature, bat_vol, VISENSE, charger_vol, ICharging
    BAT_GetVoltage();

    #ifdef CONFIG_DEBUG_MSG  
    printk(  "[BAT BATTERY] Before Battery Voltage Average : %d mV\r\n", BMT_status.bat_vol);
    printk(  "[BAT BATTERY] Before Battery Current Average : %d mA\r\n", BMT_status.ICharging);
    #endif

    if (!batteryBufferFirst)
    {
        batteryBufferFirst = TRUE;
        
        for (i=0; i<BATTERY_AVERAGE_SIZE; i++) {
            batteryVoltageBuffer[i] = BMT_status.bat_vol;
            batteryCurrentBuffer[i] = BMT_status.ICharging;
        }

        batteryVoltageSum = BMT_status.bat_vol * BATTERY_AVERAGE_SIZE;
        batteryCurrentSum = BMT_status.ICharging * BATTERY_AVERAGE_SIZE;
    }

    batteryVoltageSum -= batteryVoltageBuffer[batteryIndex];
    batteryVoltageSum += BMT_status.bat_vol;
    batteryVoltageBuffer[batteryIndex] = BMT_status.bat_vol;

    batteryCurrentSum -= batteryCurrentBuffer[batteryIndex];
    batteryCurrentSum += BMT_status.ICharging;
    batteryCurrentBuffer[batteryIndex] = BMT_status.ICharging;

    BMT_status.bat_vol = batteryVoltageSum / BATTERY_AVERAGE_SIZE;
    BMT_status.ICharging = batteryCurrentSum / BATTERY_AVERAGE_SIZE;

    batteryIndex++;
    if (batteryIndex >= BATTERY_AVERAGE_SIZE)
        batteryIndex = 0;

    #ifdef CONFIG_DEBUG_MSG  
    printk(  "[BAT BATTERY] After Battery Voltage Average : %d mV\r\n", BMT_status.bat_vol);
    printk(  "[BAT BATTERY] After Battery Current Average : %d mA\r\n", BMT_status.ICharging);
    #endif            

#if 0    
    #ifdef CONFIG_DEBUG_MSG
    printk(  "[BAT UPDATE] Battery temperature:%d vbat:%dmV isen:%dmV chrin:%dmV Icharging:%dmA!!\r\n", 
       BMT_status.temperature ,BMT_status.bat_vol, BMT_status.ISENSE, BMT_status.charger_vol, 
       BMT_status.ICharging );			
    #endif	
#endif
    
    // 2. ac information update for Android
    mt3351_ac_update(&mt3351_ac_main);
    #ifdef CONFIG_DEBUG_MSG  
    //printk(  "[BAT BATTERY] Update Android AC Information\r\n");
    #endif

    // 3. battery information update for Android    
    mt3351_battery_update(&mt3351_battery_main);   
    #ifdef CONFIG_DEBUG_MSG  
    //printk(  "[BAT BATTERY] Update Android Battery Information\r\n");
    #endif

    #ifdef CONFIG_DEBUG_MSG  
    printk( "%d,%d,%d,%d\r\n", BMT_status.temperature ,BMT_status.bat_vol, 
                BMT_status.charger_vol, BMT_status.ICharging );
    #endif

}

#if 0
UINT32 BAT_PDDGetStatus(void)
{
    volatile UINT32 u4ChrStatus;
    u4ChrStatus = PMU_GetChargerStatus();    

    // for WinCE PDD use, charger is exist
    if (u4ChrStatus & CHR_DET)
    {
        BMT_status.charger_exist = TRUE;
    }
    else
    {
        BMT_status.charger_exist = FALSE;
    }

    // for WinCE PDD use, battery is exist
    if (u4ChrStatus & BAT_ONB)
    {
        BMT_status.bat_exist = FALSE;
        return PMU_STATUS_FAIL;		
    }
    else
    {
        BMT_status.bat_exist = TRUE;
    }

    // for WinCE PDD use, battery voltage
    BAT_GetVoltage();
	
    return PMU_STATUS_OK;
}
#endif 

UINT32 BAT_CheckPMUStatusReg(void)
{    
    volatile UINT32 u4ChrStatus = 0, u4Count = 0;
        
    u4ChrStatus = PMU_GetChargerStatus(); // check PMU_CON28   

    // ADC protect
    if (u4ChrStatus & ADC_INVALID)
    {
        #ifdef CONFIG_DEBUG_MSG        
        printk(  "[BAT BATTERY] ADC_INVALID detected! Charger Stop!\r\n");	
        #endif
        BMT_status.bat_charging_state = CHR_ERROR;
        return BAT_STATUS_FAIL;		
    }

    // battery status, 1: remove
    if (u4ChrStatus & BAT_ONB)
    {
        #ifdef CONFIG_DEBUG_MSG
        printk(  "[BAT BATTERY] Battery remove!!\r\n");			
        #endif

        // Reset g_Battery_Fail value when remove battery
        g_Battery_Fail = FALSE;
           
        BMT_status.bat_exist = FALSE;

#if 0        
        // SwitchPowerSource AUTO when removing battery        
        if (BMT_status.charger_type == STANDARD_HOST) 
        {
            #ifdef CONFIG_DEBUG_MSG
            printk("[BAT BATTERY] Switch Power Source AUTO !!\r\n");
            #endif
            PMU_SwitchPowerSource(PS_SET_AUTO);
        }
#endif

        return PMU_STATUS_FAIL;		
    }
    else
    {
        #ifdef CONFIG_DEBUG_MSG
        printk(  "[BAT BATTERY] Battery exist!!\r\n");					
        #endif
        BMT_status.bat_exist = TRUE;
    }

    // ****************************************
    // If Charger exist, to set correct power source :
    // ****************************************
    // USB and VBAT >= CLV on normal  mode : VBAT
    // USB and VBAT <  CLV on normal  mode : VBAT    
    // AC  and VBAT >= CLV on normal  mode : AC
    // AC  and VBAT <  CLV on normal  mode : AC
    // ****************************************
    // USB and VBAT >= CLV on suspend mode : VBAT
    // USB and VBAT <  CLV on suspend mode : AC
    // AC  and VBAT >= CLV on suspend mode : AC
    // AC  and VBAT <  CLV on suspend mode : AC
    // ****************************************
	
    // To decide power source
    if (u4ChrStatus & CHR_DET)
    {
        BMT_status.charger_exist = TRUE;

        //PC USB
        if (BMT_status.charger_type == STANDARD_HOST) 
        {
            if (BMT_status.bat_vol >= SYSTEM_OFF_VOLTAGE )
            {                
                #ifdef CONFIG_DEBUG_MSG          	    
                printk(  "[BAT BATTERY] STANDARD_HOST, switch PWR = VBAT\r\n");
                #endif
                PMU_SwitchPowerSource(PS_SET_VBAT);		
            }
            else
            {		
                #ifdef CONFIG_DEBUG_MSG
                printk(  "[BAT BATTERY] STANDARD_HOST, switch PWR = VBAT\r\n");
                #endif
                PMU_SwitchPowerSource(PS_SET_VBAT);		
            }
        }
        //AC adaptor
        else 
        {
            #ifdef CONFIG_DEBUG_MSG            
            printk(  "[BAT BATTERY] PS_SET_AC, switch PWR = AC\r\n");
            #endif
            PMU_SwitchPowerSource(PS_SET_AC);
        }
    }
    else
    {   
        #ifdef CONFIG_DEBUG_MSG	    
        printk(  "[BAT BATTERY] Charger NOT exist, Only Battery !!\r\n");					
        #endif
        BMT_status.charger_exist = FALSE;
        BMT_status.total_charging_time = 0;
        BMT_status.CV_charging_time = 0;
        BMT_status.PRE_charging_time = 0;

        if (BMT_status.bat_vol <= SYSTEM_OFF_VOLTAGE)
        {            
            printk(  "[BAT BATTERY] VBAT(%d mv) < %d mV : Power Off System !!\r\n", 
               BMT_status.bat_vol, SYSTEM_OFF_VOLTAGE);					          
            kernel_power_off();        
        }
        
        PMU_SwitchPowerSource(PS_SET_VBAT);
        return PMU_STATUS_FAIL;				
    }	

    // charger OV protect
    if (u4ChrStatus & OVP)
    {
        #ifdef CONFIG_DEBUG_MSG	    
        printk(  "[BAT BATTERY] Charger OV detected!!\r\n");	
        #endif
        BMT_status.charger_protect_status = charger_OVER_VOL;
        BMT_status.bat_charging_state = CHR_ERROR;
        return PMU_STATUS_FAIL;		
    }
#if 0	
    // battery status, 1: remove
    if (u4ChrStatus & BAT_ONB)
    {
        #ifdef CONFIG_DEBUG_MSG
        printk(  "[BAT BATTERY] Battery remove!!\r\n");			
        #endif

        // Reset g_Battery_Fail value when remove battery
        g_Battery_Fail = FALSE;
           
        BMT_status.bat_exist = FALSE;            
        return PMU_STATUS_FAIL;		
    }
    else
    {
        #ifdef CONFIG_DEBUG_MSG
        printk(  "[BAT BATTERY] Battery exist!!\r\n");					
        #endif
        BMT_status.bat_exist = TRUE;
    }
#endif

    //******************************************
    // Charger Flow Status Check
    // To Decide Charger Status
    //******************************************

    // We need to enable charger before get charging status
    PMU_EnableCharger(TRUE);

    for(u4Count = 0; u4Count < 10000; u4Count++);
        u4ChrStatus = PMU_GetChargerStatus( );    

    // 1. trickle current mode
    if(u4ChrStatus & UV_VBAT22)
    {
        #ifdef CONFIG_DEBUG_MSG
        printk(  "[BAT BATTERY] u4ChrStatus:0x%x trickle mode!!\r\n",u4ChrStatus);							
        #endif
        BMT_status.bat_charging_state = CHR_TICKLE;
    }
    // 2. Pre-CC
    else if((u4ChrStatus & UV_VBAT32) && (!(u4ChrStatus & UV_VBAT22)))
    {
        #ifdef CONFIG_DEBUG_MSG
        printk(  "[BAT BATTERY] u4ChrStatus:0x%x Pre-CC mode!!\r\n",u4ChrStatus);							
        #endif
        BMT_status.bat_charging_state = CHR_PRE;
    }	
    else if((!(u4ChrStatus & UV_VBAT32)) && (!(u4ChrStatus & UV_VBAT22)))
    {    	
        // 3. CV mode
        if (u4ChrStatus & CV_MODE) 
        {
            #ifdef CONFIG_DEBUG_MSG
            printk(  "[BAT BATTERY] u4ChrStatus:0x%x CV mode!!\r\n",u4ChrStatus);        
            #endif
            BMT_status.bat_charging_state = CHR_CV;
        }
        // 4. CC mode
        else 
        {
            #ifdef CONFIG_DEBUG_MSG
            printk(  "[BAT BATTERY] u4ChrStatus:0x%x CC mode!!\r\n",u4ChrStatus);							
            #endif
            BMT_status.bat_charging_state = CHR_CC;
        }    		
    }
    else
    {
        #ifdef CONFIG_DEBUG_MSG
        printk(  "[BAT ERROR] error status!!\r\n");
        #endif
        PMU_EnableCharger(FALSE);	

        return PMU_STATUS_FAIL;	
    }    
    
    //PMU_EnableCharger(FALSE);
	
    return PMU_STATUS_OK;		
}    

UINT32 BAT_CheckPMURtcTmpStatus(void)
{
    volatile UINT32 u4ChrStatus = 0;    
    
    u4ChrStatus = PMU_GetPMURtcTmpStatus();
    if (u4ChrStatus & F32K_FAIL_STATUS)
    {
        #ifdef CONFIG_DEBUG_MSG
        printk(  "[BAT ERROR] RTC 32K FAIL!\r\n");                
        #endif
        return PMU_STATUS_FAIL;	
    }
    else
    {
        #ifdef CONFIG_DEBUG_MSG
        printk(  "[BAT BATTERY] RTC 32K PASS!!\r\n");        
        #endif
    }

    if ((u4ChrStatus&0x3) == CHR_WDT_TOUT)
    {
        #ifdef CONFIG_DEBUG_MSG
        printk(  "[BAT ERROR]Charging WDT timeout!\r\n");                
        #endif
        return PMU_STATUS_FAIL;	
    }
    else if ((u4ChrStatus&0x3) == CHR_UT_OR_HT)
    {
        #ifdef CONFIG_DEBUG_MSG
        printk(  "[BAT ERROR]Temperature protect Check FAIL!\r\n");        
        #endif
        return PMU_STATUS_FAIL;
    }    	

    return PMU_STATUS_OK;		
}


UINT32 BAT_CheckBatteryStatus(void)
{
    UINT16 BAT_status = PMU_STATUS_OK;

#if 0
    // ****************************
    // If BAT_PDDGetStatus() enable 
    // Do not check voltage here
    // ****************************    
    BAT_status = BAT_GetVoltage();
#endif

    #ifdef CONFIG_DEBUG_MSG
    printk(  "[BAT WARN]Battery temperature:%d vbat:%dmV isen:%dmV chrin:%dmV Icharging:%dmA!!\r\n", 
       BMT_status.temperature ,BMT_status.bat_vol, BMT_status.ISENSE, BMT_status.charger_vol, 
       BMT_status.ICharging );			
    #endif

    BAT_status = BAT_CheckPMURtcTmpStatus();
    if(BAT_status != PMU_STATUS_OK)
        return PMU_STATUS_FAIL;		      

    BAT_status = BAT_CheckPMUStatusReg();
    if(BAT_status != PMU_STATUS_OK)
        return PMU_STATUS_FAIL;		      

    if(g_BATTERY_TEMP_PROTECT)
    {
        if ((BMT_status.temperature > MAX_CHARGE_TEMPERATURE) || 
            (BMT_status.temperature < MIN_CHARGE_TEMPERATURE) || 
            (BMT_status.temperature == ERR_CHARGE_TEMPERATURE))
        {
            #ifdef CONFIG_DEBUG_MSG
            printk(  "[BAT ERROR]Temperature or NTC fail !!\n\r");			    
            #endif
            return PMU_STATUS_FAIL;       
        }
    }

    // Re-charging
    if((BMT_status.bat_vol < RECHARGING_VOLTAGE) && (BMT_status.bat_full))
        BMT_status.bat_full = FALSE;    
    
    // Trickle and Pre-CC 0.5 hour
    if(BMT_status.PRE_charging_time > MAX_PreCC_CHARGING_TIME)
    {
        #ifdef CONFIG_DEBUG_MSG
        printk(  "[BAT ERROR]Pre-Charge over time 0.5 hour , battery may fail !!\r\n");					
        #endif

        BMT_status.bat_exist = FALSE;
        g_Battery_Fail = TRUE;
        
        return PMU_STATUS_FAIL;		        
    }

    // Total 8 hours
    if(BMT_status.total_charging_time > MAX_CHARGING_TIME)
    {
        #ifdef CONFIG_DEBUG_MSG
        printk(  "[BAT ERROR]Charging over 8hr, stop charging !!\n\r");			    
        #endif
        
        return PMU_STATUS_FAIL;		      
    }
    
    if (BMT_status.charger_vol <= 4500 )
    {
        #ifdef CONFIG_DEBUG_MSG
        printk(  "[BAT ERROR]Charger under voltage!!\r\n");					
        #endif
        return PMU_STATUS_FAIL;		
    }

    if ( BMT_status.charger_vol >= 6000 )
    {
        #ifdef CONFIG_DEBUG_MSG
        printk(  "[BAT ERROR]Charger over voltage !!\r\n");					
        #endif
        BMT_status.charger_protect_status = charger_OVER_VOL;
        BMT_status.bat_charging_state = CHR_ERROR;
        return PMU_STATUS_FAIL;		
    }
    
    return PMU_STATUS_OK;
}

PMU_STATUS BAT_SetChargerTimeoutEnable(BOOL enable, RG_CHR_TIMEOUT_SEL TimeOut)
{
    volatile UINT16 val, i;
    if(enable)
    {
#ifdef CONFIG_DEBUG_MSG
        printk(  "[BAT BATTERY] enable timeout watchdog !!\r\n");					
#endif    
        // enable timeout watchdog
        val = (RG_CR_TIMEOUT_SEL_KEY | TimeOut | RG_CHR_TIMEOUT_EN);
        DRV_WriteReg32(PMU_CON25, RG_CR_TIMEOUT_SEL_KEY);
        for(i=0;i<10000;i++);
        DRV_WriteReg32(PMU_CON25, val);      
    }
    else
    {
        // set timeout watchdog off
        DRV_WriteReg32(PMU_CON25, RG_CR_TIMEOUT_SEL_KEY);             
    }    
    return PMU_STATUS_OK;
}

PMU_STATUS BAT_TrickleConstantCurrentModeAction(void)
{    
    printk(("[BAT BATTERY]Trickle-Charge mode, timer=%d Trickle=%d!!\n\r"),
                BMT_status.total_charging_time, BMT_status.PRE_charging_time);
    BMT_status.total_charging_time += BAT_TASK_PERIOD;
    BMT_status.PRE_charging_time += BAT_TASK_PERIOD;
    PMU_EnableCharger(TRUE);	
    return PMU_STATUS_OK;
}

PMU_STATUS BAT_PreConstantCurrentModeAction(void)
{
#ifdef CONFIG_DEBUG_MSG
    printk(  "[BAT BATTERY]Pre-Charge mode, timer=%d Pre=%d!!\n\r",
    BMT_status.total_charging_time, BMT_status.PRE_charging_time);
#endif
    BMT_status.total_charging_time += BAT_TASK_PERIOD;
    BMT_status.PRE_charging_time += BAT_TASK_PERIOD;                    
    BAT_SetChargerTimeoutEnable( TRUE, CHR_TIMEOUT_SEL_32sec);    
    PMU_EnableCharger(TRUE);
    return PMU_STATUS_OK;		
}    

PMU_STATUS BAT_ConstantCurrentModeAction(void)
{
#ifdef CONFIG_DEBUG_MSG
    printk(  "[BAT BATTERY]CC mode charge, timer=%d !!\n\r",BMT_status.total_charging_time);	
#endif
    BMT_status.PRE_charging_time = 0;    
    BMT_status.total_charging_time += BAT_TASK_PERIOD;                    
    
    BAT_SetChargerTimeoutEnable( TRUE, CHR_TIMEOUT_SEL_32sec);        			

    // PC USB and power source is battery, MAX 500mA current
    if (BMT_status.charger_type == STANDARD_HOST)
    {
#if 0   
        if (BMT_status.Bat_pwr_state != D0)
        {
            PMU_EnableConstantCurrentMode( g_ConstantChargingCurrent_USB);
            printk(  "[BAT BATTERY]STANDARD_HOST CC mode 300mA!!pwr state = %d\n\r",BMT_status.Bat_pwr_state);	            
        }
        else
#endif
        {
            if (BMT_status.bat_vol >= SYSTEM_OFF_VOLTAGE )
            {
#ifdef CONFIG_DEBUG_MSG
                printk(  "[BAT BATTERY]STANDARD_HOST and 300mA!!pwr state \n\r" );	            
#endif
                PMU_EnableConstantCurrentMode(g_ConstantChargingCurrent_USB);
            }
            else
            {
#ifdef CONFIG_DEBUG_MSG
                printk(  "[BAT BATTERY]STANDARD_HOST low bat: %d mV, stop charging\n\r", BMT_status.bat_vol);	                            
#endif
            }    
        }    
    }
    // CHARGER, MAX 1A current
    else 
    {
#if 0
        if (BMT_status.Bat_pwr_state != D0)
        {
            PMU_EnableConstantCurrentMode(g_ConstantChargingCurrent);
            printk(  "[BAT BATTERY]CHARGER CC mode 650mA!!pwr state = %d\n\r",BMT_status.Bat_pwr_state);	            
        }
        else
#endif
        {
            PMU_EnableConstantCurrentMode(g_ConstantChargingCurrent);
#ifdef CONFIG_DEBUG_MSG
            printk(  "[BAT BATTERY]CHARGER CC mode and D0 450mA!!pwr state \n\r" );	            
#endif
        }    
    }
    return PMU_STATUS_OK;		
}    

PMU_STATUS BAT_ConstantVoltageModeAction(void)
{
#ifdef CONFIG_DEBUG_MSG   
    printk(  "[BAT BATTERY]CV mode charge, timer=%d CV=%d!!\n\r",
    BMT_status.total_charging_time,BMT_status.CV_charging_time);	
#endif
    if(BMT_status.bat_vol <= CV_DROPDOWN_VOLTAGE)
    {
        BMT_status.total_charging_time = 0;
        BMT_status.CV_charging_time = 0;
        BMT_status.PRE_charging_time = 0;
        PMU_EnableCharger(FALSE);
        
        #ifdef CONFIG_DEBUG_MSG
        printk(  "BMT_status.bat_vol <= CV_DROPDOWN_VOLTAGE \n\r" );	
        #endif
    }
    
    BMT_status.total_charging_time += BAT_TASK_PERIOD;
    BMT_status.CV_charging_time += BAT_TASK_PERIOD;                    
        
    BAT_SetChargerTimeoutEnable(TRUE, CHR_TIMEOUT_SEL_32sec);        			
    PMU_EnableConstantVoltageMode();
    
    return PMU_STATUS_OK;		
}    

PMU_STATUS BAT_BatteryFullAction(void)
{
#ifdef CONFIG_DEBUG_MSG    
    printk(  "[BAT BATTERY]Battery full !!\n\r");			
#endif
    BMT_status.bat_full = TRUE;
    BMT_status.total_charging_time = 0;
    BMT_status.CV_charging_time = 0;
    BMT_status.PRE_charging_time = 0;
    PMU_EnableCharger(FALSE);			
    BAT_SetChargerTimeoutEnable( FALSE, 0);
    return PMU_STATUS_OK;
}

PMU_STATUS BAT_ChargingOTAction(void)
{
#ifdef CONFIG_DEBUG_MSG
    printk(  "[BAT BATTERY]Charging over 8 hr stop !!\n\r");			
#endif
    BMT_status.bat_full = TRUE;
    BMT_status.total_charging_time = 0;
    BMT_status.CV_charging_time = 0;
    BMT_status.PRE_charging_time = 0;
    PMU_EnableCharger(FALSE);			
    BAT_SetChargerTimeoutEnable( FALSE, 0);
    return PMU_STATUS_OK;
}

PMU_STATUS BAT_BatteryStatusFailAction(void)
{
#ifdef CONFIG_DEBUG_MSG    
    printk(  "[BAT BATTERY]BAD Battery status... Charging Stop !!\n\r");			
#endif
    BMT_status.total_charging_time = 0;
    BMT_status.CV_charging_time = 0;
    BMT_status.PRE_charging_time = 0;
    PMU_EnableCharger(FALSE);
    BAT_SetChargerTimeoutEnable( FALSE, 0);    
    return PMU_STATUS_OK;
}

void BAT_DumpPowerStatus(void)
{
    volatile UINT32 u4ChrStatus;
    printk("=======Battery and charging status======\n\r");        
    printk("Battery voltage : %d mV\n\r ",BMT_status.bat_vol);
    printk("Battery temperature : %d mV\n\r ",BMT_status.temperature);    
    printk("Charger voltage : %d mV\n\r ",BMT_status.charger_vol);
    printk("Charging current : %d mV\n\r ",BMT_status.ICharging);    
    printk("Charging state : %d mV\n\r ",BMT_status.bat_charging_state);

    u4ChrStatus = PMU_GetChargerStatus( );   
    if (u4ChrStatus & CHG_STATUS)
        printk("Charging is proceeding\r\n");
    else
        printk("Charging is stopped\r\n");
    
    PMU_DumpBusPowerState();
}    

#ifdef CONFIG_PM

static int BAT_suspend(struct device *dev, u32 state, u32 level)
{
    printk(  "BAT_suspend\n\r");    

#if 0    
    switch(level)
    {
        case SUSPEND_NOTIFY:
    	    printk(  "BAT_suspend SUSPEND_NOTIFY\n\r");    
            break;
        case SUSPEND_SAVE_STATE:
    	    printk(  "BAT_suspend SUSPEND_SAVE_STATE\n\r");    
            break;
        case SUSPEND_DISABLE:
    	    printk(  "BAT_suspend SUSPEND_DISABLE\n\r");                
            break;
        case SUSPEND_POWER_DOWN:
    	    printk(  "BAT_suspend SUSPEND_POWER_DOWN\n\r");
    	    hwDisableClock(MT3351_CLOCK_AUXADC);
            break;
        default:
	        printk("BAT_suspend level fail\n\r");                
    }
#endif
    
    return 0;
}

static int BAT_resume(struct device *dev, u32 level)
{
    printk(  "BAT_resume\n\r");        

#if 0    
    printk(  "dev = %p, level = %u\n", dev, level);
    
    switch(level)
    {
        case RESUME_POWER_ON:
    	     printk(  "BAT_resume RESUME_POWER_ON\n\r");
    	     hwEnableClock(MT3351_CLOCK_AUXADC);
            break;

        case RESUME_RESTORE_STATE:
    	     printk(  "BAT_resume RESUME_RESTORE_STATE\n\r");    
            break;

        case RESUME_ENABLE:
    	     printk(  "BAT_resume RESUME_ENABLE\n\r");    
            break;
        default:
	     printk("BAT_resume level fail\n\r");                
    }    
#endif
    
    return 0;
}

#else /* CONFIG_PM */
#define BAT_suspend NULL
#define BAT_resume  NULL
#endif /* CONFIG_PM */

static int BAT_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret;
    
#if 0    
    CHARGER_TYPE CHR_Type_num = CHARGER_UNKNOWN;
    
    CHR_Type_num = USB_charger_type_detection();
    switch(CHR_Type_num)
    {
				case CHARGER_UNKNOWN :
					//printk(KERN_ALERT "\n==> mt3351_CHR_DET_ISR : CHARGER_UNKNOWN!!\n");
                                   break;
				case STANDARD_HOST :
					//printk(KERN_ALERT "\n==> mt3351_CHR_DET_ISR : STANDARD_HOST!!\n");
					BMT_status.charger_type = STANDARD_HOST;
                                   break;
				case CHARGING_HOST :
					//printk(KERN_ALERT "\n==> mt3351_CHR_DET_ISR : CHARGING_HOST!!\n");
                                   break;
				case NONSTANDARD_CHARGER :
					//printk(KERN_ALERT "\n==> mt3351_CHR_DET_ISR : NONSTANDARD_CHARGER!!\n");
					BMT_status.charger_type = NONSTANDARD_CHARGER;
                                   break;
				case STANDARD_CHARGER :
					//printk(KERN_ALERT "\n==> mt3351_CHR_DET_ISR : STANDARD_CHARGER!!\n");
                                   break;
				default :
					//printk(KERN_ALERT "\n==> mt3351_CHR_DET_ISR : ERROR!!\n");	
                                   break;	
    }


    BAT_GetVoltage();
    BAT_CheckPMUStatusReg();		 

    switch (cmd) 
    {
        case IO_ENABLE_CHARGING:
            //IO_SetInput(BATT_TEMP_OVER);
            printk(  "IO_ENABLE_CHARGING\n\r");					
            ret = 0;
            break;
            
        case IO_DISABLE_CHARGING:
            //IO_Activate(BATT_TEMP_OVER);
           printk(  "IO_DISABLE_CHARGING\n\r");							
           ret = 0;
           break;
           
        case IOR_BATTERY_STATUS:
            printk(  "IOR_BATTERY_STATUS\n\r");
            if (!access_ok(VERIFY_WRITE, (void __user *) arg, sizeof(BATTERY_STATUS_IOCTL))) 
            {
                ret = -EFAULT;
            }
            else 
            {                
                BATTERY_STATUS_IOCTL bs;
                bs.u16BatteryVoltage   = BMT_status.bat_vol;
                bs.u16ChargeCurrent    = BMT_status.ICharging;
                bs.u8VoltageSource     = VOLTAGE_SOURCE_BATTERY;

                // Power state
                if(BMT_status.charger_exist)
                {
                    switch (BMT_status.bat_charging_state)
                    {
                        case CHR_TICKLE:			
                        case CHR_PRE:														
                        case CHR_CC:
                            bs.u8ChargeStatus = CHARGE_STATE_FAST_CHARGING;			
                            break;

                        case CHR_CV:
        			bs.u8ChargeStatus = CHARGE_STATE_TOPPINGUP;
                            break;

                        case CHR_BATFULL:
                            bs.u8ChargeStatus = CHARGE_STATE_COMPLETE; 							
                            break;
        			        
                        default:
                            bs.u8ChargeStatus = CHARGE_STATE_FAST_CHARGING;
                    }    
                }
                else
                {
                    bs.u8ChargeStatus = CHARGE_STATE_ACPWR_OFF;
                } 
#ifdef CONFIG_DEBUG_MSG            
                printk("IOR_BATTERY_STATUS bat:0x%x curr:0x%x chr:0x%x\n\r",
        	             bs.u16BatteryVoltage, bs.u16ChargeCurrent, bs.u8ChargeStatus);
#endif
                ret = __copy_to_user((void __user *) arg, &bs, sizeof(BATTERY_STATUS_IOCTL)) ? -EFAULT : 0;
            }
            break;
    		
        case IO_DUMP_POWER:
            printk(  "IO_DUMP_POWER\n\r");
            BAT_DumpPowerStatus();
            ret = 0;
            break;
    		
        default:
            printk(  "Invalid IOCTL, cmd = %d\n\r", cmd);			
            ret = -EINVAL;
            break;
    }

#endif

    return ret;
}


static int BAT_open(struct inode *inode, struct file *file)
{
    printk(    "MT3351 BAT_open\n");
    BAT_DumpPowerStatus();
    return nonseekable_open(inode, file);
}

static ssize_t BAT_read(struct file *file, char __user *data, size_t len, loff_t *ppos)
{
    printk(    "MT3351 BAT_read\n");
    return 0;
}

static int BAT_release(struct inode *inode, struct file *file)
{
    printk(    "MT3351 BAT_release\n");
    return 0;
}

/* Kernel interface */
static struct file_operations BAT_fops = {
	.owner    = THIS_MODULE,
	.ioctl	       = BAT_ioctl,
	.open      = BAT_open,
	.release   = BAT_release,
	.read       = BAT_read,
};

void UPDATE_GPTConfig(void);
void BAT_GPTConfig(void);

//static int BAT_probe(struct device *dev)
static int BAT_probe(struct platform_device *pdev)    
{
    int ret;
    
    printk("********* MT3351 BAT_probe *********\n");

    ret = power_supply_register(&pdev->dev, &mt3351_ac_main.psy);
    if (ret)
    {
        #ifdef CONFIG_DEBUG_MSG     
        printk("[MT3351 BAT_probe] power_supply_register AC Fail !!\n");
        #endif
        
        return ret;
    } 
    
    #ifdef CONFIG_DEBUG_MSG     
    printk("[MT3351 BAT_probe] power_supply_register AC Success !!\n");
    #endif

    ret = power_supply_register(&pdev->dev, &mt3351_battery_main.psy);
    if (ret)
    {
        #ifdef CONFIG_DEBUG_MSG 
        printk("[MT3351 BAT_probe] power_supply_register Battery Fail !!\n");
        #endif
        
        return ret;
    }

    #ifdef CONFIG_DEBUG_MSG 
    printk("[MT3351 BAT_probe] power_supply_register Battery Success !!\n");
    #endif

#ifdef CONFIG_DEBUG_MSG 
    printk("\r\nInit ADC efuse slope and offset\r\n");    
#endif   
    ADC_read_efuse(); 

#if 1
    #ifdef CONFIG_DEBUG_MSG 
    printk("\r\nInit UPDATE Function\r\n");    
    #endif   
    //UPDATE_GPTConfig();

    #ifdef CONFIG_DEBUG_MSG 
    printk("\r\nInit BAT_Thread Function\r\n");    
    #endif   
    BAT_GPTConfig();
#endif
    
    return 0;    
}

static int BAT_remove(struct device *dev)
{
    printk("MT3351 BAT_remove\n");

    power_supply_unregister(&mt3351_ac_main.psy);
    power_supply_unregister(&mt3351_battery_main.psy);    
    
    return 0;
}

static void BAT_shutdown(struct device *dev)
{
    printk("BAT_shutdown\n");
	/* Nothing yet */
}

#if 0
static struct device_driver BAT_driver = {
	.name        = "mt3351-battery", 
	.bus           = &platform_bus_type,
	.probe        = BAT_probe,
	.remove     = BAT_remove,
	.shutdown   = BAT_shutdown,
	.suspend	    = BAT_suspend,
	.resume      = BAT_resume
};
#endif

static struct platform_driver BAT_driver = {
    .probe		= BAT_probe,
    .driver = {
        .name        = "mt3351-battery", 
        //.bus           = &platform_bus_type,    
        .remove     = BAT_remove,
        .shutdown   = BAT_shutdown,
        .suspend	    = BAT_suspend,
        .resume      = BAT_resume
    }
};

void BAT_thread(UINT16 data)
{    	
    volatile UINT32 BAT_status = 0, u4ChrStatus = 0;
		
    /*
      *    Check Charger Status
      */
    u4ChrStatus = PMU_GetChargerStatus();    
               			
    if (u4ChrStatus & CHR_DET)
    {
        #ifdef CONFIG_DEBUG_MSG         
        printk(KERN_ALERT "\n[BAT BATTERY] charger insert!!\n");	
        #endif

        CHR_Type_num = USB_charger_type_detection();

        switch(CHR_Type_num)
        {
            case CHARGER_UNKNOWN :
                #ifdef CONFIG_DEBUG_MSG            						
                printk(KERN_ALERT "\n[BAT BATTERY]  CHR_Type_num : CHARGER_UNKNOWN  !!\n");			
                #endif   
                break;
            
            case STANDARD_HOST :
                BMT_status.charger_type = STANDARD_HOST;
                #ifdef CONFIG_DEBUG_MSG            						
                printk(KERN_ALERT "\n[BAT BATTERY]  CHR_Type_num : STANDARD_HOST  !!\n");			
                #endif   
                break;
            
            case CHARGING_HOST :
                break;
            
            case NONSTANDARD_CHARGER :
                BMT_status.charger_type = NONSTANDARD_CHARGER;
                #ifdef CONFIG_DEBUG_MSG            						
                printk(KERN_ALERT "\n[BAT BATTERY]  CHR_Type_num : NONSTANDARD_CHARGER  !!\n");			
                #endif 
                break;
            
            case STANDARD_CHARGER :
                break;
            
            default :
                #ifdef CONFIG_DEBUG_MSG     
                printk(KERN_ALERT "\n[BAT BATTERY]  CHR_Type_num : ERROR!!\n");	
                #endif
                break;
        }
    }
    else
    {
        // No Charger
        
        BMT_status.charger_type = CHARGER_UNKNOWN;
        
        #ifdef CONFIG_DEBUG_MSG         
        printk(KERN_ALERT "\n[BAT BATTERY] charger remove!!\n");	
        #endif
    }

    BATStatusUpdate(0);    

    /*
      *    Check Battery Status
      */
    BAT_status = BAT_CheckBatteryStatus();

    // No Charger
    if(BAT_status == PMU_STATUS_FAIL || g_Battery_Fail)
    {
        BAT_BatteryStatusFailAction();
    }
    // Battery Full
    else if (BMT_status.bat_full)
    {
        #ifdef CONFIG_DEBUG_MSG            
        printk(  "[BAT BATTERY] BAT FULL status and not recharging !!\n\r");			    		    
        #endif
    }
    // Battery Not Full and Charger exist : Do Charging
    else
    {
        // Charging 8hr
        if(BMT_status.total_charging_time >= MAX_CHARGING_TIME)
        {
            BAT_ChargingOTAction();
        }

        // Charging current < 50mA in CV mode
        if (( BMT_status.bat_charging_state == CHR_CV ) &&
             (BMT_status.ICharging <= MAX_CV_CHARGING_CURRENT))
        {
            BAT_BatteryFullAction();
        }
        
        // CV 3hr
        if (( BMT_status.bat_charging_state == CHR_CV ) &&
             (BMT_status.CV_charging_time >= MAX_CV_CHARGING_TIME))
        {
            BAT_BatteryFullAction();
        }

        // Charging flow begin
        switch(BMT_status.bat_charging_state)
        {
            case CHR_TICKLE : 
                //PMU_EnableCharger(TRUE);
                BAT_TrickleConstantCurrentModeAction();
                break;    
                
            case CHR_PRE :
                BAT_PreConstantCurrentModeAction();
                break;    
                
            case CHR_CC :
                BAT_ConstantCurrentModeAction();
                break;    
                
            case CHR_CV :
                BAT_ConstantVoltageModeAction();
                break;

            default:
                printk(  "[BAT ERROR] Invalid status!! \n\r");	                    
        }    
        
        if(BMT_status.ISENSE < BMT_status.bat_vol)
        {
            printk("[BAT ERROR] Visen < Vbat:vbat:%dmV isen:%dmV \r\n", 
                       BMT_status.bat_vol, BMT_status.ISENSE  );			
        }
        
        #ifdef CONFIG_DEBUG_MSG        	
        u4ChrStatus = PMU_GetChargerStatus( );   

        if (u4ChrStatus & CHG_STATUS)
            printk(  "[BAT BATTERY] WAIT_TIMEOUT Charging is proceeding\r\n");
        else
            printk(  "[BAT BATTERY] WAIT_TIMEOUT Charging is stopped\r\n");
        #endif        	

    }

#if 0
    // ****************************
    // If BAT_PDDGetStatus() enable 
    // Do not check voltage here
    // ****************************  

    #ifdef CONFIG_DEBUG_MSG  
    printk(  "[BAT BATTERY] Update Android AC Information\r\n");
    #endif
    mt3351_ac_update(&mt3351_ac_main);

    #ifdef CONFIG_DEBUG_MSG  
    printk(  "[BAT BATTERY] Update Android Battery Information\r\n");
    #endif
    mt3351_battery_update(&mt3351_battery_main);   
#endif

}

void BAT_GPTConfig(void)
{
    //UINT32 count, compareInt = 0;
    GPT_CONFIG config;
    GPT_NUM  gpt_num = GPT1;
    GPT_CLK_SOURCE clkSrc = GPT_CLK_RTC;
    GPT_CLK_DIV clkDiv = GPT_CLK_DIV_1;
    //unsigned long irq_flag;

    GPT_Reset(GPT1);   
    GPT_Init (gpt_num, BAT_thread);
    config.num = gpt_num;
    config.mode = GPT_REPEAT;
    config.clkSrc = clkSrc;
    config.clkDiv = clkDiv;
    config.bIrqEnable = TRUE;
    config.u4CompareL = BAT_TASK_PERIOD*32768;
    config.u4CompareH = 0;
    if (GPT_Config(config) == FALSE )
        return;                    
    GPT_Start(gpt_num);           
    return ;
}

void UPDATE_GPTConfig(void)
{
    //UINT32 count, compareInt = 0;
    GPT_CONFIG config;
    GPT_NUM  gpt_num = GPT4;
    GPT_CLK_SOURCE clkSrc = GPT_CLK_RTC;
    GPT_CLK_DIV clkDiv = GPT_CLK_DIV_1;
    //unsigned long irq_flag;

    GPT_Reset(GPT4);   
    GPT_Init (gpt_num, BATStatusUpdate);
    config.num = gpt_num;
    config.mode = GPT_REPEAT;
    config.clkSrc = clkSrc;
    config.clkDiv = clkDiv;
    config.bIrqEnable = TRUE;
    config.u4CompareL = 0.5*32768;
    config.u4CompareH = 0;
    if (GPT_Config(config) == FALSE )
        return;                    
    GPT_Start(gpt_num);           
    return ;
}


#if 0
static irqreturn_t mt3351_CHR_DET_ISR(int irq, void *dev_id, struct pt_regs *regs)
{
	CHARGER_TYPE CHR_Type_num = CHARGER_UNKNOWN;
	//int tmp32;

	// check PMU_CON28
       volatile UINT32 u4ChrStatus = 0;
	
	//MT3351_IRQMask(MT3351_CHR_DET_IRQ_CODE);
	printk(KERN_ALERT "\n==> mt3351_CHR_DET_ISR : EOI MT3351_CHR_DET_IRQ_CODE!!\n");
	MT3351_IRQClearInt(MT3351_CHR_DET_IRQ_CODE);	
	*MT3351_IRQ_EOIH = (1 << (44 - 32));

#if 0
	for(tmp32 = 0; tmp32 < 100000; tmp32++)
		printk(KERN_ALERT "*");
	printk(KERN_ALERT "\n");
#endif

	mdelay(500); 

	CHR_Type_num = USB_charger_type_detection();

	u4ChrStatus = PMU_GetChargerStatus();    
	if (u4ChrStatus & CHR_DET)
		printk(KERN_ALERT "\n==> charger insert!!\n");	
	else
		printk(KERN_ALERT "\n==> charger remove!!\n");	
	
	switch(CHR_Type_num)
	{
		case CHARGER_UNKNOWN :
			printk(KERN_ALERT "\n==> mt3351_CHR_DET_ISR : CHARGER_UNKNOWN!!\n");
			break;
		case STANDARD_HOST :
			printk(KERN_ALERT "\n==> mt3351_CHR_DET_ISR : STANDARD_HOST!!\n");
			BMT_status.charger_type = STANDARD_HOST;
			break;
		case CHARGING_HOST :
			printk(KERN_ALERT "\n==> mt3351_CHR_DET_ISR : CHARGING_HOST!!\n");
			break;
		case NONSTANDARD_CHARGER :
			printk(KERN_ALERT "\n==> mt3351_CHR_DET_ISR : NONSTANDARD_CHARGER!!\n");
			BMT_status.charger_type = NONSTANDARD_CHARGER;
			break;
		case STANDARD_CHARGER :
			printk(KERN_ALERT "\n==> mt3351_CHR_DET_ISR : STANDARD_CHARGER!!\n");
			break;
		default :
			printk(KERN_ALERT "\n==> mt3351_CHR_DET_ISR : ERROR!!\n");	
	}
#if 0
	if (CHR_Type_num == 0)
		printk(KERN_ALERT "\n==> mt3351_CHR_DET_ISR : STANDARD_CHARGER\n");
	else if (CHR_Type_num == 1)
		printk(KERN_ALERT "\n==> mt3351_CHR_DET_ISR : NON_STANDARD_CHARGER\n");
	else if (CHR_Type_num == 2)
		printk(KERN_ALERT "\n==> mt3351_CHR_DET_ISR : STANDARD_HOST\n");
	else
		printk(KERN_ALERT "\n==> mt3351_CHR_DET_ISR : ERROR!!\n");
#endif

	//MT3351_IRQUnmask(MT3351_CHR_DET_IRQ_CODE);

	return IRQ_HANDLED;
}
#endif

#if 0
static irqreturn_t SLP_interrupt_handler(int irq, void *dev_id, struct pt_regs *regs)
{
    printk("wake up interrupt\n\r");
    g_wake_up = TRUE;
    return IRQ_HANDLED;
}
#endif

#if 0
struct platform_device mt3351_bat_device = {
		.name			= "mt3351-battery",
		.id				= 0,
		.dev				= {
		}
};
#endif

void ADC_LOWBAT_THRESHOLD(int LowBatVoltage)
{
    volatile UINT32 regValue;
    UINT32 nRegVoltThreshold;

    #ifdef CONFIG_DEBUG_MSG
    printk("Start ADC_LOWBAT_THRESHOLD !!\r\n");
    #endif

    // AdcVoltageSetTriggerWay()
    regValue = DRV_Reg32(AUXADC_DET_CTL);
    regValue &= ~(1L << 15);//make sure the bit is 0: Lower   
    DRV_WriteReg32(AUXADC_DET_CTL, regValue);

    // AdcVoltageSetThreshold()
    //nRegVoltThreshold = (LowBatVoltage * 1024) / 2.8 / 10;
    nRegVoltThreshold = (LowBatVoltage * 1024) / 28;    
    regValue = DRV_Reg32(AUXADC_DET_CTL);
    regValue &= ~(0x03FF);   //clean the bits
    regValue |= nRegVoltThreshold;
    DRV_WriteReg32(AUXADC_DET_CTL, regValue);

    // AdcVoltageSetChannel()
    DRV_WriteReg32(AUXADC_DET_SEL, 6);
 
    // AdcVoltageDebounceTimes()
    DRV_WriteReg32(AUXADC_DET_DEBT, 10);

    // AdcVoltageDetectPeriod()
    DRV_WriteReg32(AUXADC_DET_PERIOD, 249*32*1024 / 1000);

    #ifdef CONFIG_DEBUG_MSG
    printk("End ADC_LOWBAT_THRESHOLD !!\r\n");
    #endif
    
}

static irqreturn_t LOW_BAT_ISR(int irq, void *dev_id, struct pt_regs *regs)
{    
    volatile UINT32 u4ChrStatus;
    u4ChrStatus = PMU_GetChargerStatus();    

    // Battery exist and Charger not exist
    if ( ( !(u4ChrStatus & BAT_ONB) ) && ( !(u4ChrStatus & CHR_DET) ) )
    {
        printk( "[BAT BATTERY] LOW_BAT_ISR, Power Off System !!\n\r");			    		    
        kernel_power_off();
    }
    
    return IRQ_HANDLED;
}

static int __init BAT_mod_init(void)
{
    int ret, i;

    ret = platform_driver_register(&BAT_driver);
    if (ret) 
    {
        printk( "[BATTERY] %s Unable to register driver (%d)\n",__FUNCTION__, ret);
        return ret;
    }
    
#if 0
    ret = platform_device_register(&mt3351_bat_device);
    if (ret)
    {
        //driver_unregister(&BAT_driver);
        platform_driver_unregister(&BAT_driver);
    }
#endif

    //enable ISENSE and BASNS divider for ADC measurement    
    DRV_WriteReg(PMU_CON22, (RG_VBAT_OUT_EN|RG_ISENSE_OUT_EN) );
    if (g_ConstantVoltage_vol == 4100)
    {
        //adjust Bank gap to calibration CV value to 4.1V
        //RG_CV_TUNE = b'001, RG_CV_RT = b'00
        //DRV_WriteReg(PMU_CON1E, 0x0800 );
        //RG_CV_TUNE = b'111, RG_CV_RT = b'00
        DRV_WriteReg(PMU_CON1E, 0x3800);
        #ifdef CONFIG_DEBUG_MSG            
        printk( "[BAT BATTERY] ConstantVoltage = 4.1V !!\n\r");			    		    
        #endif    
    }
    else
    {
        //adjust Bank gap to calibration CV value to 4.2V
        //RG_CV_TUNE = b'011, RG_CV_RT = b'01
        DRV_WriteReg(PMU_CON1E, 0x1A00);
        #ifdef CONFIG_DEBUG_MSG            
        printk( "[BAT BATTERY] ConstantVoltage = 4.2V !!\n\r");			    		    
        #endif   
    }

    //high and low temperature, 0C , 50C
    //ANA+0xF04 : 149-128 = 0x15, 613-512 = 0x65
    if(g_BATTERY_TEMP_PROTECT)
    {
        PMU_SetHiLoTemperature(0x15 , 0x65);
        PMU_EnableTempProtect(TRUE);
    }
    else
    {
        PMU_EnableTempProtect(FALSE);
    }

#if 0
 	// charger detecttion ISR
	MT3351_IRQSensitivity(MT3351_CHR_DET_IRQ_CODE, MT3351_EDGE_SENSITIVE);
	//printk(KERN_ALERT "\n==> BAT_mod_init : CHR_DET's IRQSensitivity = MT3351_EDGE_SENSITIVE\n");
	
	ret = request_irq(MT3351_CHR_DET_IRQ_CODE, mt3351_CHR_DET_ISR, SA_INTERRUPT, "mt3351 charger", NULL);
	if(ret != 0)
		printk(KERN_ALERT "==> BAT_mod_init : Failed to request CHR_DET's irq (%d)\n", ret);
	//else
	//	printk(KERN_ALERT "==> BAT_mod_init : Success to request CHR_DET's irq\n");

	MT3351_IRQUnmask(MT3351_CHR_DET_IRQ_CODE);
#endif

    
#if 0
    #ifdef CONFIG_DEBUG_MSG 
    printk("\r\nInit ADC efuse slope and offset\r\n");    
    #endif   
    ADC_read_efuse(); 

    #ifdef CONFIG_DEBUG_MSG 
    printk("\r\nInit UPDATE Function\r\n");    
    #endif   
    UPDATE_GPTConfig();

    #ifdef CONFIG_DEBUG_MSG 
    printk("\r\nInit BAT_Thread Function\r\n");    
    #endif   
    BAT_GPTConfig();

    ret = request_irq(MT3351_LOW_BAT_IRQ_CODE, LOW_BAT_ISR, 0, "mt3351 low battery interrupt", NULL);
    if(ret != 0)
        printk("==> BAT_mod_init : Failed to request LOW_BAT_IRQ (%d)\n", ret);
#endif

    // LowBat = 17 *2 * 100 = 3400
    // If battery < LowBat, then interrupt will happen.
    //work on suspned
    //ADC_LOWBAT_THRESHOLD(17); 
    
    //TODO
    //kernel_thread(BAT_thread, NULL, CLONE_KERNEL);	
    //request_irq(MT3351_IRQ_SLEEPCTRL_CODE, SLP_interrupt_handler, 0, "MT3351_SLEEP", NULL);

    for (i=0; i<BATTERY_AVERAGE_SIZE; i++) {
        batteryCurrentBuffer[i] = 0;
        batteryVoltageBuffer[i] = 0;
    }
    batteryVoltageSum = 0;
    batteryCurrentSum = 0;
    
    return 0;
}

static void __exit BAT_mod_exit(void)
{
    //driver_unregister(&BAT_driver);
    platform_driver_unregister(&BAT_driver);
}

module_init(BAT_mod_init);
module_exit(BAT_mod_exit);

MODULE_AUTHOR("James Lo <James.lo@mediatek.com>");
MODULE_DESCRIPTION("MT3351 Battery Driver");
MODULE_LICENSE("GPL");

