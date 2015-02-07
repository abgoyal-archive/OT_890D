


#include <linux/init.h>
#include <linux/pci.h>
#include <linux/firmware.h>
#include <linux/etherdevice.h>
#include <linux/delay.h>
#include <linux/completion.h>
#include <net/mac80211.h>

#include "p54.h"
#include "p54pci.h"

MODULE_AUTHOR("Michael Wu <flamingice@sourmilk.net>");
MODULE_DESCRIPTION("Prism54 PCI wireless driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("prism54pci");
MODULE_FIRMWARE("isl3886pci");

static struct pci_device_id p54p_table[] __devinitdata = {
	/* Intersil PRISM Duette/Prism GT Wireless LAN adapter */
	{ PCI_DEVICE(0x1260, 0x3890) },
	/* 3COM 3CRWE154G72 Wireless LAN adapter */
	{ PCI_DEVICE(0x10b7, 0x6001) },
	/* Intersil PRISM Indigo Wireless LAN adapter */
	{ PCI_DEVICE(0x1260, 0x3877) },
	/* Intersil PRISM Javelin/Xbow Wireless LAN adapter */
	{ PCI_DEVICE(0x1260, 0x3886) },
	{ },
};

MODULE_DEVICE_TABLE(pci, p54p_table);

static int p54p_upload_firmware(struct ieee80211_hw *dev)
{
	struct p54p_priv *priv = dev->priv;
	__le32 reg;
	int err;
	__le32 *data;
	u32 remains, left, device_addr;

	P54P_WRITE(int_enable, cpu_to_le32(0));
	P54P_READ(int_enable);
	udelay(10);

	reg = P54P_READ(ctrl_stat);
	reg &= cpu_to_le32(~ISL38XX_CTRL_STAT_RESET);
	reg &= cpu_to_le32(~ISL38XX_CTRL_STAT_RAMBOOT);
	P54P_WRITE(ctrl_stat, reg);
	P54P_READ(ctrl_stat);
	udelay(10);

	reg |= cpu_to_le32(ISL38XX_CTRL_STAT_RESET);
	P54P_WRITE(ctrl_stat, reg);
	wmb();
	udelay(10);

	reg &= cpu_to_le32(~ISL38XX_CTRL_STAT_RESET);
	P54P_WRITE(ctrl_stat, reg);
	wmb();

	/* wait for the firmware to reset properly */
	mdelay(10);

	err = p54_parse_firmware(dev, priv->firmware);
	if (err)
		return err;

	data = (__le32 *) priv->firmware->data;
	remains = priv->firmware->size;
	device_addr = ISL38XX_DEV_FIRMWARE_ADDR;
	while (remains) {
		u32 i = 0;
		left = min((u32)0x1000, remains);
		P54P_WRITE(direct_mem_base, cpu_to_le32(device_addr));
		P54P_READ(int_enable);

		device_addr += 0x1000;
		while (i < left) {
			P54P_WRITE(direct_mem_win[i], *data++);
			i += sizeof(u32);
		}

		remains -= left;
		P54P_READ(int_enable);
	}

	reg = P54P_READ(ctrl_stat);
	reg &= cpu_to_le32(~ISL38XX_CTRL_STAT_CLKRUN);
	reg &= cpu_to_le32(~ISL38XX_CTRL_STAT_RESET);
	reg |= cpu_to_le32(ISL38XX_CTRL_STAT_RAMBOOT);
	P54P_WRITE(ctrl_stat, reg);
	P54P_READ(ctrl_stat);
	udelay(10);

	reg |= cpu_to_le32(ISL38XX_CTRL_STAT_RESET);
	P54P_WRITE(ctrl_stat, reg);
	wmb();
	udelay(10);

	reg &= cpu_to_le32(~ISL38XX_CTRL_STAT_RESET);
	P54P_WRITE(ctrl_stat, reg);
	wmb();
	udelay(10);

	/* wait for the firmware to boot properly */
	mdelay(100);

	return 0;
}

static void p54p_refill_rx_ring(struct ieee80211_hw *dev,
	int ring_index, struct p54p_desc *ring, u32 ring_limit,
	struct sk_buff **rx_buf)
{
	struct p54p_priv *priv = dev->priv;
	struct p54p_ring_control *ring_control = priv->ring_control;
	u32 limit, idx, i;

	idx = le32_to_cpu(ring_control->host_idx[ring_index]);
	limit = idx;
	limit -= le32_to_cpu(ring_control->device_idx[ring_index]);
	limit = ring_limit - limit;

	i = idx % ring_limit;
	while (limit-- > 1) {
		struct p54p_desc *desc = &ring[i];

		if (!desc->host_addr) {
			struct sk_buff *skb;
			dma_addr_t mapping;
			skb = dev_alloc_skb(priv->common.rx_mtu + 32);
			if (!skb)
				break;

			mapping = pci_map_single(priv->pdev,
						 skb_tail_pointer(skb),
						 priv->common.rx_mtu + 32,
						 PCI_DMA_FROMDEVICE);
			desc->host_addr = cpu_to_le32(mapping);
			desc->device_addr = 0;	// FIXME: necessary?
			desc->len = cpu_to_le16(priv->common.rx_mtu + 32);
			desc->flags = 0;
			rx_buf[i] = skb;
		}

		i++;
		idx++;
		i %= ring_limit;
	}

	wmb();
	ring_control->host_idx[ring_index] = cpu_to_le32(idx);
}

static void p54p_check_rx_ring(struct ieee80211_hw *dev, u32 *index,
	int ring_index, struct p54p_desc *ring, u32 ring_limit,
	struct sk_buff **rx_buf)
{
	struct p54p_priv *priv = dev->priv;
	struct p54p_ring_control *ring_control = priv->ring_control;
	struct p54p_desc *desc;
	u32 idx, i;

	i = (*index) % ring_limit;
	(*index) = idx = le32_to_cpu(ring_control->device_idx[ring_index]);
	idx %= ring_limit;
	while (i != idx) {
		u16 len;
		struct sk_buff *skb;
		desc = &ring[i];
		len = le16_to_cpu(desc->len);
		skb = rx_buf[i];

		if (!skb) {
			i++;
			i %= ring_limit;
			continue;
		}
		skb_put(skb, len);

		if (p54_rx(dev, skb)) {
			pci_unmap_single(priv->pdev,
					 le32_to_cpu(desc->host_addr),
					 priv->common.rx_mtu + 32,
					 PCI_DMA_FROMDEVICE);
			rx_buf[i] = NULL;
			desc->host_addr = 0;
		} else {
			skb_trim(skb, 0);
			desc->len = cpu_to_le16(priv->common.rx_mtu + 32);
		}

		i++;
		i %= ring_limit;
	}

	p54p_refill_rx_ring(dev, ring_index, ring, ring_limit, rx_buf);
}

/* caller must hold priv->lock */
static void p54p_check_tx_ring(struct ieee80211_hw *dev, u32 *index,
	int ring_index, struct p54p_desc *ring, u32 ring_limit,
	void **tx_buf)
{
	struct p54p_priv *priv = dev->priv;
	struct p54p_ring_control *ring_control = priv->ring_control;
	struct p54p_desc *desc;
	u32 idx, i;

	i = (*index) % ring_limit;
	(*index) = idx = le32_to_cpu(ring_control->device_idx[1]);
	idx %= ring_limit;

	while (i != idx) {
		desc = &ring[i];
		if (tx_buf[i])
			if (FREE_AFTER_TX((struct sk_buff *) tx_buf[i]))
				p54_free_skb(dev, tx_buf[i]);
		tx_buf[i] = NULL;

		pci_unmap_single(priv->pdev, le32_to_cpu(desc->host_addr),
				 le16_to_cpu(desc->len), PCI_DMA_TODEVICE);

		desc->host_addr = 0;
		desc->device_addr = 0;
		desc->len = 0;
		desc->flags = 0;

		i++;
		i %= ring_limit;
	}
}

static void p54p_rx_tasklet(unsigned long dev_id)
{
	struct ieee80211_hw *dev = (struct ieee80211_hw *)dev_id;
	struct p54p_priv *priv = dev->priv;
	struct p54p_ring_control *ring_control = priv->ring_control;

	p54p_check_rx_ring(dev, &priv->rx_idx_mgmt, 2, ring_control->rx_mgmt,
		ARRAY_SIZE(ring_control->rx_mgmt), priv->rx_buf_mgmt);

	p54p_check_rx_ring(dev, &priv->rx_idx_data, 0, ring_control->rx_data,
		ARRAY_SIZE(ring_control->rx_data), priv->rx_buf_data);

	wmb();
	P54P_WRITE(dev_int, cpu_to_le32(ISL38XX_DEV_INT_UPDATE));
}

static irqreturn_t p54p_interrupt(int irq, void *dev_id)
{
	struct ieee80211_hw *dev = dev_id;
	struct p54p_priv *priv = dev->priv;
	struct p54p_ring_control *ring_control = priv->ring_control;
	__le32 reg;

	spin_lock(&priv->lock);
	reg = P54P_READ(int_ident);
	if (unlikely(reg == cpu_to_le32(0xFFFFFFFF))) {
		spin_unlock(&priv->lock);
		return IRQ_HANDLED;
	}

	P54P_WRITE(int_ack, reg);

	reg &= P54P_READ(int_enable);

	if (reg & cpu_to_le32(ISL38XX_INT_IDENT_UPDATE)) {
		p54p_check_tx_ring(dev, &priv->tx_idx_mgmt,
				   3, ring_control->tx_mgmt,
				   ARRAY_SIZE(ring_control->tx_mgmt),
				   priv->tx_buf_mgmt);

		p54p_check_tx_ring(dev, &priv->tx_idx_data,
				   1, ring_control->tx_data,
				   ARRAY_SIZE(ring_control->tx_data),
				   priv->tx_buf_data);

		tasklet_schedule(&priv->rx_tasklet);

	} else if (reg & cpu_to_le32(ISL38XX_INT_IDENT_INIT))
		complete(&priv->boot_comp);

	spin_unlock(&priv->lock);

	return reg ? IRQ_HANDLED : IRQ_NONE;
}

static void p54p_tx(struct ieee80211_hw *dev, struct sk_buff *skb)
{
	struct p54p_priv *priv = dev->priv;
	struct p54p_ring_control *ring_control = priv->ring_control;
	unsigned long flags;
	struct p54p_desc *desc;
	dma_addr_t mapping;
	u32 device_idx, idx, i;

	spin_lock_irqsave(&priv->lock, flags);

	device_idx = le32_to_cpu(ring_control->device_idx[1]);
	idx = le32_to_cpu(ring_control->host_idx[1]);
	i = idx % ARRAY_SIZE(ring_control->tx_data);

	priv->tx_buf_data[i] = skb;
	mapping = pci_map_single(priv->pdev, skb->data, skb->len,
				 PCI_DMA_TODEVICE);
	desc = &ring_control->tx_data[i];
	desc->host_addr = cpu_to_le32(mapping);
	desc->device_addr = ((struct p54_hdr *)skb->data)->req_id;
	desc->len = cpu_to_le16(skb->len);
	desc->flags = 0;

	wmb();
	ring_control->host_idx[1] = cpu_to_le32(idx + 1);
	spin_unlock_irqrestore(&priv->lock, flags);

	P54P_WRITE(dev_int, cpu_to_le32(ISL38XX_DEV_INT_UPDATE));
	P54P_READ(dev_int);
}

static void p54p_stop(struct ieee80211_hw *dev)
{
	struct p54p_priv *priv = dev->priv;
	struct p54p_ring_control *ring_control = priv->ring_control;
	unsigned int i;
	struct p54p_desc *desc;

	tasklet_kill(&priv->rx_tasklet);

	P54P_WRITE(int_enable, cpu_to_le32(0));
	P54P_READ(int_enable);
	udelay(10);

	free_irq(priv->pdev->irq, dev);

	P54P_WRITE(dev_int, cpu_to_le32(ISL38XX_DEV_INT_RESET));

	for (i = 0; i < ARRAY_SIZE(priv->rx_buf_data); i++) {
		desc = &ring_control->rx_data[i];
		if (desc->host_addr)
			pci_unmap_single(priv->pdev,
					 le32_to_cpu(desc->host_addr),
					 priv->common.rx_mtu + 32,
					 PCI_DMA_FROMDEVICE);
		kfree_skb(priv->rx_buf_data[i]);
		priv->rx_buf_data[i] = NULL;
	}

	for (i = 0; i < ARRAY_SIZE(priv->rx_buf_mgmt); i++) {
		desc = &ring_control->rx_mgmt[i];
		if (desc->host_addr)
			pci_unmap_single(priv->pdev,
					 le32_to_cpu(desc->host_addr),
					 priv->common.rx_mtu + 32,
					 PCI_DMA_FROMDEVICE);
		kfree_skb(priv->rx_buf_mgmt[i]);
		priv->rx_buf_mgmt[i] = NULL;
	}

	for (i = 0; i < ARRAY_SIZE(priv->tx_buf_data); i++) {
		desc = &ring_control->tx_data[i];
		if (desc->host_addr)
			pci_unmap_single(priv->pdev,
					 le32_to_cpu(desc->host_addr),
					 le16_to_cpu(desc->len),
					 PCI_DMA_TODEVICE);

		p54_free_skb(dev, priv->tx_buf_data[i]);
		priv->tx_buf_data[i] = NULL;
	}

	for (i = 0; i < ARRAY_SIZE(priv->tx_buf_mgmt); i++) {
		desc = &ring_control->tx_mgmt[i];
		if (desc->host_addr)
			pci_unmap_single(priv->pdev,
					 le32_to_cpu(desc->host_addr),
					 le16_to_cpu(desc->len),
					 PCI_DMA_TODEVICE);

		p54_free_skb(dev, priv->tx_buf_mgmt[i]);
		priv->tx_buf_mgmt[i] = NULL;
	}

	memset(ring_control, 0, sizeof(*ring_control));
}

static int p54p_open(struct ieee80211_hw *dev)
{
	struct p54p_priv *priv = dev->priv;
	int err;

	init_completion(&priv->boot_comp);
	err = request_irq(priv->pdev->irq, &p54p_interrupt,
			  IRQF_SHARED, "p54pci", dev);
	if (err) {
		printk(KERN_ERR "%s: failed to register IRQ handler\n",
		       wiphy_name(dev->wiphy));
		return err;
	}

	memset(priv->ring_control, 0, sizeof(*priv->ring_control));
	err = p54p_upload_firmware(dev);
	if (err) {
		free_irq(priv->pdev->irq, dev);
		return err;
	}
	priv->rx_idx_data = priv->tx_idx_data = 0;
	priv->rx_idx_mgmt = priv->tx_idx_mgmt = 0;

	p54p_refill_rx_ring(dev, 0, priv->ring_control->rx_data,
		ARRAY_SIZE(priv->ring_control->rx_data), priv->rx_buf_data);

	p54p_refill_rx_ring(dev, 2, priv->ring_control->rx_mgmt,
		ARRAY_SIZE(priv->ring_control->rx_mgmt), priv->rx_buf_mgmt);

	P54P_WRITE(ring_control_base, cpu_to_le32(priv->ring_control_dma));
	P54P_READ(ring_control_base);
	wmb();
	udelay(10);

	P54P_WRITE(int_enable, cpu_to_le32(ISL38XX_INT_IDENT_INIT));
	P54P_READ(int_enable);
	wmb();
	udelay(10);

	P54P_WRITE(dev_int, cpu_to_le32(ISL38XX_DEV_INT_RESET));
	P54P_READ(dev_int);

	if (!wait_for_completion_interruptible_timeout(&priv->boot_comp, HZ)) {
		printk(KERN_ERR "%s: Cannot boot firmware!\n",
		       wiphy_name(dev->wiphy));
		p54p_stop(dev);
		return -ETIMEDOUT;
	}

	P54P_WRITE(int_enable, cpu_to_le32(ISL38XX_INT_IDENT_UPDATE));
	P54P_READ(int_enable);
	wmb();
	udelay(10);

	P54P_WRITE(dev_int, cpu_to_le32(ISL38XX_DEV_INT_UPDATE));
	P54P_READ(dev_int);
	wmb();
	udelay(10);

	return 0;
}

static int __devinit p54p_probe(struct pci_dev *pdev,
				const struct pci_device_id *id)
{
	struct p54p_priv *priv;
	struct ieee80211_hw *dev;
	unsigned long mem_addr, mem_len;
	int err;

	err = pci_enable_device(pdev);
	if (err) {
		printk(KERN_ERR "%s (p54pci): Cannot enable new PCI device\n",
		       pci_name(pdev));
		return err;
	}

	mem_addr = pci_resource_start(pdev, 0);
	mem_len = pci_resource_len(pdev, 0);
	if (mem_len < sizeof(struct p54p_csr)) {
		printk(KERN_ERR "%s (p54pci): Too short PCI resources\n",
		       pci_name(pdev));
		goto err_disable_dev;
	}

	err = pci_request_regions(pdev, "p54pci");
	if (err) {
		printk(KERN_ERR "%s (p54pci): Cannot obtain PCI resources\n",
		       pci_name(pdev));
		goto err_disable_dev;
	}

	if (pci_set_dma_mask(pdev, DMA_32BIT_MASK) ||
	    pci_set_consistent_dma_mask(pdev, DMA_32BIT_MASK)) {
		printk(KERN_ERR "%s (p54pci): No suitable DMA available\n",
		       pci_name(pdev));
		goto err_free_reg;
	}

	pci_set_master(pdev);
	pci_try_set_mwi(pdev);

	pci_write_config_byte(pdev, 0x40, 0);
	pci_write_config_byte(pdev, 0x41, 0);

	dev = p54_init_common(sizeof(*priv));
	if (!dev) {
		printk(KERN_ERR "%s (p54pci): ieee80211 alloc failed\n",
		       pci_name(pdev));
		err = -ENOMEM;
		goto err_free_reg;
	}

	priv = dev->priv;
	priv->pdev = pdev;

	SET_IEEE80211_DEV(dev, &pdev->dev);
	pci_set_drvdata(pdev, dev);

	priv->map = ioremap(mem_addr, mem_len);
	if (!priv->map) {
		printk(KERN_ERR "%s (p54pci): Cannot map device memory\n",
		       pci_name(pdev));
		err = -EINVAL;	// TODO: use a better error code?
		goto err_free_dev;
	}

	priv->ring_control = pci_alloc_consistent(pdev, sizeof(*priv->ring_control),
						  &priv->ring_control_dma);
	if (!priv->ring_control) {
		printk(KERN_ERR "%s (p54pci): Cannot allocate rings\n",
		       pci_name(pdev));
		err = -ENOMEM;
		goto err_iounmap;
	}
	priv->common.open = p54p_open;
	priv->common.stop = p54p_stop;
	priv->common.tx = p54p_tx;

	spin_lock_init(&priv->lock);
	tasklet_init(&priv->rx_tasklet, p54p_rx_tasklet, (unsigned long)dev);

	err = request_firmware(&priv->firmware, "isl3886pci",
			       &priv->pdev->dev);
	if (err) {
		printk(KERN_ERR "%s (p54pci): cannot find firmware "
			"(isl3886pci)\n", pci_name(priv->pdev));
		err = request_firmware(&priv->firmware, "isl3886",
				       &priv->pdev->dev);
		if (err)
			goto err_free_common;
	}

	err = p54p_open(dev);
	if (err)
		goto err_free_common;
	err = p54_read_eeprom(dev);
	p54p_stop(dev);
	if (err)
		goto err_free_common;

	err = ieee80211_register_hw(dev);
	if (err) {
		printk(KERN_ERR "%s (p54pci): Cannot register netdevice\n",
		       pci_name(pdev));
		goto err_free_common;
	}

	return 0;

 err_free_common:
	release_firmware(priv->firmware);
	p54_free_common(dev);
	pci_free_consistent(pdev, sizeof(*priv->ring_control),
			    priv->ring_control, priv->ring_control_dma);

 err_iounmap:
	iounmap(priv->map);

 err_free_dev:
	pci_set_drvdata(pdev, NULL);
	ieee80211_free_hw(dev);

 err_free_reg:
	pci_release_regions(pdev);
 err_disable_dev:
	pci_disable_device(pdev);
	return err;
}

static void __devexit p54p_remove(struct pci_dev *pdev)
{
	struct ieee80211_hw *dev = pci_get_drvdata(pdev);
	struct p54p_priv *priv;

	if (!dev)
		return;

	ieee80211_unregister_hw(dev);
	priv = dev->priv;
	release_firmware(priv->firmware);
	pci_free_consistent(pdev, sizeof(*priv->ring_control),
			    priv->ring_control, priv->ring_control_dma);
	p54_free_common(dev);
	iounmap(priv->map);
	pci_release_regions(pdev);
	pci_disable_device(pdev);
	ieee80211_free_hw(dev);
}

#ifdef CONFIG_PM
static int p54p_suspend(struct pci_dev *pdev, pm_message_t state)
{
	struct ieee80211_hw *dev = pci_get_drvdata(pdev);
	struct p54p_priv *priv = dev->priv;

	if (priv->common.mode != NL80211_IFTYPE_UNSPECIFIED) {
		ieee80211_stop_queues(dev);
		p54p_stop(dev);
	}

	pci_save_state(pdev);
	pci_set_power_state(pdev, pci_choose_state(pdev, state));
	return 0;
}

static int p54p_resume(struct pci_dev *pdev)
{
	struct ieee80211_hw *dev = pci_get_drvdata(pdev);
	struct p54p_priv *priv = dev->priv;

	pci_set_power_state(pdev, PCI_D0);
	pci_restore_state(pdev);

	if (priv->common.mode != NL80211_IFTYPE_UNSPECIFIED) {
		p54p_open(dev);
		ieee80211_wake_queues(dev);
	}

	return 0;
}
#endif /* CONFIG_PM */

static struct pci_driver p54p_driver = {
	.name		= "p54pci",
	.id_table	= p54p_table,
	.probe		= p54p_probe,
	.remove		= __devexit_p(p54p_remove),
#ifdef CONFIG_PM
	.suspend	= p54p_suspend,
	.resume		= p54p_resume,
#endif /* CONFIG_PM */
};

static int __init p54p_init(void)
{
	return pci_register_driver(&p54p_driver);
}

static void __exit p54p_exit(void)
{
	pci_unregister_driver(&p54p_driver);
}

module_init(p54p_init);
module_exit(p54p_exit);
