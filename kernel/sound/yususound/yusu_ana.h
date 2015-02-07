




#ifndef _YUSU_ANA_H
#define _YUSU_ANA_H

#define ANA_REGS_END                    (PLL_BASE + 0x220)
#define ANA_OFFSET                         (0x220)
#define ANA_MASK_ALL                     (0xffffffff)

#define ASM_MIXER_LINE_MAX                  (3)
#define ASM_EQ_NUM_MAX                      (45)

//------------------------------------------------------------------------------
// Analog Block Register
//
typedef enum {
    AFE_VAG_CON         =  0x0100,    //0100..0x103
    AFE_VAC_CON0       =  0x0104,    //0104..0x107
   AFE_VAC_CON1       =  0x0108,   //0x108..0x10b
   AFE_VAPDN_CON    =  0x010c,   //0x10c..0c10f 
   AFE_VAC_CON2       =  0x0110,   // 0x110..0x113
    AFE_AAG_CON         = 0x200,    // 0200..0203
    AFE_AAC_CON         = 0x204,    // 0204..0207
    AFE_AAPDB_CON       = 0x208,    // 0208..020B
    AFE_AAC_NEW_CON     = 0x20c     // 020C..020F
} ANA_REGS;

void Ana_Set(UINT32 offset,UINT32 value,UINT32 mask);
UINT32 Ana_Get(UINT32 offset);

#endif

 
