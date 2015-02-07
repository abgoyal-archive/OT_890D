

/* Common definitions for all Efx net driver code */

#ifndef EFX_NET_DRIVER_H
#define EFX_NET_DRIVER_H

#include <linux/version.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/if_vlan.h>
#include <linux/timer.h>
#include <linux/mii.h>
#include <linux/list.h>
#include <linux/pci.h>
#include <linux/device.h>
#include <linux/highmem.h>
#include <linux/workqueue.h>
#include <linux/inet_lro.h>
#include <linux/i2c.h>

#include "enum.h"
#include "bitfield.h"

#define EFX_MAX_LRO_DESCRIPTORS 8
#define EFX_MAX_LRO_AGGR MAX_SKB_FRAGS

#ifndef EFX_DRIVER_NAME
#define EFX_DRIVER_NAME	"sfc"
#endif
#define EFX_DRIVER_VERSION	"2.3"

#ifdef EFX_ENABLE_DEBUG
#define EFX_BUG_ON_PARANOID(x) BUG_ON(x)
#define EFX_WARN_ON_PARANOID(x) WARN_ON(x)
#else
#define EFX_BUG_ON_PARANOID(x) do {} while (0)
#define EFX_WARN_ON_PARANOID(x) do {} while (0)
#endif

/* Un-rate-limited logging */
#define EFX_ERR(efx, fmt, args...) \
dev_err(&((efx)->pci_dev->dev), "ERR: %s " fmt, efx_dev_name(efx), ##args)

#define EFX_INFO(efx, fmt, args...) \
dev_info(&((efx)->pci_dev->dev), "INFO: %s " fmt, efx_dev_name(efx), ##args)

#ifdef EFX_ENABLE_DEBUG
#define EFX_LOG(efx, fmt, args...) \
dev_info(&((efx)->pci_dev->dev), "DBG: %s " fmt, efx_dev_name(efx), ##args)
#else
#define EFX_LOG(efx, fmt, args...) \
dev_dbg(&((efx)->pci_dev->dev), "DBG: %s " fmt, efx_dev_name(efx), ##args)
#endif

#define EFX_TRACE(efx, fmt, args...) do {} while (0)

#define EFX_REGDUMP(efx, fmt, args...) do {} while (0)

/* Rate-limited logging */
#define EFX_ERR_RL(efx, fmt, args...) \
do {if (net_ratelimit()) EFX_ERR(efx, fmt, ##args); } while (0)

#define EFX_INFO_RL(efx, fmt, args...) \
do {if (net_ratelimit()) EFX_INFO(efx, fmt, ##args); } while (0)

#define EFX_LOG_RL(efx, fmt, args...) \
do {if (net_ratelimit()) EFX_LOG(efx, fmt, ##args); } while (0)


#define EFX_MAX_CHANNELS 32
#define EFX_MAX_RX_QUEUES EFX_MAX_CHANNELS

#define EFX_TX_QUEUE_OFFLOAD_CSUM	0
#define EFX_TX_QUEUE_NO_CSUM		1
#define EFX_TX_QUEUE_COUNT		2

struct efx_special_buffer {
	void *addr;
	dma_addr_t dma_addr;
	unsigned int len;
	int index;
	int entries;
};

struct efx_tx_buffer {
	const struct sk_buff *skb;
	struct efx_tso_header *tsoh;
	dma_addr_t dma_addr;
	unsigned short len;
	bool continuation;
	bool unmap_single;
	unsigned short unmap_len;
};

struct efx_tx_queue {
	/* Members which don't change on the fast path */
	struct efx_nic *efx ____cacheline_aligned_in_smp;
	int queue;
	struct efx_channel *channel;
	struct efx_nic *nic;
	struct efx_tx_buffer *buffer;
	struct efx_special_buffer txd;
	bool flushed;

	/* Members used mainly on the completion path */
	unsigned int read_count ____cacheline_aligned_in_smp;
	int stopped;

	/* Members used only on the xmit path */
	unsigned int insert_count ____cacheline_aligned_in_smp;
	unsigned int write_count;
	unsigned int old_read_count;
	struct efx_tso_header *tso_headers_free;
	unsigned int tso_bursts;
	unsigned int tso_long_headers;
	unsigned int tso_packets;
};

struct efx_rx_buffer {
	dma_addr_t dma_addr;
	struct sk_buff *skb;
	struct page *page;
	char *data;
	unsigned int len;
	dma_addr_t unmap_addr;
};

struct efx_rx_queue {
	struct efx_nic *efx;
	int queue;
	struct efx_channel *channel;
	struct efx_rx_buffer *buffer;
	struct efx_special_buffer rxd;

	int added_count;
	int notified_count;
	int removed_count;
	spinlock_t add_lock;
	unsigned int max_fill;
	unsigned int fast_fill_trigger;
	unsigned int fast_fill_limit;
	unsigned int min_fill;
	unsigned int min_overfill;
	unsigned int alloc_page_count;
	unsigned int alloc_skb_count;
	struct delayed_work work;
	unsigned int slow_fill_count;

	struct page *buf_page;
	dma_addr_t buf_dma_addr;
	char *buf_data;
	bool flushed;
};

struct efx_buffer {
	void *addr;
	dma_addr_t dma_addr;
	unsigned int len;
};


/* Flags for channel->used_flags */
#define EFX_USED_BY_RX 1
#define EFX_USED_BY_TX 2
#define EFX_USED_BY_RX_TX (EFX_USED_BY_RX | EFX_USED_BY_TX)

enum efx_rx_alloc_method {
	RX_ALLOC_METHOD_AUTO = 0,
	RX_ALLOC_METHOD_SKB = 1,
	RX_ALLOC_METHOD_PAGE = 2,
};

struct efx_channel {
	struct efx_nic *efx;
	int channel;
	char name[IFNAMSIZ + 6];
	int used_flags;
	bool enabled;
	int irq;
	unsigned int irq_moderation;
	struct net_device *napi_dev;
	struct napi_struct napi_str;
	bool work_pending;
	struct efx_special_buffer eventq;
	unsigned int eventq_read_ptr;
	unsigned int last_eventq_read_ptr;
	unsigned int eventq_magic;

	struct net_lro_mgr lro_mgr;
	int rx_alloc_level;
	int rx_alloc_push_pages;
	int rx_alloc_pop_pages;

	unsigned n_rx_tobe_disc;
	unsigned n_rx_ip_frag_err;
	unsigned n_rx_ip_hdr_chksum_err;
	unsigned n_rx_tcp_udp_chksum_err;
	unsigned n_rx_frm_trunc;
	unsigned n_rx_overlength;
	unsigned n_skbuff_leaks;

	/* Used to pipeline received packets in order to optimise memory
	 * access with prefetches.
	 */
	struct efx_rx_buffer *rx_pkt;
	bool rx_pkt_csummed;

};

struct efx_blinker {
	int led_num;
	bool state;
	bool resubmit;
	struct timer_list timer;
};


struct efx_board {
	int type;
	int major;
	int minor;
	int (*init) (struct efx_nic *nic);
	/* As the LEDs are typically attached to the PHY, LEDs
	 * have a separate init callback that happens later than
	 * board init. */
	int (*init_leds)(struct efx_nic *efx);
	int (*monitor) (struct efx_nic *nic);
	void (*set_fault_led) (struct efx_nic *efx, bool state);
	void (*blink) (struct efx_nic *efx, bool start);
	void (*fini) (struct efx_nic *nic);
	struct efx_blinker blinker;
	struct i2c_client *hwmon_client, *ioexp_client;
};

#define STRING_TABLE_LOOKUP(val, member)	\
	member ## _names[val]

enum efx_int_mode {
	/* Be careful if altering to correct macro below */
	EFX_INT_MODE_MSIX = 0,
	EFX_INT_MODE_MSI = 1,
	EFX_INT_MODE_LEGACY = 2,
	EFX_INT_MODE_MAX	/* Insert any new items before this */
};
#define EFX_INT_MODE_USE_MSI(x) (((x)->interrupt_mode) <= EFX_INT_MODE_MSI)

enum phy_type {
	PHY_TYPE_NONE = 0,
	PHY_TYPE_TXC43128 = 1,
	PHY_TYPE_88E1111 = 2,
	PHY_TYPE_SFX7101 = 3,
	PHY_TYPE_QT2022C2 = 4,
	PHY_TYPE_PM8358 = 6,
	PHY_TYPE_SFT9001A = 8,
	PHY_TYPE_SFT9001B = 10,
	PHY_TYPE_MAX	/* Insert any new items before this */
};

#define PHY_ADDR_INVALID 0xff

#define EFX_IS10G(efx) ((efx)->link_speed == 10000)

enum nic_state {
	STATE_INIT = 0,
	STATE_RUNNING = 1,
	STATE_FINI = 2,
	STATE_DISABLED = 3,
	STATE_MAX,
};

#ifdef CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS
#define EFX_PAGE_IP_ALIGN 0
#else
#define EFX_PAGE_IP_ALIGN NET_IP_ALIGN
#endif

#define EFX_PAGE_SKB_ALIGN 2

/* Forward declaration */
struct efx_nic;

/* Pseudo bit-mask flow control field */
enum efx_fc_type {
	EFX_FC_RX = 1,
	EFX_FC_TX = 2,
	EFX_FC_AUTO = 4,
};

/* Supported MAC bit-mask */
enum efx_mac_type {
	EFX_GMAC = 1,
	EFX_XMAC = 2,
};

static inline unsigned int efx_fc_advertise(enum efx_fc_type wanted_fc)
{
	unsigned int adv = 0;
	if (wanted_fc & EFX_FC_RX)
		adv = ADVERTISE_PAUSE_CAP | ADVERTISE_PAUSE_ASYM;
	if (wanted_fc & EFX_FC_TX)
		adv ^= ADVERTISE_PAUSE_ASYM;
	return adv;
}

static inline enum efx_fc_type efx_fc_resolve(enum efx_fc_type wanted_fc,
					      unsigned int lpa)
{
	unsigned int adv = efx_fc_advertise(wanted_fc);

	if (!(wanted_fc & EFX_FC_AUTO))
		return wanted_fc;

	if (adv & lpa & ADVERTISE_PAUSE_CAP)
		return EFX_FC_RX | EFX_FC_TX;
	if (adv & lpa & ADVERTISE_PAUSE_ASYM) {
		if (adv & ADVERTISE_PAUSE_CAP)
			return EFX_FC_RX;
		if (lpa & ADVERTISE_PAUSE_CAP)
			return EFX_FC_TX;
	}
	return 0;
}

struct efx_mac_operations {
	void (*reconfigure) (struct efx_nic *efx);
	void (*update_stats) (struct efx_nic *efx);
	void (*irq) (struct efx_nic *efx);
	void (*poll) (struct efx_nic *efx);
};

struct efx_phy_operations {
	enum efx_mac_type macs;
	int (*init) (struct efx_nic *efx);
	void (*fini) (struct efx_nic *efx);
	void (*reconfigure) (struct efx_nic *efx);
	void (*clear_interrupt) (struct efx_nic *efx);
	void (*poll) (struct efx_nic *efx);
	void (*get_settings) (struct efx_nic *efx,
			      struct ethtool_cmd *ecmd);
	int (*set_settings) (struct efx_nic *efx,
			     struct ethtool_cmd *ecmd);
	void (*set_npage_adv) (struct efx_nic *efx, u32);
	u32 num_tests;
	const char *const *test_names;
	int (*run_tests) (struct efx_nic *efx, int *results, unsigned flags);
	int mmds;
	unsigned loopbacks;
};

enum efx_phy_mode {
	PHY_MODE_NORMAL		= 0,
	PHY_MODE_TX_DISABLED	= 1,
	PHY_MODE_LOW_POWER	= 2,
	PHY_MODE_OFF		= 4,
	PHY_MODE_SPECIAL	= 8,
};

static inline bool efx_phy_mode_disabled(enum efx_phy_mode mode)
{
	return !!(mode & ~PHY_MODE_TX_DISABLED);
}

struct efx_mac_stats {
	u64 tx_bytes;
	u64 tx_good_bytes;
	u64 tx_bad_bytes;
	unsigned long tx_packets;
	unsigned long tx_bad;
	unsigned long tx_pause;
	unsigned long tx_control;
	unsigned long tx_unicast;
	unsigned long tx_multicast;
	unsigned long tx_broadcast;
	unsigned long tx_lt64;
	unsigned long tx_64;
	unsigned long tx_65_to_127;
	unsigned long tx_128_to_255;
	unsigned long tx_256_to_511;
	unsigned long tx_512_to_1023;
	unsigned long tx_1024_to_15xx;
	unsigned long tx_15xx_to_jumbo;
	unsigned long tx_gtjumbo;
	unsigned long tx_collision;
	unsigned long tx_single_collision;
	unsigned long tx_multiple_collision;
	unsigned long tx_excessive_collision;
	unsigned long tx_deferred;
	unsigned long tx_late_collision;
	unsigned long tx_excessive_deferred;
	unsigned long tx_non_tcpudp;
	unsigned long tx_mac_src_error;
	unsigned long tx_ip_src_error;
	u64 rx_bytes;
	u64 rx_good_bytes;
	u64 rx_bad_bytes;
	unsigned long rx_packets;
	unsigned long rx_good;
	unsigned long rx_bad;
	unsigned long rx_pause;
	unsigned long rx_control;
	unsigned long rx_unicast;
	unsigned long rx_multicast;
	unsigned long rx_broadcast;
	unsigned long rx_lt64;
	unsigned long rx_64;
	unsigned long rx_65_to_127;
	unsigned long rx_128_to_255;
	unsigned long rx_256_to_511;
	unsigned long rx_512_to_1023;
	unsigned long rx_1024_to_15xx;
	unsigned long rx_15xx_to_jumbo;
	unsigned long rx_gtjumbo;
	unsigned long rx_bad_lt64;
	unsigned long rx_bad_64_to_15xx;
	unsigned long rx_bad_15xx_to_jumbo;
	unsigned long rx_bad_gtjumbo;
	unsigned long rx_overflow;
	unsigned long rx_missed;
	unsigned long rx_false_carrier;
	unsigned long rx_symbol_error;
	unsigned long rx_align_error;
	unsigned long rx_length_error;
	unsigned long rx_internal_error;
	unsigned long rx_good_lt64;
};

/* Number of bits used in a multicast filter hash address */
#define EFX_MCAST_HASH_BITS 8

/* Number of (single-bit) entries in a multicast filter hash */
#define EFX_MCAST_HASH_ENTRIES (1 << EFX_MCAST_HASH_BITS)

/* An Efx multicast filter hash */
union efx_multicast_hash {
	u8 byte[EFX_MCAST_HASH_ENTRIES / 8];
	efx_oword_t oword[EFX_MCAST_HASH_ENTRIES / sizeof(efx_oword_t) / 8];
};

struct efx_nic {
	char name[IFNAMSIZ];
	struct pci_dev *pci_dev;
	const struct efx_nic_type *type;
	int legacy_irq;
	struct workqueue_struct *workqueue;
	char workqueue_name[16];
	struct work_struct reset_work;
	struct delayed_work monitor_work;
	resource_size_t membase_phys;
	void __iomem *membase;
	spinlock_t biu_lock;
	enum efx_int_mode interrupt_mode;

	struct i2c_adapter i2c_adap;
	struct efx_board board_info;

	enum nic_state state;
	enum reset_type reset_pending;

	struct efx_tx_queue tx_queue[EFX_TX_QUEUE_COUNT];
	struct efx_rx_queue rx_queue[EFX_MAX_RX_QUEUES];
	struct efx_channel channel[EFX_MAX_CHANNELS];

	int n_rx_queues;
	int n_channels;
	unsigned int rx_buffer_len;
	unsigned int rx_buffer_order;

	struct efx_buffer irq_status;
	volatile signed int last_irq_cpu;

	struct efx_spi_device *spi_flash;
	struct efx_spi_device *spi_eeprom;
	struct mutex spi_lock;

	unsigned n_rx_nodesc_drop_cnt;

	struct falcon_nic_data *nic_data;

	struct mutex mac_lock;
	struct work_struct mac_work;
	bool port_enabled;
	bool port_inhibited;

	bool port_initialized;
	struct net_device *net_dev;
	bool rx_checksum_enabled;

	atomic_t netif_stop_count;
	spinlock_t netif_stop_lock;

	struct efx_mac_stats mac_stats;
	struct efx_buffer stats_buffer;
	spinlock_t stats_lock;
	unsigned int stats_disable_count;

	struct efx_mac_operations *mac_op;
	unsigned char mac_address[ETH_ALEN];

	enum phy_type phy_type;
	spinlock_t phy_lock;
	struct work_struct phy_work;
	struct efx_phy_operations *phy_op;
	void *phy_data;
	struct mii_if_info mii;
	enum efx_phy_mode phy_mode;

	bool mac_up;
	bool link_up;
	bool link_fd;
	enum efx_fc_type link_fc;
	unsigned int link_speed;
	unsigned int n_link_state_changes;

	bool promiscuous;
	union efx_multicast_hash multicast_hash;
	enum efx_fc_type wanted_fc;

	atomic_t rx_reset;
	enum efx_loopback_mode loopback_mode;
	unsigned int loopback_modes;

	void *loopback_selftest;
};

static inline int efx_dev_registered(struct efx_nic *efx)
{
	return efx->net_dev->reg_state == NETREG_REGISTERED;
}

static inline const char *efx_dev_name(struct efx_nic *efx)
{
	return efx_dev_registered(efx) ? efx->name : "";
}

struct efx_nic_type {
	unsigned int mem_bar;
	unsigned int mem_map_size;
	unsigned int txd_ptr_tbl_base;
	unsigned int rxd_ptr_tbl_base;
	unsigned int buf_tbl_base;
	unsigned int evq_ptr_tbl_base;
	unsigned int evq_rptr_tbl_base;

	unsigned int txd_ring_mask;
	unsigned int rxd_ring_mask;
	unsigned int evq_size;
	u64 max_dma_mask;
	unsigned int tx_dma_mask;
	unsigned bug5391_mask;

	int rx_xoff_thresh;
	int rx_xon_thresh;
	unsigned int rx_buffer_padding;
	unsigned int max_interrupt_mode;
	unsigned int phys_addr_channels;
};


/* Iterate over all used channels */
#define efx_for_each_channel(_channel, _efx)				\
	for (_channel = &_efx->channel[0];				\
	     _channel < &_efx->channel[EFX_MAX_CHANNELS];		\
	     _channel++)						\
		if (!_channel->used_flags)				\
			continue;					\
		else

/* Iterate over all used TX queues */
#define efx_for_each_tx_queue(_tx_queue, _efx)				\
	for (_tx_queue = &_efx->tx_queue[0];				\
	     _tx_queue < &_efx->tx_queue[EFX_TX_QUEUE_COUNT];		\
	     _tx_queue++)

/* Iterate over all TX queues belonging to a channel */
#define efx_for_each_channel_tx_queue(_tx_queue, _channel)		\
	for (_tx_queue = &_channel->efx->tx_queue[0];			\
	     _tx_queue < &_channel->efx->tx_queue[EFX_TX_QUEUE_COUNT];	\
	     _tx_queue++)						\
		if (_tx_queue->channel != _channel)			\
			continue;					\
		else

/* Iterate over all used RX queues */
#define efx_for_each_rx_queue(_rx_queue, _efx)				\
	for (_rx_queue = &_efx->rx_queue[0];				\
	     _rx_queue < &_efx->rx_queue[_efx->n_rx_queues];		\
	     _rx_queue++)

/* Iterate over all RX queues belonging to a channel */
#define efx_for_each_channel_rx_queue(_rx_queue, _channel)		\
	for (_rx_queue = &_channel->efx->rx_queue[_channel->channel];	\
	     _rx_queue;							\
	     _rx_queue = NULL)						\
		if (_rx_queue->channel != _channel)			\
			continue;					\
		else

static inline struct efx_rx_buffer *efx_rx_buffer(struct efx_rx_queue *rx_queue,
						  unsigned int index)
{
	return (&rx_queue->buffer[index]);
}

/* Set bit in a little-endian bitfield */
static inline void set_bit_le(unsigned nr, unsigned char *addr)
{
	addr[nr / 8] |= (1 << (nr % 8));
}

/* Clear bit in a little-endian bitfield */
static inline void clear_bit_le(unsigned nr, unsigned char *addr)
{
	addr[nr / 8] &= ~(1 << (nr % 8));
}


#define EFX_MAX_FRAME_LEN(mtu) \
	((((mtu) + ETH_HLEN + VLAN_HLEN + 4/* FCS */) + 7) & ~7)


#endif /* EFX_NET_DRIVER_H */
