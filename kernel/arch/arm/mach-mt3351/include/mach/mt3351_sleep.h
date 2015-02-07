


#ifndef _SLPCTL_H
#define _SLPCTL_H

#include    <mach/mt3351_sleep.h>
#include    <mach/mt3351_typedefs.h>
#include    <mach/mt3351_reg_base.h>


#define SLP_PAUSE_H            (SLPCTRL_BASE+0x0200)
#define SLP_PAUSE_L 	       (SLPCTRL_BASE+0x0204)
#define SLP_ECLK_SETTLE 	   (SLPCTRL_BASE+0x0208)
#define SLP_FPAUSE_H  	       (SLPCTRL_BASE+0x020C)
#define SLP_FPAUSE_L           (SLPCTRL_BASE+0x0210)
#define SLP_CNTL               (SLPCTRL_BASE+0x0218)
#define SLP_STAT               (SLPCTRL_BASE+0x021C)
#define SLP_CFG                (SLPCTRL_BASE+0x022C)
#define WAKE_PLL               (SLPCTRL_BASE+0x0238)

/*SLP_PAUSE_H*/
#define SLP_PAUSE_H_MASK 0x0007

/*SLP_PAUSE_L*/
#define SLP_PAUSE_L_MASK 0xFFFF

/*SLP_ECLK_SETTLE*/
#define SLP_ECLK_SETTLE_MASK 0x3FFF

/*SLP_FPAUSE_H*/
#define SLP_FPAUSE_H_MASK 0x0007

/*SLP_FPAUSE_L*/
#define SLP_FPAUSE_L_MASK 0xFFFF

/*SLP_CNTL*/
#define SLP_CNTL_START 0x0002

/*SLP_STAT*/
#define PAUSE_ABT 0x0100
#define SETTLT_CPL 0x0080
#define PAUSE_CPL 0x0040
#define PAUSE_INT 0x0020
#define PAUSE_REQ 0x0010

/*SLP_CFG*/
#define SLEEP_CNT_EN  0x1
#define SLEEP_INTR_EN 0x2
#define WAKEUP_SOURCE_ALL_UNMASK 0x1FC


/*WAKE_PLL*/
#define PLL_RESET_EN 0x8000
#define PLL_RESET_WIDTH_MASK 0x3FFF 

typedef enum
{
    WAKE_INT_EN = 0x0002,
    WAKE_KP = 0x0004,
    WAKE_EINT = 0x0008,
    WAKE_RTC = 0x0010,
    WAKE_MSDC = 0x0020,
    WAKE_TMR = 0x0040,
    WAKE_TP = 0x0080,
    WAKE_LOWBAT = 0x0100
}WAKE_SOURCE;

typedef enum
{
    WAKE_REASON_UNKNOWN = 0,
    WAKE_REASON_SOURCE,
    WAKE_REASON_TMR,
}WAKE_REASON;



BOOL SLPCTL_Set_Sleep_Pause_Duration(UINT32 millisec);
BOOL SLPCTL_Set_Ext_CLK_Settle_Time(UINT32 units);
BOOL SLPCTL_Set_Final_Sleep_Pause_Counter(UINT32 uints);
UINT16 SLPCTL_Get_Pause_Status(void); //new
UINT16 SLPCTL_Get_WakeUp_Source(void); //new
void SLPCTL_Set_Pause_Start(BOOL start);
void SLPCTL_Reset_WakeUp_Source(void); //new
void SLPCTL_Set_WakeUp_Source(WAKE_SOURCE Source);
void SLPCTL_Clear_WakeUp_Source(WAKE_SOURCE Source); //new
void SLPCTL_Set_PLL_WakeUp_Reset(BOOL enable, UINT32 units);

#endif  




