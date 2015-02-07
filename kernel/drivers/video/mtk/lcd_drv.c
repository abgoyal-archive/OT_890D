

#define ENABLE_LCD_INTERRUPT 1

#if defined(CONFIG_ARCH_MT6516)
    #include <mach/mt6516_typedefs.h>
    #include <mach/mt6516_reg_base.h>
    #include <mach/mt6516_pll.h>
#else
    #error "unknown arch"
#endif

#include "lcd_reg.h"
#include "lcd_drv.h"

#if ENABLE_LCD_INTERRUPT
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <asm/tcm.h>
#include <mach/irqs.h>
#include "mtkfb.h"
static wait_queue_head_t _lcd_wait_queue;
#endif

#include "debug.h"

PLCD_REGS const LCD_REG = (PLCD_REGS)(LCD_BASE);

static const UINT32 TO_BPP[LCD_FB_FORMAT_NUM] = {2, 3, 4};

typedef struct
{
    LCD_FB_FORMAT fbFormat;
    UINT32 fbPitchInBytes;
    LCD_REG_SIZE roiWndSize;
    LCD_OUTPUT_MODE outputMode;
    LCD_REGS regBackup;
} LCD_CONTEXT;

static LCD_CONTEXT _lcdContext = {0};


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

extern int MT6516IDP_DisableDirectLink(void);
#if ENABLE_LCD_INTERRUPT
static __tcmfunc irqreturn_t _LCD_InterruptHandler(int irq, void *dev_id)
{   
    LCD_REG_INTERRUPT status = LCD_REG->INT_STATUS;

    if (status.COMPLETED)
    {
        DBG_OnLcdDone();
        wake_up_interruptible(&_lcd_wait_queue);
    }

    if (status.TE_PEND)
    {
        DBG_OnTeDelayDone();
    }

    MT6516IDP_DisableDirectLink();

    return IRQ_HANDLED;
}
#endif


static BOOL _IsEngineBusy(void)
{
    LCD_REG_STATUS status;

    status = LCD_REG->STATUS;
    if (status.RUN || status.TE_PEND) return TRUE;
    
    // Since there are 2 cycles of state transition (STATUS == 0)
    // from TE_PEND to RUN, double confirm if the engine is idle again
    //
    status = LCD_REG->STATUS;
    if (status.RUN || status.TE_PEND) return TRUE;

    return FALSE;
}


static void _WaitForEngineNotBusy(void)
{
#if ENABLE_LCD_INTERRUPT
    static const long WAIT_TIMEOUT = 2 * HZ;    // 2 sec

    if (in_interrupt())
    {
        // perform busy waiting if in interrupt context
        while(_IsEngineBusy()) {}
    }
    else
    {
        while (_IsEngineBusy())
        {
            long ret = wait_event_interruptible_timeout(_lcd_wait_queue, 
                                                        !_IsEngineBusy(),
                                                        WAIT_TIMEOUT);
            if (0 == ret) {
                printk("[WARNING] Wait for LCD engine not busy timeout!!!\n");
            }
        }
    }
#else
    while(_IsEngineBusy()) {}
#endif    
}


static void _BackupLCDRegisters(void)
{
    memcpy((void*)&(_lcdContext.regBackup), (void*)LCD_REG, sizeof(LCD_REGS));
}


static void _RestoreLCDRegisters(void)
{
    LCD_REGS *regs = &(_lcdContext.regBackup);
    UINT32 i;

    OUTREG32(&LCD_REG->INT_ENABLE, AS_UINT32(&regs->INT_ENABLE));
    OUTREG32(&LCD_REG->SERIAL_CFG, AS_UINT32(&regs->SERIAL_CFG));

    for(i = 0; i < ARY_SIZE(LCD_REG->PARALLEL_CFG); ++i)
    {
        OUTREG32(&LCD_REG->PARALLEL_CFG[i], AS_UINT32(&regs->PARALLEL_CFG[i]));
    }

    OUTREG32(&LCD_REG->TEARING_CFG, AS_UINT32(&regs->TEARING_CFG));
    OUTREG32(&LCD_REG->PARALLEL_DW, AS_UINT32(&regs->PARALLEL_DW));

    for(i = 0; i < ARY_SIZE(LCD_REG->WROI_W2M_ADDR); ++i)
    {
        OUTREG32(&LCD_REG->WROI_W2M_ADDR[i], AS_UINT32(&regs->WROI_W2M_ADDR[i]));
    }

    OUTREG32(&LCD_REG->MWIN_SIZE, AS_UINT32(&regs->MWIN_SIZE));
    OUTREG32(&LCD_REG->WROI_W2M_OFFSET, AS_UINT32(&regs->WROI_W2M_OFFSET));
    OUTREG32(&LCD_REG->WROI_W2M_CONTROL, AS_UINT32(&regs->WROI_W2M_CONTROL));
    OUTREG32(&LCD_REG->WROI_FORMAT, AS_UINT32(&regs->WROI_FORMAT));
    OUTREG32(&LCD_REG->WROI_CONTROL, AS_UINT32(&regs->WROI_CONTROL));
    OUTREG32(&LCD_REG->WROI_OFFSET, AS_UINT32(&regs->WROI_OFFSET));
    OUTREG32(&LCD_REG->WROI_CMD_ADDR, AS_UINT32(&regs->WROI_CMD_ADDR));
    OUTREG32(&LCD_REG->WROI_DATA_ADDR, AS_UINT32(&regs->WROI_DATA_ADDR));
    OUTREG32(&LCD_REG->WROI_SIZE, AS_UINT32(&regs->WROI_SIZE));
    OUTREG32(&LCD_REG->WROI_HW_REFRESH, AS_UINT32(&regs->WROI_HW_REFRESH));
    OUTREG32(&LCD_REG->WROI_DC, AS_UINT32(&regs->WROI_DC));
    OUTREG32(&LCD_REG->WROI_BG_COLOR, AS_UINT32(&regs->WROI_BG_COLOR));

    for(i = 0; i < ARY_SIZE(LCD_REG->LAYER); ++i)
    {
        OUTREG32(&LCD_REG->LAYER[i].CONTROL, AS_UINT32(&regs->LAYER[i].CONTROL));
        OUTREG32(&LCD_REG->LAYER[i].SRCKEY, AS_UINT32(&regs->LAYER[i].SRCKEY));
        OUTREG32(&LCD_REG->LAYER[i].OFFSET, AS_UINT32(&regs->LAYER[i].OFFSET));
        OUTREG32(&LCD_REG->LAYER[i].ADDRESS, AS_UINT32(&regs->LAYER[i].ADDRESS));
        OUTREG32(&LCD_REG->LAYER[i].SIZE, AS_UINT32(&regs->LAYER[i].SIZE));
        OUTREG32(&LCD_REG->LAYER[i].SCRL_OFFSET, AS_UINT32(&regs->LAYER[i].SCRL_OFFSET));
    }

    for(i = 0; i < ARY_SIZE(LCD_REG->COEF_ROW); ++i)
    {
        OUTREG32(&LCD_REG->COEF_ROW[i], AS_UINT32(&regs->COEF_ROW[i]));
    }

    for(i = 0; i < ARY_SIZE(LCD_REG->GAMMA); ++i)
    {
        OUTREG32(&LCD_REG->GAMMA[i], AS_UINT32(&regs->GAMMA[i]));
    }
    
    OUTREG32(&LCD_REG->FBCE_TARGET_RATE, AS_UINT32(&regs->FBCE_TARGET_RATE));
    OUTREG32(&LCD_REG->FBCE_INPUT_MODE, AS_UINT32(&regs->FBCE_INPUT_MODE));
    OUTREG32(&LCD_REG->FBCE_ACT_EN, AS_UINT32(&regs->FBCE_ACT_EN));
    OUTREG32(&LCD_REG->FBCE_CLR_EN, AS_UINT32(&regs->FBCE_CLR_EN));
    OUTREG32(&LCD_REG->FBCD_CON, AS_UINT32(&regs->FBCD_CON));
}


static void _ResetBackupedLCDRegisterValues(void)
{
    LCD_REGS *regs = &_lcdContext.regBackup;
    memset((void*)regs, 0, sizeof(LCD_REGS));

    OUTREG32(&regs->SERIAL_CFG, 0x00003000);
    OUTREG32(&regs->PARALLEL_CFG[0], 0x00FC0000);
    OUTREG32(&regs->PARALLEL_CFG[1], 0x00300000);
    OUTREG32(&regs->PARALLEL_CFG[2], 0x00300000);
}


// ---------------------------------------------------------------------------
//  LCD Controller API Implementations
// ---------------------------------------------------------------------------

LCD_STATUS LCD_Init(void)
{
    LCD_STATUS ret = LCD_STATUS_OK;

    memset(&_lcdContext, 0, sizeof(_lcdContext));

    // LCD controller would NOT reset register as default values
    // Do it by SW here
    //
    _ResetBackupedLCDRegisterValues();

    ret = LCD_PowerOn();

    ASSERT(ret == LCD_STATUS_OK);

#if ENABLE_LCD_INTERRUPT
    if (request_irq(MT6516_LCD_IRQ_LINE,
        _LCD_InterruptHandler, 0, MTKFB_DRIVER, NULL) < 0)
    {
        printk("[LCD][ERROR] fail to request LCD irq\n"); 
        return LCD_STATUS_ERROR;
    }

    init_waitqueue_head(&_lcd_wait_queue);
    LCD_REG->INT_ENABLE.COMPLETED = 1;
    LCD_REG->INT_ENABLE.TE_PEND = 1;
#endif
    
    return LCD_STATUS_OK;
}


LCD_STATUS LCD_Deinit(void)
{
    LCD_STATUS ret = LCD_PowerOff();

    ASSERT(ret == LCD_STATUS_OK);

    return LCD_STATUS_OK;
}


static BOOL s_isLcdPowerOn = FALSE;

LCD_STATUS LCD_PowerOn(void)
{
    if (!s_isLcdPowerOn)
    {
        BOOL ret = hwEnableClock(MT6516_PDN_MM_LCD, "LCD");
        ASSERT(ret);
        _RestoreLCDRegisters();
        s_isLcdPowerOn = TRUE;
    }

    return LCD_STATUS_OK;
}


LCD_STATUS LCD_PowerOff(void)
{
    if (s_isLcdPowerOn)
    {
        BOOL ret = TRUE;
        _WaitForEngineNotBusy();
        _BackupLCDRegisters();
        ret = hwDisableClock(MT6516_PDN_MM_LCD, "LCD");
        ASSERT(ret);
        s_isLcdPowerOn = FALSE;
    }

    return LCD_STATUS_OK;
}


LCD_STATUS LCD_WaitForNotBusy(void)
{
    _WaitForEngineNotBusy();
    return LCD_STATUS_OK;
}

// -------------------- LCD Controller Interface --------------------

LCD_STATUS LCD_ConfigParallelIF(LCD_IF_ID id,
                                LCD_IF_PARALLEL_BITS ifDataWidth,
                                LCD_IF_PARALLEL_CLK_DIV clkDivisor,
                                UINT32 writeSetup,
                                UINT32 writeHold,
                                UINT32 writeWait,
                                UINT32 readSetup,
                                UINT32 readLatency,
                                UINT32 waitPeriod)
{
    ASSERT(id >= LCD_IF_PARALLEL_0 && id <= LCD_IF_PARALLEL_2);
    ASSERT(writeSetup <= 3U);
    ASSERT(writeHold <= 4U);
    ASSERT(writeWait <= 32U);
    ASSERT(readSetup <= 15U);
    ASSERT(readLatency <= 32U);
    ASSERT(waitPeriod <= 1023U);

    if (0 == writeHold)   writeHold = 1;
    if (0 == writeWait)   writeWait = 1;
    if (0 == readLatency) readLatency = 1;

    _WaitForEngineNotBusy();

    // (1) Config Data Width
    {
        LCD_REG_PCNFDW pcnfdw = LCD_REG->PARALLEL_DW;

        switch(id)
        {
        case LCD_IF_PARALLEL_0: pcnfdw.PCNF0_DW = (UINT32)ifDataWidth; break;
        case LCD_IF_PARALLEL_1: pcnfdw.PCNF1_DW = (UINT32)ifDataWidth; break;
        case LCD_IF_PARALLEL_2: pcnfdw.PCNF2_DW = (UINT32)ifDataWidth; break;
        default : ASSERT(0);
        };

        OUTREG32(&LCD_REG->PARALLEL_DW, AS_UINT32(&pcnfdw));
    }

    // (2) Config Timing
    {
        UINT32 i;
        LCD_REG_PCNF config;
        
        i = (UINT32)id - LCD_IF_PARALLEL_0;
        config = LCD_REG->PARALLEL_CFG[i];

        config.C2WS = writeSetup;
        config.C2WH = writeHold - 1;
        config.WST  = writeWait - 1;
        config.C2RS = readSetup;
        config.RLT  = readLatency - 1;

        config.CLK_52M = config.CLK_26M = 0;

        switch(clkDivisor)
        {
        case LCD_IF_PARALLEL_CLK_DIV_1 : break;
        case LCD_IF_PARALLEL_CLK_DIV_2 : config.CLK_52M = 1; break;
        case LCD_IF_PARALLEL_CLK_DIV_4 : config.CLK_26M = 1; break;
        default :
            ASSERT(0);
        }

        OUTREG32(&LCD_REG->PARALLEL_CFG[i], AS_UINT32(&config));
    }

    // (3) Config Delay Between Commands
    {
        LCD_REG_WROI_CON ctrl = LCD_REG->WROI_CONTROL;
        ctrl.PERIOD = waitPeriod;
        OUTREG32(&LCD_REG->WROI_CONTROL, AS_UINT32(&ctrl));
    }

    return LCD_STATUS_OK;
}



LCD_STATUS LCD_ConfigIfFormat(LCD_IF_FMT_COLOR_ORDER order,
                              LCD_IF_FMT_TRANS_SEQ transSeq,
                              LCD_IF_FMT_PADDING padding,
                              LCD_IF_FORMAT format,
                              LCD_IF_WIDTH busWidth)
{
    LCD_REG_WROI_FMT fmt = LCD_REG->WROI_FORMAT;

    fmt.RGB_ORDER  = order;
    fmt.BYTE_ORDER = transSeq;
    fmt.PADDING    = padding;
    fmt.DATA_FMT   = (UINT32)format;
    fmt.INTF_FMT   = (UINT32)busWidth;

    OUTREG32(&LCD_REG->WROI_FORMAT, AS_UINT32(&fmt));

    return LCD_STATUS_OK;
}


LCD_STATUS LCD_ConfigSerialIF(LCD_IF_ID id,
                              LCD_IF_SERIAL_BITS bits,
                              BOOL clockPolarity,
                              BOOL clockPhase,
                              BOOL csPolarity,
                              LCD_IF_SERIAL_CLK_DIV clkDivisor,
                              BOOL mode)
{
    LCD_REG_SCNF config;

    ASSERT(id >= LCD_IF_SERIAL_0 && id <= LCD_IF_SERIAL_1);
    ASSERT(bits == LCD_IF_SERIAL_8BITS || bits == LCD_IF_SERIAL_9BITS);

    _WaitForEngineNotBusy();

    memset(&config, 0, sizeof(config));
    config.GAMMA_ID = 0x3;  // no table selected

    config.IF_9_BIT = (UINT32)bits;

    config.SPO = clockPolarity ? 1 : 0;
    config.SPH = clockPhase ? 1 : 0;

    if (LCD_IF_SERIAL_0 == id)
    {
        config.CSP0 = csPolarity ? 1 : 0;
    }
    else
    {
        config.CSP1 = csPolarity ? 1 : 0;
    }

    switch(clkDivisor)
    {
    case LCD_IF_SERIAL_CLK_DIV_1  : break;
    case LCD_IF_SERIAL_CLK_DIV_2  : config.DIV = 1; break;
    case LCD_IF_SERIAL_CLK_DIV_4  : config.DIV = 3; break;
    case LCD_IF_SERIAL_CLK_DIV_8  : config.DIV = 3; config.CLK_52M = 1; break;
    case LCD_IF_SERIAL_CLK_DIV_16 : config.DIV = 3; config.CLK_26M = 1; break;
    default :
        ASSERT(0);
    }

    config.MODE = mode ? 1 : 0;

    OUTREG32(&LCD_REG->SERIAL_CFG, AS_UINT32(&config));

    return LCD_STATUS_OK;
}


LCD_STATUS LCD_SetResetSignal(BOOL high)
{
    LCD_REG->RESET = high ? 1 : 0;

    return LCD_STATUS_OK;
}

// -------------------- Command Queue --------------------

LCD_STATUS LCD_CmdQueueEnable(BOOL enabled)
{
    LCD_REG_WROI_CON ctrl;

    _WaitForEngineNotBusy();

    ctrl = LCD_REG->WROI_CONTROL;
    ctrl.ENC = enabled ? 1 : 0;
    OUTREG32(&LCD_REG->WROI_CONTROL, AS_UINT32(&ctrl));

    return LCD_STATUS_OK;
}


LCD_STATUS LCD_CmdQueueSelect(LCD_CMDQ_ID id)
{
    LCD_REG_WROI_CON ctrl;

    _WaitForEngineNotBusy();
    ctrl = LCD_REG->WROI_CONTROL;

    switch(id)
    {
    case LCD_CMDQ_0 :
    case LCD_CMDQ_1 :
        ctrl.COM_SEL = (UINT32)id;
        break;
    default : ASSERT(0);
    }

    OUTREG32(&LCD_REG->WROI_CONTROL, AS_UINT32(&ctrl));

    return LCD_STATUS_OK;
}


LCD_STATUS LCD_CmdQueueSetWaitPeriod(UINT32 period)
{
    LCD_REG_WROI_CON ctrl;

    ASSERT(period < 1024);
    
    _WaitForEngineNotBusy();

    ctrl = LCD_REG->WROI_CONTROL;
    ctrl.PERIOD = period;
    OUTREG32(&LCD_REG->WROI_CONTROL, AS_UINT32(&ctrl));

    return LCD_STATUS_OK;
}


LCD_STATUS LCD_CmdQueueWrite(LCD_CMDQ_ID id, UINT32 *cmds, UINT32 cmdCount)
{
    LCD_REG_WROI_CON ctrl;
    UINT32 i;

    ASSERT(cmdCount >= 0 && cmdCount < ARY_SIZE(LCD_REG->CMDQ[0]));
    
    _WaitForEngineNotBusy();
    ctrl = LCD_REG->WROI_CONTROL;
    ctrl.COMMAND = cmdCount - 1;
    OUTREG32(&LCD_REG->WROI_CONTROL, AS_UINT32(&ctrl));

    for (i = 0; i < cmdCount; ++ i)
    {
        LCD_REG->CMDQ[id][i] = cmds[i];
    }

    return LCD_STATUS_OK;
}


// -------------------- Layer Configurations --------------------

LCD_STATUS LCD_LayerEnable(LCD_LAYER_ID id, BOOL enable)
{
    LCD_REG_WROI_CON ctrl;

    _WaitForEngineNotBusy();

    ctrl = LCD_REG->WROI_CONTROL;
    
    switch(id)
    {
    case LCD_LAYER_0 : ctrl.EN0 = enable ? 1 : 0; break;
    case LCD_LAYER_1 : ctrl.EN1 = enable ? 1 : 0; break;
    case LCD_LAYER_2 : ctrl.EN2 = enable ? 1 : 0; break;
    case LCD_LAYER_3 : ctrl.EN3 = enable ? 1 : 0; break;
    case LCD_LAYER_4 : ctrl.EN4 = enable ? 1 : 0; break;
    case LCD_LAYER_5 : ctrl.EN5 = enable ? 1 : 0; break;
    case LCD_LAYER_ALL :
        MASKREG32(&ctrl, 0xFC000000, enable ? 0xFC000000 : 0);
        break;
    default : ASSERT(0);
    }

    OUTREG32(&LCD_REG->WROI_CONTROL, AS_UINT32(&ctrl));
    
    return LCD_STATUS_OK;
}


LCD_STATUS LCD_LayerSetAddress(LCD_LAYER_ID id, UINT32 address)
{
    ASSERT(id < LCD_LAYER_NUM || LCD_LAYER_ALL == id);
    ASSERT((address & 0x7) == 0);   // layer address should be 8-byte-aligned

    _WaitForEngineNotBusy();

    if (LCD_LAYER_ALL == id)
    {
        NOT_IMPLEMENTED();
    }
    else if (id < LCD_LAYER_NUM)
    {
        LCD_REG->LAYER[id].ADDRESS = address;
    }
    
    return LCD_STATUS_OK;
}


LCD_STATUS LCD_LayerSetSize(LCD_LAYER_ID id, UINT32 width, UINT32 height)
{
    LCD_REG_SIZE size;
    size.WIDTH = (UINT16)width;
    size.HEIGHT = (UINT16)height;

    ASSERT(id < LCD_LAYER_NUM || LCD_LAYER_ALL == id);

    _WaitForEngineNotBusy();

    if (LCD_LAYER_ALL == id)
    {
        NOT_IMPLEMENTED();
    }
    else if (id < LCD_LAYER_NUM)
    {
        OUTREG32(&LCD_REG->LAYER[id].SIZE, AS_UINT32(&size));
    }
    
    return LCD_STATUS_OK;
}


LCD_STATUS LCD_LayerSetOffset(LCD_LAYER_ID id, UINT32 x, UINT32 y)
{
    LCD_REG_COORD offset;
    offset.X = (UINT16)x;
    offset.Y = (UINT16)y;

    ASSERT(id < LCD_LAYER_NUM || LCD_LAYER_ALL == id);

    _WaitForEngineNotBusy();
    
    if (LCD_LAYER_ALL == id)
    {
        NOT_IMPLEMENTED();
    }
    else if (id < LCD_LAYER_NUM)
    {
        OUTREG32(&LCD_REG->LAYER[id].OFFSET, AS_UINT32(&offset));
    }
    
    return LCD_STATUS_OK;
}


LCD_STATUS LCD_LayerSetFormat(LCD_LAYER_ID id, LCD_LAYER_FORMAT format)
{
    ASSERT(id < LCD_LAYER_NUM || LCD_LAYER_ALL == id);

    _WaitForEngineNotBusy();
    
    if (LCD_LAYER_ALL == id)
    {
        NOT_IMPLEMENTED();
    }
    else if (id < LCD_LAYER_NUM)
    {
        LCD_REG_LAYER_CON ctrl = LCD_REG->LAYER[id].CONTROL;
        ctrl.CLRDPT = (UINT32)format;
        OUTREG32(&LCD_REG->LAYER[id].CONTROL, AS_UINT32(&ctrl));
    }
    
    return LCD_STATUS_OK;
}


LCD_STATUS LCD_LayerEnableByteSwap(LCD_LAYER_ID id, BOOL enable)
{
    ASSERT(id < LCD_LAYER_NUM || LCD_LAYER_ALL == id);

    _WaitForEngineNotBusy();

    if (LCD_LAYER_ALL == id)
    {
        UINT32 i;
        for (i = 0; i < ARY_SIZE(LCD_REG->LAYER); ++ i)
        {
            LCD_LayerEnableByteSwap((LCD_LAYER_ID)i, enable);
        }
    }
    else if (id < LCD_LAYER_NUM)
    {
        LCD_REG_LAYER_CON ctrl = LCD_REG->LAYER[id].CONTROL;
        ctrl.SWP = enable ? 1 : 0;
        OUTREG32(&LCD_REG->LAYER[id].CONTROL, AS_UINT32(&ctrl));
    }

    return LCD_STATUS_OK;
}


LCD_STATUS LCD_LayerSetRotation(LCD_LAYER_ID id, LCD_LAYER_ROTATION rotation)
{
    ASSERT(id < LCD_LAYER_NUM || LCD_LAYER_ALL == id);

    _WaitForEngineNotBusy();

    if (LCD_LAYER_ALL == id)
    {
        NOT_IMPLEMENTED();
    }
    else if (id < LCD_LAYER_NUM)
    {
        LCD_REG_LAYER_CON ctrl = LCD_REG->LAYER[id].CONTROL;
        ctrl.ROTATE = (UINT32)rotation;
        OUTREG32(&LCD_REG->LAYER[id].CONTROL, AS_UINT32(&ctrl));
    }

    return LCD_STATUS_OK;
}


LCD_STATUS LCD_LayerSetAlphaBlending(LCD_LAYER_ID id, BOOL enable, UINT8 alpha)
{
    ASSERT(id < LCD_LAYER_NUM || LCD_LAYER_ALL == id);

    _WaitForEngineNotBusy();

    if (LCD_LAYER_ALL == id)
    {
        NOT_IMPLEMENTED();
    }
    else if (id < LCD_LAYER_NUM)
    {
        LCD_REG_LAYER_CON ctrl = LCD_REG->LAYER[id].CONTROL;
        ctrl.OPAEN = enable ? 1 : 0;
        ctrl.OPA   = alpha;
        OUTREG32(&LCD_REG->LAYER[id].CONTROL, AS_UINT32(&ctrl));
    }
    
    return LCD_STATUS_OK;
}


LCD_STATUS LCD_LayerSetSourceColorKey(LCD_LAYER_ID id, BOOL enable, UINT32 colorKey)
{
    ASSERT(id < LCD_LAYER_NUM || LCD_LAYER_ALL == id);

    _WaitForEngineNotBusy();

    if (LCD_LAYER_ALL == id)
    {
        NOT_IMPLEMENTED();
    }
    else if (id < LCD_LAYER_NUM)
    {
        LCD_REG_LAYER_CON ctrl = LCD_REG->LAYER[id].CONTROL;
        ctrl.KEY_EN = enable ? 1 : 0;
        OUTREG32(&LCD_REG->LAYER[id].CONTROL, AS_UINT32(&ctrl));
        LCD_REG->LAYER[id].SRCKEY = colorKey;
    }
    
    return LCD_STATUS_OK;
}

// -------------------- HW Trigger Configurations --------------------

LCD_STATUS LCD_LayerSetTriggerMode(LCD_LAYER_ID id, LCD_LAYER_TRIGGER_MODE mode)
{
    LCD_WROI_HWREF hwref;
    LCD_REG_WROI_DC dc;

    BOOL enHwTrigger = (LCD_SW_TRIGGER != mode);
    BOOL enDirectCouple = (LCD_HW_TRIGGER_DIRECT_COUPLE == mode);

    _WaitForEngineNotBusy();

    hwref = LCD_REG->WROI_HW_REFRESH;
    dc = LCD_REG->WROI_DC;
        
    switch(id)
    {
    case LCD_LAYER_0 :
        hwref.EN0 = enHwTrigger ? 1 : 0;
        dc.EN0 = enDirectCouple ? 1 : 0;
        break;
    case LCD_LAYER_1 : 
        hwref.EN1 = enHwTrigger ? 1 : 0;
        dc.EN1 = enDirectCouple ? 1 : 0;
        break;
    case LCD_LAYER_2 : 
        hwref.EN2 = enHwTrigger ? 1 : 0;
        dc.EN2 = enDirectCouple ? 1 : 0;
        break;
    case LCD_LAYER_3 : 
        hwref.EN3 = enHwTrigger ? 1 : 0;
        dc.EN3 = enDirectCouple ? 1 : 0;
        break;
    case LCD_LAYER_4 : 
        hwref.EN4 = enHwTrigger ? 1 : 0;
        dc.EN4 = enDirectCouple ? 1 : 0;
        break;
    case LCD_LAYER_5 : 
        hwref.EN5 = enHwTrigger ? 1 : 0;
        dc.EN5 = enDirectCouple ? 1 : 0;
        break;
    case LCD_LAYER_ALL :
        MASKREG32(&hwref, 0xFC000000, enHwTrigger ? 0xFC000000 : 0);
        MASKREG32(&dc, 0xFC000000, enDirectCouple ? 0xFC000000 : 0);
        break;
    default : ASSERT(0);
    }

    OUTREG32(&LCD_REG->WROI_HW_REFRESH, AS_UINT32(&hwref));
    OUTREG32(&LCD_REG->WROI_DC, AS_UINT32(&dc));

    return LCD_STATUS_OK;
}


LCD_STATUS LCD_LayerSetHwTriggerSrc(LCD_LAYER_ID id, LCD_HW_TRIGGER_SRC src)
{
    LCD_WROI_HWREF hwref;
    LCD_REG_WROI_DC dc;

    _WaitForEngineNotBusy();

    ASSERT(src == LCD_HW_TRIGGER_SRC_IRT1 ||
           src == LCD_HW_TRIGGER_SRC_IBW1 ||
           src == LCD_HW_TRIGGER_SRC_IBW2);

    hwref = LCD_REG->WROI_HW_REFRESH;
    dc = LCD_REG->WROI_DC;

    hwref.HREF_SEL = (UINT32)src;

    switch(id)
    {
    case LCD_LAYER_0 : dc.DC_SEL0 = (UINT32)src; break;
    case LCD_LAYER_1 : dc.DC_SEL1 = (UINT32)src; break;
    case LCD_LAYER_2 : dc.DC_SEL2 = (UINT32)src; break;
    case LCD_LAYER_3 : dc.DC_SEL3 = (UINT32)src; break;
    case LCD_LAYER_4 : dc.DC_SEL4 = (UINT32)src; break;
    case LCD_LAYER_5 : dc.DC_SEL5 = (UINT32)src; break;
    case LCD_LAYER_ALL :
        switch(src)
        {
        case LCD_HW_TRIGGER_SRC_IRT1 :
            MASKREG32(&dc, 0xFFF0, 0x0000);
            break;
        case LCD_HW_TRIGGER_SRC_IBW1 :
            MASKREG32(&dc, 0xFFF0, 0x5550);
            break;
        case LCD_HW_TRIGGER_SRC_IBW2 :
            MASKREG32(&dc, 0xFFF0, 0xFFF0);
            break;
        default : ASSERT(0);
        }
        break;
    default : ASSERT(0);
    }

    OUTREG32(&LCD_REG->WROI_HW_REFRESH, AS_UINT32(&hwref));
    OUTREG32(&LCD_REG->WROI_DC, AS_UINT32(&dc));

    return LCD_STATUS_OK;
}


LCD_STATUS LCD_EnableHwTrigger(BOOL enable)
{
    LCD_WROI_HWREF hwref = LCD_REG->WROI_HW_REFRESH;
    hwref.HWEN = enable ? 1 : 0;
    OUTREG32(&LCD_REG->WROI_HW_REFRESH, AS_UINT32(&hwref));

    // Disable all layers direct-couple
    //
    if (!enable && (INREG32(&LCD_REG->WROI_DC) & 0xFC000000))
    {
        MASKREG32(&LCD_REG->WROI_DC, 0xFC000000, 0);
    }

    return LCD_STATUS_OK;
}


// -------------------- ROI Window Configurations --------------------

LCD_STATUS LCD_SetBackgroundColor(UINT32 bgColor)
{
    _WaitForEngineNotBusy();
    LCD_REG->WROI_BG_COLOR = bgColor;
    
    return LCD_STATUS_OK;
}


LCD_STATUS LCD_SetRoiWindow(UINT32 x, UINT32 y, UINT32 width, UINT32 height)
{
    LCD_REG_COORD offset;
    LCD_REG_SIZE size;
    
    offset.X = (UINT16)x;
    offset.Y = (UINT16)y;
    size.WIDTH = (UINT16)width;
    size.HEIGHT = (UINT16)height;

    _WaitForEngineNotBusy();
    OUTREG32(&LCD_REG->WROI_OFFSET, AS_UINT32(&offset));
    OUTREG32(&LCD_REG->WROI_SIZE, AS_UINT32(&size));

    _lcdContext.roiWndSize = size;
        
    return LCD_STATUS_OK;
}

// -------------------- Output to Memory Configurations --------------------

LCD_STATUS LCD_SetOutputMode(LCD_OUTPUT_MODE mode)
{
    LCD_REG_WROI_CON    roiCtrl;
    LCD_REG_WROI_W2MCON w2mCtrl;

    _WaitForEngineNotBusy();

    roiCtrl = LCD_REG->WROI_CONTROL;
    w2mCtrl = LCD_REG->WROI_W2M_CONTROL;

    switch(mode)
    {
    case LCD_OUTPUT_TO_LCM :
        roiCtrl.W2M = 0;
        break;

    case LCD_OUTPUT_TO_LCM_MEM :
        roiCtrl.W2M = 1;
        w2mCtrl.W2LCM = 1;
        w2mCtrl.DISCON = 1; // always block write mode
        break;
        
    case LCD_OUTPUT_TO_MEM :
        roiCtrl.W2M = 1;
        w2mCtrl.W2LCM = 0;
        w2mCtrl.DISCON = 1; // always block write mode
        break;

    default :
        ASSERT(0);
    }

    OUTREG32(&LCD_REG->WROI_CONTROL, AS_UINT32(&roiCtrl));
    OUTREG32(&LCD_REG->WROI_W2M_CONTROL, AS_UINT32(&w2mCtrl));

    _lcdContext.outputMode = mode;

    return LCD_STATUS_OK;    
}


LCD_STATUS LCD_WaitDPIIndication(BOOL enable)
{
    LCD_REG_WROI_W2MCON w2mCtrl;

    _WaitForEngineNotBusy();

    w2mCtrl = LCD_REG->WROI_W2M_CONTROL;
    w2mCtrl.DLY_EN = enable ? 1 : 0;
    OUTREG32(&LCD_REG->WROI_W2M_CONTROL, AS_UINT32(&w2mCtrl));
    
    return LCD_STATUS_OK;
}


LCD_STATUS LCD_FBSetFormat(LCD_FB_FORMAT format)
{
    LCD_REG_WROI_W2MCON w2mCtrl;

    _WaitForEngineNotBusy();

    w2mCtrl = LCD_REG->WROI_W2M_CONTROL;
    w2mCtrl.W2M_FMT = (UINT32)format;
    OUTREG32(&LCD_REG->WROI_W2M_CONTROL, AS_UINT32(&w2mCtrl));

    _lcdContext.fbFormat = format;

    return LCD_STATUS_OK;
}


LCD_STATUS LCD_FBSetPitch(UINT32 pitchInByte)
{
    _lcdContext.fbPitchInBytes = pitchInByte;

    return LCD_STATUS_OK;
}


LCD_STATUS LCD_FBEnable(LCD_FB_ID id, BOOL enable)
{
    LCD_REG_WROI_W2MCON w2mCtrl;

    _WaitForEngineNotBusy();

    w2mCtrl = LCD_REG->WROI_W2M_CONTROL;

    switch(id)
    {
    case LCD_FB_0 : // do nothing
        break;
        
    case LCD_FB_1 :
        w2mCtrl.FB1_EN = enable ? 1 : 0;
        break;

    case LCD_FB_2 :
        w2mCtrl.FB2_EN = enable ? 1 : 0;
        break;

    default:
        ASSERT(0);
    }

    OUTREG32(&LCD_REG->WROI_W2M_CONTROL, AS_UINT32(&w2mCtrl));

    return LCD_STATUS_OK;
}


LCD_STATUS LCD_FBSetAddress(LCD_FB_ID id, UINT32 address)
{
    ASSERT(id < LCD_FB_NUM);

    _WaitForEngineNotBusy();
    LCD_REG->WROI_W2M_ADDR[id] = address;

    return LCD_STATUS_OK;
}


LCD_STATUS LCD_FBSetStartCoord(UINT32 x, UINT32 y)
{
    LCD_REG_COORD offset;
    offset.X = (UINT16)x;
    offset.Y = (UINT16)y;

    _WaitForEngineNotBusy();

    //WDT_SW_MM_PERI_RESET(MM_PERI_LCD);
    
    OUTREG32(&LCD_REG->WROI_W2M_OFFSET, AS_UINT32(&offset));

    return LCD_STATUS_OK;
}

// -------------------- Color Matrix --------------------

LCD_STATUS LCD_EnableColorMatrix(LCD_IF_ID id, BOOL enable)
{
    switch(id)
    {
    case LCD_IF_PARALLEL_0 :
    case LCD_IF_PARALLEL_1 :
    case LCD_IF_PARALLEL_2 :
    {
        UINT32 i = id - LCD_IF_PARALLEL_0;
        LCD_REG_PCNF config = LCD_REG->PARALLEL_CFG[i];
        config.CMR = config.CMG = config.CMB = (enable ? 1 : 0);
        OUTREG32(&LCD_REG->PARALLEL_CFG[i], AS_UINT32(&config));
        break;
    }
    case LCD_IF_SERIAL_0 :
    case LCD_IF_SERIAL_1 :
    {
        LCD_REG_SCNF config = LCD_REG->SERIAL_CFG;
        config.CMR = config.CMG = config.CMB = (enable ? 1 : 0);
        OUTREG32(&LCD_REG->SERIAL_CFG, AS_UINT32(&config));
        break;
    }
    case LCD_IF_ALL :
    {
        LCD_EnableColorMatrix(LCD_IF_PARALLEL_0, enable);
        LCD_EnableColorMatrix(LCD_IF_PARALLEL_1, enable);
        LCD_EnableColorMatrix(LCD_IF_PARALLEL_2, enable);
        LCD_EnableColorMatrix(LCD_IF_SERIAL_0, enable);
        break;
    }
    default: ASSERT(0);
    }

    return LCD_STATUS_OK;
}

LCD_STATUS LCD_SetColorMatrix(const S2_8 mat[9])
{
    UINT32 i, j = 0;

    for (i = 0; i < ARY_SIZE(LCD_REG->COEF_ROW); ++ i)
    {
        LCD_REG_COEF_ROW row = LCD_REG->COEF_ROW[i];
        row.COL0 = mat[j++];
        row.COL1 = mat[j++];
        row.COL2 = mat[j++];
        OUTREG32(&LCD_REG->COEF_ROW[i], AS_UINT32(&row));
    }

    return LCD_STATUS_OK;
}

// -------------------- Tearing Control --------------------

LCD_STATUS LCD_TE_Enable(BOOL enable)
{
    LCD_REG_TECON tecon = LCD_REG->TEARING_CFG;
    tecon.ENABLE = enable ? 1 : 0;
    OUTREG32(&LCD_REG->TEARING_CFG, AS_UINT32(&tecon));

    return LCD_STATUS_OK;
}


LCD_STATUS LCD_TE_SetMode(LCD_TE_MODE mode)
{
    LCD_REG_TECON tecon = LCD_REG->TEARING_CFG;
    tecon.MODE = (LCD_TE_MODE_VSYNC_OR_HSYNC == mode) ? 1 : 0;
    OUTREG32(&LCD_REG->TEARING_CFG, AS_UINT32(&tecon));

    return LCD_STATUS_OK;
}


LCD_STATUS LCD_TE_SetEdgePolarity(BOOL polarity)
{
    LCD_REG_TECON tecon = LCD_REG->TEARING_CFG;
    tecon.EDGE_SEL = (polarity ? 1 : 0);
    OUTREG32(&LCD_REG->TEARING_CFG, AS_UINT32(&tecon));

    return LCD_STATUS_OK;
}


LCD_STATUS LCD_TE_ConfigVHSyncMode(UINT32 hsDelayCnt,
                                   UINT32 vsWidthCnt,
                                   LCD_TE_VS_WIDTH_CNT_DIV vsWidthCntDiv)
{
    LCD_REG_TECON tecon = LCD_REG->TEARING_CFG;
    tecon.HS_MCH_CNT = (hsDelayCnt ? hsDelayCnt - 1 : 0);
    tecon.VS_WLMT = (vsWidthCnt ? vsWidthCnt - 1 : 0);
    tecon.VS_CNT_DIV = vsWidthCntDiv;
    OUTREG32(&LCD_REG->TEARING_CFG, AS_UINT32(&tecon));

    return LCD_STATUS_OK;
}

// -------------------- Operations --------------------

LCD_STATUS LCD_SelectWriteIF(LCD_IF_ID id)
{
    DWORD baseAddr = 0;

    switch(id)
    {
    case LCD_IF_PARALLEL_0 : baseAddr = (DWORD)&LCD_REG->PDATA0; break;
    case LCD_IF_PARALLEL_1 : baseAddr = (DWORD)&LCD_REG->PDATA1; break;
    case LCD_IF_PARALLEL_2 : baseAddr = (DWORD)&LCD_REG->PDATA2; break;
    case LCD_IF_SERIAL_0   : baseAddr = (DWORD)&LCD_REG->SDATA0; break;
    case LCD_IF_SERIAL_1   : baseAddr = (DWORD)&LCD_REG->SDATA1; break;
    default:
        ASSERT(0);
    }

    OUTREG16(&LCD_REG->WROI_CMD_ADDR, baseAddr);
    OUTREG16(&LCD_REG->WROI_DATA_ADDR, baseAddr + LCD_A0_HIGH_OFFSET);

    return LCD_STATUS_OK;
}


__inline static void _LCD_WriteIF(DWORD baseAddr, UINT32 value, LCD_IF_MCU_WRITE_BITS bits)
{
    switch(bits)
    {
    case LCD_IF_MCU_WRITE_8BIT :
        OUTREG8(baseAddr, value);
        break;
        
    case LCD_IF_MCU_WRITE_16BIT :
        OUTREG16(baseAddr, value);
        break;
        
    case LCD_IF_MCU_WRITE_32BIT :
        OUTREG32(baseAddr, value);
        break;

    default:
        ASSERT(0);
    }
}


LCD_STATUS LCD_WriteIF(LCD_IF_ID id, LCD_IF_A0_MODE a0,
                       UINT32 value, LCD_IF_MCU_WRITE_BITS bits)
{
    DWORD baseAddr = 0;

    switch(id)
    {
    case LCD_IF_PARALLEL_0 : baseAddr = (DWORD)&LCD_REG->PDATA0; break;
    case LCD_IF_PARALLEL_1 : baseAddr = (DWORD)&LCD_REG->PDATA1; break;
    case LCD_IF_PARALLEL_2 : baseAddr = (DWORD)&LCD_REG->PDATA2; break;
    case LCD_IF_SERIAL_0   : baseAddr = (DWORD)&LCD_REG->SDATA0; break;
    case LCD_IF_SERIAL_1   : baseAddr = (DWORD)&LCD_REG->SDATA1; break;
    default:
        ASSERT(0);
    }

    if (LCD_IF_A0_HIGH == a0)
    {
        baseAddr += LCD_A0_HIGH_OFFSET;
    }

    _LCD_WriteIF(baseAddr, value, bits);

    return LCD_STATUS_OK;
}


__inline static UINT32 _LCD_ReadIF(DWORD baseAddr, LCD_IF_MCU_WRITE_BITS bits)
{
    switch(bits)
    {
    case LCD_IF_MCU_WRITE_8BIT :
        return (UINT32)INREG8(baseAddr);
        
    case LCD_IF_MCU_WRITE_16BIT :
        return (UINT32)INREG16(baseAddr);
        
    case LCD_IF_MCU_WRITE_32BIT :
        return (UINT32)INREG32(baseAddr);

    default:
        ASSERT(0);
    }
}


LCD_STATUS LCD_ReadIF(LCD_IF_ID id, LCD_IF_A0_MODE a0,
                      UINT32 *value, LCD_IF_MCU_WRITE_BITS bits)
{
    DWORD baseAddr = 0;

    if (NULL == value) return LCD_STATUS_ERROR;

    switch(id)
    {
    case LCD_IF_PARALLEL_0 : baseAddr = (DWORD)&LCD_REG->PDATA0; break;
    case LCD_IF_PARALLEL_1 : baseAddr = (DWORD)&LCD_REG->PDATA1; break;
    case LCD_IF_PARALLEL_2 : baseAddr = (DWORD)&LCD_REG->PDATA2; break;
    case LCD_IF_SERIAL_0   : baseAddr = (DWORD)&LCD_REG->SDATA0; break;
    case LCD_IF_SERIAL_1   : baseAddr = (DWORD)&LCD_REG->SDATA1; break;
    default:
        ASSERT(0);
    }

    if (LCD_IF_A0_HIGH == a0)
    {
        baseAddr += LCD_A0_HIGH_OFFSET;
    }

    *value = _LCD_ReadIF(baseAddr, bits);

    return LCD_STATUS_OK;
}


LCD_STATUS LCD_StartTransfer(BOOL blocking)
{
    LCD_REG_SIZE mainWndSize = _lcdContext.roiWndSize;

    if (_lcdContext.outputMode & LCD_OUTPUT_TO_MEM)
    {
        UINT32 bpp = TO_BPP[_lcdContext.fbFormat];

        // Set pitch in pixel here according to frame buffer BPP
        ASSERT(_lcdContext.fbPitchInBytes);
        ASSERT(_lcdContext.fbPitchInBytes % bpp == 0);
        mainWndSize.WIDTH = (UINT16)(_lcdContext.fbPitchInBytes / bpp);
    }

    OUTREG32(&LCD_REG->MWIN_SIZE, AS_UINT32(&mainWndSize));

    _WaitForEngineNotBusy();

    DBG_OnTriggerLcd();

    OUTREG32(&LCD_REG->START, 0);
    OUTREG32(&LCD_REG->START, (1 << 15));

    if (blocking)
    {
        _WaitForEngineNotBusy();
    }

    return LCD_STATUS_OK;
}

// -------------------- Retrieve Information --------------------

LCD_STATE  LCD_GetState(void)
{
    if (_IsEngineBusy())
    {
        return LCD_STATE_BUSY;
    }

    return LCD_STATE_IDLE;
}


LCD_STATUS LCD_DumpRegisters(void)
{
    UINT32 i;

    printk("---------- Start dump LCD registers ----------\n");
    
    for (i = 0; i < offsetof(LCD_REGS, COEF_ROW); i += 4)
    {
        printk("LCD+%04x : 0x%08x\n", i, INREG32(LCD_BASE + i));
    }

    for (i = offsetof(LCD_REGS, CMDQ); i < sizeof(LCD_REGS); i += 4)
    {
        printk("LCD+%04x : 0x%08x\n", i, INREG32(LCD_BASE + i));
    }

    return LCD_STATUS_OK;
}
