

#ifndef N_HCI
#define N_HCI	15
#endif

/* Ioctls */
#define HCIUARTSETPROTO		_IOW('U', 200, int)
#define HCIUARTGETPROTO		_IOR('U', 201, int)
#define HCIUARTGETDEVICE	_IOR('U', 202, int)

/* UART protocols */
#define HCI_UART_MAX_PROTO	5

#define HCI_UART_H4	0
#define HCI_UART_BCSP	1
#define HCI_UART_3WIRE	2
#define HCI_UART_H4DS	3
#define HCI_UART_LL	4

struct hci_uart;

struct hci_uart_proto {
	unsigned int id;
	int (*open)(struct hci_uart *hu);
	int (*close)(struct hci_uart *hu);
	int (*flush)(struct hci_uart *hu);
	int (*recv)(struct hci_uart *hu, void *data, int len);
	int (*enqueue)(struct hci_uart *hu, struct sk_buff *skb);
	struct sk_buff *(*dequeue)(struct hci_uart *hu);
};

struct hci_uart {
	struct tty_struct	*tty;
	struct hci_dev		*hdev;
	unsigned long		flags;

	struct hci_uart_proto	*proto;
	void			*priv;

	struct sk_buff		*tx_skb;
	unsigned long		tx_state;
	spinlock_t		rx_lock;
};

/* HCI_UART flag bits */
#define HCI_UART_PROTO_SET	0

/* TX states  */
#define HCI_UART_SENDING	1
#define HCI_UART_TX_WAKEUP	2

int hci_uart_register_proto(struct hci_uart_proto *p);
int hci_uart_unregister_proto(struct hci_uart_proto *p);
int hci_uart_tx_wakeup(struct hci_uart *hu);

#ifdef CONFIG_BT_HCIUART_H4
int h4_init(void);
int h4_deinit(void);
#endif

#ifdef CONFIG_BT_HCIUART_BCSP
int bcsp_init(void);
int bcsp_deinit(void);
#endif

#ifdef CONFIG_BT_HCIUART_LL
int ll_init(void);
int ll_deinit(void);
#endif

/* Debugging Purpose */
extern unsigned int gDbgLevel;
#define PFX                         "[BT] "
#define BT_LOG_DBG                  3
#define BT_LOG_INFO                 2
#define BT_LOG_WARN                 1
#define BT_LOG_ERR                  0

#define BT_DBG_FUNC(fmt, arg...)    if(gDbgLevel >= BT_LOG_DBG){ printk(PFX "%s: " fmt, __FUNCTION__ ,##arg);}
#define BT_INFO_FUNC(fmt, arg...)   if(gDbgLevel >= BT_LOG_INFO){ printk(PFX "%s: " fmt, __FUNCTION__ ,##arg);}
#define BT_WARN_FUNC(fmt, arg...)   if(gDbgLevel >= BT_LOG_WARN){ printk(PFX "%s: " fmt, __FUNCTION__ ,##arg);}
#define BT_ERR_FUNC(fmt, arg...)    if(gDbgLevel >= BT_LOG_DBG){ printk(PFX "%s: " fmt, __FUNCTION__ ,##arg);}
#define BT_TRC_FUNC(f)              printk(PFX "<%s> <%d>\n", __FUNCTION__, __LINE__);

