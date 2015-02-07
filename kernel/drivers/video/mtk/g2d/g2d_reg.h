

#ifndef __G2D_REG_H__
#define __G2D_REG_H__

#include <stddef.h>

#include <mach/mt6516_typedefs.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    unsigned G2D_ENG_MODE   : 4;
    unsigned rsv_04         : 1;
    unsigned DST_CLR_MODE   : 3;
    unsigned rsv_08         : 1;
    unsigned SRC_CLR_MODE   : 3;
    unsigned rsv_12         : 1;
    unsigned PAT_CLR_MODE   : 3;
    unsigned rsv_15         : 16;
} G2D_REG_FMODE_CON, *PG2D_REG_FMODE_CON;


typedef struct
{
    unsigned BROT       : 3;
    unsigned BITA       : 1;
    unsigned BDIR       : 2;
    unsigned CLRGD_EN   : 1;
    unsigned DST_KEY_EN : 1;
    unsigned rsv_08     : 1;
    unsigned IMG_EN     : 1;
    unsigned rsv_10     : 2;
    unsigned BIDX       : 2;
    unsigned BIDX_EN    : 1;
    unsigned rsv_15     : 1;

    unsigned ALPHA      : 8;
    unsigned ROP3_EN    : 1;
    unsigned ROP4_EN    : 1;
    unsigned rsv_26     : 1;
	unsigned CREP_EN    : 1;
	unsigned RSZ_EN     : 1;
	unsigned FMSB_FIRST : 1;
	unsigned FNGB       : 1;
	unsigned FITA       : 1;
} G2D_REG_SMODE_CON, *PG2D_REG_SMODE_CON;


typedef struct
{
    unsigned RST        : 1;
    unsigned SRC_KEY_EN : 1;
    unsigned CLP_EN     : 1;
    unsigned RD_BUF_EN  : 1;
    unsigned WR_BUF_EN  : 1;
    unsigned SRC_CLP_EN : 1;
    unsigned WM_CLR_X   : 1;
    unsigned WM_CPY_X   : 1;
    unsigned WM_ALP_RND : 1;
    unsigned WM_PMUL    : 1;
    unsigned WM_PIT_U   : 1;
    unsigned WM_DST_XP  : 1;
    unsigned rsv_12     : 3;
    unsigned TIDLE_RST  : 1;
    unsigned rsv_16     : 16;
} G2D_REG_COM_CON, *PG2D_REG_COM_CON;


typedef struct {
    INT16 Y;
    INT16 X;
} G2D_REG_COORD, *PG2D_REG_COORD;


typedef struct {
    UINT16 HEIGHT;
    UINT16 WIDTH;
} G2D_REG_SIZE, *PG2D_REG_SIZE;


typedef struct {
    UINT8 HEIGHT;
    UINT8 WIDTH;
    UINT8 Y;
    UINT8 X;
} G2D_REG_PAT_RECT, *PG2D_REG_PAT_RECT;


typedef struct {
    UINT8  Y;
    UINT8  X;
    UINT16 rsv_16;
} G2D_REG_PAT_OFS, *PG2D_REG_PAT_OFS;


typedef struct {
    UINT16 DOWN;
    UINT16 UP;
} G2D_REG_RESIZE, *PG2D_REG_RESIZE;


typedef struct {
    UINT8  CODE1;
    UINT8  CODE0;
    UINT16 rsv_16;
} G2D_REG_ROP_CODE, *PG2D_REG_ROP_CODE;


typedef struct {
    INT32 A;
    INT32 R;
    INT32 G;
    INT32 B;
} G2D_REG_GRADIENT, *PG2D_REG_GRADIENT;


typedef struct {
    UINT32 rsvr_0000[0x40];             // 0000..00FC
    
    G2D_REG_FMODE_CON FIRE_MODE_CTRL;   // 0100
    G2D_REG_SMODE_CON SMODE_CON;        // 0104
    G2D_REG_COM_CON   COM_CON;          // 0108
    UINT32            IRQ_CON;          // 010C
    UINT32            COM_STA;          // 0110
    UINT32            rsvr_0114[0x3B];  // 0114..01FC
    UINT32            SRC_BASE;         // 0200
    UINT32            SRC_PITCH;        // 0204
    G2D_REG_COORD     SRC_XY;           // 0208
    G2D_REG_SIZE      SRC_SIZE;         // 020C
    UINT32            SRC_KEY;          // 0210

    UINT32    DST_AVO_COLOR;            // 0214
    UINT32    DST_REP_COLOR;            // 0218

    UINT32    rsvr_021C[0x39];          // 021C..02FC

    UINT32        DST_BASE;             // 0300
    UINT32        DST_PITCH;            // 0304
    G2D_REG_COORD DST_XY;               // 0308
    UINT32        rsvr_030C[3];         // 030C..0314
    G2D_REG_SIZE  DST_SIZE;             // 0318
    UINT32        rsvr_031C;            // 031C

    UINT32            PAT_BASE;         // 0320
    UINT32            PAT_PITCH;        // 0324
    G2D_REG_PAT_RECT  PAT_RECT;         // 0328
    G2D_REG_PAT_OFS   PAT_XY_OFS;       // 032C

    UINT32        MSK_BASE;             // 0330
    UINT32        MSK_PITCH;            // 0334
    G2D_REG_COORD MSK_XY;               // 0338
    G2D_REG_SIZE  MSK_SIZE;             // 033C

    G2D_REG_RESIZE RSIZE;               // 0340
        
    G2D_REG_ROP_CODE ROP_CODE;          // 0344

    UINT32 rsvr_0348[0x2E];             // 0348..03FC

    UINT32 FOREGROUND_COLOR;            // 0400
    UINT32 BACKGROUND_COLOR;            // 0404

    G2D_REG_COORD CLIP_MIN;             // 0408
    G2D_REG_COORD CLIP_MAX;             // 040C

    G2D_REG_GRADIENT GRADIENT_X;        // 0410..041C
    G2D_REG_GRADIENT GRADIENT_Y;        // 0420..042C

    UINT32 BUF_START_ADDR[2];           // 0430..0434

    UINT32 rsvr_0438[0xB2];             // 0438..06FC

    UINT8 TILT[32];                     // 0700..071F
} volatile G2D_REGS, *PG2D_REGS;

STATIC_ASSERT(0x100 == offsetof(G2D_REGS, FIRE_MODE_CTRL));
STATIC_ASSERT(0x200 == offsetof(G2D_REGS, SRC_BASE));
STATIC_ASSERT(0x300 == offsetof(G2D_REGS, DST_BASE));
STATIC_ASSERT(0x400 == offsetof(G2D_REGS, FOREGROUND_COLOR));
STATIC_ASSERT(0x720 == sizeof(G2D_REGS));

#ifdef __cplusplus
}
#endif

extern PG2D_REGS const G2D_REG;


#endif  // __G2D_REG_H__
