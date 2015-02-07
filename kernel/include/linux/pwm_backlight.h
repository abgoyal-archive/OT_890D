
#ifndef __LINUX_PWM_BACKLIGHT_H
#define __LINUX_PWM_BACKLIGHT_H

struct platform_pwm_backlight_data {
	int pwm_id;
	unsigned int max_brightness;
	unsigned int dft_brightness;
	unsigned int pwm_period_ns;
	int (*init)(struct device *dev);
	int (*notify)(int brightness);
	void (*exit)(struct device *dev);
};

#endif
