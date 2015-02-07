

#ifndef WDT_SW_H
#define WDT_SW_H

#include "mt3351_typedefs.h"

void MT3351_WDT_SetValue(kal_uint16 value);
kal_uint8 MT3351_WDT_CheckStat(void);
void MT3351_WDT_Enable(kal_bool en, kal_bool auto_rstart, kal_bool pwr_key_dis ,kal_bool IRQ);
void MT3351_WDT_SetExten(kal_bool en);
kal_bool MT3351_WDT_SW_MCU_PERI_RESET(MT3351_MCU_PERIPH MCU_peri);
kal_bool MT3351_WDT_SW_MM_PERI_RESET(MT3351_MM_PERIPH MCU_peri);

#endif

