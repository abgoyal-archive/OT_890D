

#include <linux/delay.h>

#include <mach/mt6516_typedefs.h>
#include <mach/mt6516_gpio.h>
#include <mach/mt6516_ap_config.h>

#include "disp_drv.h"
#include "lcd_drv.h"
#include "lcm_drv.h"

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------
#define LCM_FOXCONN 1
#define LCM_HIMAX 0

static int lcm_type = LCM_FOXCONN;
static const DISP_DRIVER *disp_drv = NULL;
static const LCM_DRIVER  *lcm_drv  = NULL;
static LCM_PARAMS lcm_params = {0};
static LCD_IF_ID ctrl_if = LCD_IF_PARALLEL_0;

static volatile int direct_link_layer = -1;

DECLARE_MUTEX(sem_update_screen);
static BOOL is_engine_in_suspend_mode = FALSE;
static BOOL is_lcm_in_suspend_mode    = FALSE;


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

static void lcm_set_reset_pin(UINT32 value)
{
    LCD_SetResetSignal(value);
}

static void lcm_udelay(UINT32 us)
{
    udelay(us);
}

static void lcm_mdelay(UINT32 ms)
{
    msleep(ms);
}

static void lcm_send_cmd(UINT32 cmd)
{
    ASSERT(LCM_CTRL_SERIAL_DBI   == lcm_params.ctrl ||
           LCM_CTRL_PARALLEL_DBI == lcm_params.ctrl);

    LCD_CHECK_RET(LCD_WriteIF(ctrl_if, LCD_IF_A0_LOW,
                              cmd, lcm_params.dbi.cpu_write_bits));
}

static void lcm_send_data(UINT32 data)
{
    ASSERT(LCM_CTRL_SERIAL_DBI   == lcm_params.ctrl ||
           LCM_CTRL_PARALLEL_DBI == lcm_params.ctrl);

    LCD_CHECK_RET(LCD_WriteIF(ctrl_if, LCD_IF_A0_HIGH,
                              data, lcm_params.dbi.cpu_write_bits));
}

static UINT32 lcm_read_data(void)
{
    UINT32 data = 0;
    
    ASSERT(LCM_CTRL_SERIAL_DBI   == lcm_params.ctrl ||
           LCM_CTRL_PARALLEL_DBI == lcm_params.ctrl);

    LCD_CHECK_RET(LCD_ReadIF(ctrl_if, LCD_IF_A0_HIGH,
                             &data, lcm_params.dbi.cpu_write_bits));

    return data;
}

static BOOL disp_drv_init_context(void)
{
    static const LCM_UTIL_FUNCS lcm_utils =
    {
        .set_reset_pin      = lcm_set_reset_pin,
        .set_gpio_out       = mt_set_gpio_out,
        .udelay             = lcm_udelay,
        .mdelay             = lcm_mdelay,
        .send_cmd           = lcm_send_cmd,
        .send_data          = lcm_send_data,
        .read_data          = lcm_read_data,

        /** FIXME: GPIO mode should not be configured in lcm driver
                   REMOVE ME after GPIO customization is done    
        */
        .set_gpio_mode        = mt_set_gpio_mode,
        .set_gpio_dir         = mt_set_gpio_dir,
        .set_gpio_pull_enable = mt_set_gpio_pull_enable
    };

    //if (disp_drv != NULL && lcm_drv != NULL) return TRUE;

    if(lcm_type==LCM_FOXCONN)
    	lcm_drv = LCM_GetDriver();
    else
	lcm_drv = LCM_GetJrdDriver();
    if (NULL == lcm_drv) return FALSE;
    
    lcm_drv->set_util_funcs(&lcm_utils);
    lcm_drv->get_params(&lcm_params);

    switch(lcm_params.type)
    {
    case LCM_TYPE_DBI : disp_drv = DISP_GetDriverDBI(); break;
    case LCM_TYPE_DPI : disp_drv = DISP_GetDriverDPI(); break;
    default : ASSERT(0);
    }

    if (!disp_drv) return FALSE;

    return TRUE;
}


static __inline LCD_IF_WIDTH to_lcd_if_width(LCM_DBI_DATA_WIDTH data_width)
{
    switch(data_width)
    {
    case LCM_DBI_DATA_WIDTH_8BITS  : return LCD_IF_WIDTH_8_BITS;
    case LCM_DBI_DATA_WIDTH_9BITS  : return LCD_IF_WIDTH_9_BITS;
    case LCM_DBI_DATA_WIDTH_16BITS : return LCD_IF_WIDTH_16_BITS;
    case LCM_DBI_DATA_WIDTH_18BITS : return LCD_IF_WIDTH_18_BITS;
    case LCM_DBI_DATA_WIDTH_24BITS : return LCD_IF_WIDTH_24_BITS;
    default : ASSERT(0);
    }
    return LCD_IF_WIDTH_18_BITS;
}


static void disp_drv_init_ctrl_if(void)
{
    const LCM_DBI_PARAMS *dbi = &lcm_params.dbi;

    switch(lcm_params.ctrl)
    {
    case LCM_CTRL_NONE :
    case LCM_CTRL_GPIO : return;

    case LCM_CTRL_SERIAL_DBI :
        ASSERT(dbi->port <= 1);
        ctrl_if = LCD_IF_SERIAL_0 + dbi->port;
        LCD_ConfigSerialIF(ctrl_if,
                           (LCD_IF_SERIAL_BITS)dbi->data_width,
                           dbi->serial.clk_polarity,
                           dbi->serial.clk_phase,
                           dbi->serial.cs_polarity,
                           (LCD_IF_SERIAL_CLK_DIV)dbi->clock_freq,
                           dbi->serial.is_non_dbi_mode);
        break;
        
    case LCM_CTRL_PARALLEL_DBI :
        ASSERT(dbi->port <= 2);
        ctrl_if = LCD_IF_PARALLEL_0 + dbi->port;
        LCD_ConfigParallelIF(ctrl_if,
                             (LCD_IF_PARALLEL_BITS)dbi->data_width,
                             (LCD_IF_PARALLEL_CLK_DIV)dbi->clock_freq,
                             dbi->parallel.write_setup,
                             dbi->parallel.write_hold,
                             dbi->parallel.write_wait,
                             dbi->parallel.read_setup,
                             dbi->parallel.read_latency,
                             dbi->parallel.wait_period);
        break;

    default : ASSERT(0);
    }

    LCD_CHECK_RET(LCD_SelectWriteIF(ctrl_if));

    LCD_CHECK_RET(LCD_ConfigIfFormat(dbi->data_format.color_order,
                                     dbi->data_format.trans_seq,
                                     dbi->data_format.padding,
                                     dbi->data_format.format,
                                     to_lcd_if_width(dbi->data_format.width)));
}


#define SET_DRV_CURRENT(offset, value) \
    MASKREG32(IO_DRV0, (0xF << offset), ((value) & 0xF) << (offset))

static void disp_drv_set_io_driving_current(void)
{
    if (LCM_CTRL_SERIAL_DBI == lcm_params.ctrl) {
        SET_DRV_CURRENT(0, lcm_params.dbi.io_driving_current);
    } else if (LCM_CTRL_PARALLEL_DBI == lcm_params.ctrl) {
        SET_DRV_CURRENT(4, lcm_params.dbi.io_driving_current);
    }    

    if (LCM_TYPE_DPI == lcm_params.type) {
        SET_DRV_CURRENT(8, lcm_params.dpi.io_driving_current);
    }
}


// ---------------------------------------------------------------------------
//  DISP Driver Implementations
// ---------------------------------------------------------------------------

DISP_STATUS DISP_Init(UINT32 fbVA, UINT32 fbPA, BOOL isLcmInited)
{
    if (!disp_drv_init_context()) {
        return DISP_STATUS_NOT_IMPLEMENTED;
    }

    /* power on LCD before config its registers*/
    LCD_CHECK_RET(LCD_Init());

    disp_drv_init_ctrl_if();
    disp_drv_set_io_driving_current();
    
    return (disp_drv->init) ?
           (disp_drv->init(fbVA, fbPA, isLcmInited)) :
           DISP_STATUS_NOT_IMPLEMENTED;
}


DISP_STATUS DISP_Deinit(void)
{
    DISP_CHECK_RET(DISP_PanelEnable(FALSE));
    DISP_CHECK_RET(DISP_PowerEnable(FALSE));
    
    return DISP_STATUS_OK;
}

// -----

DISP_STATUS DISP_PowerEnable(BOOL enable)
{
    DISP_STATUS ret = DISP_STATUS_OK;

    if (down_interruptible(&sem_update_screen)) {
        printk("[DISP] ERROR: Can't get sem_update_screen in DISP_PowerEnable()\n");
        return DISP_STATUS_ERROR;
    }
    
    disp_drv_init_context();
        
    is_engine_in_suspend_mode = enable ? FALSE : TRUE;

    ret = (disp_drv->enable_power) ?
          (disp_drv->enable_power(enable)) :
          DISP_STATUS_NOT_IMPLEMENTED;

    // If in direct link mode, re-start LCD after system resume
    //
    if (enable && -1 != direct_link_layer) {
        LCD_CHECK_RET(LCD_StartTransfer(FALSE));
    }

    up(&sem_update_screen);

    return ret;
}


DISP_STATUS DISP_PanelEnable(BOOL enable)
{
    static BOOL s_enabled = TRUE;

    DISP_STATUS ret = DISP_STATUS_OK;

    if (down_interruptible(&sem_update_screen)) {
        printk("[DISP] ERROR: Can't get sem_update_screen in DISP_PanelEnable()\n");
        return DISP_STATUS_ERROR;
    }

    disp_drv_init_context();
    
    is_lcm_in_suspend_mode = enable ? FALSE : TRUE;

    if (!lcm_drv->suspend || !lcm_drv->resume) {
        ret = DISP_STATUS_NOT_IMPLEMENTED;
        goto End;
    }

    if (enable && !s_enabled) {
        s_enabled = TRUE;
        lcm_drv->resume();
    }
    else if (!enable && s_enabled)
    {
        s_enabled = FALSE;
        LCD_CHECK_RET(LCD_WaitForNotBusy());
        lcm_drv->suspend();
    }

End:
    up(&sem_update_screen);

    return ret;
}

// -----

DISP_STATUS DISP_SetFrameBufferAddr(UINT32 fbPhysAddr)
{
    disp_drv_init_context();
        
    return (disp_drv->set_fb_addr) ?
           (disp_drv->set_fb_addr(fbPhysAddr)) :
           DISP_STATUS_NOT_IMPLEMENTED;
}

const unsigned int get_disp_id(void)
{	
	mt_set_gpio_mode(46, GPIO_MODE_GPIO);
	mt_set_gpio_dir(46, GPIO_DIR_IN);
	udelay(100);

	if(mt_get_gpio_in(46)==1)
		lcm_type = LCM_FOXCONN;
	else
		lcm_type = LCM_HIMAX;
	printk(KERN_ERR"******lcm id = %d\n",lcm_type);
	
	return lcm_type;	
	//return lcm_id_detect();
}

// -----

static BOOL is_overlaying = FALSE;

DISP_STATUS DISP_EnterOverlayMode(void)
{
    if (is_overlaying) {
        return DISP_STATUS_ALREADY_SET;
    } else {
        is_overlaying = TRUE;
    }

    return DISP_STATUS_OK;
}


DISP_STATUS DISP_LeaveOverlayMode(void)
{
    if (!is_overlaying) {
        return DISP_STATUS_ALREADY_SET;
    } else {
        is_overlaying = FALSE;
    }

    return DISP_STATUS_OK;
}


// -----

DISP_STATUS DISP_EnableDirectLinkMode(UINT32 layer)
{
    if (layer != direct_link_layer) {
        LCD_CHECK_RET(LCD_LayerSetTriggerMode(layer, LCD_HW_TRIGGER_DIRECT_COUPLE));
        LCD_CHECK_RET(LCD_LayerSetHwTriggerSrc(layer, LCD_HW_TRIGGER_SRC_IBW2));
        LCD_CHECK_RET(LCD_EnableHwTrigger(TRUE));
        LCD_CHECK_RET(LCD_StartTransfer(FALSE));
        direct_link_layer = layer;
    }

    return DISP_STATUS_OK;
}


DISP_STATUS DISP_DisableDirectLinkMode(UINT32 layer)
{
    if (layer == direct_link_layer) {
        LCD_CHECK_RET(LCD_EnableHwTrigger(FALSE));
        direct_link_layer = -1;
    }
    LCD_CHECK_RET(LCD_LayerSetTriggerMode(layer, LCD_SW_TRIGGER));

    return DISP_STATUS_OK;
}

// -----

extern int MT6516IDP_EnableDirectLink(void);

DISP_STATUS DISP_UpdateScreen(UINT32 x, UINT32 y, UINT32 width, UINT32 height)
{
    if (down_interruptible(&sem_update_screen)) {
        printk("[DISP] ERROR: Can't get sem_update_screen in DISP_UpdateScreen()\n");
        return DISP_STATUS_ERROR;
    }

    // if LCM is powered down, LCD would never recieve the TE signal
    //
    if (is_lcm_in_suspend_mode || is_engine_in_suspend_mode) goto End;

    LCD_CHECK_RET(LCD_WaitForNotBusy());

    if (lcm_drv->update) {
        lcm_drv->update(x, y, width, height);
    }

    LCD_CHECK_RET(LCD_SetRoiWindow(x, y, width, height));
    LCD_CHECK_RET(LCD_FBSetStartCoord(x, y));
           
    if (-1 != direct_link_layer) {
        MT6516IDP_EnableDirectLink();
    } else {
        LCD_CHECK_RET(LCD_StartTransfer(FALSE));
    }

End:
    up(&sem_update_screen);

    return DISP_STATUS_OK;
}


// ---------------------------------------------------------------------------
//  Retrieve Information
// ---------------------------------------------------------------------------

UINT32 DISP_GetScreenWidth(void)
{
    disp_drv_init_context();
    return lcm_params.width;
}


UINT32 DISP_GetScreenHeight(void)
{
    disp_drv_init_context();
    return lcm_params.height;
}


UINT32 DISP_GetScreenBpp(void)
{
    return 32;  // ARGB8888
}


UINT32 DISP_GetPages(void)
{
    return 2;   // Double Buffers
}


BOOL DISP_IsDirectLinkMode(void)
{
    return (-1 != direct_link_layer) ? TRUE : FALSE;
}


BOOL DISP_IsInOverlayMode(void)
{
    return is_overlaying;
}


#define ALIGN_TO_POW_OF_2(x, n)  \
    (((x) + ((n) - 1)) & ~((n) - 1))

UINT32 DISP_GetVRamSize(void)
{
    // Use a local static variable to cache the calculated vram size
    //    
    static UINT32 vramSize = 0;

    if (0 == vramSize)
    {
        disp_drv_init_context();

        vramSize = disp_drv->get_vram_size();
        
        // Align vramSize to 1MB
        //
        vramSize = ALIGN_TO_POW_OF_2(vramSize, 0x100000);

        printk("DISP_GetVRamSize: %u bytes\n", vramSize);
    }

    return vramSize;
}


PANEL_COLOR_FORMAT DISP_GetPanelColorFormat(void)
{
    disp_drv_init_context();
        
    return (disp_drv->get_panel_color_format) ?
           (disp_drv->get_panel_color_format()) :
           DISP_STATUS_NOT_IMPLEMENTED;
}

