

#ifndef __NETXEN_NIC_HW_H_
#define __NETXEN_NIC_HW_H_

#include "netxen_nic_hdr.h"

/* Hardware memory size of 128 meg */
#define NETXEN_MEMADDR_MAX (128 * 1024 * 1024)

#ifndef readq
static inline u64 readq(void __iomem * addr)
{
	return readl(addr) | (((u64) readl(addr + 4)) << 32LL);
}
#endif

#ifndef writeq
static inline void writeq(u64 val, void __iomem * addr)
{
	writel(((u32) (val)), (addr));
	writel(((u32) (val >> 32)), (addr + 4));
}
#endif

static inline void netxen_nic_hw_block_write64(u64 __iomem * data_ptr,
					       u64 __iomem * addr,
					       int num_words)
{
	int num;
	for (num = 0; num < num_words; num++) {
		writeq(readq((void __iomem *)data_ptr), addr);
		addr++;
		data_ptr++;
	}
}

static inline void netxen_nic_hw_block_read64(u64 __iomem * data_ptr,
					      u64 __iomem * addr, int num_words)
{
	int num;
	for (num = 0; num < num_words; num++) {
		writeq(readq((void __iomem *)addr), data_ptr);
		addr++;
		data_ptr++;
	}

}

struct netxen_adapter;

#define NETXEN_PCI_MAPSIZE_BYTES  (NETXEN_PCI_MAPSIZE << 20)

struct netxen_port;
void netxen_nic_set_link_parameters(struct netxen_adapter *adapter);
void netxen_nic_flash_print(struct netxen_adapter *adapter);

typedef u8 netxen_ethernet_macaddr_t[6];

/* Nibble or Byte mode for phy interface (GbE mode only) */
typedef enum {
	NETXEN_NIU_10_100_MB = 0,
	NETXEN_NIU_1000_MB
} netxen_niu_gbe_ifmode_t;

#define _netxen_crb_get_bit(var, bit)  ((var >> bit) & 0x1)


#define netxen_gb_enable_tx(config_word)	\
	((config_word) |= 1 << 0)
#define netxen_gb_enable_rx(config_word)	\
	((config_word) |= 1 << 2)
#define netxen_gb_tx_flowctl(config_word)	\
	((config_word) |= 1 << 4)
#define netxen_gb_rx_flowctl(config_word)	\
	((config_word) |= 1 << 5)
#define netxen_gb_tx_reset_pb(config_word)	\
	((config_word) |= 1 << 16)
#define netxen_gb_rx_reset_pb(config_word)	\
	((config_word) |= 1 << 17)
#define netxen_gb_tx_reset_mac(config_word)	\
	((config_word) |= 1 << 18)
#define netxen_gb_rx_reset_mac(config_word)	\
	((config_word) |= 1 << 19)
#define netxen_gb_soft_reset(config_word)	\
	((config_word) |= 1 << 31)

#define netxen_gb_unset_tx_flowctl(config_word)	\
	((config_word) &= ~(1 << 4))
#define netxen_gb_unset_rx_flowctl(config_word)	\
	((config_word) &= ~(1 << 5))

#define netxen_gb_get_tx_synced(config_word)	\
		_netxen_crb_get_bit((config_word), 1)
#define netxen_gb_get_rx_synced(config_word)	\
		_netxen_crb_get_bit((config_word), 3)
#define netxen_gb_get_tx_flowctl(config_word)	\
		_netxen_crb_get_bit((config_word), 4)
#define netxen_gb_get_rx_flowctl(config_word)	\
		_netxen_crb_get_bit((config_word), 5)
#define netxen_gb_get_soft_reset(config_word)	\
		_netxen_crb_get_bit((config_word), 31)


#define netxen_gb_set_duplex(config_word)	\
		((config_word) |= 1 << 0)
#define netxen_gb_set_crc_enable(config_word)	\
		((config_word) |= 1 << 1)
#define netxen_gb_set_padshort(config_word)	\
		((config_word) |= 1 << 2)
#define netxen_gb_set_checklength(config_word)	\
		((config_word) |= 1 << 4)
#define netxen_gb_set_hugeframes(config_word)	\
		((config_word) |= 1 << 5)
#define netxen_gb_set_preamblelen(config_word, val)	\
		((config_word) |= ((val) << 12) & 0xF000)
#define netxen_gb_set_intfmode(config_word, val)		\
		((config_word) |= ((val) << 8) & 0x300)

#define netxen_gb_get_stationaddress_low(config_word) ((config_word) >> 16)

#define netxen_gb_set_mii_mgmt_clockselect(config_word, val)	\
		((config_word) |= ((val) & 0x07))
#define netxen_gb_mii_mgmt_reset(config_word)	\
		((config_word) |= 1 << 31)
#define netxen_gb_mii_mgmt_unset(config_word)	\
		((config_word) &= ~(1 << 31))


#define netxen_gb_mii_mgmt_set_read_cycle(config_word)	\
		((config_word) |= 1 << 0)
#define netxen_gb_mii_mgmt_reg_addr(config_word, val)	\
		((config_word) |= ((val) & 0x1F))
#define netxen_gb_mii_mgmt_phy_addr(config_word, val)	\
		((config_word) |= (((val) & 0x1F) << 8))

#define netxen_get_gb_mii_mgmt_busy(config_word)	\
		_netxen_crb_get_bit(config_word, 0)
#define netxen_get_gb_mii_mgmt_scanning(config_word)	\
		_netxen_crb_get_bit(config_word, 1)
#define netxen_get_gb_mii_mgmt_notvalid(config_word)	\
		_netxen_crb_get_bit(config_word, 2)

#define netxen_xg_set_xg0_mask(config_word)    \
	((config_word) |= 1 << 0)
#define netxen_xg_set_xg1_mask(config_word)    \
	((config_word) |= 1 << 3)

#define netxen_xg_get_xg0_mask(config_word)    \
	_netxen_crb_get_bit((config_word), 0)
#define netxen_xg_get_xg1_mask(config_word)    \
	_netxen_crb_get_bit((config_word), 3)

#define netxen_xg_unset_xg0_mask(config_word)  \
	((config_word) &= ~(1 << 0))
#define netxen_xg_unset_xg1_mask(config_word)  \
	((config_word) &= ~(1 << 3))

#define netxen_gb_set_gb0_mask(config_word)    \
	((config_word) |= 1 << 0)
#define netxen_gb_set_gb1_mask(config_word)    \
	((config_word) |= 1 << 2)
#define netxen_gb_set_gb2_mask(config_word)    \
	((config_word) |= 1 << 4)
#define netxen_gb_set_gb3_mask(config_word)    \
	((config_word) |= 1 << 6)

#define netxen_gb_get_gb0_mask(config_word)    \
	_netxen_crb_get_bit((config_word), 0)
#define netxen_gb_get_gb1_mask(config_word)    \
	_netxen_crb_get_bit((config_word), 2)
#define netxen_gb_get_gb2_mask(config_word)    \
	_netxen_crb_get_bit((config_word), 4)
#define netxen_gb_get_gb3_mask(config_word)    \
	_netxen_crb_get_bit((config_word), 6)

#define netxen_gb_unset_gb0_mask(config_word)  \
	((config_word) &= ~(1 << 0))
#define netxen_gb_unset_gb1_mask(config_word)  \
	((config_word) &= ~(1 << 2))
#define netxen_gb_unset_gb2_mask(config_word)  \
	((config_word) &= ~(1 << 4))
#define netxen_gb_unset_gb3_mask(config_word)  \
	((config_word) &= ~(1 << 6))


typedef enum {
	NETXEN_NIU_GB_MII_MGMT_ADDR_CONTROL = 0,
	NETXEN_NIU_GB_MII_MGMT_ADDR_STATUS = 1,
	NETXEN_NIU_GB_MII_MGMT_ADDR_PHY_ID_0 = 2,
	NETXEN_NIU_GB_MII_MGMT_ADDR_PHY_ID_1 = 3,
	NETXEN_NIU_GB_MII_MGMT_ADDR_AUTONEG = 4,
	NETXEN_NIU_GB_MII_MGMT_ADDR_LNKPART = 5,
	NETXEN_NIU_GB_MII_MGMT_ADDR_AUTONEG_MORE = 6,
	NETXEN_NIU_GB_MII_MGMT_ADDR_NEXTPAGE_XMIT = 7,
	NETXEN_NIU_GB_MII_MGMT_ADDR_LNKPART_NEXTPAGE = 8,
	NETXEN_NIU_GB_MII_MGMT_ADDR_1000BT_CONTROL = 9,
	NETXEN_NIU_GB_MII_MGMT_ADDR_1000BT_STATUS = 10,
	NETXEN_NIU_GB_MII_MGMT_ADDR_EXTENDED_STATUS = 15,
	NETXEN_NIU_GB_MII_MGMT_ADDR_PHY_CONTROL = 16,
	NETXEN_NIU_GB_MII_MGMT_ADDR_PHY_STATUS = 17,
	NETXEN_NIU_GB_MII_MGMT_ADDR_INT_ENABLE = 18,
	NETXEN_NIU_GB_MII_MGMT_ADDR_INT_STATUS = 19,
	NETXEN_NIU_GB_MII_MGMT_ADDR_PHY_CONTROL_MORE = 20,
	NETXEN_NIU_GB_MII_MGMT_ADDR_RECV_ERROR_COUNT = 21,
	NETXEN_NIU_GB_MII_MGMT_ADDR_LED_CONTROL = 24,
	NETXEN_NIU_GB_MII_MGMT_ADDR_LED_OVERRIDE = 25,
	NETXEN_NIU_GB_MII_MGMT_ADDR_PHY_CONTROL_MORE_YET = 26,
	NETXEN_NIU_GB_MII_MGMT_ADDR_PHY_STATUS_MORE = 27
} netxen_niu_phy_register_t;


#define netxen_get_phy_cablelen(config_word) (((config_word) >> 7) & 0x07)
#define netxen_get_phy_speed(config_word) (((config_word) >> 14) & 0x03)

#define netxen_set_phy_speed(config_word, val)	\
		((config_word) |= ((val & 0x03) << 14))
#define netxen_set_phy_duplex(config_word)	\
		((config_word) |= 1 << 13)
#define netxen_clear_phy_duplex(config_word)	\
		((config_word) &= ~(1 << 13))

#define netxen_get_phy_jabber(config_word)	\
		_netxen_crb_get_bit(config_word, 0)
#define netxen_get_phy_polarity(config_word)	\
		_netxen_crb_get_bit(config_word, 1)
#define netxen_get_phy_recvpause(config_word)	\
		_netxen_crb_get_bit(config_word, 2)
#define netxen_get_phy_xmitpause(config_word)	\
		_netxen_crb_get_bit(config_word, 3)
#define netxen_get_phy_energydetect(config_word) \
		_netxen_crb_get_bit(config_word, 4)
#define netxen_get_phy_downshift(config_word)	\
		_netxen_crb_get_bit(config_word, 5)
#define netxen_get_phy_crossover(config_word)	\
		_netxen_crb_get_bit(config_word, 6)
#define netxen_get_phy_link(config_word)	\
		_netxen_crb_get_bit(config_word, 10)
#define netxen_get_phy_resolved(config_word)	\
		_netxen_crb_get_bit(config_word, 11)
#define netxen_get_phy_pagercvd(config_word)	\
		_netxen_crb_get_bit(config_word, 12)
#define netxen_get_phy_duplex(config_word)	\
		_netxen_crb_get_bit(config_word, 13)


#define netxen_get_phy_int_jabber(config_word)	\
		_netxen_crb_get_bit(config_word, 0)
#define netxen_get_phy_int_polarity_changed(config_word)	\
		_netxen_crb_get_bit(config_word, 1)
#define netxen_get_phy_int_energy_detect(config_word)	\
		_netxen_crb_get_bit(config_word, 4)
#define netxen_get_phy_int_downshift(config_word)	\
		_netxen_crb_get_bit(config_word, 5)
#define netxen_get_phy_int_mdi_xover_changed(config_word)	\
		_netxen_crb_get_bit(config_word, 6)
#define netxen_get_phy_int_fifo_over_underflow(config_word)	\
		_netxen_crb_get_bit(config_word, 7)
#define netxen_get_phy_int_false_carrier(config_word)	\
		_netxen_crb_get_bit(config_word, 8)
#define netxen_get_phy_int_symbol_error(config_word)	\
		_netxen_crb_get_bit(config_word, 9)
#define netxen_get_phy_int_link_status_changed(config_word)	\
		_netxen_crb_get_bit(config_word, 10)
#define netxen_get_phy_int_autoneg_completed(config_word)	\
		_netxen_crb_get_bit(config_word, 11)
#define netxen_get_phy_int_page_received(config_word)	\
		_netxen_crb_get_bit(config_word, 12)
#define netxen_get_phy_int_duplex_changed(config_word)	\
		_netxen_crb_get_bit(config_word, 13)
#define netxen_get_phy_int_speed_changed(config_word)	\
		_netxen_crb_get_bit(config_word, 14)
#define netxen_get_phy_int_autoneg_error(config_word)	\
		_netxen_crb_get_bit(config_word, 15)

#define netxen_set_phy_int_link_status_changed(config_word)	\
		((config_word) |= 1 << 10)
#define netxen_set_phy_int_autoneg_completed(config_word)	\
		((config_word) |= 1 << 11)
#define netxen_set_phy_int_speed_changed(config_word)	\
		((config_word) |= 1 << 14)


#define netxen_get_niu_enable_ge(config_word)	\
		_netxen_crb_get_bit(config_word, 1)

#define NETXEN_NIU_NON_PROMISC_MODE	0
#define NETXEN_NIU_PROMISC_MODE		1
#define NETXEN_NIU_ALLMULTI_MODE	2


#define netxen_set_gb_drop_gb0(config_word)	\
		((config_word) |= 1 << 0)
#define netxen_set_gb_drop_gb1(config_word)	\
		((config_word) |= 1 << 1)
#define netxen_set_gb_drop_gb2(config_word)	\
		((config_word) |= 1 << 2)
#define netxen_set_gb_drop_gb3(config_word)	\
		((config_word) |= 1 << 3)

#define netxen_clear_gb_drop_gb0(config_word)	\
		((config_word) &= ~(1 << 0))
#define netxen_clear_gb_drop_gb1(config_word)	\
		((config_word) &= ~(1 << 1))
#define netxen_clear_gb_drop_gb2(config_word)	\
		((config_word) &= ~(1 << 2))
#define netxen_clear_gb_drop_gb3(config_word)	\
		((config_word) &= ~(1 << 3))


#define netxen_xg_soft_reset(config_word)	\
		((config_word) |= 1 << 4)

/* Set promiscuous mode for a GbE interface */
int netxen_niu_set_promiscuous_mode(struct netxen_adapter *adapter,
				    u32 mode);
int netxen_niu_xg_set_promiscuous_mode(struct netxen_adapter *adapter,
				       u32 mode);

/* set the MAC address for a given MAC */
int netxen_niu_macaddr_set(struct netxen_adapter *adapter,
			   netxen_ethernet_macaddr_t addr);

/* XG version */
int netxen_niu_xg_macaddr_set(struct netxen_adapter *adapter,
			      netxen_ethernet_macaddr_t addr);

/* Generic enable for GbE ports. Will detect the speed of the link. */
int netxen_niu_gbe_init_port(struct netxen_adapter *adapter, int port);

int netxen_niu_xg_init_port(struct netxen_adapter *adapter, int port);

/* Disable a GbE interface */
int netxen_niu_disable_gbe_port(struct netxen_adapter *adapter);

int netxen_niu_disable_xg_port(struct netxen_adapter *adapter);

typedef struct {
	unsigned valid;
	unsigned start_128M;
	unsigned end_128M;
	unsigned start_2M;
} crb_128M_2M_sub_block_map_t;

typedef struct {
	crb_128M_2M_sub_block_map_t sub_block[16];
} crb_128M_2M_block_map_t;

#endif				/* __NETXEN_NIC_HW_H_ */
