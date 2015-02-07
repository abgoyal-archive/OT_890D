

#define GCT_ANTENNA 0xA3


// we use the untouched eeprom value- cross your finger ;-)
#define GCT_ANAPARAM_PWR1_ON ??
#define GCT_ANAPARAM_PWR0_ON ??



void gct_rf_init(struct net_device *dev);
void gct_rf_set_chan(struct net_device *dev,short ch);

void gct_rf_close(struct net_device *dev);
