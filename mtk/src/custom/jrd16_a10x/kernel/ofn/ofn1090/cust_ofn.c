
#include <cust_ofn.h>
#include <mach/mt6516_gpt_sw.h>
#include <mach/mt6516_pll.h>
static struct ofn_hw cust_ofn_hw = {
    .power_id   = MT6516_POWER_NONE,    /*!< LDO is not used */
    .power_vol  = VOL_DEFAULT,          /*!< LDO is not used */

    .report_cls = OFN_CLASS_TRACKBALL,        
    .chip_id    = OFN1090,
    .slave_addr = 0x33,
    .i2c_num    = 2,
    .layout     = 0x06,         /*(XY_SWAP, Y_INV, X_INV) = (1,0,0)*/

    /*trackball class*/
    .quan_x     = 12,
    .quan_y     = 12,
    .accu_max   = 8,
    
    /*keyboard class*/
    .detect     = 1,            /*eint*/
    .gpt_num    = XGPT6,
    .gpt_period = 33,           /* GPT ticks --> 33 ticks is about 1ms */

    .acc_cnt    = 1000,
    .inact_cnt  = 600,
    .act_cnt    = 0,
    .sup_cnt    = 20,
    .step       = 50,
};

struct ofn_hw *get_cust_ofn_hw(void)
{
    return &cust_ofn_hw;
}

