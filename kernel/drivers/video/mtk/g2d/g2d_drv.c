

#include <linux/stddef.h>

#include <mach/mt6516_typedefs.h>
#include <mach/mt6516_reg_base.h>
#include <mach/mt6516_pll.h>

#include "g2d_reg.h"
#include "g2d_drv.h"
#include "g2d_structure.h"
#include "mtk_g2d.h"

#define ENABLE_G2D_INTERRUPT    1
#define G2D_PERF_PROF_ENABLE    0

#if ENABLE_G2D_INTERRUPT
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <asm/tcm.h>
#include <mach/irqs.h>
static wait_queue_head_t _g2d_wait_queue;
#endif


PG2D_REGS const G2D_REG = (PG2D_REGS)(G2D_BASE);

static BOOL _g2dInitialized = FALSE;
static G2D_CONTEXT _g2dContext = {0};

#if G2D_PERF_PROF_ENABLE
static struct timeval _t1, _t2;
#endif

// ---------------------------------------------------------------------------
//  Backup/Restore G2D registers before/after MTCMOS GRAPHSYS1 power off/on
// ---------------------------------------------------------------------------

static BOOL s_isG2dPowerOn = FALSE;
static G2D_REGS regBackup;

#define G2D_REG_OFFSET(r)       offsetof(G2D_REGS, r)
#define REG_ADDR(base, offset)  (((BYTE *)(base)) + (offset))

const UINT32 BACKUP_G2D_REG_OFFSETS[] =
{
    // WARNING: don't backup this register, it will trigger G2D HW
    // G2D_REG_OFFSET(FIRE_MODE_CTRL), 
    
    G2D_REG_OFFSET(SMODE_CON),
    G2D_REG_OFFSET(COM_CON),
    G2D_REG_OFFSET(IRQ_CON),
    
    G2D_REG_OFFSET(SRC_BASE),
    G2D_REG_OFFSET(SRC_PITCH),
    G2D_REG_OFFSET(SRC_XY),
    G2D_REG_OFFSET(SRC_SIZE),
    G2D_REG_OFFSET(SRC_KEY),

    G2D_REG_OFFSET(DST_AVO_COLOR),
    G2D_REG_OFFSET(DST_REP_COLOR),
    G2D_REG_OFFSET(DST_BASE),
    G2D_REG_OFFSET(DST_PITCH),
    G2D_REG_OFFSET(DST_XY),
    G2D_REG_OFFSET(DST_SIZE),
    
    G2D_REG_OFFSET(PAT_BASE),
    G2D_REG_OFFSET(PAT_PITCH),
    G2D_REG_OFFSET(PAT_RECT),
    G2D_REG_OFFSET(PAT_XY_OFS),
    
    G2D_REG_OFFSET(MSK_BASE),
    G2D_REG_OFFSET(MSK_PITCH),
    G2D_REG_OFFSET(MSK_XY),
    G2D_REG_OFFSET(MSK_SIZE),
    
    G2D_REG_OFFSET(RSIZE),
    G2D_REG_OFFSET(ROP_CODE),
    
    G2D_REG_OFFSET(FOREGROUND_COLOR),
    G2D_REG_OFFSET(BACKGROUND_COLOR),

    G2D_REG_OFFSET(CLIP_MIN),
    G2D_REG_OFFSET(CLIP_MAX),

    G2D_REG_OFFSET(GRADIENT_X),
    G2D_REG_OFFSET(GRADIENT_Y),

    G2D_REG_OFFSET(BUF_START_ADDR[0]),
    G2D_REG_OFFSET(BUF_START_ADDR[1]),
};

static void _BackupG2DRegisters(void)
{
    G2D_REGS *reg = &regBackup;
    UINT32 i;
    
    for (i = 0; i < ARY_SIZE(BACKUP_G2D_REG_OFFSETS); ++ i)
    {
        OUTREG32(REG_ADDR(reg, BACKUP_G2D_REG_OFFSETS[i]),
                 AS_UINT32(REG_ADDR(G2D_REG, BACKUP_G2D_REG_OFFSETS[i])));
    }
}

static void _RestoreG2DRegisters(void)
{
    G2D_REGS *reg = &regBackup;
    UINT32 i;

    for (i = 0; i < ARY_SIZE(BACKUP_G2D_REG_OFFSETS); ++ i)
    {
        OUTREG32(REG_ADDR(G2D_REG, BACKUP_G2D_REG_OFFSETS[i]),
                 AS_UINT32(REG_ADDR(reg, BACKUP_G2D_REG_OFFSETS[i])));
    }

}

static void _ResetBackupedG2DRegisterValues(void)
{
    G2D_REGS *regs = &regBackup;
    memset((void*)regs, 0, sizeof(G2D_REGS));

    OUTREG32(&regs->SMODE_CON, 0x00000037);
    OUTREG32(&regs->CLIP_MAX, 0x03FF03FF);
}

// ---------------------------------------------------------------------------
//  Utility Functions
// ---------------------------------------------------------------------------

#if NO_FLOAT
#define TO_S9_16(x)  (x)
#else
__inline INT32 TO_S9_16(FLOAT x)
{
    return (INT32)(x * (1 << 16));
}
#endif

__inline INT16 TO_S12(INT32 x)
{
    ASSERT(x <= 2047 && x >= -2048);
    return (INT16)x;
}

// ---------------------------------------------------------------------------
//  Local Static Functions
// ---------------------------------------------------------------------------

static void _InitG2DContext(void)
{
    // Clean up G2D Context
    memset(&_g2dContext, 0, sizeof(_g2dContext));

    _g2dContext.rotate    = G2D_ROTATE_0;
    _g2dContext.direction = G2D_DIR_UPPER_LEFT;
}


static __inline UINT32 _IsG2DBusy(void)
{
    return (G2D_REG->COM_STA & 0x1);
}


static void _WaitForEngineNotBusy(void)
{
#if ENABLE_G2D_INTERRUPT
    while (_IsG2DBusy())
    {
        wait_event_interruptible(_g2d_wait_queue, !_IsG2DBusy());
    }
#else
    while (_IsG2DBusy()) ;
#endif
}


static void _ConfigClipping(void)
{
    G2D_REG_COM_CON ctrl = G2D_REG->COM_CON;

    if (_g2dContext.enableClip)
    {
        G2D_REG_COORD min, max;

        min.X = TO_S12(_g2dContext.clipWnd.x);
        min.Y = TO_S12(_g2dContext.clipWnd.y);
        max.X = TO_S12(_g2dContext.clipWnd.x + 
                (INT32)_g2dContext.clipWnd.width - 1);
        max.Y = TO_S12(_g2dContext.clipWnd.y + 
                (INT32)_g2dContext.clipWnd.height - 1);
        
        ctrl.CLP_EN = 0x1;
        OUTREG32(&G2D_REG->CLIP_MIN, AS_UINT32(&min));
        OUTREG32(&G2D_REG->CLIP_MAX, AS_UINT32(&max));
    }
    else
    {
        ctrl.CLP_EN = 0x0;
    }

    OUTREG32(&G2D_REG->COM_CON, AS_UINT32(&ctrl));
}


static void _ConfigRotateDirection(void)
{
    G2D_REG_SMODE_CON ctrl = G2D_REG->SMODE_CON;
    
    ctrl.BROT = (_g2dContext.rotate & 0x7);
    ctrl.BDIR = (_g2dContext.direction & 0x3);

    OUTREG32(&G2D_REG->SMODE_CON, AS_UINT32(&ctrl));
}


static void _ConfigSrcColorKeying(void)
{
    G2D_REG_COM_CON ctrl = G2D_REG->COM_CON;

    if (_g2dContext.enableSrcColorKey)
    {
        ctrl.SRC_KEY_EN = 0x1;
        G2D_REG->SRC_KEY = _g2dContext.srcColorKey;
    }
    else
    {
        ctrl.SRC_KEY_EN = 0x0;
    }

    OUTREG32(&G2D_REG->COM_CON, AS_UINT32(&ctrl));
}


static void _ConfigDstColorKeying(void)
{
    G2D_REG_SMODE_CON ctrl = G2D_REG->SMODE_CON;

    if (_g2dContext.enableDstColorKey)
    {
        ctrl.DST_KEY_EN = 0x1;
        G2D_REG->BACKGROUND_COLOR = _g2dContext.dstColorKey;
    }
    else
    {
        ctrl.DST_KEY_EN = 0x0;
    }

    OUTREG32(&G2D_REG->SMODE_CON, AS_UINT32(&ctrl));
}


static void _ConfigColorReplacement(void)
{
    G2D_REG_SMODE_CON ctrl = G2D_REG->SMODE_CON;

    if (_g2dContext.enableColorReplacement)
    {
        ctrl.CREP_EN = 0x1;
        G2D_REG->DST_AVO_COLOR = _g2dContext.avoidColor;
        G2D_REG->DST_REP_COLOR = _g2dContext.replaceColor;
    }
    else
    {
        ctrl.CREP_EN = 0x0;
    }

    OUTREG32(&G2D_REG->SMODE_CON, AS_UINT32(&ctrl));
}


static void _ConfigStretchBlt(void)
{
    G2D_REG_SMODE_CON ctrl = G2D_REG->SMODE_CON;

    if (_g2dContext.enableStretch)
    {
        G2D_REG_RESIZE resize;
        resize.DOWN = _g2dContext.stretchFactor.down;
        resize.UP   = _g2dContext.stretchFactor.up;
        
        ctrl.RSZ_EN = 0x1;
        OUTREG32(&G2D_REG->RSIZE, AS_UINT32(&resize));
    }
    else
    {
        ctrl.RSZ_EN = 0x0;
    }

    OUTREG32(&G2D_REG->SMODE_CON, AS_UINT32(&ctrl));
}


static void _ConfigTiltBlt(G2D_FIRE_MODE fireMode)
{
    G2D_REG_SMODE_CON ctrl = G2D_REG->SMODE_CON;

    if (_g2dContext.enableItalicBitblt)
    {
        UINT32 i;
        
        if (G2D_FIRE_FONT_DRAWING == fireMode)
        {
            ctrl.FITA = 0x1;
        }
        else
        {
            ctrl.BITA= 0x1;
        }

        for (i = 0; i < 32; ++ i)
        {
            G2D_REG->TILT[i] = _g2dContext.tiltValues[i];
        }
    }
    else
    {
        if (G2D_FIRE_FONT_DRAWING == fireMode)
        {
            ctrl.FITA = 0x0;
        }
        else
        {
            ctrl.BITA = 0x0;
        }
    }

    OUTREG32(&G2D_REG->SMODE_CON, AS_UINT32(&ctrl));
}


#define DEFINE_SURFACE_CONFIG_FUNCTION(name1, name2, name3)                  \
    static void _Config##name2##Surface(void)                                \
    {                                                                        \
        G2D_REG_COORD coord;                                                 \
        G2D_REG_SIZE  size;                                                  \
                                                                             \
        G2D_REG->name3##_BASE  = (UINT32)_g2dContext.name1##Surf.addr;       \
        G2D_REG->name3##_PITCH = _g2dContext.name1##Surf.width;              \
                                                                             \
        coord.X     = _g2dContext.name1##Surf.rect.x;                        \
        coord.Y     = _g2dContext.name1##Surf.rect.y;                        \
        size.WIDTH  = _g2dContext.name1##Surf.rect.width;                    \
        size.HEIGHT = _g2dContext.name1##Surf.rect.height;                   \
                                                                             \
        OUTREG32(&G2D_REG->name3##_XY, AS_UINT32(&coord));                   \
        OUTREG32(&G2D_REG->name3##_SIZE, AS_UINT32(&size));                  \
    }

DEFINE_SURFACE_CONFIG_FUNCTION(dst, Dst, DST);
DEFINE_SURFACE_CONFIG_FUNCTION(src, Src, SRC);
DEFINE_SURFACE_CONFIG_FUNCTION(msk, Msk, MSK);

static void _ConfigPatSurface(void)
{                                                                        
    G2D_REG_PAT_RECT rect;

    G2D_REG->PAT_BASE  = (UINT32)_g2dContext.patSurf.addr;       
    G2D_REG->PAT_PITCH = _g2dContext.patSurf.width;
                                                                         
    rect.X      = _g2dContext.patSurf.rect.x;
    rect.Y      = _g2dContext.patSurf.rect.y;
    rect.WIDTH  = _g2dContext.patSurf.rect.width;
    rect.HEIGHT = _g2dContext.patSurf.rect.height;

    OUTREG32(&G2D_REG->PAT_RECT, AS_UINT32(&rect));

    // Config Pattern Offset
    {
        G2D_REG_PAT_OFS offset;

        offset.X = (UINT8)(_g2dContext.patOffset.x % _g2dContext.patSurf.width);
        offset.Y = (UINT8)(_g2dContext.patOffset.y % _g2dContext.patSurf.height);
        
        OUTREG32(&G2D_REG->PAT_XY_OFS, AS_UINT32(&offset));
    }
}

#if G2D_PERF_PROF_ENABLE
__inline static void _StartPerfProfile(void)
{
    do_gettimeofday(&_t1);
}

__inline static void _StopPerfProfile(void)
{
    do_gettimeofday(&_t2);
    _g2dContext.lastOpDuration = (unsigned long)(_t2.tv_usec - _t1.tv_usec);
}
#endif  // G2D_PERF_PROF_ENABLE


static void _FirePrimitive(G2D_FIRE_MODE fireMode)
{
    G2D_REG_FMODE_CON fireCtrl = {0};

    fireCtrl.DST_CLR_MODE = _g2dContext.dstSurf.format;
    fireCtrl.SRC_CLR_MODE = _g2dContext.srcSurf.format;
    fireCtrl.PAT_CLR_MODE = _g2dContext.patSurf.format;
    fireCtrl.G2D_ENG_MODE = fireMode;

#if G2D_PERF_PROF_ENABLE
    _StartPerfProfile();
#endif  // G2D_PERF_PROF_ENABLE
    
    OUTREG32(&G2D_REG->FIRE_MODE_CTRL, AS_UINT32(&fireCtrl));

#if G2D_PERF_PROF_ENABLE
    _WaitForEngineNotBusy();
    _StopPerfProfile();
#endif
}


// ---------------------------------------------------------------------------
//  G2D ISR
// ---------------------------------------------------------------------------

#if ENABLE_G2D_INTERRUPT
static __tcmfunc irqreturn_t _G2D_InterruptHandler(int irq, void *dev_id)
{
    wake_up_interruptible(&_g2d_wait_queue);

    if (_g2dContext.callback)
    {
        _g2dContext.callback();
    }

    return IRQ_HANDLED;
}
#endif

// ---------------------------------------------------------------------------
//  G2D Driver Functions Implementation
// ---------------------------------------------------------------------------

G2D_STATUS G2D_PowerOn(void)
{
    if (!s_isG2dPowerOn)
    {
        BOOL ret = hwEnableClock(MT6516_PDN_MM_G2D, "G2D");
        ASSERT(ret);
        _RestoreG2DRegisters();
        s_isG2dPowerOn = TRUE;
    }

    return G2D_STATUS_OK;
}


G2D_STATUS G2D_PowerOff(void)
{
    if (s_isG2dPowerOn)
    {
        BOOL ret = TRUE;
        _WaitForEngineNotBusy();
        _BackupG2DRegisters();
        ret = hwDisableClock(MT6516_PDN_MM_G2D, "G2D");
        ASSERT(ret);
        s_isG2dPowerOn = FALSE;
    }

    return G2D_STATUS_OK;
}


G2D_STATUS G2D_Init(void)
{
    if (_g2dInitialized)
    {
        return G2D_STATUS_OK;
    }

    _InitG2DContext();

    _ResetBackupedG2DRegisterValues();

    G2D_PowerOn();

    // Enable Read Buffer and Write Buffer
    {
        G2D_REG_COM_CON ctrl = G2D_REG->COM_CON;
        ctrl.RD_BUF_EN = 1;
        ctrl.WR_BUF_EN = 1;
        OUTREG32(&G2D_REG->COM_CON, AS_UINT32(&ctrl));
    }

#if ENABLE_G2D_INTERRUPT
    // Initialize G2D ISR
    if (request_irq(MT6516_G2D_IRQ_LINE,
        _G2D_InterruptHandler, 0, M2D_DEV_NAME, NULL) < 0)
    {
        printk("[G2D][ERROR] fail to request G2D irq\n"); 
        return G2D_STATUS_ERROR;
    }
    
    init_waitqueue_head(&_g2d_wait_queue);
    G2D_REG->IRQ_CON = 1;
#endif    

    _g2dInitialized = TRUE;

    return G2D_STATUS_OK;
}


G2D_STATUS G2D_Deinit(void)
{
    if (!_g2dInitialized)
    {
        return G2D_STATUS_OK;
    }

    G2D_PowerOff();

     _g2dInitialized = FALSE;

    return G2D_STATUS_OK;
}


G2D_STATE G2D_GetStatus(void)
{
    if (G2D_REG->COM_STA & 0x1)
    {
        return G2D_STATE_BUSY;
    }
    else
    {
        return G2D_STATE_IDLE;
    }
}


G2D_STATUS G2D_WaitForNotBusy(void)
{
    _WaitForEngineNotBusy();
    return G2D_STATUS_OK;
}


G2D_STATUS G2D_EnableCommandQueue(BOOL bEnable)
{
    if (bEnable)
    {
        NOT_IMPLEMENTED();
    }
    return G2D_STATUS_OK;
}


G2D_STATUS G2D_SetCallbackFunction(G2D_CALLBACK pfCallback)
{
    _g2dContext.callback = pfCallback;

    return G2D_STATUS_OK;
}


G2D_STATUS G2D_EnableClipping(BOOL bEnable)
{
    _g2dContext.enableClip = bEnable;
        
    return G2D_STATUS_OK;
}


G2D_STATUS G2D_SetClippingWindow(INT32 i4X, INT32 i4Y, UINT32 u4Width, UINT32 u4Height)
{
    _g2dContext.clipWnd.x      = i4X;
    _g2dContext.clipWnd.y      = i4Y;
    _g2dContext.clipWnd.width  = u4Width;
    _g2dContext.clipWnd.height = u4Height;

    return G2D_STATUS_OK;
}


G2D_STATUS G2D_SetRotateMode(G2D_ROTATE eRotation)
{
    _g2dContext.rotate = eRotation;
    
    return G2D_STATUS_OK;
}


G2D_STATUS G2D_SetDirection(G2D_DIRECTION eDirection)
{
    _g2dContext.direction = eDirection;
    
    return G2D_STATUS_OK;
}


G2D_STATUS G2D_SetSrcColorKey(BOOL bEnable, UINT32 u4ColorKey)
{
    _g2dContext.enableSrcColorKey = bEnable;
    _g2dContext.srcColorKey = u4ColorKey;
    
    return G2D_STATUS_OK;
}


G2D_STATUS G2D_SetDstColorKey(BOOL bEnable, UINT32 u4ColorKey)
{
    _g2dContext.enableDstColorKey = bEnable;
    _g2dContext.dstColorKey = u4ColorKey;
    
    return G2D_STATUS_OK;
}


G2D_STATUS G2D_SetColorReplace(BOOL bEnable, UINT32 u4AvdColor, UINT32 u4RepColor)
{
    _g2dContext.enableColorReplacement = bEnable;
    _g2dContext.avoidColor = u4AvdColor;
    _g2dContext.replaceColor = u4RepColor;
    
    return G2D_STATUS_OK;
}


#define DEFINE_G2D_SET_BUFFER_FUNCTION(name1, name2)                    \
    G2D_STATUS G2D_Set##name2##Buffer(UINT8 *pAddr,                     \
                                      UINT32 u4Width,                   \
                                      UINT32 u4Height,                  \
                                      G2D_COLOR_FORMAT eFormat)         \
    {                                                                   \
        _g2dContext.name1##Surf.addr   = pAddr;                         \
        _g2dContext.name1##Surf.width  = u4Width;                       \
        _g2dContext.name1##Surf.height = u4Height;                      \
        _g2dContext.name1##Surf.format = eFormat;                       \
                                                                        \
        return G2D_STATUS_OK;                                           \
    }                                                                   \

DEFINE_G2D_SET_BUFFER_FUNCTION(dst, Dst);
DEFINE_G2D_SET_BUFFER_FUNCTION(src, Src);
DEFINE_G2D_SET_BUFFER_FUNCTION(pat, Pat);

G2D_STATUS G2D_SetMskBuffer(UINT8 *pAddr, UINT32 u4Width, UINT32 u4Height)
{
    _g2dContext.mskSurf.addr   = pAddr;
    _g2dContext.mskSurf.width  = u4Width;
    _g2dContext.mskSurf.height = u4Height;
    _g2dContext.mskSurf.format = G2D_COLOR_UNKOWN;

    return G2D_STATUS_OK;
}


#define DEFINE_G2D_SET_RECT_FUNCTION(name1, name2)                      \
                                                                        \
    G2D_STATUS G2D_Set##name2##Rect(UINT32 u4X, UINT32 u4Y,             \
                                    UINT32 u4Width, UINT32 u4Height)    \
    {                                                                   \
        _g2dContext.name1##Surf.rect.x      = u4X;                      \
        _g2dContext.name1##Surf.rect.y      = u4Y;                      \
        _g2dContext.name1##Surf.rect.width  = u4Width;                  \
        _g2dContext.name1##Surf.rect.height = u4Height;                 \
                                                                        \
        return G2D_STATUS_OK;                                           \
    }                                                                   \


DEFINE_G2D_SET_RECT_FUNCTION(dst, Dst);
DEFINE_G2D_SET_RECT_FUNCTION(src, Src);
DEFINE_G2D_SET_RECT_FUNCTION(pat, Pat);
DEFINE_G2D_SET_RECT_FUNCTION(msk, Msk);



G2D_STATUS G2D_SetPatOffset(INT32 i4XOffset, INT32 i4YOffset)
{
    _g2dContext.patOffset.x = i4XOffset;
    _g2dContext.patOffset.y = i4YOffset;

    return G2D_STATUS_OK;
}


G2D_STATUS G2D_SetStretchBitblt(BOOL bEnable, UINT16 u2ScaleUp, UINT16 u2ScaleDown)
{
    _g2dContext.enableStretch = bEnable;
    _g2dContext.stretchFactor.up = u2ScaleUp;
    _g2dContext.stretchFactor.down = u2ScaleDown;

    return G2D_STATUS_OK;
}


G2D_STATUS G2D_SetItalicBitblt(BOOL bEnable, const UINT8 *pTiltValues)
{
    UINT32 i;
    
    _g2dContext.enableItalicBitblt = bEnable;

    memset(_g2dContext.tiltValues, 0, sizeof(_g2dContext.tiltValues));
    
    for (i = 0; i < ARY_SIZE(_g2dContext.tiltValues); ++i)
    {
        _g2dContext.tiltValues[i] = *pTiltValues++;
        if (*pTiltValues == G2D_TILT_END) break;
    }

    ASSERT(*pTiltValues == G2D_TILT_END);

    return G2D_STATUS_OK;
}


G2D_STATUS G2D_StartRectangleFill(UINT32 u4FilledColor,
                                  G2D_GRADIENT *pGradientX,
                                  G2D_GRADIENT *pGradientY)
{
    G2D_REG_SMODE_CON ctrl;

    _WaitForEngineNotBusy();

    OUTREG32(&G2D_REG->SMODE_CON, 0);   // reset smode

    _ConfigClipping();
    _ConfigRotateDirection();
    _ConfigColorReplacement();
    _ConfigTiltBlt(G2D_FIRE_RECT_FILL);
    _ConfigDstSurface();

    // Config Filled Color
    
    G2D_REG->FOREGROUND_COLOR = u4FilledColor;

    ctrl = G2D_REG->SMODE_CON;
    
    if (pGradientX && pGradientY)
    {
        G2D_REG_GRADIENT x, y;

        x.A = TO_S9_16(pGradientX->a);
        x.R = TO_S9_16(pGradientX->r);
        x.G = TO_S9_16(pGradientX->g);
        x.B = TO_S9_16(pGradientX->b);
        
        y.A = TO_S9_16(pGradientY->a);
        y.R = TO_S9_16(pGradientY->r);
        y.G = TO_S9_16(pGradientY->g);
        y.B = TO_S9_16(pGradientY->b);

        ctrl.CLRGD_EN = 1;

        G2D_REG->GRADIENT_X.A = x.A;
        G2D_REG->GRADIENT_X.R = x.R;
        G2D_REG->GRADIENT_X.G = x.G;
        G2D_REG->GRADIENT_X.B = x.B;

        G2D_REG->GRADIENT_Y.A = y.A;
        G2D_REG->GRADIENT_Y.R = y.R;
        G2D_REG->GRADIENT_Y.G = y.G;
        G2D_REG->GRADIENT_Y.B = y.B;
    }
    else
    {
        ctrl.CLRGD_EN = 0;
    }

    OUTREG32(&G2D_REG->SMODE_CON, AS_UINT32(&ctrl));

    _FirePrimitive(G2D_FIRE_RECT_FILL);

    return G2D_STATUS_OK;
}


G2D_STATUS G2D_StartBitblt(void)
{
#if 0
    return G2D_StartRopBitblt(G2D_ROP4_SRCCOPY);
#else
    _WaitForEngineNotBusy();

    OUTREG32(&G2D_REG->SMODE_CON, 0);   // reset smode

    _ConfigClipping();
    _ConfigRotateDirection();
    _ConfigSrcColorKeying();
    _ConfigDstColorKeying();
    _ConfigColorReplacement();
    _ConfigTiltBlt(G2D_FIRE_BLT);
    _ConfigStretchBlt();

    _ConfigDstSurface();
    _ConfigSrcSurface();

    _FirePrimitive(G2D_FIRE_BLT);

    return G2D_STATUS_OK;
#endif
}


G2D_STATUS G2D_StartRopBitblt(UINT16 u2Rop4Code)
{
#if 0
    BOOL needDst = ((u2Rop4Code ^ (u2Rop4Code >> 1)) & 0x5555) != 0;
    BOOL needSrc = ((u2Rop4Code ^ (u2Rop4Code >> 2)) & 0x3333) != 0;
    BOOL needPat = ((u2Rop4Code ^ (u2Rop4Code >> 4)) & 0x0F0F) != 0;
    BOOL needMsk = ((u2Rop4Code ^ (u2Rop4Code >> 8)) & 0x00FF) != 0;
#endif

    _WaitForEngineNotBusy();

    OUTREG32(&G2D_REG->SMODE_CON, 0);   // reset smode

    _ConfigClipping();
    _ConfigRotateDirection();
    _ConfigSrcColorKeying();
    _ConfigDstColorKeying();
    _ConfigColorReplacement();
    _ConfigTiltBlt(G2D_FIRE_ROP_BLT);
    _ConfigStretchBlt();

    _ConfigDstSurface();
    _ConfigSrcSurface();
    _ConfigPatSurface();
    _ConfigMskSurface();

    if (G2D_ROP4_SRCCOPY == u2Rop4Code)
    {
        // Is the performance of FIRE_BLT the same as FIRE_ROP_BLT ?
        // If it's true, always use ROP_BLT for simplification
        // Need further investigation...
        //
        _FirePrimitive(G2D_FIRE_BLT);
    }
    else
    {
        // Always enable ROP4 mode
        G2D_REG_SMODE_CON ctrl = G2D_REG->SMODE_CON;
        ctrl.ROP3_EN = 0;
        ctrl.ROP4_EN = 1;
        OUTREG32(&G2D_REG->SMODE_CON, AS_UINT32(&ctrl));
        OUTREG16(&G2D_REG->ROP_CODE, u2Rop4Code);

        _FirePrimitive(G2D_FIRE_ROP_BLT);
    }

    return G2D_STATUS_OK;
}


G2D_STATUS G2D_StartAlphaBitblt(UINT8 ucAlphaValue, BOOL isPremultipledAlpha)
{
    _WaitForEngineNotBusy();

    OUTREG32(&G2D_REG->SMODE_CON, 0);   // reset smode

    _ConfigClipping();
    _ConfigRotateDirection();
    _ConfigSrcColorKeying();
    _ConfigDstColorKeying();
    _ConfigColorReplacement();
    _ConfigTiltBlt(G2D_FIRE_ALPHA_BLT);
    _ConfigStretchBlt();

    _ConfigDstSurface();
    _ConfigSrcSurface();

    {
        G2D_REG_SMODE_CON ctrl = G2D_REG->SMODE_CON;
        ctrl.ALPHA = ucAlphaValue;
        ctrl.ROP3_EN = 0;
        ctrl.ROP4_EN = 0;
        OUTREG32(&G2D_REG->SMODE_CON, AS_UINT32(&ctrl));
    }

    {
        G2D_REG_COM_CON ctrl = G2D_REG->COM_CON;
        ctrl.WM_PMUL = (isPremultipledAlpha ? 1 : 0);   // config premultiplied alpha
        ctrl.WM_DST_XP = 1;                             // force enable alpha channel blending
        OUTREG32(&G2D_REG->COM_CON, AS_UINT32(&ctrl));
    }
    
    _FirePrimitive(G2D_FIRE_ALPHA_BLT);

    return G2D_STATUS_OK;
}


G2D_STATUS G2D_StartFontDrawing(BOOL bEnableBgColor,
                                UINT32 u4FgColor, UINT32 u4BgColor,
                                UINT8 *pFontBitmap, UINT32 u4StartBit)
{
    G2D_REG_SMODE_CON ctrl;

    _WaitForEngineNotBusy();

    OUTREG32(&G2D_REG->SMODE_CON, 0);   // reset smode

    _ConfigClipping();
    _ConfigRotateDirection();
    _ConfigColorReplacement();
    _ConfigTiltBlt(G2D_FIRE_FONT_DRAWING);

    _ConfigDstSurface();

    ctrl = G2D_REG->SMODE_CON;

    G2D_REG->SRC_BASE = (UINT32)pFontBitmap;
    ctrl.ALPHA = u4StartBit;

    G2D_REG->FOREGROUND_COLOR = u4FgColor;

    if (bEnableBgColor)
    {
        ctrl.FNGB = 0;
        G2D_REG->BACKGROUND_COLOR = u4BgColor;
    }
    else
    {
        ctrl.FNGB = 1;
    }
    
    ctrl.FMSB_FIRST = 1;  // force MSB first

    OUTREG32(&G2D_REG->SMODE_CON, AS_UINT32(&ctrl));

    _FirePrimitive(G2D_FIRE_FONT_DRAWING);

    return G2D_STATUS_OK;
}


UINT32 G2D_GetLastOpDuration(void)
{
    _WaitForEngineNotBusy();

#if G2D_PERF_PROF_ENABLE
    return _g2dContext.lastOpDuration;
#else
    printk("To profile G2D performance, "
           "please define G2D_PERF_PROF in G2d_drv.c\n");

    NOT_IMPLEMENTED();
#endif
}


// ---------------------------------------------------------------------------
//  G2D Benchmarking Functions
// ---------------------------------------------------------------------------

#if G2D_PERF_PROF_ENABLE

#include <linux/fb.h>
#include "../disp_drv.h"

static const char* Narrate2DFormat(G2D_COLOR_FORMAT fmt)
{
    switch(fmt)
    {
    case G2D_COLOR_RGB565   : return "RGB565";
    case G2D_COLOR_ARGB8888 : return "ARGB8888";
    default : return "UNKOWN";
    }
}

static void _G2D_PerfTestImpl(struct fb_info *info,
                              G2D_COLOR_FORMAT srcFormat,
                              G2D_COLOR_FORMAT dstFormat,
                              BOOL rotation)
{
    UINT32 dst = info->fix.smem_start;
    UINT32 src = info->fix.smem_start + info->fix.line_length * info->var.yres;

    UINT32 srcWidth, srcHeight, dstWidth, dstHeight;
    UINT32 dummySize = 32;

    UINT32 i = 0;
    UINT32 duration = 0;
    UINT32 totalDuration = 0;

    srcWidth = dstWidth = DISP_GetScreenWidth();
    srcHeight = dstHeight = DISP_GetScreenHeight();

    ASSERT(srcWidth <= info->var.xres);
    ASSERT(srcHeight <= info->var.yres);

    G2D_EnableClipping(FALSE);
    G2D_SetStretchBitblt(FALSE, 0, 0);

    if (rotation)
    {
        UINT32 tmp;
        tmp = dstWidth; dstWidth = dstHeight; dstHeight = tmp;
        G2D_SetRotateMode(G2D_ROTATE_90);
    }
    else
    {
        G2D_SetRotateMode(G2D_ROTATE_0);
    }

    G2D_SetDstBuffer((UINT8*)dst, dstWidth, dstHeight, dstFormat);
    G2D_SetDstRect(0, 0, dstWidth, dstHeight);

    G2D_SetSrcBuffer((UINT8*)src, srcWidth, srcHeight, srcFormat);
    G2D_SetSrcRect(0, 0, srcWidth, srcHeight);

    G2D_SetPatBuffer((UINT8*)src, dummySize, dummySize, srcFormat);
    G2D_SetPatRect(0, 0, dummySize, dummySize);

    G2D_SetMskBuffer((UINT8*)src, dummySize, dummySize);
    G2D_SetMskRect(0, 0, dummySize, dummySize);

    for (i = 0; i < 3; ++ i)
    {
        if (G2D_COLOR_RGB565 == srcFormat) {
            G2D_StartBitblt();
        } else if (G2D_COLOR_ARGB8888 == srcFormat) {
            G2D_StartAlphaBitblt(0x1, TRUE);
        } else {
            ASSERT(0);
        }

        duration = G2D_GetLastOpDuration();
        totalDuration += duration;

        printk("[%2d] %s -> %s, %d x %d, rotation: %d, %u usec\n",
               i, Narrate2DFormat(srcFormat), Narrate2DFormat(dstFormat),
               srcWidth, srcHeight, rotation, duration);
    }

    printk("[Total] %s -> %s, %d x %d, %u usec\n",
           Narrate2DFormat(srcFormat), Narrate2DFormat(dstFormat),
           srcWidth, srcHeight, totalDuration);
}


void G2D_Perf_Test(void *fbInfo)
{
    struct fb_info *info = (struct fb_info*)fbInfo;
    
    G2D_Init();
    
    _G2D_PerfTestImpl(info, G2D_COLOR_RGB565,   G2D_COLOR_ARGB8888, FALSE);
    _G2D_PerfTestImpl(info, G2D_COLOR_ARGB8888, G2D_COLOR_ARGB8888, FALSE);
    _G2D_PerfTestImpl(info, G2D_COLOR_RGB565,   G2D_COLOR_ARGB8888, TRUE);
    _G2D_PerfTestImpl(info, G2D_COLOR_ARGB8888, G2D_COLOR_ARGB8888, TRUE);

    _G2D_PerfTestImpl(info, G2D_COLOR_RGB565,   G2D_COLOR_RGB565, FALSE);
    _G2D_PerfTestImpl(info, G2D_COLOR_ARGB8888, G2D_COLOR_RGB565, FALSE);
    _G2D_PerfTestImpl(info, G2D_COLOR_RGB565,   G2D_COLOR_RGB565, TRUE);
    _G2D_PerfTestImpl(info, G2D_COLOR_ARGB8888, G2D_COLOR_RGB565, TRUE);
}

#endif  // G2D_PERF_PROF_ENABLE
