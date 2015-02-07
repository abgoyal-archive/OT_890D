
#ifndef __NEC_VR41XX_GIU_H
#define __NEC_VR41XX_GIU_H

enum {
	GPIO_50PINS_PULLUPDOWN,
	GPIO_36PINS,
	GPIO_48PINS_EDGE_SELECT,
};

typedef enum {
	IRQ_TRIGGER_LEVEL,
	IRQ_TRIGGER_EDGE,
	IRQ_TRIGGER_EDGE_FALLING,
	IRQ_TRIGGER_EDGE_RISING,
} irq_trigger_t;

typedef enum {
	IRQ_SIGNAL_THROUGH,
	IRQ_SIGNAL_HOLD,
} irq_signal_t;

extern void vr41xx_set_irq_trigger(unsigned int pin, irq_trigger_t trigger, irq_signal_t signal);

typedef enum {
	IRQ_LEVEL_LOW,
	IRQ_LEVEL_HIGH,
} irq_level_t;

extern void vr41xx_set_irq_level(unsigned int pin, irq_level_t level);

typedef enum {
	GPIO_DATA_LOW,
	GPIO_DATA_HIGH,
	GPIO_DATA_INVAL,
} gpio_data_t;

extern gpio_data_t vr41xx_gpio_get_pin(unsigned int pin);
extern int vr41xx_gpio_set_pin(unsigned int pin, gpio_data_t data);

typedef enum {
	GPIO_INPUT,
	GPIO_OUTPUT,
	GPIO_OUTPUT_DISABLE,
} gpio_direction_t;

extern int vr41xx_gpio_set_direction(unsigned int pin, gpio_direction_t dir);

typedef enum {
	GPIO_PULL_DOWN,
	GPIO_PULL_UP,
	GPIO_PULL_DISABLE,
} gpio_pull_t;

extern int vr41xx_gpio_pullupdown(unsigned int pin, gpio_pull_t pull);

#endif /* __NEC_VR41XX_GIU_H */
