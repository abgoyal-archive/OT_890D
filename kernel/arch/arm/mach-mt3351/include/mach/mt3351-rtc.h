
#ifndef __MT3351_RTC_H

#include <mach/mt3351_reg_base.h>
#define MT3351_RTC_PROT1       ((volatile unsigned int   *)(RTC_base+0x0004))   
#define MT3351_RTC_PROT2       ((volatile unsigned int   *)(RTC_base+0x0008))   
#define MT3351_RTC_PROT3       ((volatile unsigned int   *)(RTC_base+0x000C))   
#define MT3351_RTC_PROT4       ((volatile unsigned int   *)(RTC_base+0x0010))   
#define MT3351_RTC_KEY         ((volatile unsigned int   *)(RTC_base+0x0014))   
#define MT3351_RTC_CTL         ((volatile unsigned int   *)(RTC_base+0x0018))  
#define MT3351_RTC_TC_YEA      ((volatile unsigned int   *)(RTC_base+0x001c))   
#define MT3351_RTC_TC_MON      ((volatile unsigned int   *)(RTC_base+0x0020))   
#define MT3351_RTC_TC_DOM      ((volatile unsigned int   *)(RTC_base+0x0024))   
#define MT3351_RTC_TC_DOW      ((volatile unsigned int   *)(RTC_base+0x0028))   
#define MT3351_RTC_TC_HOU      ((volatile unsigned int   *)(RTC_base+0x002C))   
#define MT3351_RTC_TC_MIN      ((volatile unsigned int   *)(RTC_base+0x0030))   
#define MT3351_RTC_TC_SEC      ((volatile unsigned int   *)(RTC_base+0x0034))   
#define MT3351_RTC_AL_CTL      ((volatile unsigned int   *)(RTC_base+0x0038))   
#define MT3351_RTC_AL_YEAR     ((volatile unsigned int   *)(RTC_base+0x003c))   
#define MT3351_RTC_AL_MON      ((volatile unsigned int   *)(RTC_base+0x0040))   
#define MT3351_RTC_AL_DOM      ((volatile unsigned int   *)(RTC_base+0x0044))   

#define MT3351_RTC_AL_DOW      ((volatile unsigned int   *)(RTC_base+0x0048))  
#define MT3351_RTC_AL_HOU      ((volatile unsigned int   *)(RTC_base+0x004c))   
#define MT3351_RTC_AL_MIN      ((volatile unsigned int   *)(RTC_base+0x0050))   
#define MT3351_RTC_AL_SEC      ((volatile unsigned int   *)(RTC_base+0x0054))   

#define MT3351_RTC_RIP_CTL     ((volatile unsigned int   *)(RTC_base+0x0058))   
#define MT3351_RTC_RIP_CNTH    ((volatile unsigned int   *)(RTC_base+0x005c))  
#define MT3351_RTC_RIP_CNTL    ((volatile unsigned int   *)(RTC_base+0x0060))  
#define MT3351_RTC_TIMER_CTL   ((volatile unsigned int   *)(RTC_base+0x0064))   

#define MT3351_RTC_TIMER_CNTH  ((volatile unsigned int   *)(RTC_base+0x0068))   
#define MT3351_RTC_TIMER_CNTL  ((volatile unsigned int   *)(RTC_base+0x006c))   

#define MT3351_RTC_XOSC_CFG    ((volatile unsigned int   *)(RTC_base+0x0070))   
#define MT3351_RTC_DEBOUNCE    ((volatile unsigned int   *)(RTC_base+0x0074))   

#define MT3351_RTC_WAVEOUT     ((volatile unsigned int   *)(RTC_base+0x0078))   
#define MT3351_RTC_IRQ_STA     ((volatile unsigned int   *)(RTC_base+0x007c))  


// RTC IRQ Status
#define MT3351_RTC_STA_ALARM_MASK 0x01
#define MT3351_RTC_STA_TIMER_MASK 0x02

#define MT3351_RTC_STA_ALARM      0x01
#define MT3351_RTC_STA_TIMER      0x02
//RTC internal control registers
#define MT3351_RC_STOP_MASK       0x01
#define MT3351_SIM_RTC_MASK       0x02
#define MT3351_RESERVE1_MASK      0x04
#define MT3351_RESERVE2_MASK      0x08
#define MT3351_PORT_PASS_MASK     0x10
#define MT3351_INVALID_WR_MASK    0x20
#define MT3351_INHIBIT_WR_MASK    0x40
#define MT3351_DEBNCE_OK_MASK     0x80

//RTC Alarm control
#define MT3351_Alarm_EN_MASK      0x01
#define MT3351_Alarm_SEC_MASK     0x02
#define MT3351_Alarm_MIN_MASK     0x04
#define MT3351_Alarm_HOR_MASK     0x08
#define MT3351_Alarm_DOW_MASK     0x10
#define MT3351_Alarm_DOM_MASK     0x20
#define MT3351_Alarm_MON_MASK     0x40
#define MT3351_Alarm_YEA_MASK     0x80

// RTC_TIMER_CTL
#define MT3351_EXTEN_MASK         0x01
#define MT3351_INTEN_MASK         0x02
#define MT3351_DRV_MASK           0x0C
#define MT3351_32K_OUT_EN_MASK    0x10

#define MT3351_INTEN_ON           0x02
#define MT3351_TR_DRV             0x08   
// Software Key
#define MT3351_RTC_KEY_VALUE      0x12

// hardware Key
#define RTC_PROT1_KEY             0xA3
#define RTC_PROT2_KEY             0x57
#define RTC_PROT3_KEY             0x67
#define RTC_PROT4_KEY             0xD2

// default year to second for RTC timer
#define MT3351_RTC_YEAR           8
#define MT3351_RTC_MON            1
#define MT3351_RTC_DOM            1
#define MT3351_RTC_HOU            0
#define MT3351_RTC_MIN            0
#define MT3351_RTC_SEC            0

#define MAX_DEBNCE_TIMES          0x1fffffff
#define RTC_DEVICE	              "mt3351-rtc"
#define MT3351_RTC_VERSION	      "v 0.2"

/* Those are the bits from a classic RTC we want to mimic */
#define RTC_IRQF		          0x80	/* any of the following 3 is active */
#define RTC_PF			          0x40
#define RTC_AF		              0x20
#define RTC_UF		              0x10
#define RTC_IS_OPEN		          0x01	/* means /dev/rtc is in use	*/

#define TIMER_SETCNT_TIME         1 
//bool type
#ifndef bool
    #define bool                  unsigned char
#endif
#ifndef FALSE
    #define false                 0
#endif

#ifndef TRUE
    #define true                  1
#endif


int get_rtc_time(struct device *dev,struct rtc_time *rtc_tm);
int set_rtc_time(struct device *dev, struct rtc_time *rtc_tm);
int get_rtc_alm_time(struct device *dev, struct rtc_time *rtc_tm);
int set_rtc_alm_time(struct device *dev, struct rtc_time *rtc_tm);
#endif
