

#ifndef __PLAT_GPIO_H
#define __PLAT_GPIO_H

int gpio_request(unsigned pin, const char *label);
void gpio_free(unsigned pin);
int gpio_direction_input(unsigned pin);
int gpio_direction_output(unsigned pin, int value);
int gpio_get_value(unsigned pin);
void gpio_set_value(unsigned pin, int value);

void orion_gpio_set_unused(unsigned pin);
void orion_gpio_set_valid(unsigned pin, int valid);
void orion_gpio_set_blink(unsigned pin, int blink);

extern struct irq_chip orion_gpio_irq_chip;
void orion_gpio_irq_handler(int irqoff);


#endif
