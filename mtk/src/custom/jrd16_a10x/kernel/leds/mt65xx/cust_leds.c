
#include <cust_leds.h>
#include <mach/mt6516_pll.h>
#include <mach/mt6516_pwm.h>

static int brightness_set_bct3222(int level)
{
	static int clock_enable = 0;
        unsigned int con;
        int phylevel;

        // mapping 0~255 level to 0~8 level
        if (level < 1)
                phylevel = 0;
        else if (level < 29)
                phylevel = 1;
        else if (level <= 120)
                phylevel = (level+40) / 35;
        else
                phylevel = (level + 14) / 30;

        if (phylevel)
                con = 0x00005E87 + ((8 - phylevel) << 10);
        else
                con = 0x00005E07;

	if (!clock_enable)
		hwEnableClock(MT6516_PDN_PERI_PWM,"PWM6");

	CLRREG32(PWM_ENABLE, PWM6_ENABLE);
	OUTREG32(PWM6_CON, con);
	OUTREG32(PWM6_HDURATION, 0);
	OUTREG32(PWM6_LDURATION, 2);
	OUTREG32(PWM6_GDURATION, 0);
	OUTREG32(PWM6_BUF0_BASE_ADDR, 0);
	OUTREG32(PWM6_BUF0_SIZE, 0);
	OUTREG32(PWM6_BUF1_BASE_ADDR, 0);
	OUTREG32(PWM6_BUF1_SIZE, 0);
	OUTREG32(PWM6_SEND_DATA0, 0);
	OUTREG32(PWM6_SEND_DATA1, 0xD555FFFF);
	OUTREG32(PWM6_WAVE_NUM, 0x1);
	OUTREG32(PWM6_VALID, 0x0);
	SETREG32(PWM_ENABLE, PWM6_ENABLE);

	if (!phylevel) {
		CLRREG32(PWM_ENABLE, PWM6_ENABLE);
		hwDisableClock(MT6516_PDN_PERI_PWM,"PWM6");
	}

        return 0;
}

struct cust_mt65xx_led cust_led_list[MT65XX_LED_TYPE_TOTAL] = {
	{"red",               MT65XX_LED_MODE_GPIO, 14},
	{"green",             MT65XX_LED_MODE_NONE, -1},
	{"blue",              MT65XX_LED_MODE_NONE, -1},
	{"jogball-backlight", MT65XX_LED_MODE_NONE, -1},
	{"keyboard-backlight",MT65XX_LED_MODE_NONE, -1},
	{"button-backlight",  MT65XX_LED_MODE_PMIC, MT65XX_LED_PMIC_BUTTON},
	{"lcd-backlight",     MT65XX_LED_MODE_PMIC, MT65XX_LED_PMIC_LCD},
};

struct cust_mt65xx_led *get_cust_led_list(void)
{
	return cust_led_list;
}
