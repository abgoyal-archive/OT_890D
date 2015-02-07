

/* Including this file like a header is a temporary hack, I promise. -- CHS */
#ifndef ATLX_C
#define ATLX_C

#include <linux/device.h>
#include <linux/errno.h>
#include <linux/etherdevice.h>
#include <linux/if.h>
#include <linux/netdevice.h>
#include <linux/socket.h>
#include <linux/sockios.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/workqueue.h>

#include "atlx.h"

static struct atlx_spi_flash_dev flash_table[] = {
/*	MFR_NAME  WRSR  READ  PRGM  WREN  WRDI  RDSR  RDID  SEC_ERS CHIP_ERS */
	{"Atmel", 0x00, 0x03, 0x02, 0x06, 0x04, 0x05, 0x15, 0x52,   0x62},
	{"SST",   0x01, 0x03, 0x02, 0x06, 0x04, 0x05, 0x90, 0x20,   0x60},
	{"ST",    0x01, 0x03, 0x02, 0x06, 0x04, 0x05, 0xAB, 0xD8,   0xC7},
};

static int atlx_ioctl(struct net_device *netdev, struct ifreq *ifr, int cmd)
{
	switch (cmd) {
	case SIOCGMIIPHY:
	case SIOCGMIIREG:
	case SIOCSMIIREG:
		return atlx_mii_ioctl(netdev, ifr, cmd);
	default:
		return -EOPNOTSUPP;
	}
}

static int atlx_set_mac(struct net_device *netdev, void *p)
{
	struct atlx_adapter *adapter = netdev_priv(netdev);
	struct sockaddr *addr = p;

	if (netif_running(netdev))
		return -EBUSY;

	if (!is_valid_ether_addr(addr->sa_data))
		return -EADDRNOTAVAIL;

	memcpy(netdev->dev_addr, addr->sa_data, netdev->addr_len);
	memcpy(adapter->hw.mac_addr, addr->sa_data, netdev->addr_len);

	atlx_set_mac_addr(&adapter->hw);
	return 0;
}

static void atlx_check_for_link(struct atlx_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	u16 phy_data = 0;

	spin_lock(&adapter->lock);
	adapter->phy_timer_pending = false;
	atlx_read_phy_reg(&adapter->hw, MII_BMSR, &phy_data);
	atlx_read_phy_reg(&adapter->hw, MII_BMSR, &phy_data);
	spin_unlock(&adapter->lock);

	/* notify upper layer link down ASAP */
	if (!(phy_data & BMSR_LSTATUS)) {
		/* Link Down */
		if (netif_carrier_ok(netdev)) {
			/* old link state: Up */
			dev_info(&adapter->pdev->dev, "%s link is down\n",
				netdev->name);
			adapter->link_speed = SPEED_0;
			netif_carrier_off(netdev);
		}
	}
	schedule_work(&adapter->link_chg_task);
}

static void atlx_set_multi(struct net_device *netdev)
{
	struct atlx_adapter *adapter = netdev_priv(netdev);
	struct atlx_hw *hw = &adapter->hw;
	struct dev_mc_list *mc_ptr;
	u32 rctl;
	u32 hash_value;

	/* Check for Promiscuous and All Multicast modes */
	rctl = ioread32(hw->hw_addr + REG_MAC_CTRL);
	if (netdev->flags & IFF_PROMISC)
		rctl |= MAC_CTRL_PROMIS_EN;
	else if (netdev->flags & IFF_ALLMULTI) {
		rctl |= MAC_CTRL_MC_ALL_EN;
		rctl &= ~MAC_CTRL_PROMIS_EN;
	} else
		rctl &= ~(MAC_CTRL_PROMIS_EN | MAC_CTRL_MC_ALL_EN);

	iowrite32(rctl, hw->hw_addr + REG_MAC_CTRL);

	/* clear the old settings from the multicast hash table */
	iowrite32(0, hw->hw_addr + REG_RX_HASH_TABLE);
	iowrite32(0, (hw->hw_addr + REG_RX_HASH_TABLE) + (1 << 2));

	/* compute mc addresses' hash value ,and put it into hash table */
	for (mc_ptr = netdev->mc_list; mc_ptr; mc_ptr = mc_ptr->next) {
		hash_value = atlx_hash_mc_addr(hw, mc_ptr->dmi_addr);
		atlx_hash_set(hw, hash_value);
	}
}

static void atlx_irq_enable(struct atlx_adapter *adapter)
{
	iowrite32(IMR_NORMAL_MASK, adapter->hw.hw_addr + REG_IMR);
	ioread32(adapter->hw.hw_addr + REG_IMR);
}

static void atlx_irq_disable(struct atlx_adapter *adapter)
{
	iowrite32(0, adapter->hw.hw_addr + REG_IMR);
	ioread32(adapter->hw.hw_addr + REG_IMR);
	synchronize_irq(adapter->pdev->irq);
}

static void atlx_clear_phy_int(struct atlx_adapter *adapter)
{
	u16 phy_data;
	unsigned long flags;

	spin_lock_irqsave(&adapter->lock, flags);
	atlx_read_phy_reg(&adapter->hw, 19, &phy_data);
	spin_unlock_irqrestore(&adapter->lock, flags);
}

static void atlx_tx_timeout(struct net_device *netdev)
{
	struct atlx_adapter *adapter = netdev_priv(netdev);
	/* Do the reset outside of interrupt context */
	schedule_work(&adapter->tx_timeout_task);
}

static void atlx_link_chg_task(struct work_struct *work)
{
	struct atlx_adapter *adapter;
	unsigned long flags;

	adapter = container_of(work, struct atlx_adapter, link_chg_task);

	spin_lock_irqsave(&adapter->lock, flags);
	atlx_check_link(adapter);
	spin_unlock_irqrestore(&adapter->lock, flags);
}

static void atlx_vlan_rx_register(struct net_device *netdev,
	struct vlan_group *grp)
{
	struct atlx_adapter *adapter = netdev_priv(netdev);
	unsigned long flags;
	u32 ctrl;

	spin_lock_irqsave(&adapter->lock, flags);
	/* atlx_irq_disable(adapter); FIXME: confirm/remove */
	adapter->vlgrp = grp;

	if (grp) {
		/* enable VLAN tag insert/strip */
		ctrl = ioread32(adapter->hw.hw_addr + REG_MAC_CTRL);
		ctrl |= MAC_CTRL_RMV_VLAN;
		iowrite32(ctrl, adapter->hw.hw_addr + REG_MAC_CTRL);
	} else {
		/* disable VLAN tag insert/strip */
		ctrl = ioread32(adapter->hw.hw_addr + REG_MAC_CTRL);
		ctrl &= ~MAC_CTRL_RMV_VLAN;
		iowrite32(ctrl, adapter->hw.hw_addr + REG_MAC_CTRL);
	}

	/* atlx_irq_enable(adapter); FIXME */
	spin_unlock_irqrestore(&adapter->lock, flags);
}

static void atlx_restore_vlan(struct atlx_adapter *adapter)
{
	atlx_vlan_rx_register(adapter->netdev, adapter->vlgrp);
}

#endif /* ATLX_C */