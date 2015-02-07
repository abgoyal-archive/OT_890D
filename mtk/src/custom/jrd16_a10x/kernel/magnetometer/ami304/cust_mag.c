
#include <cust_mag.h>
#include <mach/mt6516_pll.h>
static struct mag_hw cust_mag_hw = {
    .i2c_num = 2,
    .direction = 4,
    .power_id = MT6516_POWER_NONE,  /*!< LDO is not used */
    .power_vol= VOL_DEFAULT,        /*!< LDO is not used */
};
struct mag_hw* get_cust_mag_hw(void) 
{
    return &cust_mag_hw;
}
