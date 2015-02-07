




#include <mach/mt6516_typedefs.h>
#include "mt6516_ana_regs.h"

 void Ana_Set(UINT32 offset, UINT32 value , UINT32 mask)
 {
     volatile UINT32 address = (ANA_REGS_BASE+ offset);
     volatile UINT32 *ANA_REG = ( volatile UINT32 *)address;
     *ANA_REG &= (~mask);
     *ANA_REG |= (value&mask);
 }

UINT32 Ana_Get(UINT32 offset)
{
     volatile UINT32 *value;
     UINT32 address = (ANA_REGS_BASE+ offset);
     value = (volatile UINT32  *)(address);
     return *value;
}


