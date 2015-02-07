

#ifndef __PCSP_INPUT_H__
#define __PCSP_INPUT_H__

int __devinit pcspkr_input_init(struct input_dev **rdev, struct device *dev);
int pcspkr_input_remove(struct input_dev *dev);
void pcspkr_stop_sound(void);

#endif