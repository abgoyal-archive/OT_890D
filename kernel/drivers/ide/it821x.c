

#include <linux/types.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/ide.h>
#include <linux/init.h>

#define DRV_NAME "it821x"

#define QUIRK_VORTEX86 1

struct it821x_dev
{
	unsigned int smart:1,		/* Are we in smart raid mode */
		timing10:1;		/* Rev 0x10 */
	u8	clock_mode;		/* 0, ATA_50 or ATA_66 */
	u8	want[2][2];		/* Mode/Pri log for master slave */
	/* We need these for switching the clock when DMA goes on/off
	   The high byte is the 66Mhz timing */
	u16	pio[2];			/* Cached PIO values */
	u16	mwdma[2];		/* Cached MWDMA values */
	u16	udma[2];		/* Cached UDMA values (per drive) */
	u16	quirks;
};

#define ATA_66		0
#define ATA_50		1
#define ATA_ANY		2

#define UDMA_OFF	0
#define MWDMA_OFF	0


static int it8212_noraid;


static void it821x_program(ide_drive_t *drive, u16 timing)
{
	ide_hwif_t *hwif = drive->hwif;
	struct pci_dev *dev = to_pci_dev(hwif->dev);
	struct it821x_dev *itdev = ide_get_hwifdata(hwif);
	int channel = hwif->channel;
	u8 conf;

	/* Program PIO/MWDMA timing bits */
	if(itdev->clock_mode == ATA_66)
		conf = timing >> 8;
	else
		conf = timing & 0xFF;

	pci_write_config_byte(dev, 0x54 + 4 * channel, conf);
}


static void it821x_program_udma(ide_drive_t *drive, u16 timing)
{
	ide_hwif_t *hwif = drive->hwif;
	struct pci_dev *dev = to_pci_dev(hwif->dev);
	struct it821x_dev *itdev = ide_get_hwifdata(hwif);
	int channel = hwif->channel;
	u8 unit = drive->dn & 1, conf;

	/* Program UDMA timing bits */
	if(itdev->clock_mode == ATA_66)
		conf = timing >> 8;
	else
		conf = timing & 0xFF;

	if (itdev->timing10 == 0)
		pci_write_config_byte(dev, 0x56 + 4 * channel + unit, conf);
	else {
		pci_write_config_byte(dev, 0x56 + 4 * channel, conf);
		pci_write_config_byte(dev, 0x56 + 4 * channel + 1, conf);
	}
}


static void it821x_clock_strategy(ide_drive_t *drive)
{
	ide_hwif_t *hwif = drive->hwif;
	struct pci_dev *dev = to_pci_dev(hwif->dev);
	struct it821x_dev *itdev = ide_get_hwifdata(hwif);
	ide_drive_t *pair = ide_get_pair_dev(drive);
	int clock, altclock, sel = 0;
	u8 unit = drive->dn & 1, v;

	if(itdev->want[0][0] > itdev->want[1][0]) {
		clock = itdev->want[0][1];
		altclock = itdev->want[1][1];
	} else {
		clock = itdev->want[1][1];
		altclock = itdev->want[0][1];
	}

	/*
	 * if both clocks can be used for the mode with the higher priority
	 * use the clock needed by the mode with the lower priority
	 */
	if (clock == ATA_ANY)
		clock = altclock;

	/* Nobody cares - keep the same clock */
	if(clock == ATA_ANY)
		return;
	/* No change */
	if(clock == itdev->clock_mode)
		return;

	/* Load this into the controller ? */
	if(clock == ATA_66)
		itdev->clock_mode = ATA_66;
	else {
		itdev->clock_mode = ATA_50;
		sel = 1;
	}

	pci_read_config_byte(dev, 0x50, &v);
	v &= ~(1 << (1 + hwif->channel));
	v |= sel << (1 + hwif->channel);
	pci_write_config_byte(dev, 0x50, v);

	/*
	 *	Reprogram the UDMA/PIO of the pair drive for the switch
	 *	MWDMA will be dealt with by the dma switcher
	 */
	if(pair && itdev->udma[1-unit] != UDMA_OFF) {
		it821x_program_udma(pair, itdev->udma[1-unit]);
		it821x_program(pair, itdev->pio[1-unit]);
	}
	/*
	 *	Reprogram the UDMA/PIO of our drive for the switch.
	 *	MWDMA will be dealt with by the dma switcher
	 */
	if(itdev->udma[unit] != UDMA_OFF) {
		it821x_program_udma(drive, itdev->udma[unit]);
		it821x_program(drive, itdev->pio[unit]);
	}
}


static void it821x_set_pio_mode(ide_drive_t *drive, const u8 pio)
{
	ide_hwif_t *hwif = drive->hwif;
	struct it821x_dev *itdev = ide_get_hwifdata(hwif);
	ide_drive_t *pair = ide_get_pair_dev(drive);
	u8 unit = drive->dn & 1, set_pio = pio;

	/* Spec says 89 ref driver uses 88 */
	static u16 pio_timings[]= { 0xAA88, 0xA382, 0xA181, 0x3332, 0x3121 };
	static u8 pio_want[]    = { ATA_66, ATA_66, ATA_66, ATA_66, ATA_ANY };

	/*
	 * Compute the best PIO mode we can for a given device. We must
	 * pick a speed that does not cause problems with the other device
	 * on the cable.
	 */
	if (pair) {
		u8 pair_pio = ide_get_best_pio_mode(pair, 255, 4);
		/* trim PIO to the slowest of the master/slave */
		if (pair_pio < set_pio)
			set_pio = pair_pio;
	}

	/* We prefer 66Mhz clock for PIO 0-3, don't care for PIO4 */
	itdev->want[unit][1] = pio_want[set_pio];
	itdev->want[unit][0] = 1;	/* PIO is lowest priority */
	itdev->pio[unit] = pio_timings[set_pio];
	it821x_clock_strategy(drive);
	it821x_program(drive, itdev->pio[unit]);
}


static void it821x_tune_mwdma(ide_drive_t *drive, u8 mode_wanted)
{
	ide_hwif_t *hwif = drive->hwif;
	struct pci_dev *dev = to_pci_dev(hwif->dev);
	struct it821x_dev *itdev = (void *)ide_get_hwifdata(hwif);
	u8 unit = drive->dn & 1, channel = hwif->channel, conf;

	static u16 dma[]	= { 0x8866, 0x3222, 0x3121 };
	static u8 mwdma_want[]	= { ATA_ANY, ATA_66, ATA_ANY };

	itdev->want[unit][1] = mwdma_want[mode_wanted];
	itdev->want[unit][0] = 2;	/* MWDMA is low priority */
	itdev->mwdma[unit] = dma[mode_wanted];
	itdev->udma[unit] = UDMA_OFF;

	/* UDMA bits off - Revision 0x10 do them in pairs */
	pci_read_config_byte(dev, 0x50, &conf);
	if (itdev->timing10)
		conf |= channel ? 0x60: 0x18;
	else
		conf |= 1 << (3 + 2 * channel + unit);
	pci_write_config_byte(dev, 0x50, conf);

	it821x_clock_strategy(drive);
	/* FIXME: do we need to program this ? */
	/* it821x_program(drive, itdev->mwdma[unit]); */
}


static void it821x_tune_udma(ide_drive_t *drive, u8 mode_wanted)
{
	ide_hwif_t *hwif = drive->hwif;
	struct pci_dev *dev = to_pci_dev(hwif->dev);
	struct it821x_dev *itdev = ide_get_hwifdata(hwif);
	u8 unit = drive->dn & 1, channel = hwif->channel, conf;

	static u16 udma[]	= { 0x4433, 0x4231, 0x3121, 0x2121, 0x1111, 0x2211, 0x1111 };
	static u8 udma_want[]	= { ATA_ANY, ATA_50, ATA_ANY, ATA_66, ATA_66, ATA_50, ATA_66 };

	itdev->want[unit][1] = udma_want[mode_wanted];
	itdev->want[unit][0] = 3;	/* UDMA is high priority */
	itdev->mwdma[unit] = MWDMA_OFF;
	itdev->udma[unit] = udma[mode_wanted];
	if(mode_wanted >= 5)
		itdev->udma[unit] |= 0x8080;	/* UDMA 5/6 select on */

	/* UDMA on. Again revision 0x10 must do the pair */
	pci_read_config_byte(dev, 0x50, &conf);
	if (itdev->timing10)
		conf &= channel ? 0x9F: 0xE7;
	else
		conf &= ~ (1 << (3 + 2 * channel + unit));
	pci_write_config_byte(dev, 0x50, conf);

	it821x_clock_strategy(drive);
	it821x_program_udma(drive, itdev->udma[unit]);

}


static void it821x_dma_start(ide_drive_t *drive)
{
	ide_hwif_t *hwif = drive->hwif;
	struct it821x_dev *itdev = ide_get_hwifdata(hwif);
	u8 unit = drive->dn & 1;

	if(itdev->mwdma[unit] != MWDMA_OFF)
		it821x_program(drive, itdev->mwdma[unit]);
	else if(itdev->udma[unit] != UDMA_OFF && itdev->timing10)
		it821x_program_udma(drive, itdev->udma[unit]);
	ide_dma_start(drive);
}


static int it821x_dma_end(ide_drive_t *drive)
{
	ide_hwif_t *hwif = drive->hwif;
	struct it821x_dev *itdev = ide_get_hwifdata(hwif);
	int ret = ide_dma_end(drive);
	u8 unit = drive->dn & 1;

	if(itdev->mwdma[unit] != MWDMA_OFF)
		it821x_program(drive, itdev->pio[unit]);
	return ret;
}


static void it821x_set_dma_mode(ide_drive_t *drive, const u8 speed)
{
	/*
	 * MWDMA tuning is really hard because our MWDMA and PIO
	 * timings are kept in the same place.  We can switch in the
	 * host dma on/off callbacks.
	 */
	if (speed >= XFER_UDMA_0 && speed <= XFER_UDMA_6)
		it821x_tune_udma(drive, speed - XFER_UDMA_0);
	else if (speed >= XFER_MW_DMA_0 && speed <= XFER_MW_DMA_2)
		it821x_tune_mwdma(drive, speed - XFER_MW_DMA_0);
}


static u8 it821x_cable_detect(ide_hwif_t *hwif)
{
	/* The reference driver also only does disk side */
	return ATA_CBL_PATA80;
}


static void it821x_quirkproc(ide_drive_t *drive)
{
	struct it821x_dev *itdev = ide_get_hwifdata(drive->hwif);
	u16 *id = drive->id;

	if (!itdev->smart) {
		/*
		 *	If we are in pass through mode then not much
		 *	needs to be done, but we do bother to clear the
		 *	IRQ mask as we may well be in PIO (eg rev 0x10)
		 *	for now and we know unmasking is safe on this chipset.
		 */
		drive->dev_flags |= IDE_DFLAG_UNMASK;
	} else {
	/*
	 *	Perform fixups on smart mode. We need to "lose" some
	 *	capabilities the firmware lacks but does not filter, and
	 *	also patch up some capability bits that it forgets to set
	 *	in RAID mode.
	 */

		/* Check for RAID v native */
		if (strstr((char *)&id[ATA_ID_PROD],
			   "Integrated Technology Express")) {
			/* In raid mode the ident block is slightly buggy
			   We need to set the bits so that the IDE layer knows
			   LBA28. LBA48 and DMA ar valid */
			id[ATA_ID_CAPABILITY]    |= (3 << 8); /* LBA28, DMA */
			id[ATA_ID_COMMAND_SET_2] |= 0x0400;   /* LBA48 valid */
			id[ATA_ID_CFS_ENABLE_2]  |= 0x0400;   /* LBA48 on */
			/* Reporting logic */
			printk(KERN_INFO "%s: IT8212 %sRAID %d volume",
				drive->name, id[147] ? "Bootable " : "",
				id[ATA_ID_CSFO]);
			if (id[ATA_ID_CSFO] != 1)
				printk(KERN_CONT "(%dK stripe)", id[146]);
			printk(KERN_CONT ".\n");
		} else {
			/* Non RAID volume. Fixups to stop the core code
			   doing unsupported things */
			id[ATA_ID_FIELD_VALID]	 &= 3;
			id[ATA_ID_QUEUE_DEPTH]	  = 0;
			id[ATA_ID_COMMAND_SET_1]  = 0;
			id[ATA_ID_COMMAND_SET_2] &= 0xC400;
			id[ATA_ID_CFSSE]	 &= 0xC000;
			id[ATA_ID_CFS_ENABLE_1]	  = 0;
			id[ATA_ID_CFS_ENABLE_2]	 &= 0xC400;
			id[ATA_ID_CSF_DEFAULT]	 &= 0xC000;
			id[127]			  = 0;
			id[ATA_ID_DLF]		  = 0;
			id[ATA_ID_CSFO]		  = 0;
			id[ATA_ID_CFA_POWER]	  = 0;
			printk(KERN_INFO "%s: Performing identify fixups.\n",
				drive->name);
		}

		/*
		 * Set MWDMA0 mode as enabled/support - just to tell
		 * IDE core that DMA is supported (it821x hardware
		 * takes care of DMA mode programming).
		 */
		if (ata_id_has_dma(id)) {
			id[ATA_ID_MWDMA_MODES] |= 0x0101;
			drive->current_speed = XFER_MW_DMA_0;
		}
	}

}

static struct ide_dma_ops it821x_pass_through_dma_ops = {
	.dma_host_set		= ide_dma_host_set,
	.dma_setup		= ide_dma_setup,
	.dma_exec_cmd		= ide_dma_exec_cmd,
	.dma_start		= it821x_dma_start,
	.dma_end		= it821x_dma_end,
	.dma_test_irq		= ide_dma_test_irq,
	.dma_timeout		= ide_dma_timeout,
	.dma_lost_irq		= ide_dma_lost_irq,
	.dma_sff_read_status	= ide_dma_sff_read_status,
};


static void __devinit init_hwif_it821x(ide_hwif_t *hwif)
{
	struct pci_dev *dev = to_pci_dev(hwif->dev);
	struct ide_host *host = pci_get_drvdata(dev);
	struct it821x_dev *itdevs = host->host_priv;
	struct it821x_dev *idev = itdevs + hwif->channel;
	u8 conf;

	ide_set_hwifdata(hwif, idev);

	pci_read_config_byte(dev, 0x50, &conf);
	if (conf & 1) {
		idev->smart = 1;
		hwif->host_flags |= IDE_HFLAG_NO_ATAPI_DMA;
		/* Long I/O's although allowed in LBA48 space cause the
		   onboard firmware to enter the twighlight zone */
		hwif->rqsize = 256;
	}

	/* Pull the current clocks from 0x50 also */
	if (conf & (1 << (1 + hwif->channel)))
		idev->clock_mode = ATA_50;
	else
		idev->clock_mode = ATA_66;

	idev->want[0][1] = ATA_ANY;
	idev->want[1][1] = ATA_ANY;

	/*
	 *	Not in the docs but according to the reference driver
	 *	this is necessary.
	 */

	if (dev->revision == 0x10) {
		idev->timing10 = 1;
		hwif->host_flags |= IDE_HFLAG_NO_ATAPI_DMA;
		if (idev->smart == 0)
			printk(KERN_WARNING DRV_NAME " %s: revision 0x10, "
				"workarounds activated\n", pci_name(dev));
	}

	if (idev->smart == 0) {
		/* MWDMA/PIO clock switching for pass through mode */
		hwif->dma_ops = &it821x_pass_through_dma_ops;
	} else
		hwif->host_flags |= IDE_HFLAG_NO_SET_MODE;

	if (hwif->dma_base == 0)
		return;

	hwif->ultra_mask = ATA_UDMA6;
	hwif->mwdma_mask = ATA_MWDMA2;

	/* Vortex86SX quirk: prevent Ultra-DMA mode to fix BadCRC issue */
	if (idev->quirks & QUIRK_VORTEX86) {
		if (dev->revision == 0x11)
			hwif->ultra_mask = 0;
	}
}

static void it8212_disable_raid(struct pci_dev *dev)
{
	/* Reset local CPU, and set BIOS not ready */
	pci_write_config_byte(dev, 0x5E, 0x01);

	/* Set to bypass mode, and reset PCI bus */
	pci_write_config_byte(dev, 0x50, 0x00);
	pci_write_config_word(dev, PCI_COMMAND,
			      PCI_COMMAND_PARITY | PCI_COMMAND_IO |
			      PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER);
	pci_write_config_word(dev, 0x40, 0xA0F3);

	pci_write_config_dword(dev,0x4C, 0x02040204);
	pci_write_config_byte(dev, 0x42, 0x36);
	pci_write_config_byte(dev, PCI_LATENCY_TIMER, 0x20);
}

static unsigned int init_chipset_it821x(struct pci_dev *dev)
{
	u8 conf;
	static char *mode[2] = { "pass through", "smart" };

	/* Force the card into bypass mode if so requested */
	if (it8212_noraid) {
		printk(KERN_INFO DRV_NAME " %s: forcing bypass mode\n",
			pci_name(dev));
		it8212_disable_raid(dev);
	}
	pci_read_config_byte(dev, 0x50, &conf);
	printk(KERN_INFO DRV_NAME " %s: controller in %s mode\n",
		pci_name(dev), mode[conf & 1]);
	return 0;
}

static const struct ide_port_ops it821x_port_ops = {
	/* it821x_set_{pio,dma}_mode() are only used in pass-through mode */
	.set_pio_mode		= it821x_set_pio_mode,
	.set_dma_mode		= it821x_set_dma_mode,
	.quirkproc		= it821x_quirkproc,
	.cable_detect		= it821x_cable_detect,
};

static const struct ide_port_info it821x_chipset __devinitdata = {
	.name		= DRV_NAME,
	.init_chipset	= init_chipset_it821x,
	.init_hwif	= init_hwif_it821x,
	.port_ops	= &it821x_port_ops,
	.pio_mask	= ATA_PIO4,
};


static int __devinit it821x_init_one(struct pci_dev *dev, const struct pci_device_id *id)
{
	struct it821x_dev *itdevs;
	int rc;

	itdevs = kzalloc(2 * sizeof(*itdevs), GFP_KERNEL);
	if (itdevs == NULL) {
		printk(KERN_ERR DRV_NAME " %s: out of memory\n", pci_name(dev));
		return -ENOMEM;
	}

	itdevs->quirks = id->driver_data;

	rc = ide_pci_init_one(dev, &it821x_chipset, itdevs);
	if (rc)
		kfree(itdevs);

	return rc;
}

static void __devexit it821x_remove(struct pci_dev *dev)
{
	struct ide_host *host = pci_get_drvdata(dev);
	struct it821x_dev *itdevs = host->host_priv;

	ide_pci_remove(dev);
	kfree(itdevs);
}

static const struct pci_device_id it821x_pci_tbl[] = {
	{ PCI_VDEVICE(ITE, PCI_DEVICE_ID_ITE_8211), 0 },
	{ PCI_VDEVICE(ITE, PCI_DEVICE_ID_ITE_8212), 0 },
	{ PCI_VDEVICE(RDC, PCI_DEVICE_ID_RDC_D1010), QUIRK_VORTEX86 },
	{ 0, },
};

MODULE_DEVICE_TABLE(pci, it821x_pci_tbl);

static struct pci_driver it821x_pci_driver = {
	.name		= "ITE821x IDE",
	.id_table	= it821x_pci_tbl,
	.probe		= it821x_init_one,
	.remove		= __devexit_p(it821x_remove),
	.suspend	= ide_pci_suspend,
	.resume		= ide_pci_resume,
};

static int __init it821x_ide_init(void)
{
	return ide_pci_register_driver(&it821x_pci_driver);
}

static void __exit it821x_ide_exit(void)
{
	pci_unregister_driver(&it821x_pci_driver);
}

module_init(it821x_ide_init);
module_exit(it821x_ide_exit);

module_param_named(noraid, it8212_noraid, int, S_IRUGO);
MODULE_PARM_DESC(noraid, "Force card into bypass mode");

MODULE_AUTHOR("Alan Cox");
MODULE_DESCRIPTION("PCI driver module for the ITE 821x");
MODULE_LICENSE("GPL");
