


#include <linux/pci.h>

#define DRIVER_NAME "ricoh-mmc"

static const struct pci_device_id pci_ids[] __devinitdata = {
	{
		.vendor		= PCI_VENDOR_ID_RICOH,
		.device		= PCI_DEVICE_ID_RICOH_R5C843,
		.subvendor	= PCI_ANY_ID,
		.subdevice	= PCI_ANY_ID,
	},
	{ /* end: all zeroes */ },
};

MODULE_DEVICE_TABLE(pci, pci_ids);

static int ricoh_mmc_disable(struct pci_dev *fw_dev)
{
	u8 write_enable;
	u8 write_target;
	u8 disable;

	if (fw_dev->device == PCI_DEVICE_ID_RICOH_RL5C476) {
		/* via RL5C476 */

		pci_read_config_byte(fw_dev, 0xB7, &disable);
		if (disable & 0x02) {
			printk(KERN_INFO DRIVER_NAME
				": Controller already disabled. " \
				"Nothing to do.\n");
			return -ENODEV;
		}

		pci_read_config_byte(fw_dev, 0x8E, &write_enable);
		pci_write_config_byte(fw_dev, 0x8E, 0xAA);
		pci_read_config_byte(fw_dev, 0x8D, &write_target);
		pci_write_config_byte(fw_dev, 0x8D, 0xB7);
		pci_write_config_byte(fw_dev, 0xB7, disable | 0x02);
		pci_write_config_byte(fw_dev, 0x8E, write_enable);
		pci_write_config_byte(fw_dev, 0x8D, write_target);
	} else {
		/* via R5C832 */

		pci_read_config_byte(fw_dev, 0xCB, &disable);
		if (disable & 0x02) {
			printk(KERN_INFO DRIVER_NAME
			       ": Controller already disabled. " \
				"Nothing to do.\n");
			return -ENODEV;
		}

		pci_read_config_byte(fw_dev, 0xCA, &write_enable);
		pci_write_config_byte(fw_dev, 0xCA, 0x57);
		pci_write_config_byte(fw_dev, 0xCB, disable | 0x02);
		pci_write_config_byte(fw_dev, 0xCA, write_enable);
	}

	printk(KERN_INFO DRIVER_NAME
	       ": Controller is now disabled.\n");

	return 0;
}

static int ricoh_mmc_enable(struct pci_dev *fw_dev)
{
	u8 write_enable;
	u8 write_target;
	u8 disable;

	if (fw_dev->device == PCI_DEVICE_ID_RICOH_RL5C476) {
		/* via RL5C476 */

		pci_read_config_byte(fw_dev, 0x8E, &write_enable);
		pci_write_config_byte(fw_dev, 0x8E, 0xAA);
		pci_read_config_byte(fw_dev, 0x8D, &write_target);
		pci_write_config_byte(fw_dev, 0x8D, 0xB7);
		pci_read_config_byte(fw_dev, 0xB7, &disable);
		pci_write_config_byte(fw_dev, 0xB7, disable & ~0x02);
		pci_write_config_byte(fw_dev, 0x8E, write_enable);
		pci_write_config_byte(fw_dev, 0x8D, write_target);
	} else {
		/* via R5C832 */

		pci_read_config_byte(fw_dev, 0xCA, &write_enable);
		pci_read_config_byte(fw_dev, 0xCB, &disable);
		pci_write_config_byte(fw_dev, 0xCA, 0x57);
		pci_write_config_byte(fw_dev, 0xCB, disable & ~0x02);
		pci_write_config_byte(fw_dev, 0xCA, write_enable);
	}

	printk(KERN_INFO DRIVER_NAME
	       ": Controller is now re-enabled.\n");

	return 0;
}

static int __devinit ricoh_mmc_probe(struct pci_dev *pdev,
				     const struct pci_device_id *ent)
{
	u8 rev;
	u8 ctrlfound = 0;

	struct pci_dev *fw_dev = NULL;

	BUG_ON(pdev == NULL);
	BUG_ON(ent == NULL);

	pci_read_config_byte(pdev, PCI_CLASS_REVISION, &rev);

	printk(KERN_INFO DRIVER_NAME
		": Ricoh MMC controller found at %s [%04x:%04x] (rev %x)\n",
		pci_name(pdev), (int)pdev->vendor, (int)pdev->device,
		(int)rev);

	while ((fw_dev =
		pci_get_device(PCI_VENDOR_ID_RICOH,
			PCI_DEVICE_ID_RICOH_RL5C476, fw_dev))) {
		if (PCI_SLOT(pdev->devfn) == PCI_SLOT(fw_dev->devfn) &&
		    PCI_FUNC(fw_dev->devfn) == 0 &&
		    pdev->bus == fw_dev->bus) {
			if (ricoh_mmc_disable(fw_dev) != 0)
				return -ENODEV;

			pci_set_drvdata(pdev, fw_dev);

			++ctrlfound;
			break;
		}
	}

	fw_dev = NULL;

	while (!ctrlfound &&
	    (fw_dev = pci_get_device(PCI_VENDOR_ID_RICOH,
					PCI_DEVICE_ID_RICOH_R5C832, fw_dev))) {
		if (PCI_SLOT(pdev->devfn) == PCI_SLOT(fw_dev->devfn) &&
		    PCI_FUNC(fw_dev->devfn) == 0 &&
		    pdev->bus == fw_dev->bus) {
			if (ricoh_mmc_disable(fw_dev) != 0)
				return -ENODEV;

			pci_set_drvdata(pdev, fw_dev);

			++ctrlfound;
		}
	}

	if (!ctrlfound) {
		printk(KERN_WARNING DRIVER_NAME
		       ": Main Ricoh function not found. Cannot disable controller.\n");
		return -ENODEV;
	}

	return 0;
}

static void __devexit ricoh_mmc_remove(struct pci_dev *pdev)
{
	struct pci_dev *fw_dev = NULL;

	fw_dev = pci_get_drvdata(pdev);
	BUG_ON(fw_dev == NULL);

	ricoh_mmc_enable(fw_dev);

	pci_set_drvdata(pdev, NULL);
}

static int ricoh_mmc_suspend_late(struct pci_dev *pdev, pm_message_t state)
{
	struct pci_dev *fw_dev = NULL;

	fw_dev = pci_get_drvdata(pdev);
	BUG_ON(fw_dev == NULL);

	printk(KERN_INFO DRIVER_NAME ": Suspending.\n");

	ricoh_mmc_enable(fw_dev);

	return 0;
}

static int ricoh_mmc_resume_early(struct pci_dev *pdev)
{
	struct pci_dev *fw_dev = NULL;

	fw_dev = pci_get_drvdata(pdev);
	BUG_ON(fw_dev == NULL);

	printk(KERN_INFO DRIVER_NAME ": Resuming.\n");

	ricoh_mmc_disable(fw_dev);

	return 0;
}

static struct pci_driver ricoh_mmc_driver = {
	.name = 	DRIVER_NAME,
	.id_table =	pci_ids,
	.probe = 	ricoh_mmc_probe,
	.remove =	__devexit_p(ricoh_mmc_remove),
	.suspend_late =	ricoh_mmc_suspend_late,
	.resume_early =	ricoh_mmc_resume_early,
};


static int __init ricoh_mmc_drv_init(void)
{
	printk(KERN_INFO DRIVER_NAME
		": Ricoh MMC Controller disabling driver\n");
	printk(KERN_INFO DRIVER_NAME ": Copyright(c) Philip Langdale\n");

	return pci_register_driver(&ricoh_mmc_driver);
}

static void __exit ricoh_mmc_drv_exit(void)
{
	pci_unregister_driver(&ricoh_mmc_driver);
}

module_init(ricoh_mmc_drv_init);
module_exit(ricoh_mmc_drv_exit);

MODULE_AUTHOR("Philip Langdale <philipl@alumni.utexas.net>");
MODULE_DESCRIPTION("Ricoh MMC Controller disabling driver");
MODULE_LICENSE("GPL");

