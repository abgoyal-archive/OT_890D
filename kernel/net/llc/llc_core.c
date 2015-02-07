

#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/if_ether.h>
#include <linux/netdevice.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/init.h>
#include <net/net_namespace.h>
#include <net/llc.h>

LIST_HEAD(llc_sap_list);
DEFINE_RWLOCK(llc_sap_list_lock);

static struct llc_sap *llc_sap_alloc(void)
{
	struct llc_sap *sap = kzalloc(sizeof(*sap), GFP_ATOMIC);

	if (sap) {
		/* sap->laddr.mac - leave as a null, it's filled by bind */
		sap->state = LLC_SAP_STATE_ACTIVE;
		rwlock_init(&sap->sk_list.lock);
		atomic_set(&sap->refcnt, 1);
	}
	return sap;
}

static void llc_add_sap(struct llc_sap *sap)
{
	list_add_tail(&sap->node, &llc_sap_list);
}

static void llc_del_sap(struct llc_sap *sap)
{
	write_lock_bh(&llc_sap_list_lock);
	list_del(&sap->node);
	write_unlock_bh(&llc_sap_list_lock);
}

static struct llc_sap *__llc_sap_find(unsigned char sap_value)
{
	struct llc_sap* sap;

	list_for_each_entry(sap, &llc_sap_list, node)
		if (sap->laddr.lsap == sap_value)
			goto out;
	sap = NULL;
out:
	return sap;
}

struct llc_sap *llc_sap_find(unsigned char sap_value)
{
	struct llc_sap* sap;

	read_lock_bh(&llc_sap_list_lock);
	sap = __llc_sap_find(sap_value);
	if (sap)
		llc_sap_hold(sap);
	read_unlock_bh(&llc_sap_list_lock);
	return sap;
}

struct llc_sap *llc_sap_open(unsigned char lsap,
			     int (*func)(struct sk_buff *skb,
					 struct net_device *dev,
					 struct packet_type *pt,
					 struct net_device *orig_dev))
{
	struct llc_sap *sap = NULL;

	write_lock_bh(&llc_sap_list_lock);
	if (__llc_sap_find(lsap)) /* SAP already exists */
		goto out;
	sap = llc_sap_alloc();
	if (!sap)
		goto out;
	sap->laddr.lsap = lsap;
	sap->rcv_func	= func;
	llc_add_sap(sap);
out:
	write_unlock_bh(&llc_sap_list_lock);
	return sap;
}

void llc_sap_close(struct llc_sap *sap)
{
	WARN_ON(!hlist_empty(&sap->sk_list.list));
	llc_del_sap(sap);
	kfree(sap);
}

static struct packet_type llc_packet_type = {
	.type = __constant_htons(ETH_P_802_2),
	.func = llc_rcv,
};

static struct packet_type llc_tr_packet_type = {
	.type = __constant_htons(ETH_P_TR_802_2),
	.func = llc_rcv,
};

static int __init llc_init(void)
{
	struct net_device *dev;

	dev = first_net_device(&init_net);
	if (dev != NULL)
		dev = next_net_device(dev);

	dev_add_pack(&llc_packet_type);
	dev_add_pack(&llc_tr_packet_type);
	return 0;
}

static void __exit llc_exit(void)
{
	dev_remove_pack(&llc_packet_type);
	dev_remove_pack(&llc_tr_packet_type);
}

module_init(llc_init);
module_exit(llc_exit);

EXPORT_SYMBOL(llc_sap_list);
EXPORT_SYMBOL(llc_sap_list_lock);
EXPORT_SYMBOL(llc_sap_find);
EXPORT_SYMBOL(llc_sap_open);
EXPORT_SYMBOL(llc_sap_close);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Procom 1997, Jay Schullist 2001, Arnaldo C. Melo 2001-2003");
MODULE_DESCRIPTION("LLC IEEE 802.2 core support");
