
/* atp.c: Attached (pocket) ethernet adapter driver for linux. */

static const char version[] =
"atp.c:v1.09=ac 2002/10/01 Donald Becker <becker@scyld.com>\n";


static int debug = 1; 			/* 1 normal messages, 0 quiet .. 7 verbose. */
#define net_debug debug

/* Maximum events (Rx packets, etc.) to handle at each interrupt. */
static int max_interrupt_work = 15;

#define NUM_UNITS 2
/* The standard set of ISA module parameters. */
static int io[NUM_UNITS];
static int irq[NUM_UNITS];
static int xcvr[NUM_UNITS]; 			/* The data transfer mode. */

/* Operational parameters that are set at compile time. */

/* Time in jiffies before concluding the transmitter is hung. */
#define TX_TIMEOUT  (400*HZ/1000)


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/in.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/crc32.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/bitops.h>

#include <asm/system.h>
#include <asm/io.h>
#include <asm/dma.h>

#include "atp.h"

MODULE_AUTHOR("Donald Becker <becker@scyld.com>");
MODULE_DESCRIPTION("RealTek RTL8002/8012 parallel port Ethernet driver");
MODULE_LICENSE("GPL");

module_param(max_interrupt_work, int, 0);
module_param(debug, int, 0);
module_param_array(io, int, NULL, 0);
module_param_array(irq, int, NULL, 0);
module_param_array(xcvr, int, NULL, 0);
MODULE_PARM_DESC(max_interrupt_work, "ATP maximum events handled per interrupt");
MODULE_PARM_DESC(debug, "ATP debug level (0-7)");
MODULE_PARM_DESC(io, "ATP I/O base address(es)");
MODULE_PARM_DESC(irq, "ATP IRQ number(s)");
MODULE_PARM_DESC(xcvr, "ATP transceiver(s) (0=internal, 1=external)");

/* The number of low I/O ports used by the ethercard. */
#define ETHERCARD_TOTAL_SIZE	3

/* Sequence to switch an 8012 from printer mux to ethernet mode. */
static char mux_8012[] = { 0xff, 0xf7, 0xff, 0xfb, 0xf3, 0xfb, 0xff, 0xf7,};

struct net_local {
    spinlock_t lock;
    struct net_device *next_module;
    struct timer_list timer;	/* Media selection timer. */
    long last_rx_time;		/* Last Rx, in jiffies, to handle Rx hang. */
    int saved_tx_size;
    unsigned int tx_unit_busy:1;
    unsigned char re_tx,	/* Number of packet retransmissions. */
		addr_mode,		/* Current Rx filter e.g. promiscuous, etc. */
		pac_cnt_in_tx_buf,
		chip_type;
};

#define TIMED_CHECKER (HZ/4)
#ifdef TIMED_CHECKER
#include <linux/timer.h>
static void atp_timed_checker(unsigned long ignored);
#endif

/* Index to functions, as function prototypes. */

static int atp_probe1(long ioaddr);
static void get_node_ID(struct net_device *dev);
static unsigned short eeprom_op(long ioaddr, unsigned int cmd);
static int net_open(struct net_device *dev);
static void hardware_init(struct net_device *dev);
static void write_packet(long ioaddr, int length, unsigned char *packet, int pad, int mode);
static void trigger_send(long ioaddr, int length);
static int	atp_send_packet(struct sk_buff *skb, struct net_device *dev);
static irqreturn_t atp_interrupt(int irq, void *dev_id);
static void net_rx(struct net_device *dev);
static void read_block(long ioaddr, int length, unsigned char *buffer, int data_mode);
static int net_close(struct net_device *dev);
static void set_rx_mode(struct net_device *dev);
static void tx_timeout(struct net_device *dev);


/* A list of all installed ATP devices, for removing the driver module. */
static struct net_device *root_atp_dev;

static int __init atp_init(void)
{
	int *port, ports[] = {0x378, 0x278, 0x3bc, 0};
	int base_addr = io[0];

	if (base_addr > 0x1ff)		/* Check a single specified location. */
		return atp_probe1(base_addr);
	else if (base_addr == 1)	/* Don't probe at all. */
		return -ENXIO;

	for (port = ports; *port; port++) {
		long ioaddr = *port;
		outb(0x57, ioaddr + PAR_DATA);
		if (inb(ioaddr + PAR_DATA) != 0x57)
			continue;
		if (atp_probe1(ioaddr) == 0)
			return 0;
	}

	return -ENODEV;
}

static const struct net_device_ops atp_netdev_ops = {
	.ndo_open		= net_open,
	.ndo_stop		= net_close,
	.ndo_start_xmit		= atp_send_packet,
	.ndo_set_multicast_list = set_rx_mode,
	.ndo_tx_timeout		= tx_timeout,
	.ndo_change_mtu		= eth_change_mtu,
	.ndo_set_mac_address 	= eth_mac_addr,
	.ndo_validate_addr	= eth_validate_addr,
};

static int __init atp_probe1(long ioaddr)
{
	struct net_device *dev = NULL;
	struct net_local *lp;
	int saved_ctrl_reg, status, i;
	int res;

	outb(0xff, ioaddr + PAR_DATA);
	/* Save the original value of the Control register, in case we guessed
	   wrong. */
	saved_ctrl_reg = inb(ioaddr + PAR_CONTROL);
	if (net_debug > 3)
		printk("atp: Control register was %#2.2x.\n", saved_ctrl_reg);
	/* IRQEN=0, SLCTB=high INITB=high, AUTOFDB=high, STBB=high. */
	outb(0x04, ioaddr + PAR_CONTROL);
#ifndef final_version
	if (net_debug > 3) {
		/* Turn off the printer multiplexer on the 8012. */
		for (i = 0; i < 8; i++)
			outb(mux_8012[i], ioaddr + PAR_DATA);
		write_reg(ioaddr, MODSEL, 0x00);
		printk("atp: Registers are ");
		for (i = 0; i < 32; i++)
			printk(" %2.2x", read_nibble(ioaddr, i));
		printk(".\n");
	}
#endif
	/* Turn off the printer multiplexer on the 8012. */
	for (i = 0; i < 8; i++)
		outb(mux_8012[i], ioaddr + PAR_DATA);
	write_reg_high(ioaddr, CMR1, CMR1h_RESET);
	/* udelay() here? */
	status = read_nibble(ioaddr, CMR1);

	if (net_debug > 3) {
		printk(KERN_DEBUG "atp: Status nibble was %#2.2x..", status);
		for (i = 0; i < 32; i++)
			printk(" %2.2x", read_nibble(ioaddr, i));
		printk("\n");
	}

	if ((status & 0x78) != 0x08) {
		/* The pocket adapter probe failed, restore the control register. */
		outb(saved_ctrl_reg, ioaddr + PAR_CONTROL);
		return -ENODEV;
	}
	status = read_nibble(ioaddr, CMR2_h);
	if ((status & 0x78) != 0x10) {
		outb(saved_ctrl_reg, ioaddr + PAR_CONTROL);
		return -ENODEV;
	}

	dev = alloc_etherdev(sizeof(struct net_local));
	if (!dev)
		return -ENOMEM;

	/* Find the IRQ used by triggering an interrupt. */
	write_reg_byte(ioaddr, CMR2, 0x01);			/* No accept mode, IRQ out. */
	write_reg_high(ioaddr, CMR1, CMR1h_RxENABLE | CMR1h_TxENABLE);	/* Enable Tx and Rx. */

	/* Omit autoIRQ routine for now. Use "table lookup" instead.  Uhgggh. */
	if (irq[0])
		dev->irq = irq[0];
	else if (ioaddr == 0x378)
		dev->irq = 7;
	else
		dev->irq = 5;
	write_reg_high(ioaddr, CMR1, CMR1h_TxRxOFF); /* Disable Tx and Rx units. */
	write_reg(ioaddr, CMR2, CMR2_NULL);

	dev->base_addr = ioaddr;

	/* Read the station address PROM.  */
	get_node_ID(dev);

#ifndef MODULE
	if (net_debug)
		printk(KERN_INFO "%s", version);
#endif

	printk(KERN_NOTICE "%s: Pocket adapter found at %#3lx, IRQ %d, "
	       "SAPROM %pM.\n",
	       dev->name, dev->base_addr, dev->irq, dev->dev_addr);

	/* Reset the ethernet hardware and activate the printer pass-through. */
	write_reg_high(ioaddr, CMR1, CMR1h_RESET | CMR1h_MUX);

	lp = netdev_priv(dev);
	lp->chip_type = RTL8002;
	lp->addr_mode = CMR2h_Normal;
	spin_lock_init(&lp->lock);

	/* For the ATP adapter the "if_port" is really the data transfer mode. */
	if (xcvr[0])
		dev->if_port = xcvr[0];
	else
		dev->if_port = (dev->mem_start & 0xf) ? (dev->mem_start & 0x7) : 4;
	if (dev->mem_end & 0xf)
		net_debug = dev->mem_end & 7;

	dev->netdev_ops 	= &atp_netdev_ops;
	dev->watchdog_timeo	= TX_TIMEOUT;

	res = register_netdev(dev);
	if (res) {
		free_netdev(dev);
		return res;
	}

	lp->next_module = root_atp_dev;
	root_atp_dev = dev;

	return 0;
}

/* Read the station address PROM, usually a word-wide EEPROM. */
static void __init get_node_ID(struct net_device *dev)
{
	long ioaddr = dev->base_addr;
	int sa_offset = 0;
	int i;

	write_reg(ioaddr, CMR2, CMR2_EEPROM);	  /* Point to the EEPROM control registers. */

	/* Some adapters have the station address at offset 15 instead of offset
	   zero.  Check for it, and fix it if needed. */
	if (eeprom_op(ioaddr, EE_READ(0)) == 0xffff)
		sa_offset = 15;

	for (i = 0; i < 3; i++)
		((__be16 *)dev->dev_addr)[i] =
			cpu_to_be16(eeprom_op(ioaddr, EE_READ(sa_offset + i)));

	write_reg(ioaddr, CMR2, CMR2_NULL);
}


static unsigned short __init eeprom_op(long ioaddr, u32 cmd)
{
	unsigned eedata_out = 0;
	int num_bits = EE_CMD_SIZE;

	while (--num_bits >= 0) {
		char outval = (cmd & (1<<num_bits)) ? EE_DATA_WRITE : 0;
		write_reg_high(ioaddr, PROM_CMD, outval | EE_CLK_LOW);
		write_reg_high(ioaddr, PROM_CMD, outval | EE_CLK_HIGH);
		eedata_out <<= 1;
		if (read_nibble(ioaddr, PROM_DATA) & EE_DATA_READ)
			eedata_out++;
	}
	write_reg_high(ioaddr, PROM_CMD, EE_CLK_LOW & ~EE_CS);
	return eedata_out;
}


static int net_open(struct net_device *dev)
{
	struct net_local *lp = netdev_priv(dev);
	int ret;

	/* The interrupt line is turned off (tri-stated) when the device isn't in
	   use.  That's especially important for "attached" interfaces where the
	   port or interrupt may be shared. */
	ret = request_irq(dev->irq, &atp_interrupt, 0, dev->name, dev);
	if (ret)
		return ret;

	hardware_init(dev);

	init_timer(&lp->timer);
	lp->timer.expires = jiffies + TIMED_CHECKER;
	lp->timer.data = (unsigned long)dev;
	lp->timer.function = &atp_timed_checker;    /* timer handler */
	add_timer(&lp->timer);

	netif_start_queue(dev);
	return 0;
}

static void hardware_init(struct net_device *dev)
{
	struct net_local *lp = netdev_priv(dev);
	long ioaddr = dev->base_addr;
    int i;

	/* Turn off the printer multiplexer on the 8012. */
	for (i = 0; i < 8; i++)
		outb(mux_8012[i], ioaddr + PAR_DATA);
	write_reg_high(ioaddr, CMR1, CMR1h_RESET);

    for (i = 0; i < 6; i++)
		write_reg_byte(ioaddr, PAR0 + i, dev->dev_addr[i]);

	write_reg_high(ioaddr, CMR2, lp->addr_mode);

	if (net_debug > 2) {
		printk(KERN_DEBUG "%s: Reset: current Rx mode %d.\n", dev->name,
			   (read_nibble(ioaddr, CMR2_h) >> 3) & 0x0f);
	}

    write_reg(ioaddr, CMR2, CMR2_IRQOUT);
    write_reg_high(ioaddr, CMR1, CMR1h_RxENABLE | CMR1h_TxENABLE);

	/* Enable the interrupt line from the serial port. */
	outb(Ctrl_SelData + Ctrl_IRQEN, ioaddr + PAR_CONTROL);

	/* Unmask the interesting interrupts. */
    write_reg(ioaddr, IMR, ISR_RxOK | ISR_TxErr | ISR_TxOK);
    write_reg_high(ioaddr, IMR, ISRh_RxErr);

	lp->tx_unit_busy = 0;
    lp->pac_cnt_in_tx_buf = 0;
	lp->saved_tx_size = 0;
}

static void trigger_send(long ioaddr, int length)
{
	write_reg_byte(ioaddr, TxCNT0, length & 0xff);
	write_reg(ioaddr, TxCNT1, length >> 8);
	write_reg(ioaddr, CMR1, CMR1_Xmit);
}

static void write_packet(long ioaddr, int length, unsigned char *packet, int pad_len, int data_mode)
{
    if (length & 1)
    {
    	length++;
    	pad_len++;
    }

    outb(EOC+MAR, ioaddr + PAR_DATA);
    if ((data_mode & 1) == 0) {
		/* Write the packet out, starting with the write addr. */
		outb(WrAddr+MAR, ioaddr + PAR_DATA);
		do {
			write_byte_mode0(ioaddr, *packet++);
		} while (--length > pad_len) ;
		do {
			write_byte_mode0(ioaddr, 0);
		} while (--length > 0) ;
    } else {
		/* Write the packet out in slow mode. */
		unsigned char outbyte = *packet++;

		outb(Ctrl_LNibWrite + Ctrl_IRQEN, ioaddr + PAR_CONTROL);
		outb(WrAddr+MAR, ioaddr + PAR_DATA);

		outb((outbyte & 0x0f)|0x40, ioaddr + PAR_DATA);
		outb(outbyte & 0x0f, ioaddr + PAR_DATA);
		outbyte >>= 4;
		outb(outbyte & 0x0f, ioaddr + PAR_DATA);
		outb(Ctrl_HNibWrite + Ctrl_IRQEN, ioaddr + PAR_CONTROL);
		while (--length > pad_len)
			write_byte_mode1(ioaddr, *packet++);
		while (--length > 0)
			write_byte_mode1(ioaddr, 0);
    }
    /* Terminate the Tx frame.  End of write: ECB. */
    outb(0xff, ioaddr + PAR_DATA);
    outb(Ctrl_HNibWrite | Ctrl_SelData | Ctrl_IRQEN, ioaddr + PAR_CONTROL);
}

static void tx_timeout(struct net_device *dev)
{
	long ioaddr = dev->base_addr;

	printk(KERN_WARNING "%s: Transmit timed out, %s?\n", dev->name,
		   inb(ioaddr + PAR_CONTROL) & 0x10 ? "network cable problem"
		   :  "IRQ conflict");
	dev->stats.tx_errors++;
	/* Try to restart the adapter. */
	hardware_init(dev);
	dev->trans_start = jiffies;
	netif_wake_queue(dev);
	dev->stats.tx_errors++;
}

static int atp_send_packet(struct sk_buff *skb, struct net_device *dev)
{
	struct net_local *lp = netdev_priv(dev);
	long ioaddr = dev->base_addr;
	int length;
	unsigned long flags;

	length = ETH_ZLEN < skb->len ? skb->len : ETH_ZLEN;

	netif_stop_queue(dev);

	/* Disable interrupts by writing 0x00 to the Interrupt Mask Register.
	   This sequence must not be interrupted by an incoming packet. */

	spin_lock_irqsave(&lp->lock, flags);
	write_reg(ioaddr, IMR, 0);
	write_reg_high(ioaddr, IMR, 0);
	spin_unlock_irqrestore(&lp->lock, flags);

	write_packet(ioaddr, length, skb->data, length-skb->len, dev->if_port);

	lp->pac_cnt_in_tx_buf++;
	if (lp->tx_unit_busy == 0) {
		trigger_send(ioaddr, length);
		lp->saved_tx_size = 0; 				/* Redundant */
		lp->re_tx = 0;
		lp->tx_unit_busy = 1;
	} else
		lp->saved_tx_size = length;
	/* Re-enable the LPT interrupts. */
	write_reg(ioaddr, IMR, ISR_RxOK | ISR_TxErr | ISR_TxOK);
	write_reg_high(ioaddr, IMR, ISRh_RxErr);

	dev->trans_start = jiffies;
	dev_kfree_skb (skb);
	return 0;
}


static irqreturn_t atp_interrupt(int irq, void *dev_instance)
{
	struct net_device *dev = dev_instance;
	struct net_local *lp;
	long ioaddr;
	static int num_tx_since_rx;
	int boguscount = max_interrupt_work;
	int handled = 0;

	ioaddr = dev->base_addr;
	lp = netdev_priv(dev);

	spin_lock(&lp->lock);

	/* Disable additional spurious interrupts. */
	outb(Ctrl_SelData, ioaddr + PAR_CONTROL);

	/* The adapter's output is currently the IRQ line, switch it to data. */
	write_reg(ioaddr, CMR2, CMR2_NULL);
	write_reg(ioaddr, IMR, 0);

	if (net_debug > 5) printk(KERN_DEBUG "%s: In interrupt ", dev->name);
    while (--boguscount > 0) {
		int status = read_nibble(ioaddr, ISR);
		if (net_debug > 5) printk("loop status %02x..", status);

		if (status & (ISR_RxOK<<3)) {
			handled = 1;
			write_reg(ioaddr, ISR, ISR_RxOK); /* Clear the Rx interrupt. */
			do {
				int read_status = read_nibble(ioaddr, CMR1);
				if (net_debug > 6)
					printk("handling Rx packet %02x..", read_status);
				/* We acknowledged the normal Rx interrupt, so if the interrupt
				   is still outstanding we must have a Rx error. */
				if (read_status & (CMR1_IRQ << 3)) { /* Overrun. */
					dev->stats.rx_over_errors++;
					/* Set to no-accept mode long enough to remove a packet. */
					write_reg_high(ioaddr, CMR2, CMR2h_OFF);
					net_rx(dev);
					/* Clear the interrupt and return to normal Rx mode. */
					write_reg_high(ioaddr, ISR, ISRh_RxErr);
					write_reg_high(ioaddr, CMR2, lp->addr_mode);
				} else if ((read_status & (CMR1_BufEnb << 3)) == 0) {
					net_rx(dev);
					num_tx_since_rx = 0;
				} else
					break;
			} while (--boguscount > 0);
		} else if (status & ((ISR_TxErr + ISR_TxOK)<<3)) {
			handled = 1;
			if (net_debug > 6)  printk("handling Tx done..");
			/* Clear the Tx interrupt.  We should check for too many failures
			   and reinitialize the adapter. */
			write_reg(ioaddr, ISR, ISR_TxErr + ISR_TxOK);
			if (status & (ISR_TxErr<<3)) {
				dev->stats.collisions++;
				if (++lp->re_tx > 15) {
					dev->stats.tx_aborted_errors++;
					hardware_init(dev);
					break;
				}
				/* Attempt to retransmit. */
				if (net_debug > 6)  printk("attempting to ReTx");
				write_reg(ioaddr, CMR1, CMR1_ReXmit + CMR1_Xmit);
			} else {
				/* Finish up the transmit. */
				dev->stats.tx_packets++;
				lp->pac_cnt_in_tx_buf--;
				if ( lp->saved_tx_size) {
					trigger_send(ioaddr, lp->saved_tx_size);
					lp->saved_tx_size = 0;
					lp->re_tx = 0;
				} else
					lp->tx_unit_busy = 0;
				netif_wake_queue(dev);	/* Inform upper layers. */
			}
			num_tx_since_rx++;
		} else if (num_tx_since_rx > 8
				   && time_after(jiffies, dev->last_rx + HZ)) {
			if (net_debug > 2)
				printk(KERN_DEBUG "%s: Missed packet? No Rx after %d Tx and "
					   "%ld jiffies status %02x  CMR1 %02x.\n", dev->name,
					   num_tx_since_rx, jiffies - dev->last_rx, status,
					   (read_nibble(ioaddr, CMR1) >> 3) & 15);
			dev->stats.rx_missed_errors++;
			hardware_init(dev);
			num_tx_since_rx = 0;
			break;
		} else
			break;
    }

	/* This following code fixes a rare (and very difficult to track down)
	   problem where the adapter forgets its ethernet address. */
	{
		int i;
		for (i = 0; i < 6; i++)
			write_reg_byte(ioaddr, PAR0 + i, dev->dev_addr[i]);
#if 0 && defined(TIMED_CHECKER)
		mod_timer(&lp->timer, jiffies + TIMED_CHECKER);
#endif
	}

	/* Tell the adapter that it can go back to using the output line as IRQ. */
    write_reg(ioaddr, CMR2, CMR2_IRQOUT);
	/* Enable the physical interrupt line, which is sure to be low until.. */
	outb(Ctrl_SelData + Ctrl_IRQEN, ioaddr + PAR_CONTROL);
	/* .. we enable the interrupt sources. */
	write_reg(ioaddr, IMR, ISR_RxOK | ISR_TxErr | ISR_TxOK);
	write_reg_high(ioaddr, IMR, ISRh_RxErr); 			/* Hmmm, really needed? */

	spin_unlock(&lp->lock);

	if (net_debug > 5) printk("exiting interrupt.\n");
	return IRQ_RETVAL(handled);
}

#ifdef TIMED_CHECKER
static void atp_timed_checker(unsigned long data)
{
	struct net_device *dev = (struct net_device *)data;
	long ioaddr = dev->base_addr;
	struct net_local *lp = netdev_priv(dev);
	int tickssofar = jiffies - lp->last_rx_time;
	int i;

	spin_lock(&lp->lock);
	if (tickssofar > 2*HZ) {
#if 1
		for (i = 0; i < 6; i++)
			write_reg_byte(ioaddr, PAR0 + i, dev->dev_addr[i]);
		lp->last_rx_time = jiffies;
#else
		for (i = 0; i < 6; i++)
			if (read_cmd_byte(ioaddr, PAR0 + i) != atp_timed_dev->dev_addr[i])
				{
			struct net_local *lp = netdev_priv(atp_timed_dev);
			write_reg_byte(ioaddr, PAR0 + i, atp_timed_dev->dev_addr[i]);
			if (i == 2)
			  dev->stats.tx_errors++;
			else if (i == 3)
			  dev->stats.tx_dropped++;
			else if (i == 4)
			  dev->stats.collisions++;
			else
			  dev->stats.rx_errors++;
		  }
#endif
	}
	spin_unlock(&lp->lock);
	lp->timer.expires = jiffies + TIMED_CHECKER;
	add_timer(&lp->timer);
}
#endif

/* We have a good packet(s), get it/them out of the buffers. */
static void net_rx(struct net_device *dev)
{
	struct net_local *lp = netdev_priv(dev);
	long ioaddr = dev->base_addr;
	struct rx_header rx_head;

	/* Process the received packet. */
	outb(EOC+MAR, ioaddr + PAR_DATA);
	read_block(ioaddr, 8, (unsigned char*)&rx_head, dev->if_port);
	if (net_debug > 5)
		printk(KERN_DEBUG " rx_count %04x %04x %04x %04x..", rx_head.pad,
			   rx_head.rx_count, rx_head.rx_status, rx_head.cur_addr);
	if ((rx_head.rx_status & 0x77) != 0x01) {
		dev->stats.rx_errors++;
		if (rx_head.rx_status & 0x0004) dev->stats.rx_frame_errors++;
		else if (rx_head.rx_status & 0x0002) dev->stats.rx_crc_errors++;
		if (net_debug > 3)
			printk(KERN_DEBUG "%s: Unknown ATP Rx error %04x.\n",
				   dev->name, rx_head.rx_status);
		if  (rx_head.rx_status & 0x0020) {
			dev->stats.rx_fifo_errors++;
			write_reg_high(ioaddr, CMR1, CMR1h_TxENABLE);
			write_reg_high(ioaddr, CMR1, CMR1h_RxENABLE | CMR1h_TxENABLE);
		} else if (rx_head.rx_status & 0x0050)
			hardware_init(dev);
		return;
	} else {
		/* Malloc up new buffer. The "-4" omits the FCS (CRC). */
		int pkt_len = (rx_head.rx_count & 0x7ff) - 4;
		struct sk_buff *skb;

		skb = dev_alloc_skb(pkt_len + 2);
		if (skb == NULL) {
			printk(KERN_ERR "%s: Memory squeeze, dropping packet.\n",
				   dev->name);
			dev->stats.rx_dropped++;
			goto done;
		}

		skb_reserve(skb, 2);	/* Align IP on 16 byte boundaries */
		read_block(ioaddr, pkt_len, skb_put(skb,pkt_len), dev->if_port);
		skb->protocol = eth_type_trans(skb, dev);
		netif_rx(skb);
		dev->last_rx = jiffies;
		dev->stats.rx_packets++;
		dev->stats.rx_bytes += pkt_len;
	}
 done:
	write_reg(ioaddr, CMR1, CMR1_NextPkt);
	lp->last_rx_time = jiffies;
	return;
}

static void read_block(long ioaddr, int length, unsigned char *p, int data_mode)
{
	if (data_mode <= 3) { /* Mode 0 or 1 */
		outb(Ctrl_LNibRead, ioaddr + PAR_CONTROL);
		outb(length == 8  ?  RdAddr | HNib | MAR  :  RdAddr | MAR,
			 ioaddr + PAR_DATA);
		if (data_mode <= 1) { /* Mode 0 or 1 */
			do { *p++ = read_byte_mode0(ioaddr); } while (--length > 0);
		} else { /* Mode 2 or 3 */
			do { *p++ = read_byte_mode2(ioaddr); } while (--length > 0);
		}
	} else if (data_mode <= 5) {
		do { *p++ = read_byte_mode4(ioaddr); } while (--length > 0);
	} else {
		do { *p++ = read_byte_mode6(ioaddr); } while (--length > 0);
	}

	outb(EOC+HNib+MAR, ioaddr + PAR_DATA);
	outb(Ctrl_SelData, ioaddr + PAR_CONTROL);
}

/* The inverse routine to net_open(). */
static int
net_close(struct net_device *dev)
{
	struct net_local *lp = netdev_priv(dev);
	long ioaddr = dev->base_addr;

	netif_stop_queue(dev);

	del_timer_sync(&lp->timer);

	/* Flush the Tx and disable Rx here. */
	lp->addr_mode = CMR2h_OFF;
	write_reg_high(ioaddr, CMR2, CMR2h_OFF);

	/* Free the IRQ line. */
	outb(0x00, ioaddr + PAR_CONTROL);
	free_irq(dev->irq, dev);

	/* Reset the ethernet hardware and activate the printer pass-through. */
	write_reg_high(ioaddr, CMR1, CMR1h_RESET | CMR1h_MUX);
	return 0;
}


static void set_rx_mode_8002(struct net_device *dev)
{
	struct net_local *lp = netdev_priv(dev);
	long ioaddr = dev->base_addr;

	if (dev->mc_count > 0 || (dev->flags & (IFF_ALLMULTI|IFF_PROMISC)))
		lp->addr_mode = CMR2h_PROMISC;
	else
		lp->addr_mode = CMR2h_Normal;
	write_reg_high(ioaddr, CMR2, lp->addr_mode);
}

static void set_rx_mode_8012(struct net_device *dev)
{
	struct net_local *lp = netdev_priv(dev);
	long ioaddr = dev->base_addr;
	unsigned char new_mode, mc_filter[8]; /* Multicast hash filter */
	int i;

	if (dev->flags & IFF_PROMISC) {			/* Set promiscuous. */
		new_mode = CMR2h_PROMISC;
	} else if ((dev->mc_count > 1000)  ||  (dev->flags & IFF_ALLMULTI)) {
		/* Too many to filter perfectly -- accept all multicasts. */
		memset(mc_filter, 0xff, sizeof(mc_filter));
		new_mode = CMR2h_Normal;
	} else {
		struct dev_mc_list *mclist;

		memset(mc_filter, 0, sizeof(mc_filter));
		for (i = 0, mclist = dev->mc_list; mclist && i < dev->mc_count;
			 i++, mclist = mclist->next)
		{
			int filterbit = ether_crc_le(ETH_ALEN, mclist->dmi_addr) & 0x3f;
			mc_filter[filterbit >> 5] |= 1 << (filterbit & 31);
		}
		new_mode = CMR2h_Normal;
	}
	lp->addr_mode = new_mode;
    write_reg(ioaddr, CMR2, CMR2_IRQOUT | 0x04); /* Switch to page 1. */
    for (i = 0; i < 8; i++)
		write_reg_byte(ioaddr, i, mc_filter[i]);
	if (net_debug > 2 || 1) {
		lp->addr_mode = 1;
		printk(KERN_DEBUG "%s: Mode %d, setting multicast filter to",
			   dev->name, lp->addr_mode);
		for (i = 0; i < 8; i++)
			printk(" %2.2x", mc_filter[i]);
		printk(".\n");
	}

	write_reg_high(ioaddr, CMR2, lp->addr_mode);
    write_reg(ioaddr, CMR2, CMR2_IRQOUT); /* Switch back to page 0 */
}

static void set_rx_mode(struct net_device *dev)
{
	struct net_local *lp = netdev_priv(dev);

	if (lp->chip_type == RTL8002)
		return set_rx_mode_8002(dev);
	else
		return set_rx_mode_8012(dev);
}


static int __init atp_init_module(void) {
	if (debug)					/* Emit version even if no cards detected. */
		printk(KERN_INFO "%s", version);
	return atp_init();
}

static void __exit atp_cleanup_module(void) {
	struct net_device *next_dev;

	while (root_atp_dev) {
		struct net_local *atp_local = netdev_priv(root_atp_dev);
		next_dev = atp_local->next_module;
		unregister_netdev(root_atp_dev);
		/* No need to release_region(), since we never snarf it. */
		free_netdev(root_atp_dev);
		root_atp_dev = next_dev;
	}
}

module_init(atp_init_module);
module_exit(atp_cleanup_module);