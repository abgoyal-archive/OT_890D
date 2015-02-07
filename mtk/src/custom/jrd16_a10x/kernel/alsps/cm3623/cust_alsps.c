
#include <cust_alsps.h>
#include <mach/mt6516_pll.h>
static struct alsps_hw cust_alsps_hw = {
    .i2c_num    = 2,
    .power_id   = MT6516_POWER_NONE,    /*LDO is not used*/
    .power_vol  = VOL_DEFAULT,          /*LDO is not used*/
    .i2c_addr   = {0x0C, 0x48, 0x78, 0x00},
    .als_level  = { 0,  0,  0,   0,   0,   7,   15,  200,  300,   500,   900,  1200,  2000, 2500,  20000},
    .als_value  = {40, 40, 90,  90, 160, 160,  225,  320,  640,  1280,  1280,  2600,  2600, 2600,  10240, 10240},
    .ps_threshold = 3,
};
struct alsps_hw *get_cust_alsps_hw(void) {
    return &cust_alsps_hw;
}
