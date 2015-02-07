

#ifndef PN_DEV_H
#define PN_DEV_H

struct phonet_device_list {
	struct list_head list;
	spinlock_t lock;
};

extern struct phonet_device_list pndevs;

struct phonet_device {
	struct list_head list;
	struct net_device *netdev;
	DECLARE_BITMAP(addrs, 64);
};

void phonet_device_init(void);
void phonet_device_exit(void);
struct net_device *phonet_device_get(struct net *net);

int phonet_address_add(struct net_device *dev, u8 addr);
int phonet_address_del(struct net_device *dev, u8 addr);
u8 phonet_address_get(struct net_device *dev, u8 addr);
int phonet_address_lookup(struct net *net, u8 addr);

#define PN_NO_ADDR	0xff

#endif
