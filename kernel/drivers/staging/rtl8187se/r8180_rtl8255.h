

#define RTL8255_ANAPARAM_ON 0xa0000b59
#define RTL8255_ANAPARAM2_ON 0x840cf311


void rtl8255_rf_init(struct net_device *dev);
void rtl8255_rf_set_chan(struct net_device *dev,short ch);
void rtl8255_rf_close(struct net_device *dev);
