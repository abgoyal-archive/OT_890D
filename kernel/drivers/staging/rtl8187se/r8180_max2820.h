

#define MAXIM_ANTENNA 0xb3
#define MAXIM_ANAPARAM_PWR1_ON 0x8
#define MAXIM_ANAPARAM_PWR0_ON 0x0


void maxim_rf_init(struct net_device *dev);
void maxim_rf_set_chan(struct net_device *dev,short ch);

void maxim_rf_close(struct net_device *dev);
