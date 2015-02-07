

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/blkdev.h>
#include <linux/delay.h>
#include <scsi/scsi_host.h>
#include <linux/libata.h>

#define DRV_NAME	"pata_rz1000"
#define DRV_VERSION	"0.2.4"



static int rz1000_set_mode(struct ata_link *link, struct ata_device **unused)
{
	struct ata_device *dev;

	ata_for_each_dev(dev, link, ENABLED) {
		/* We don't really care */
		dev->pio_mode = XFER_PIO_0;
		dev->xfer_mode = XFER_PIO_0;
		dev->xfer_shift = ATA_SHIFT_PIO;
		dev->flags |= ATA_DFLAG_PIO;
		ata_dev_printk(dev, KERN_INFO, "configured for PIO\n");
	}
	return 0;
}


static struct scsi_host_template rz1000_sht = {
	ATA_PIO_SHT(DRV_NAME),
};

static struct ata_port_operations rz1000_port_ops = {
	.inherits	= &ata_sff_port_ops,
	.cable_detect	= ata_cable_40wire,
	.set_mode	= rz1000_set_mode,
};

static int rz1000_fifo_disable(struct pci_dev *pdev)
{
	u16 reg;
	/* Be exceptionally paranoid as we must be sure to apply the fix */
	if (pci_read_config_word(pdev, 0x40, &reg) != 0)
		return -1;
	reg &= 0xDFFF;
	if (pci_write_config_word(pdev, 0x40, reg) != 0)
		return -1;
	printk(KERN_INFO DRV_NAME ": disabled chipset readahead.\n");
	return 0;
}


static int rz1000_init_one (struct pci_dev *pdev, const struct pci_device_id *ent)
{
	static int printed_version;
	static const struct ata_port_info info = {
		.flags = ATA_FLAG_SLAVE_POSS,
		.pio_mask = 0x1f,
		.port_ops = &rz1000_port_ops
	};
	const struct ata_port_info *ppi[] = { &info, NULL };

	if (!printed_version++)
		printk(KERN_DEBUG DRV_NAME " version " DRV_VERSION "\n");

	if (rz1000_fifo_disable(pdev) == 0)
		return ata_pci_sff_init_one(pdev, ppi, &rz1000_sht, NULL);

	printk(KERN_ERR DRV_NAME ": failed to disable read-ahead on chipset..\n");
	/* Not safe to use so skip */
	return -ENODEV;
}

#ifdef CONFIG_PM
static int rz1000_reinit_one(struct pci_dev *pdev)
{
	/* If this fails on resume (which is a "cant happen" case), we
	   must stop as any progress risks data loss */
	if (rz1000_fifo_disable(pdev))
		panic("rz1000 fifo");
	return ata_pci_device_resume(pdev);
}
#endif

static const struct pci_device_id pata_rz1000[] = {
	{ PCI_VDEVICE(PCTECH, PCI_DEVICE_ID_PCTECH_RZ1000), },
	{ PCI_VDEVICE(PCTECH, PCI_DEVICE_ID_PCTECH_RZ1001), },

	{ },
};

static struct pci_driver rz1000_pci_driver = {
	.name 		= DRV_NAME,
	.id_table	= pata_rz1000,
	.probe 		= rz1000_init_one,
	.remove		= ata_pci_remove_one,
#ifdef CONFIG_PM
	.suspend	= ata_pci_device_suspend,
	.resume		= rz1000_reinit_one,
#endif
};

static int __init rz1000_init(void)
{
	return pci_register_driver(&rz1000_pci_driver);
}

static void __exit rz1000_exit(void)
{
	pci_unregister_driver(&rz1000_pci_driver);
}

MODULE_AUTHOR("Alan Cox");
MODULE_DESCRIPTION("low-level driver for RZ1000 PCI ATA");
MODULE_LICENSE("GPL");
MODULE_DEVICE_TABLE(pci, pata_rz1000);
MODULE_VERSION(DRV_VERSION);

module_init(rz1000_init);
module_exit(rz1000_exit);
