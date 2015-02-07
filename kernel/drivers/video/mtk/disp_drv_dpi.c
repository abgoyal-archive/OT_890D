

#include <mach/mt6516_graphsys.h>

#include "disp_drv.h"
#include "disp_assert_layer.h"
#include "lcd_drv.h"
#include "dpi_drv.h"
#include "dsi_reg.h"

#include "lcm_drv.h"

PDSI_PHY_REGS const DSI_PHY_REG = (PDSI_PHY_REGS)(DSI_PHY_BASE);


// ---------------------------------------------------------------------------
//  Private Variables
// ---------------------------------------------------------------------------

static const LCM_DRIVER *lcm_drv = NULL;
static LCM_PARAMS lcm_params = {0};

typedef struct
{
    UINT32 pa;
    UINT32 pitchInBytes;
} TempBuffer;

static TempBuffer s_tmpBuffers[3];


// ---------------------------------------------------------------------------
//  Private Functions
// ---------------------------------------------------------------------------

__inline DPI_FB_FORMAT get_tmp_buffer_dpi_format(void)
{
    switch(lcm_params.dpi.format)
    {
    case LCM_DPI_FORMAT_RGB565 : return DPI_FB_FORMAT_RGB565;
    case LCM_DPI_FORMAT_RGB666 :
    case LCM_DPI_FORMAT_RGB888 : return DPI_FB_FORMAT_RGB888;
    default : ASSERT(0);
    }
    return DPI_FB_FORMAT_RGB888;
}


__inline UINT32 get_tmp_buffer_bpp(void)
{
    static const UINT32 TO_BPP[] = {2, 3};
    
    return TO_BPP[get_tmp_buffer_dpi_format()];
}


__inline LCD_FB_FORMAT get_tmp_buffer_lcd_format(void)
{
    static const UINT32 TO_LCD_FORMAT[] = {
        LCD_FB_FORMAT_RGB565,
        LCD_FB_FORMAT_RGB888
    };
    
    return TO_LCD_FORMAT[get_tmp_buffer_dpi_format()];
}


static BOOL disp_drv_dpi_init_context(void)
{
    if (lcm_drv != NULL) return TRUE;

    lcm_drv = LCM_GetDriver();

    if (NULL == lcm_drv) {
        ASSERT(0);
        return FALSE;
    }

    lcm_drv->get_params(&lcm_params);
    
    return TRUE;
}


static UINT32 get_fb_size(void)
{
    return DISP_GetScreenWidth() * 
           DISP_GetScreenHeight() * 
           ((DISP_GetScreenBpp() + 7) >> 3) * 
           DISP_GetPages();
}


static UINT32 get_intermediate_buffer_size(void)
{
    disp_drv_dpi_init_context();

    return 
#ifdef CONFIG_FB_MT6516_SIMULATE_MULTIPLE_RESOLUTIONS_ON_OPPO
    480 * 800 *
#else   
    DISP_GetScreenWidth() *
    DISP_GetScreenHeight() *
#endif              
    get_tmp_buffer_bpp() *
    lcm_params.dpi.intermediat_buffer_num;
}


static UINT32 get_assert_layer_size(void)
{
    return DAL_GetLayerSize();
}


static void init_intermediate_buffers(UINT32 fbPhysAddr)
{
    UINT32 tmpFbStartPA = fbPhysAddr + get_fb_size();

#ifdef CONFIG_FB_MT6516_SIMULATE_MULTIPLE_RESOLUTIONS_ON_OPPO
    UINT32 tmpFbPitchInBytes = 480 * get_tmp_buffer_bpp();
    UINT32 tmpFbSizeInBytes  = tmpFbPitchInBytes * 800;
#else	
    UINT32 tmpFbPitchInBytes = DISP_GetScreenWidth() * get_tmp_buffer_bpp();
    UINT32 tmpFbSizeInBytes  = tmpFbPitchInBytes * DISP_GetScreenHeight();
#endif	

    UINT32 i;
    
    for (i = 0; i < lcm_params.dpi.intermediat_buffer_num; ++ i)
    {
        TempBuffer *b = &s_tmpBuffers[i];
        
        b->pitchInBytes = tmpFbPitchInBytes;
        b->pa = tmpFbStartPA;
        ASSERT((tmpFbStartPA & 0x7) == 0);  // check if 8-byte-aligned
        tmpFbStartPA += tmpFbSizeInBytes;
    }
}


static void init_assertion_layer(UINT32 fbVA, UINT32 fbPA)
{
    UINT32 offset = get_fb_size() + get_intermediate_buffer_size();
    DAL_STATUS ret = DAL_Init(fbVA + offset, fbPA + offset);
    ASSERT(DAL_STATUS_OK == ret);
}


static void init_mipi_pll(void)
{
    DSI_PHY_REG_ANACON0 con0 = DSI_PHY_REG->ANACON0;
    DSI_PHY_REG_ANACON1 con1 = DSI_PHY_REG->ANACON1;
    DSI_PHY_REG_ANACON2 con2 = DSI_PHY_REG->ANACON2;

    con0.PLL_EN = 1;
    con0.RG_PLL_DIV1 = lcm_params.dpi.mipi_pll_clk_div1;
    con1.RG_PLL_CLKR = lcm_params.dpi.mipi_pll_clk_ref;
    con1.RG_PLL_DIV2 = lcm_params.dpi.mipi_pll_clk_div2;

    // FIXME: don't know why should we confgure the following bits

    con0.RG_PLL_LN        = 0x3;
    con2.RG_LNT_BGR_EN    = 0x0;
    con2.RG_LNT_BGR_CHPEN = 0x0;

    // Set to DSI_PHY_REG
    
    OUTREG32(&DSI_PHY_REG->ANACON0, AS_UINT32(&con0));
    OUTREG32(&DSI_PHY_REG->ANACON1, AS_UINT32(&con1));
    OUTREG32(&DSI_PHY_REG->ANACON2, AS_UINT32(&con2));
}


static void init_io_pad(void)
{
    GRAPH1SYS_LCD_IO_SEL_MODE sel_mode;

    if (lcm_params.dpi.is_serial_output) {
        sel_mode = GRAPH1SYS_LCD_IO_SEL_18CPU_8RGB;
    } else if (LCM_DPI_FORMAT_RGB565 == lcm_params.dpi.format) {
        sel_mode = GRAPH1SYS_LCD_IO_SEL_9CPU_16RGB;
    } else {
        sel_mode = GRAPH1SYS_LCD_IO_SEL_8CPU_18RGB;
    }

    MASKREG32(GRAPH1SYS_LCD_IO_SEL, GRAPH1SYS_LCD_IO_SEL_MASK, sel_mode);
}


static void init_lcd(void)
{
    UINT32 i;

    LCD_CHECK_RET(LCD_LayerEnable(LCD_LAYER_ALL, FALSE));
    LCD_CHECK_RET(LCD_LayerSetTriggerMode(LCD_LAYER_ALL, LCD_SW_TRIGGER));
    LCD_CHECK_RET(LCD_EnableHwTrigger(FALSE));

    LCD_CHECK_RET(LCD_SetBackgroundColor(0));
    LCD_CHECK_RET(LCD_SetRoiWindow(0, 0, DISP_GetScreenWidth(), DISP_GetScreenHeight()));

    LCD_CHECK_RET(LCD_FBSetFormat(get_tmp_buffer_lcd_format()));
    LCD_CHECK_RET(LCD_FBSetPitch(s_tmpBuffers[0].pitchInBytes));
    LCD_CHECK_RET(LCD_FBSetStartCoord(0, 0));

    for (i = 0; i < lcm_params.dpi.intermediat_buffer_num; ++ i)
    {
        LCD_CHECK_RET(LCD_FBSetAddress(LCD_FB_0 + i, s_tmpBuffers[i].pa));
        LCD_CHECK_RET(LCD_FBEnable(LCD_FB_0 + i, TRUE));
    }
    
    LCD_CHECK_RET(LCD_SetOutputMode(LCD_OUTPUT_TO_MEM));
    /**
       "LCD Delay Enable" function should be used when there is only
       single buffer between LCD and DPI.
       Double buffer even triple buffer need not enable it.
    */
    LCD_CHECK_RET(LCD_WaitDPIIndication(FALSE));
}


static void init_dpi(BOOL isDpiPoweredOn)
{
    const LCM_DPI_PARAMS *dpi = &lcm_params.dpi;
    UINT32 i;

    DPI_CHECK_RET(DPI_Init(isDpiPoweredOn));

    DPI_CHECK_RET(DPI_EnableSeqOutput(FALSE));

    DPI_CHECK_RET(DPI_ConfigPixelClk((DPI_POLARITY)dpi->clk_pol,
                                     dpi->dpi_clk_div, dpi->dpi_clk_duty));

    DPI_CHECK_RET(DPI_ConfigDataEnable((DPI_POLARITY)dpi->de_pol));

    DPI_CHECK_RET(DPI_ConfigHsync((DPI_POLARITY)dpi->hsync_pol,
                                  dpi->hsync_pulse_width,
                                  dpi->hsync_back_porch,
                                  dpi->hsync_front_porch));

    DPI_CHECK_RET(DPI_ConfigVsync((DPI_POLARITY)dpi->vsync_pol,
                                  dpi->vsync_pulse_width,
                                  dpi->vsync_back_porch,
                                  dpi->vsync_front_porch));

#ifdef CONFIG_FB_MT6516_SIMULATE_MULTIPLE_RESOLUTIONS_ON_OPPO
    DPI_CHECK_RET(DPI_FBSetSize(480, 800));
#else	
    DPI_CHECK_RET(DPI_FBSetSize(DISP_GetScreenWidth(), DISP_GetScreenHeight()));
#endif	
    
    for (i = 0; i < dpi->intermediat_buffer_num; ++ i)
    {
        DPI_CHECK_RET(DPI_FBSetAddress(DPI_FB_0 + i, s_tmpBuffers[i].pa));
        DPI_CHECK_RET(DPI_FBSetPitch(DPI_FB_0 + i, s_tmpBuffers[i].pitchInBytes));
        DPI_CHECK_RET(DPI_FBEnable(DPI_FB_0 + i, TRUE));
    }
    DPI_CHECK_RET(DPI_FBSetFormat(get_tmp_buffer_dpi_format()));
    DPI_CHECK_RET(DPI_FBSyncFlipWithLCD(TRUE));

    if (LCM_COLOR_ORDER_BGR == dpi->rgb_order) {
        DPI_CHECK_RET(DPI_SetRGBOrder(DPI_RGB_ORDER_RGB, DPI_RGB_ORDER_BGR));
    } else {
        DPI_CHECK_RET(DPI_SetRGBOrder(DPI_RGB_ORDER_RGB, DPI_RGB_ORDER_RGB));
    }

    DPI_CHECK_RET(DPI_EnableClk());
}
    

// ---------------------------------------------------------------------------
//  DPI Display Driver Public Functions
// ---------------------------------------------------------------------------

static DISP_STATUS dpi_init(UINT32 fbVA, UINT32 fbPA, BOOL isLcmInited)
{
    if (!disp_drv_dpi_init_context()) 
        return DISP_STATUS_NOT_IMPLEMENTED;

    init_intermediate_buffers(fbPA);

    init_mipi_pll();
    init_io_pad();

    init_lcd();
    init_dpi(isLcmInited);

    init_assertion_layer(fbVA, fbPA);

    if (NULL != lcm_drv->init && !isLcmInited) {
        lcm_drv->init();
    }

    return DISP_STATUS_OK;
}


static DISP_STATUS dpi_enable_power(BOOL enable)
{
    if (enable) {
        DPI_CHECK_RET(DPI_PowerOn());
        LCD_CHECK_RET(LCD_PowerOn());
        DPI_CHECK_RET(DPI_EnableClk());
    } else {
        DPI_CHECK_RET(DPI_DisableClk());
        LCD_CHECK_RET(LCD_PowerOff());
        DPI_CHECK_RET(DPI_PowerOff());
    }
    return DISP_STATUS_OK;
}


static DISP_STATUS dpi_set_fb_addr(UINT32 fbPhysAddr)
{
    LCD_CHECK_RET(LCD_LayerSetAddress(FB_LAYER, fbPhysAddr));

    return DISP_STATUS_OK;
}


static UINT32 dpi_get_vram_size(void)
{
    return get_fb_size() +
           get_intermediate_buffer_size() +
           get_assert_layer_size();
}


static PANEL_COLOR_FORMAT dpi_get_panel_color_format(void)
{
    disp_drv_dpi_init_context();

    switch(lcm_params.dpi.format)
    {
    case LCM_DPI_FORMAT_RGB565 : return PANEL_COLOR_FORMAT_RGB565;
    case LCM_DPI_FORMAT_RGB666 : return PANEL_COLOR_FORMAT_RGB666;
    case LCM_DPI_FORMAT_RGB888 : return PANEL_COLOR_FORMAT_RGB888;
    default : ASSERT(0);
    }
    return PANEL_COLOR_FORMAT_RGB888;
}


const DISP_DRIVER *DISP_GetDriverDPI()
{
    static const DISP_DRIVER DPI_DISP_DRV =
    {
        .init                   = dpi_init,
        .enable_power           = dpi_enable_power,
        .set_fb_addr            = dpi_set_fb_addr,
        .get_vram_size          = dpi_get_vram_size,
        .get_panel_color_format = dpi_get_panel_color_format
    };

    return &DPI_DISP_DRV;
}

