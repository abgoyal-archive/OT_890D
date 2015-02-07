

#include <linux/module.h>

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/interrupt.h>
#include <linux/ptrace.h>
#include <linux/poll.h>

#include <linux/slab.h>
#include <linux/tty.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/signal.h>
#include <linux/ioctl.h>
#include <linux/skbuff.h>

#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>

/* for interrutp handler */
#include <mach/mt_bt.h>
/* for wake lock */
#include <linux/wakelock.h>

/* for debugging message level */
#include <linux/proc_fs.h>

#include "hci_uart.h"

#define VERSION "2.2"

/* Add wake lock mechamism */
#define USE_WAKE_LOCK  1
#define WAKE_LOCK_TIME_OUT ( 6 * HZ)

#if USE_WAKE_LOCK
static struct wake_lock bt_wakelock;
#endif

unsigned int gDbgLevel = BT_LOG_INFO;

static int reset = 0;

static struct hci_uart_proto *hup[HCI_UART_MAX_PROTO];

int hci_uart_register_proto(struct hci_uart_proto *p)
{
	if (p->id >= HCI_UART_MAX_PROTO)
		return -EINVAL;

	if (hup[p->id])
		return -EEXIST;

	hup[p->id] = p;

	return 0;
}

int hci_uart_unregister_proto(struct hci_uart_proto *p)
{
	if (p->id >= HCI_UART_MAX_PROTO)
		return -EINVAL;

	if (!hup[p->id])
		return -EINVAL;

	hup[p->id] = NULL;

	return 0;
}

static struct hci_uart_proto *hci_uart_get_proto(unsigned int id)
{
	if (id >= HCI_UART_MAX_PROTO)
		return NULL;

	return hup[id];
}

static inline void hci_uart_tx_complete(struct hci_uart *hu, int pkt_type)
{
	struct hci_dev *hdev = hu->hdev;

	/* Update HCI stat counters */
	switch (pkt_type) {
	case HCI_COMMAND_PKT:
		hdev->stat.cmd_tx++;
		break;

	case HCI_ACLDATA_PKT:
		hdev->stat.acl_tx++;
		break;

	case HCI_SCODATA_PKT:
		hdev->stat.cmd_tx++;
		break;
	}
}

static inline struct sk_buff *hci_uart_dequeue(struct hci_uart *hu)
{
	struct sk_buff *skb = hu->tx_skb;

	if (!skb)
		skb = hu->proto->dequeue(hu);
	else
		hu->tx_skb = NULL;

	return skb;
}

int hci_uart_tx_wakeup(struct hci_uart *hu)
{
	struct tty_struct *tty = hu->tty;
	struct hci_dev *hdev = hu->hdev;
	struct sk_buff *skb;
    
    BT_DBG_FUNC("hci_uart_tx_wakeup %d\n", __LINE__);
    
	if (test_and_set_bit(HCI_UART_SENDING, &hu->tx_state)) {
		set_bit(HCI_UART_TX_WAKEUP, &hu->tx_state);
		return 0;
	}
    BT_DBG_FUNC("hci_uart_tx_wakeup %d\n", __LINE__);    
	BT_DBG("");

restart:
	clear_bit(HCI_UART_TX_WAKEUP, &hu->tx_state);

	while ((skb = hci_uart_dequeue(hu))) {
		int len;

		set_bit(TTY_DO_WRITE_WAKEUP, &tty->flags);
		len = tty->ops->write(tty, skb->data, skb->len);
		hdev->stat.byte_tx += len;

		skb_pull(skb, len);
		if (skb->len) {
			hu->tx_skb = skb;
			break;
		}
        //printk(KERN_NOTICE "[BT] hci_uart_tx_wakeup %d\n", __LINE__);
		hci_uart_tx_complete(hu, bt_cb(skb)->pkt_type);
		kfree_skb(skb);
	}

	if (test_bit(HCI_UART_TX_WAKEUP, &hu->tx_state))
		goto restart;

	clear_bit(HCI_UART_SENDING, &hu->tx_state);
#if USE_WAKE_LOCK
   /* we need to hold host to awake for 5 sec before entering sleep */   
   BT_DBG_FUNC("wake_lock_timeout %d\n", __LINE__);
   wake_lock_timeout(&bt_wakelock, WAKE_LOCK_TIME_OUT);
#endif
	return 0;
}

/* ------- Interface to HCI layer ------ */
/* Initialize device */
static int hci_uart_open(struct hci_dev *hdev)
{
	BT_DBG("%s %p", hdev->name, hdev);

	/* Nothing to do for UART driver */

	set_bit(HCI_RUNNING, &hdev->flags);

    /* CH enable IRQ handler */
    mt_bt_enable_irq();
    
	return 0;
}

/* Reset device */
static int hci_uart_flush(struct hci_dev *hdev)
{
	struct hci_uart *hu  = (struct hci_uart *) hdev->driver_data;
	struct tty_struct *tty = hu->tty;

	BT_DBG("hdev %p tty %p", hdev, tty);

	if (hu->tx_skb) {
		kfree_skb(hu->tx_skb); hu->tx_skb = NULL;
	}

	/* Flush any pending characters in the driver and discipline. */
	tty_ldisc_flush(tty);
	tty_driver_flush_buffer(tty);

	if (test_bit(HCI_UART_PROTO_SET, &hu->flags))
		hu->proto->flush(hu);

	return 0;
}

/* Close device */
static int hci_uart_close(struct hci_dev *hdev)
{
	BT_DBG("hdev %p", hdev);

	if (!test_and_clear_bit(HCI_RUNNING, &hdev->flags))
		return 0;

    /* CH enable IRQ handler */
    mt_bt_disable_irq();
        
	hci_uart_flush(hdev);
	hdev->flush = NULL;
	return 0;
}

/* Send frames from HCI layer */
static int hci_uart_send_frame(struct sk_buff *skb)
{
	struct hci_dev* hdev = (struct hci_dev *) skb->dev;
	struct tty_struct *tty;
	struct hci_uart *hu;

	if (!hdev) {
		BT_ERR("Frame for uknown device (hdev=NULL)");
		return -ENODEV;
	}

	if (!test_bit(HCI_RUNNING, &hdev->flags))
		return -EBUSY;

	hu = (struct hci_uart *) hdev->driver_data;
	tty = hu->tty;

	BT_DBG("%s: type %d len %d", hdev->name, bt_cb(skb)->pkt_type, skb->len);

	hu->proto->enqueue(hu, skb);

	hci_uart_tx_wakeup(hu);

	return 0;
}

static void hci_uart_destruct(struct hci_dev *hdev)
{
	if (!hdev)
		return;

	BT_DBG("%s", hdev->name);
	kfree(hdev->driver_data);
}

/* ------ LDISC part ------ */
static int hci_uart_tty_open(struct tty_struct *tty)
{
	struct hci_uart *hu = (void *) tty->disc_data;

	BT_DBG("tty %p", tty);

	if (hu)
		return -EEXIST;

	if (!(hu = kzalloc(sizeof(struct hci_uart), GFP_KERNEL))) {
		BT_ERR("Can't allocate control structure");
		return -ENFILE;
	}

	tty->disc_data = hu;
	hu->tty = tty;
	tty->receive_room = 65536;

	spin_lock_init(&hu->rx_lock);

	/* Flush any pending characters in the driver and line discipline. */

	/* FIXME: why is this needed. Note don't use ldisc_ref here as the
	   open path is before the ldisc is referencable */

	if (tty->ldisc.ops->flush_buffer)
		tty->ldisc.ops->flush_buffer(tty);
	tty_driver_flush_buffer(tty);

	return 0;
}

static void hci_uart_tty_close(struct tty_struct *tty)
{
	struct hci_uart *hu = (void *)tty->disc_data;

	BT_DBG("tty %p", tty);

	/* Detach from the tty */
	tty->disc_data = NULL;

	if (hu) {
		struct hci_dev *hdev = hu->hdev;

		if (hdev)
			hci_uart_close(hdev);

		if (test_and_clear_bit(HCI_UART_PROTO_SET, &hu->flags)) {
#if 1/* CH unregistery interrupt handler */
            mt_bt_pm_deinit(hdev);
#endif
			hu->proto->close(hu);
			hci_unregister_dev(hdev);
			hci_free_dev(hdev);
		}
	}
}

static void hci_uart_tty_wakeup(struct tty_struct *tty)
{
	struct hci_uart *hu = (void *)tty->disc_data;

	BT_DBG("");

	if (!hu)
		return;

	clear_bit(TTY_DO_WRITE_WAKEUP, &tty->flags);

	if (tty != hu->tty)
		return;

	if (test_bit(HCI_UART_PROTO_SET, &hu->flags))
		hci_uart_tx_wakeup(hu);
}

static void hci_uart_tty_receive(struct tty_struct *tty, const u8 *data, char *flags, int count)
{
	struct hci_uart *hu = (void *)tty->disc_data;

	if (!hu || tty != hu->tty)
		return;

	if (!test_bit(HCI_UART_PROTO_SET, &hu->flags))
		return;
    BT_DBG_FUNC("hci_uart_tty_receive count %d [1] 0x%02x\n", count, (int)data[0]);
    BT_DBG_FUNC("hci_uart_tty_receive %02x-%02x-%02x\n", (int)data[0], (int)data[1], (int)data[2]);

	spin_lock(&hu->rx_lock);
	hu->proto->recv(hu, (void *) data, count);
	hu->hdev->stat.byte_rx += count;
	spin_unlock(&hu->rx_lock);

	tty_unthrottle(tty);
}

static int hci_uart_register_dev(struct hci_uart *hu)
{
	struct hci_dev *hdev;

	BT_DBG("");

	/* Initialize and register HCI device */
	hdev = hci_alloc_dev();
	if (!hdev) {
		BT_ERR("Can't allocate HCI device");
		return -ENOMEM;
	}

	hu->hdev = hdev;

	hdev->type = HCI_UART;
	hdev->driver_data = hu;

	hdev->open  = hci_uart_open;
	hdev->close = hci_uart_close;
	hdev->flush = hci_uart_flush;
	hdev->send  = hci_uart_send_frame;
	hdev->destruct = hci_uart_destruct;

	hdev->owner = THIS_MODULE;

	if (!reset)
		set_bit(HCI_QUIRK_NO_RESET, &hdev->quirks);

	if (hci_register_dev(hdev) < 0) {
		BT_ERR("Can't register HCI device");
		hci_free_dev(hdev);
		return -ENODEV;
	}

#if 1/* CH moved to bluetooth driver */
    /* register interrupt handler */
    mt_bt_pm_init(hdev);
#endif
	return 0;
}

static int hci_uart_set_proto(struct hci_uart *hu, int id)
{
	struct hci_uart_proto *p;
	int err;

	p = hci_uart_get_proto(id);
	if (!p)
		return -EPROTONOSUPPORT;

	err = p->open(hu);
	if (err)
		return err;

	hu->proto = p;

	err = hci_uart_register_dev(hu);
	if (err) {
		p->close(hu);
		return err;
	}

	return 0;
}

static int hci_uart_tty_ioctl(struct tty_struct *tty, struct file * file,
					unsigned int cmd, unsigned long arg)
{
	struct hci_uart *hu = (void *)tty->disc_data;
	int err = 0;

	BT_DBG("");

	/* Verify the status of the device */
	if (!hu)
		return -EBADF;

	switch (cmd) {
	case HCIUARTSETPROTO:
		if (!test_and_set_bit(HCI_UART_PROTO_SET, &hu->flags)) {
			err = hci_uart_set_proto(hu, arg);
			if (err) {
				clear_bit(HCI_UART_PROTO_SET, &hu->flags);
				return err;
			}
			tty->low_latency = 1;
		} else
			return -EBUSY;
		break;

	case HCIUARTGETPROTO:
		if (test_bit(HCI_UART_PROTO_SET, &hu->flags))
			return hu->proto->id;
		return -EUNATCH;

	case HCIUARTGETDEVICE:
		if (test_bit(HCI_UART_PROTO_SET, &hu->flags))
			return hu->hdev->id;
		return -EUNATCH;

	default:
		err = n_tty_ioctl_helper(tty, file, cmd, arg);
		break;
	};

	return err;
}

static ssize_t hci_uart_tty_read(struct tty_struct *tty, struct file *file,
					unsigned char __user *buf, size_t nr)
{
	return 0;
}

static ssize_t hci_uart_tty_write(struct tty_struct *tty, struct file *file,
					const unsigned char *data, size_t count)
{
	return 0;
}

static unsigned int hci_uart_tty_poll(struct tty_struct *tty,
					struct file *filp, poll_table *wait)
{
	return 0;
}

static int btDbgFlagRead(char *page, char **start, off_t off,
			       int count, int *eof, void *data)
{
	int len = 0;
	char *p = page;

    p += sprintf(p, "Mediatek BT DEBUG flag\n\r" );
    p += sprintf(p, "=========================================\n\r" );
    p += sprintf(p, "DBG_FLAG = 0x%x\n\r",gDbgLevel);

	*start = page + off;

	len = p - page;
	if (len > off)
		len -= off;
	else
		len = 0;

	return len < count ? len  : count;
}    

static int
btDbgFlagWrite (
    struct file *file,
    const char *buffer,
    unsigned long count,
    void *data
    )
{
    char acBuf[4]; 
    unsigned int u4CopySize = 0;
    unsigned int u4Flag = 0;

    u4CopySize = (count < (sizeof(acBuf) - 1)) ? count : (sizeof(acBuf) - 1);
    if(copy_from_user(acBuf, buffer, u4CopySize))
		return 0;
    acBuf[u4CopySize] = '\0';

    if (sscanf(acBuf, "%x", &u4Flag) == 1) {
        gDbgLevel = u4Flag;
    }
    return count;
}

static int __init hci_uart_init(void)
{
	static struct tty_ldisc_ops hci_uart_ldisc;
	int err;
    struct proc_dir_entry *prEntry;

	BT_INFO("HCI UART driver ver %s", VERSION);

#if USE_WAKE_LOCK
    /*  */
   wake_lock_init(&bt_wakelock, WAKE_LOCK_SUSPEND, "BT_WAKELOCK");     
#endif    
	/* Register the tty discipline */

	memset(&hci_uart_ldisc, 0, sizeof (hci_uart_ldisc));
	hci_uart_ldisc.magic		= TTY_LDISC_MAGIC;
	hci_uart_ldisc.name		= "n_hci";
	hci_uart_ldisc.open		= hci_uart_tty_open;
	hci_uart_ldisc.close		= hci_uart_tty_close;
	hci_uart_ldisc.read		= hci_uart_tty_read;
	hci_uart_ldisc.write		= hci_uart_tty_write;
	hci_uart_ldisc.ioctl		= hci_uart_tty_ioctl;
	hci_uart_ldisc.poll		= hci_uart_tty_poll;
	hci_uart_ldisc.receive_buf	= hci_uart_tty_receive;
	hci_uart_ldisc.write_wakeup	= hci_uart_tty_wakeup;
	hci_uart_ldisc.owner		= THIS_MODULE;

	if ((err = tty_register_ldisc(N_HCI, &hci_uart_ldisc))) {
		BT_ERR("HCI line discipline registration failed. (%d)", err);
		return err;
	}

#ifdef CONFIG_BT_HCIUART_H4
	h4_init();
#endif
#ifdef CONFIG_BT_HCIUART_BCSP
	bcsp_init();
#endif
#ifdef CONFIG_BT_HCIUART_LL
	ll_init();
#endif

    prEntry = create_proc_entry("bt_dbg_flag", 0, NULL);
    if (prEntry) {
        prEntry->read_proc = btDbgFlagRead;
        prEntry->write_proc = btDbgFlagWrite;
    }

	return 0;
}

static void __exit hci_uart_exit(void)
{
	int err;

    remove_proc_entry("bt_dbg_flag", NULL);

#ifdef CONFIG_BT_HCIUART_H4
	h4_deinit();
#endif
#ifdef CONFIG_BT_HCIUART_BCSP
	bcsp_deinit();
#endif
#ifdef CONFIG_BT_HCIUART_LL
	ll_deinit();
#endif

	/* Release tty registration of line discipline */
	if ((err = tty_unregister_ldisc(N_HCI)))
		BT_ERR("Can't unregister HCI line discipline (%d)", err);
#if USE_WAKE_LOCK
    /*  */
   wake_lock_destroy(&bt_wakelock);
#endif    
}

module_init(hci_uart_init);
module_exit(hci_uart_exit);

module_param(reset, bool, 0644);
MODULE_PARM_DESC(reset, "Send HCI reset command on initialization");

MODULE_AUTHOR("Marcel Holtmann <marcel@holtmann.org>");
MODULE_DESCRIPTION("Bluetooth HCI UART driver ver " VERSION);
MODULE_VERSION(VERSION);
MODULE_LICENSE("GPL");
MODULE_ALIAS_LDISC(N_HCI);
