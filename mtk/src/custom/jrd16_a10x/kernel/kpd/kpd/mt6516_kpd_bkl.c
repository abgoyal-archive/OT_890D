

#include <mach/mt6516_typedefs.h>
#include <mt6516_kpd.h>

extern void pmic_kp_dim_div(kal_uint8 val);
extern void pmic_kp_dim_duty(kal_uint8 duty);
extern ssize_t mt6326_kpled_Enable(void);
extern ssize_t mt6326_kpled_Disable(void);
extern ssize_t mt6326_kpled_dim_duty_Full(void);
extern ssize_t mt6326_kpled_dim_duty_0(void);

#if KPD_DRV_CTRL_BACKLIGHT
void kpd_enable_backlight(void)
{
	mt6326_kpled_dim_duty_Full();
	mt6326_kpled_Enable();
}

void kpd_disable_backlight(void)
{
	mt6326_kpled_dim_duty_0();
	mt6326_kpled_Disable();
}
#endif

/* for META tool */
void kpd_set_backlight(bool onoff, void *val1, void *val2)
{
	u8 div = *(u8 *)val1;
	u8 duty = *(u8 *)val2;

	if (div > 15)
		div = 15;
	pmic_kp_dim_div(div);

	if (duty > 31)
		duty = 31;
	pmic_kp_dim_duty(duty);

	if (onoff)
		mt6326_kpled_Enable();
	else
		mt6326_kpled_Disable();
}
