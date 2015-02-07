

#ifndef __DSI_REG_H__
#define __DSI_REG_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct
{
    unsigned PLL_EN        : 1;
    unsigned RG_PLL_LN     : 2;
    unsigned RG_CK_SEL     : 2;
    unsigned RG_NX_CK_SEL  : 2;
    unsigned RG_PLL_DIV1   : 6;
    unsigned DSI_FIX_PAT   : 1;
    unsigned DSI_BIST_EN   : 1;
    unsigned PHY_BIST_MODE : 1;
    unsigned rsv_16        : 16;
} DSI_PHY_REG_ANACON0, *PDSI_PHY_REG_ANACON0;


typedef struct
{
    unsigned RG_PLL_DIV2     : 4;
    unsigned RG_PLL_CCP      : 4;
    unsigned RG_PLL_CVC_OB   : 2;
    unsigned RG_PLL_CLF      : 2;
    unsigned RG_PLL_CLKR     : 1;
    unsigned RG_LNT_LOOPBACK : 3;
    unsigned rsv_16          : 16;
} DSI_PHY_REG_ANACON1, *PDSI_PHY_REG_ANACON1;


typedef struct
{
    unsigned RG_LNT_BGR_EN        : 1;
    unsigned RG_LNT_BGR_CHPEN     : 1;
    unsigned RG_LNT_BGR_SELPH     : 1;
    unsigned RG_LNT_BGR_DIV       : 2;
    unsigned RG_LNT_BGR_DOUT1_SEL : 2;
    unsigned RG_LNT_BGR_DOUT2_SEL : 2;
    unsigned RG_LNT_HSRX_CALI     : 3;
    unsigned RG_LNT_LPCD_CALI     : 3;
    unsigned rsv_15               : 17;
} DSI_PHY_REG_ANACON2, *PDSI_PHY_REG_ANACON2;


typedef struct
{
    DSI_PHY_REG_ANACON0 ANACON0;        // 0000
    DSI_PHY_REG_ANACON1 ANACON1;        // 0004
    DSI_PHY_REG_ANACON2 ANACON2;        // 0008
} volatile DSI_PHY_REGS, *PDSI_PHY_REGS;


STATIC_ASSERT(0x000C == sizeof(DSI_PHY_REGS));

extern PDSI_PHY_REGS const DSI_PHY_REG;

#ifdef __cplusplus
}
#endif

#endif // __DSI_REG_H__

