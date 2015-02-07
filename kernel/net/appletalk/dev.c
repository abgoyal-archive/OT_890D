

#include <linux/errno.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/if_arp.h>
#include <linux/if_ltalk.h>

static int ltalk_change_mtu(struct net_device *dev, int mtu)
{
	return -EINVAL;
}

static int ltalk_mac_addr(struct net_device *dev, void *addr)
{
	return -EINVAL;
}

static void ltalk_setup(struct net_device *dev)
{
	/* Fill in the fields of the device structure with localtalk-generic values. */

	dev->change_mtu		= ltalk_change_mtu;
	dev->set_mac_address 	= ltalk_mac_addr;

	dev->type		= ARPHRD_LOCALTLK;
	dev->hard_header_len 	= LTALK_HLEN;
	dev->mtu		= LTALK_MTU;
	dev->addr_len		= LTALK_ALEN;
	dev->tx_queue_len	= 10;

	dev->broadcast[0]	= 0xFF;

	dev->flags		= IFF_BROADCAST|IFF_MULTICAST|IFF_NOARP;
}


struct net_device *alloc_ltalkdev(int sizeof_priv)
{
	return alloc_netdev(sizeof_priv, "lt%d", ltalk_setup);
}
EXPORT_SYMBOL(alloc_ltalkdev);
