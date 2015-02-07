
/* 3c501.c: A 3Com 3c501 Ethernet driver for Linux. */



#define DRV_NAME	"3c501"
#define DRV_VERSION	"2002/10/09"


static const char version[] =
	DRV_NAME ".c: " DRV_VERSION " Alan Cox (alan@lxorguk.ukuu.org.uk).\n";


#include <linux/module.h>

#include <linux/kernel.h>
#include <linux/fcntl.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/spinlock.h>
#include <linux/ethtool.h>
#include <linux/delay.h>
#include <linux/bitops.h>

#include <asm/uaccess.h>
#include <asm/io.h>

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/init.h>

#include "3c501.h"


static int io = 0x280;
static int irq = 5;
static int mem_start;


struct net_device * __init el1_probe(int unit)
{
	struct net_device *dev = alloc_etherdev(sizeof(struct net_local));
	static unsigned ports[] = { 0x280, 0x300, 0};
	unsigned *port;
	int err = 0;

	if (!dev)
		return ERR_PTR(-ENOMEM);

	if (unit >= 0) {
		sprintf(dev->name, "eth%d", unit);
		netdev_boot_setup_check(dev);
		io = dev->base_addr;
		irq = dev->irq;
		mem_start = dev->mem_start & 7;
	}

	if (io > 0x1ff) {	/* Check a single specified location. */
		err = el1_probe1(dev, io);
	} else if (io != 0) {
		err = -ENXIO;		/* Don't probe at all. */
	} else {
		for (port = ports; *port && el1_probe1(dev, *port); port++)
			;
		if (!*port)
			err = -ENODEV;
	}
	if (err)
		goto out;
	err = register_netdev(dev);
	if (err)
		goto out1;
	return dev;
out1:
	release_region(dev->base_addr, EL1_IO_EXTENT);
out:
	free_netdev(dev);
	return ERR_PTR(err);
}


static int __init el1_probe1(struct net_device *dev, int ioaddr)
{
	struct net_local *lp;
	const char *mname;		/* Vendor name */
	unsigned char station_addr[6];
	int autoirq = 0;
	int i;

	/*
	 *	Reserve I/O resource for exclusive use by this driver
	 */

	if (!request_region(ioaddr, EL1_IO_EXTENT, DRV_NAME))
		return -ENODEV;

	/*
	 *	Read the station address PROM data from the special port.
	 */

	for (i = 0; i < 6; i++) {
		outw(i, ioaddr + EL1_DATAPTR);
		station_addr[i] = inb(ioaddr + EL1_SAPROM);
	}
	/*
	 *	Check the first three octets of the S.A. for 3Com's prefix, or
	 *	for the Sager NP943 prefix.
	 */

	if (station_addr[0] == 0x02  &&  station_addr[1] == 0x60
						&& station_addr[2] == 0x8c)
		mname = "3c501";
	else if (station_addr[0] == 0x00  &&  station_addr[1] == 0x80
						&& station_addr[2] == 0xC8)
		mname = "NP943";
	else {
		release_region(ioaddr, EL1_IO_EXTENT);
		return -ENODEV;
	}

	/*
	 *	We auto-IRQ by shutting off the interrupt line and letting it
	 *	float high.
	 */

	dev->irq = irq;

	if (dev->irq < 2) {
		unsigned long irq_mask;

		irq_mask = probe_irq_on();
		inb(RX_STATUS);		/* Clear pending interrupts. */
		inb(TX_STATUS);
		outb(AX_LOOP + 1, AX_CMD);

		outb(0x00, AX_CMD);

		mdelay(20);
		autoirq = probe_irq_off(irq_mask);

		if (autoirq == 0) {
			printk(KERN_WARNING "%s probe at %#x failed to detect IRQ line.\n",
				mname, ioaddr);
			release_region(ioaddr, EL1_IO_EXTENT);
			return -EAGAIN;
		}
	}

	outb(AX_RESET+AX_LOOP, AX_CMD);			/* Loopback mode. */
	dev->base_addr = ioaddr;
	memcpy(dev->dev_addr, station_addr, ETH_ALEN);

	if (mem_start & 0xf)
		el_debug = mem_start & 0x7;
	if (autoirq)
		dev->irq = autoirq;

	printk(KERN_INFO "%s: %s EtherLink at %#lx, using %sIRQ %d.\n",
			dev->name, mname, dev->base_addr,
			autoirq ? "auto":"assigned ", dev->irq);

#ifdef CONFIG_IP_MULTICAST
	printk(KERN_WARNING "WARNING: Use of the 3c501 in a multicast kernel is NOT recommended.\n");
#endif

	if (el_debug)
		printk(KERN_DEBUG "%s", version);

	lp = netdev_priv(dev);
	memset(lp, 0, sizeof(struct net_local));
	spin_lock_init(&lp->lock);

	/*
	 *	The EL1-specific entries in the device structure.
	 */

	dev->open = &el_open;
	dev->hard_start_xmit = &el_start_xmit;
	dev->tx_timeout = &el_timeout;
	dev->watchdog_timeo = HZ;
	dev->stop = &el1_close;
	dev->set_multicast_list = &set_multicast_list;
	dev->ethtool_ops = &netdev_ethtool_ops;
	return 0;
}


static int el_open(struct net_device *dev)
{
	int retval;
	int ioaddr = dev->base_addr;
	struct net_local *lp = netdev_priv(dev);
	unsigned long flags;

	if (el_debug > 2)
		printk(KERN_DEBUG "%s: Doing el_open()...", dev->name);

	retval = request_irq(dev->irq, &el_interrupt, 0, dev->name, dev);
	if (retval)
		return retval;

	spin_lock_irqsave(&lp->lock, flags);
	el_reset(dev);
	spin_unlock_irqrestore(&lp->lock, flags);

	lp->txing = 0;		/* Board in RX mode */
	outb(AX_RX, AX_CMD);	/* Aux control, irq and receive enabled */
	netif_start_queue(dev);
	return 0;
}


static void el_timeout(struct net_device *dev)
{
	struct net_local *lp = netdev_priv(dev);
	int ioaddr = dev->base_addr;

	if (el_debug)
		printk(KERN_DEBUG "%s: transmit timed out, txsr %#2x axsr=%02x rxsr=%02x.\n",
			dev->name, inb(TX_STATUS),
			inb(AX_STATUS), inb(RX_STATUS));
	dev->stats.tx_errors++;
	outb(TX_NORM, TX_CMD);
	outb(RX_NORM, RX_CMD);
	outb(AX_OFF, AX_CMD);	/* Just trigger a false interrupt. */
	outb(AX_RX, AX_CMD);	/* Aux control, irq and receive enabled */
	lp->txing = 0;		/* Ripped back in to RX */
	netif_wake_queue(dev);
}



static int el_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct net_local *lp = netdev_priv(dev);
	int ioaddr = dev->base_addr;
	unsigned long flags;

	/*
	 *	Avoid incoming interrupts between us flipping txing and flipping
	 *	mode as the driver assumes txing is a faithful indicator of card
	 *	state
	 */

	spin_lock_irqsave(&lp->lock, flags);

	/*
	 *	Avoid timer-based retransmission conflicts.
	 */

	netif_stop_queue(dev);

	do {
		int len = skb->len;
		int pad = 0;
		int gp_start;
		unsigned char *buf = skb->data;

		if (len < ETH_ZLEN)
			pad = ETH_ZLEN - len;

		gp_start = 0x800 - (len + pad);

		lp->tx_pkt_start = gp_start;
		lp->collisions = 0;

		dev->stats.tx_bytes += skb->len;

		/*
		 *	Command mode with status cleared should [in theory]
		 *	mean no more interrupts can be pending on the card.
		 */

		outb_p(AX_SYS, AX_CMD);
		inb_p(RX_STATUS);
		inb_p(TX_STATUS);

		lp->loading = 1;
		lp->txing = 1;

		/*
		 *	Turn interrupts back on while we spend a pleasant
		 *	afternoon loading bytes into the board
		 */

		spin_unlock_irqrestore(&lp->lock, flags);

		/* Set rx packet area to 0. */
		outw(0x00, RX_BUF_CLR);
		/* aim - packet will be loaded into buffer start */
		outw(gp_start, GP_LOW);
		/* load buffer (usual thing each byte increments the pointer) */
		outsb(DATAPORT, buf, len);
		if (pad) {
			while (pad--)		/* Zero fill buffer tail */
				outb(0, DATAPORT);
		}
		/* the board reuses the same register */
		outw(gp_start, GP_LOW);

		if (lp->loading != 2) {
			/* fire ... Trigger xmit.  */
			outb(AX_XMIT, AX_CMD);
			lp->loading = 0;
			dev->trans_start = jiffies;
			if (el_debug > 2)
				printk(KERN_DEBUG " queued xmit.\n");
			dev_kfree_skb(skb);
			return 0;
		}
		/* A receive upset our load, despite our best efforts */
		if (el_debug > 2)
			printk(KERN_DEBUG "%s: burped during tx load.\n",
				dev->name);
		spin_lock_irqsave(&lp->lock, flags);
	} while (1);
}


static irqreturn_t el_interrupt(int irq, void *dev_id)
{
	struct net_device *dev = dev_id;
	struct net_local *lp;
	int ioaddr;
	int axsr;			/* Aux. status reg. */

	ioaddr = dev->base_addr;
	lp = netdev_priv(dev);

	spin_lock(&lp->lock);

	/*
	 *	What happened ?
	 */

	axsr = inb(AX_STATUS);

	/*
	 *	Log it
	 */

	if (el_debug > 3)
		printk(KERN_DEBUG "%s: el_interrupt() aux=%#02x",
							dev->name, axsr);

	if (lp->loading == 1 && !lp->txing)
		printk(KERN_WARNING "%s: Inconsistent state loading while not in tx\n",
			dev->name);

	if (lp->txing) {
		/*
		 *	Board in transmit mode. May be loading. If we are
		 *	loading we shouldn't have got this.
		 */
		int txsr = inb(TX_STATUS);

		if (lp->loading == 1) {
			if (el_debug > 2) {
				printk(KERN_DEBUG "%s: Interrupt while loading [",
					dev->name);
				printk(" txsr=%02x gp=%04x rp=%04x]\n",
					txsr, inw(GP_LOW), inw(RX_LOW));
			}
			/* Force a reload */
			lp->loading = 2;
			spin_unlock(&lp->lock);
			goto out;
		}
		if (el_debug > 6)
			printk(KERN_DEBUG " txsr=%02x gp=%04x rp=%04x",
					txsr, inw(GP_LOW), inw(RX_LOW));

		if ((axsr & 0x80) && (txsr & TX_READY) == 0) {
			/*
			 *	FIXME: is there a logic to whether to keep
			 *	on trying or reset immediately ?
			 */
			if (el_debug > 1)
				printk(KERN_DEBUG "%s: Unusual interrupt during Tx, txsr=%02x axsr=%02x gp=%03x rp=%03x.\n",
					dev->name, txsr, axsr,
					inw(ioaddr + EL1_DATAPTR),
					inw(ioaddr + EL1_RXPTR));
			lp->txing = 0;
			netif_wake_queue(dev);
		} else if (txsr & TX_16COLLISIONS) {
			/*
			 *	Timed out
			 */
			if (el_debug)
				printk(KERN_DEBUG "%s: Transmit failed 16 times, Ethernet jammed?\n", dev->name);
			outb(AX_SYS, AX_CMD);
			lp->txing = 0;
			dev->stats.tx_aborted_errors++;
			netif_wake_queue(dev);
		} else if (txsr & TX_COLLISION) {
			/*
			 *	Retrigger xmit.
			 */

			if (el_debug > 6)
				printk(KERN_DEBUG " retransmitting after a collision.\n");
			/*
			 *	Poor little chip can't reset its own start
			 *	pointer
			 */

			outb(AX_SYS, AX_CMD);
			outw(lp->tx_pkt_start, GP_LOW);
			outb(AX_XMIT, AX_CMD);
			dev->stats.collisions++;
			spin_unlock(&lp->lock);
			goto out;
		} else {
			/*
			 *	It worked.. we will now fall through and receive
			 */
			dev->stats.tx_packets++;
			if (el_debug > 6)
				printk(KERN_DEBUG " Tx succeeded %s\n",
					(txsr & TX_RDY) ? "." :
							"but tx is busy!");
			/*
			 *	This is safe the interrupt is atomic WRT itself.
			 */
			lp->txing = 0;
			/* In case more to transmit */
			netif_wake_queue(dev);
		}
	} else {
		/*
		 *	In receive mode.
		 */

		int rxsr = inb(RX_STATUS);
		if (el_debug > 5)
			printk(KERN_DEBUG " rxsr=%02x txsr=%02x rp=%04x", rxsr, inb(TX_STATUS), inw(RX_LOW));
		/*
		 *	Just reading rx_status fixes most errors.
		 */
		if (rxsr & RX_MISSED)
			dev->stats.rx_missed_errors++;
		else if (rxsr & RX_RUNT) {
			/* Handled to avoid board lock-up. */
			dev->stats.rx_length_errors++;
			if (el_debug > 5)
				printk(KERN_DEBUG " runt.\n");
		} else if (rxsr & RX_GOOD) {
			/*
			 *	Receive worked.
			 */
			el_receive(dev);
		} else {
			/*
			 *	Nothing?  Something is broken!
			 */
			if (el_debug > 2)
				printk(KERN_DEBUG "%s: No packet seen, rxsr=%02x **resetting 3c501***\n",
					dev->name, rxsr);
			el_reset(dev);
		}
		if (el_debug > 3)
			printk(KERN_DEBUG ".\n");
	}

	/*
	 *	Move into receive mode
	 */

	outb(AX_RX, AX_CMD);
	outw(0x00, RX_BUF_CLR);
	inb(RX_STATUS);		/* Be certain that interrupts are cleared. */
	inb(TX_STATUS);
	spin_unlock(&lp->lock);
out:
	return IRQ_HANDLED;
}



static void el_receive(struct net_device *dev)
{
	int ioaddr = dev->base_addr;
	int pkt_len;
	struct sk_buff *skb;

	pkt_len = inw(RX_LOW);

	if (el_debug > 4)
		printk(KERN_DEBUG " el_receive %d.\n", pkt_len);

	if (pkt_len < 60 || pkt_len > 1536) {
		if (el_debug)
			printk(KERN_DEBUG "%s: bogus packet, length=%d\n",
						dev->name, pkt_len);
		dev->stats.rx_over_errors++;
		return;
	}

	/*
	 *	Command mode so we can empty the buffer
	 */

	outb(AX_SYS, AX_CMD);
	skb = dev_alloc_skb(pkt_len+2);

	/*
	 *	Start of frame
	 */

	outw(0x00, GP_LOW);
	if (skb == NULL) {
		printk(KERN_INFO "%s: Memory squeeze, dropping packet.\n",
								dev->name);
		dev->stats.rx_dropped++;
		return;
	} else {
		skb_reserve(skb, 2);	/* Force 16 byte alignment */
		/*
		 *	The read increments through the bytes. The interrupt
		 *	handler will fix the pointer when it returns to
		 *	receive mode.
		 */
		insb(DATAPORT, skb_put(skb, pkt_len), pkt_len);
		skb->protocol = eth_type_trans(skb, dev);
		netif_rx(skb);
		dev->stats.rx_packets++;
		dev->stats.rx_bytes += pkt_len;
	}
	return;
}


static void  el_reset(struct net_device *dev)
{
	struct net_local *lp = netdev_priv(dev);
	int ioaddr = dev->base_addr;

	if (el_debug > 2)
		printk(KERN_INFO "3c501 reset...");
	outb(AX_RESET, AX_CMD);		/* Reset the chip */
	/* Aux control, irq and loopback enabled */
	outb(AX_LOOP, AX_CMD);
	{
		int i;
		for (i = 0; i < 6; i++)	/* Set the station address. */
			outb(dev->dev_addr[i], ioaddr + i);
	}

	outw(0, RX_BUF_CLR);		/* Set rx packet area to 0. */
	outb(TX_NORM, TX_CMD);		/* tx irq on done, collision */
	outb(RX_NORM, RX_CMD);		/* Set Rx commands. */
	inb(RX_STATUS);			/* Clear status. */
	inb(TX_STATUS);
	lp->txing = 0;
}


static int el1_close(struct net_device *dev)
{
	int ioaddr = dev->base_addr;

	if (el_debug > 2)
		printk(KERN_INFO "%s: Shutting down Ethernet card at %#x.\n",
						dev->name, ioaddr);

	netif_stop_queue(dev);

	/*
	 *	Free and disable the IRQ.
	 */

	free_irq(dev->irq, dev);
	outb(AX_RESET, AX_CMD);		/* Reset the chip */

	return 0;
}


static void set_multicast_list(struct net_device *dev)
{
	int ioaddr = dev->base_addr;

	if (dev->flags & IFF_PROMISC) {
		outb(RX_PROM, RX_CMD);
		inb(RX_STATUS);
	} else if (dev->mc_list || dev->flags & IFF_ALLMULTI) {
		/* Multicast or all multicast is the same */
		outb(RX_MULT, RX_CMD);
		inb(RX_STATUS);		/* Clear status. */
	} else {
		outb(RX_NORM, RX_CMD);
		inb(RX_STATUS);
	}
}


static void netdev_get_drvinfo(struct net_device *dev,
			       struct ethtool_drvinfo *info)
{
	strcpy(info->driver, DRV_NAME);
	strcpy(info->version, DRV_VERSION);
	sprintf(info->bus_info, "ISA 0x%lx", dev->base_addr);
}

static u32 netdev_get_msglevel(struct net_device *dev)
{
	return debug;
}

static void netdev_set_msglevel(struct net_device *dev, u32 level)
{
	debug = level;
}

static const struct ethtool_ops netdev_ethtool_ops = {
	.get_drvinfo		= netdev_get_drvinfo,
	.get_msglevel		= netdev_get_msglevel,
	.set_msglevel		= netdev_set_msglevel,
};

#ifdef MODULE

static struct net_device *dev_3c501;

module_param(io, int, 0);
module_param(irq, int, 0);
MODULE_PARM_DESC(io, "EtherLink I/O base address");
MODULE_PARM_DESC(irq, "EtherLink IRQ number");


int __init init_module(void)
{
	dev_3c501 = el1_probe(-1);
	if (IS_ERR(dev_3c501))
		return PTR_ERR(dev_3c501);
	return 0;
}


void __exit cleanup_module(void)
{
	struct net_device *dev = dev_3c501;
	unregister_netdev(dev);
	release_region(dev->base_addr, EL1_IO_EXTENT);
	free_netdev(dev);
}

#endif /* MODULE */

MODULE_AUTHOR("Donald Becker, Alan Cox");
MODULE_DESCRIPTION("Support for the ancient 3Com 3c501 ethernet card");
MODULE_LICENSE("GPL");

