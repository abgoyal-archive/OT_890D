

#include <linux/module.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <linux/bitops.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/socket.h>
#include <linux/sockios.h>
#include <linux/in.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/if_ether.h>
#include <linux/inet.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/init.h>
#include <net/net_namespace.h>
#include <net/ip.h>
#include <net/route.h>
#include <linux/skbuff.h>
#include <net/sock.h>
#include <net/arp.h>




int dev_mc_delete(struct net_device *dev, void *addr, int alen, int glbl)
{
	int err;

	netif_addr_lock_bh(dev);
	err = __dev_addr_delete(&dev->mc_list, &dev->mc_count,
				addr, alen, glbl);
	if (!err) {
		/*
		 *	We have altered the list, so the card
		 *	loaded filter is now wrong. Fix it
		 */

		__dev_set_rx_mode(dev);
	}
	netif_addr_unlock_bh(dev);
	return err;
}


int dev_mc_add(struct net_device *dev, void *addr, int alen, int glbl)
{
	int err;

	netif_addr_lock_bh(dev);
	err = __dev_addr_add(&dev->mc_list, &dev->mc_count, addr, alen, glbl);
	if (!err)
		__dev_set_rx_mode(dev);
	netif_addr_unlock_bh(dev);
	return err;
}

int dev_mc_sync(struct net_device *to, struct net_device *from)
{
	int err = 0;

	netif_addr_lock_bh(to);
	err = __dev_addr_sync(&to->mc_list, &to->mc_count,
			      &from->mc_list, &from->mc_count);
	if (!err)
		__dev_set_rx_mode(to);
	netif_addr_unlock_bh(to);

	return err;
}
EXPORT_SYMBOL(dev_mc_sync);


void dev_mc_unsync(struct net_device *to, struct net_device *from)
{
	netif_addr_lock_bh(from);
	netif_addr_lock(to);

	__dev_addr_unsync(&to->mc_list, &to->mc_count,
			  &from->mc_list, &from->mc_count);
	__dev_set_rx_mode(to);

	netif_addr_unlock(to);
	netif_addr_unlock_bh(from);
}
EXPORT_SYMBOL(dev_mc_unsync);

#ifdef CONFIG_PROC_FS
static int dev_mc_seq_show(struct seq_file *seq, void *v)
{
	struct dev_addr_list *m;
	struct net_device *dev = v;

	if (v == SEQ_START_TOKEN)
		return 0;

	netif_addr_lock_bh(dev);
	for (m = dev->mc_list; m; m = m->next) {
		int i;

		seq_printf(seq, "%-4d %-15s %-5d %-5d ", dev->ifindex,
			   dev->name, m->dmi_users, m->dmi_gusers);

		for (i = 0; i < m->dmi_addrlen; i++)
			seq_printf(seq, "%02x", m->dmi_addr[i]);

		seq_putc(seq, '\n');
	}
	netif_addr_unlock_bh(dev);
	return 0;
}

static const struct seq_operations dev_mc_seq_ops = {
	.start = dev_seq_start,
	.next  = dev_seq_next,
	.stop  = dev_seq_stop,
	.show  = dev_mc_seq_show,
};

static int dev_mc_seq_open(struct inode *inode, struct file *file)
{
	return seq_open_net(inode, file, &dev_mc_seq_ops,
			    sizeof(struct seq_net_private));
}

static const struct file_operations dev_mc_seq_fops = {
	.owner	 = THIS_MODULE,
	.open    = dev_mc_seq_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = seq_release_net,
};

#endif

static int __net_init dev_mc_net_init(struct net *net)
{
	if (!proc_net_fops_create(net, "dev_mcast", 0, &dev_mc_seq_fops))
		return -ENOMEM;
	return 0;
}

static void __net_exit dev_mc_net_exit(struct net *net)
{
	proc_net_remove(net, "dev_mcast");
}

static struct pernet_operations __net_initdata dev_mc_net_ops = {
	.init = dev_mc_net_init,
	.exit = dev_mc_net_exit,
};

void __init dev_mcast_init(void)
{
	register_pernet_subsys(&dev_mc_net_ops);
}

EXPORT_SYMBOL(dev_mc_add);
EXPORT_SYMBOL(dev_mc_delete);
