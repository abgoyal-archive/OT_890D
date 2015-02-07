

#include <linux/if_arp.h>
#include <linux/if_tr.h>
#include <linux/netdevice.h>
#include <linux/trdevice.h>
#include <linux/skbuff.h>
#include <net/llc.h>
#include <net/llc_pdu.h>

int llc_mac_hdr_init(struct sk_buff *skb,
		     const unsigned char *sa, const unsigned char *da)
{
	int rc = 0;

	switch (skb->dev->type) {
#ifdef CONFIG_TR
	case ARPHRD_IEEE802_TR: {
		struct net_device *dev = skb->dev;
		struct trh_hdr *trh;

		skb_push(skb, sizeof(*trh));
		skb_reset_mac_header(skb);
		trh = tr_hdr(skb);
		trh->ac = AC;
		trh->fc = LLC_FRAME;
		if (sa)
			memcpy(trh->saddr, sa, dev->addr_len);
		else
			memset(trh->saddr, 0, dev->addr_len);
		if (da) {
			memcpy(trh->daddr, da, dev->addr_len);
			tr_source_route(skb, trh, dev);
			skb_reset_mac_header(skb);
		}
		break;
	}
#endif
	case ARPHRD_ETHER:
	case ARPHRD_LOOPBACK: {
		unsigned short len = skb->len;
		struct ethhdr *eth;

		skb_push(skb, sizeof(*eth));
		skb_reset_mac_header(skb);
		eth = eth_hdr(skb);
		eth->h_proto = htons(len);
		memcpy(eth->h_dest, da, ETH_ALEN);
		memcpy(eth->h_source, sa, ETH_ALEN);
		break;
	}
	default:
		printk(KERN_WARNING "device type not supported: %d\n",
		       skb->dev->type);
		rc = -EINVAL;
	}
	return rc;
}

int llc_build_and_send_ui_pkt(struct llc_sap *sap, struct sk_buff *skb,
			      unsigned char *dmac, unsigned char dsap)
{
	int rc;
	llc_pdu_header_init(skb, LLC_PDU_TYPE_U, sap->laddr.lsap,
			    dsap, LLC_PDU_CMD);
	llc_pdu_init_as_ui_cmd(skb);
	rc = llc_mac_hdr_init(skb, skb->dev->dev_addr, dmac);
	if (likely(!rc))
		rc = dev_queue_xmit(skb);
	return rc;
}

EXPORT_SYMBOL(llc_mac_hdr_init);
EXPORT_SYMBOL(llc_build_and_send_ui_pkt);
