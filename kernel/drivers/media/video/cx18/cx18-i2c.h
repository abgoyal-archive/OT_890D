

int cx18_i2c_hw_addr(struct cx18 *cx, u32 hw);
int cx18_i2c_hw(struct cx18 *cx, u32 hw, unsigned int cmd, void *arg);
int cx18_call_i2c_client(struct cx18 *cx, int addr, unsigned cmd, void *arg);
void cx18_call_i2c_clients(struct cx18 *cx, unsigned int cmd, void *arg);
int cx18_i2c_register(struct cx18 *cx, unsigned idx);

/* init + register i2c algo-bit adapter */
int init_cx18_i2c(struct cx18 *cx);
void exit_cx18_i2c(struct cx18 *cx);
