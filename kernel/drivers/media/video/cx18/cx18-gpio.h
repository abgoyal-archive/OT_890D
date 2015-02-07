

void cx18_gpio_init(struct cx18 *cx);
void cx18_reset_i2c_slaves_gpio(struct cx18 *cx);
void cx18_reset_ir_gpio(void *data);
int cx18_reset_tuner_gpio(void *dev, int component, int cmd, int value);
int cx18_gpio(struct cx18 *cx, unsigned int command, void *arg);
