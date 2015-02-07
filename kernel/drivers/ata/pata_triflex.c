

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/blkdev.h>
#include <linux/delay.h>
#include <scsi/scsi_host.h>
#include <linux/libata.h>

#define DRV_NAME "pata_triflex"
#define DRV_VERSION "0.2.8"


static int triflex_prereset(struct ata_link *link, unsigned long deadline)
{
	static const struct pci_bits triflex_enable_bits[] = {
		{ 0x80, 1, 0x01, 0x01 },
		{ 0x80, 1, 0x02, 0x02 }
	};

	struct ata_port *ap = link->ap;
	struct pci_dev *pdev = to_pci_dev(ap->host->dev);

	if (!pci_test_config_bits(pdev, &triflex_enable_bits[ap->port_no]))
		return -ENOENT;

	return ata_sff_prereset(link, deadline);
}




static void triflex_load_timing(struct ata_port *ap, struct ata_device *adev, int speed)
{
	struct pci_dev *pdev = to_pci_dev(ap->host->dev);
	u32 timing = 0;
	u32 triflex_timing, old_triflex_timing;
	int channel_offset = ap->port_no ? 0x74: 0x70;
	unsigned int is_slave	= (adev->devno != 0);


	pci_read_config_dword(pdev, channel_offset, &old_triflex_timing);
	triflex_timing = old_triflex_timing;

	switch(speed)
	{
		case XFER_MW_DMA_2:
			timing = 0x0103;break;
		case XFER_MW_DMA_1:
			timing = 0x0203;break;
		case XFER_MW_DMA_0:
			timing = 0x0808;break;
		case XFER_SW_DMA_2:
		case XFER_SW_DMA_1:
		case XFER_SW_DMA_0:
			timing = 0x0F0F;break;
		case XFER_PIO_4:
			timing = 0x0202;break;
		case XFER_PIO_3:
			timing = 0x0204;break;
		case XFER_PIO_2:
			timing = 0x0404;break;
		case XFER_PIO_1:
			timing = 0x0508;break;
		case XFER_PIO_0:
			timing = 0x0808;break;
		default:
			BUG();
	}
	triflex_timing &= ~ (0xFFFF << (16 * is_slave));
	triflex_timing |= (timing << (16 * is_slave));

	if (triflex_timing != old_triflex_timing)
		pci_write_config_dword(pdev, channel_offset, triflex_timing);
}

static void triflex_set_piomode(struct ata_port *ap, struct ata_device *adev)
{
	triflex_load_timing(ap, adev, adev->pio_mode);
}


static void triflex_bmdma_start(struct ata_queued_cmd *qc)
{
	triflex_load_timing(qc->ap, qc->dev, qc->dev->dma_mode);
	ata_bmdma_start(qc);
}


static void triflex_bmdma_stop(struct ata_queued_cmd *qc)
{
	ata_bmdma_stop(qc);
	triflex_load_timing(qc->ap, qc->dev, qc->dev->pio_mode);
}

static struct scsi_host_template triflex_sht = {
	ATA_BMDMA_SHT(DRV_NAME),
};

static struct ata_port_operations triflex_port_ops = {
	.inherits	= &ata_bmdma_port_ops,
	.bmdma_start 	= triflex_bmdma_start,
	.bmdma_stop	= triflex_bmdma_stop,
	.cable_detect	= ata_cable_40wire,
	.set_piomode	= triflex_set_piomode,
	.prereset	= triflex_prereset,
};

static int triflex_init_one(struct pci_dev *dev, const struct pci_device_id *id)
{
	static const struct ata_port_info info = {
		.flags = ATA_FLAG_SLAVE_POSS,
		.pio_mask = 0x1f,
		.mwdma_mask = 0x07,
		.port_ops = &triflex_port_ops
	};
	const struct ata_port_info *ppi[] = { &info, NULL };
	static int printed_version;

	if (!printed_version++)
		dev_printk(KERN_DEBUG, &dev->dev, "version " DRV_VERSION "\n");

	return ata_pci_sff_init_one(dev, ppi, &triflex_sht, NULL);
}

static const struct pci_device_id triflex[] = {
	{ PCI_VDEVICE(COMPAQ, PCI_DEVICE_ID_COMPAQ_TRIFLEX_IDE), },

	{ },
};

static struct pci_driver triflex_pci_driver = {
	.name 		= DRV_NAME,
	.id_table	= triflex,
	.probe 		= triflex_init_one,
	.remove		= ata_pci_remove_one,
#ifdef CONFIG_PM
	.suspend	= ata_pci_device_suspend,
	.resume		= ata_pci_device_resume,
#endif
};

static int __init triflex_init(void)
{
	return pci_register_driver(&triflex_pci_driver);
}

static void __exit triflex_exit(void)
{
	pci_unregister_driver(&triflex_pci_driver);
}

MODULE_AUTHOR("Alan Cox");
MODULE_DESCRIPTION("low-level driver for Compaq Triflex");
MODULE_LICENSE("GPL");
MODULE_DEVICE_TABLE(pci, triflex);
MODULE_VERSION(DRV_VERSION);

module_init(triflex_init);
module_exit(triflex_exit);
