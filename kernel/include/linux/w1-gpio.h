
#ifndef _LINUX_W1_GPIO_H
#define _LINUX_W1_GPIO_H

struct w1_gpio_platform_data {
	unsigned int pin;
	unsigned int is_open_drain:1;
};

#endif /* _LINUX_W1_GPIO_H */
