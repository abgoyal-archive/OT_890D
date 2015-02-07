




#include <mach/mt6516_typedefs.h>
#include "mt6516_asm_regs.h"
#include "yusu_asm.h"
#include "yusu_audio_stream.h"
#include <linux/spinlock.h>

 void Asm_Set(UINT32 offset, UINT32 value , UINT32 mask)
 {
     volatile UINT32 address = (ASM_REGS_BASE+ offset);
     volatile UINT32 *ASM_REG = (volatile UINT32 *)address;
     *ASM_REG &= (~mask);
     *ASM_REG |= (value&mask);
 }

UINT32 Asm_Get(UINT32 offset)
{
     volatile UINT32 *value;
     UINT32 address = (ASM_REGS_BASE+ offset);
     value = (volatile UINT32  *)(address);
     return *value;
}

void Asm_Enable_Memory_Power()
{
    UINT32 mask = 0xffffffe3;
    volatile UINT32 *ASM_REG =(volatile UINT32 *) (ASM_MEMORY_CONTROL);
    *ASM_REG &= mask;
}

void Asm_Disable_Memory_Power()
{
    UINT32 mask = 0xc;
    volatile UINT32 *ASM_REG = (volatile UINT32 *) (ASM_MEMORY_CONTROL);
    *ASM_REG |=  mask;
}

