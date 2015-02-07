

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/blkdev.h>
#include <linux/delay.h>
#include <scsi/scsi_host.h>
#include <linux/libata.h>


#define DRV_NAME "pata_it821x"
#define DRV_VERSION "0.4.2"

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
	u16	last_device;		/* Master or slave loaded ? */
};

#define ATA_66		0
#define ATA_50		1
#define ATA_ANY		2

#define UDMA_OFF	0
#define MWDMA_OFF	0


static int it8212_noraid;


static void it821x_program(struct ata_port *ap, struct ata_device *adev, u16 timing)
{
	struct pci_dev *pdev = to_pci_dev(ap->host->dev);
	struct it821x_dev *itdev = ap->private_data;
	int channel = ap->port_no;
	u8 conf;

	/* Program PIO/MWDMA timing bits */
	if (itdev->clock_mode == ATA_66)
		conf = timing >> 8;
	else
		conf = timing & 0xFF;
	pci_write_config_byte(pdev, 0x54 + 4 * channel, conf);
}



static void it821x_program_udma(struct ata_port *ap, struct ata_device *adev, u16 timing)
{
	struct it821x_dev *itdev = ap->private_data;
	struct pci_dev *pdev = to_pci_dev(ap->host->dev);
	int channel = ap->port_no;
	int unit = adev->devno;
	u8 conf;

	/* Program UDMA timing bits */
	if (itdev->clock_mode == ATA_66)
		conf = timing >> 8;
	else
		conf = timing & 0xFF;
	if (itdev->timing10 == 0)
		pci_write_config_byte(pdev, 0x56 + 4 * channel + unit, conf);
	else {
		/* Early revision must be programmed for both together */
		pci_write_config_byte(pdev, 0x56 + 4 * channel, conf);
		pci_write_config_byte(pdev, 0x56 + 4 * channel + 1, conf);
	}
}


static void it821x_clock_strategy(struct ata_port *ap, struct ata_device *adev)
{
	struct pci_dev *pdev = to_pci_dev(ap->host->dev);
	struct it821x_dev *itdev = ap->private_data;
	u8 unit = adev->devno;
	struct ata_device *pair = ata_dev_pair(adev);

	int clock, altclock;
	u8 v;
	int sel = 0;

	/* Look for the most wanted clocking */
	if (itdev->want[0][0] > itdev->want[1][0]) {
		clock = itdev->want[0][1];
		altclock = itdev->want[1][1];
	} else {
		clock = itdev->want[1][1];
		altclock = itdev->want[0][1];
	}

	/* Master doesn't care does the slave ? */
	if (clock == ATA_ANY)
		clock = altclock;

	/* Nobody cares - keep the same clock */
	if (clock == ATA_ANY)
		return;
	/* No change */
	if (clock == itdev->clock_mode)
		return;

	/* Load this into the controller */
	if (clock == ATA_66)
		itdev->clock_mode = ATA_66;
	else {
		itdev->clock_mode = ATA_50;
		sel = 1;
	}
	pci_read_config_byte(pdev, 0x50, &v);
	v &= ~(1 << (1 + ap->port_no));
	v |= sel << (1 + ap->port_no);
	pci_write_config_byte(pdev, 0x50, v);

	/*
	 *	Reprogram the UDMA/PIO of the pair drive for the switch
	 *	MWDMA will be dealt with by the dma switcher
	 */
	if (pair && itdev->udma[1-unit] != UDMA_OFF) {
		it821x_program_udma(ap, pair, itdev->udma[1-unit]);
		it821x_program(ap, pair, itdev->pio[1-unit]);
	}
	/*
	 *	Reprogram the UDMA/PIO of our drive for the switch.
	 *	MWDMA will be dealt with by the dma switcher
	 */
	if (itdev->udma[unit] != UDMA_OFF) {
		it821x_program_udma(ap, adev, itdev->udma[unit]);
		it821x_program(ap, adev, itdev->pio[unit]);
	}
}


static void it821x_passthru_set_piomode(struct ata_port *ap, struct ata_device *adev)
{
	/* Spec says 89 ref driver uses 88 */
	static const u16 pio[]	= { 0xAA88, 0xA382, 0xA181, 0x3332, 0x3121 };
	static const u8 pio_want[]    = { ATA_66, ATA_66, ATA_66, ATA_66, ATA_ANY };

	struct it821x_dev *itdev = ap->private_data;
	int unit = adev->devno;
	int mode_wanted = adev->pio_mode - XFER_PIO_0;

	/* We prefer 66Mhz clock for PIO 0-3, don't care for PIO4 */
	itdev->want[unit][1] = pio_want[mode_wanted];
	itdev->want[unit][0] = 1;	/* PIO is lowest priority */
	itdev->pio[unit] = pio[mode_wanted];
	it821x_clock_strategy(ap, adev);
	it821x_program(ap, adev, itdev->pio[unit]);
}


static void it821x_passthru_set_dmamode(struct ata_port *ap, struct ata_device *adev)
{
	static const u16 dma[]	= 	{ 0x8866, 0x3222, 0x3121 };
	static const u8 mwdma_want[] =  { ATA_ANY, ATA_66, ATA_ANY };
	static const u16 udma[]	= 	{ 0x4433, 0x4231, 0x3121, 0x2121, 0x1111, 0x2211, 0x1111 };
	static const u8 udma_want[] =   { ATA_ANY, ATA_50, ATA_ANY, ATA_66, ATA_66, ATA_50, ATA_66 };

	struct pci_dev *pdev = to_pci_dev(ap->host->dev);
	struct it821x_dev *itdev = ap->private_data;
	int channel = ap->port_no;
	int unit = adev->devno;
	u8 conf;

	if (adev->dma_mode >= XFER_UDMA_0) {
		int mode_wanted = adev->dma_mode - XFER_UDMA_0;

		itdev->want[unit][1] = udma_want[mode_wanted];
		itdev->want[unit][0] = 3;	/* UDMA is high priority */
		itdev->mwdma[unit] = MWDMA_OFF;
		itdev->udma[unit] = udma[mode_wanted];
		if (mode_wanted >= 5)
			itdev->udma[unit] |= 0x8080;	/* UDMA 5/6 select on */

		/* UDMA on. Again revision 0x10 must do the pair */
		pci_read_config_byte(pdev, 0x50, &conf);
		if (itdev->timing10)
			conf &= channel ? 0x9F: 0xE7;
		else
			conf &= ~ (1 << (3 + 2 * channel + unit));
		pci_write_config_byte(pdev, 0x50, conf);
		it821x_clock_strategy(ap, adev);
		it821x_program_udma(ap, adev, itdev->udma[unit]);
	} else {
		int mode_wanted = adev->dma_mode - XFER_MW_DMA_0;

		itdev->want[unit][1] = mwdma_want[mode_wanted];
		itdev->want[unit][0] = 2;	/* MWDMA is low priority */
		itdev->mwdma[unit] = dma[mode_wanted];
		itdev->udma[unit] = UDMA_OFF;

		/* UDMA bits off - Revision 0x10 do them in pairs */
		pci_read_config_byte(pdev, 0x50, &conf);
		if (itdev->timing10)
			conf |= channel ? 0x60: 0x18;
		else
			conf |= 1 << (3 + 2 * channel + unit);
		pci_write_config_byte(pdev, 0x50, conf);
		it821x_clock_strategy(ap, adev);
	}
}


static void it821x_passthru_bmdma_start(struct ata_queued_cmd *qc)
{
	struct ata_port *ap = qc->ap;
	struct ata_device *adev = qc->dev;
	struct it821x_dev *itdev = ap->private_data;
	int unit = adev->devno;

	if (itdev->mwdma[unit] != MWDMA_OFF)
		it821x_program(ap, adev, itdev->mwdma[unit]);
	else if (itdev->udma[unit] != UDMA_OFF && itdev->timing10)
		it821x_program_udma(ap, adev, itdev->udma[unit]);
	ata_bmdma_start(qc);
}


static void it821x_passthru_bmdma_stop(struct ata_queued_cmd *qc)
{
	struct ata_port *ap = qc->ap;
	struct ata_device *adev = qc->dev;
	struct it821x_dev *itdev = ap->private_data;
	int unit = adev->devno;

	ata_bmdma_stop(qc);
	if (itdev->mwdma[unit] != MWDMA_OFF)
		it821x_program(ap, adev, itdev->pio[unit]);
}



static void it821x_passthru_dev_select(struct ata_port *ap,
				       unsigned int device)
{
	struct it821x_dev *itdev = ap->private_data;
	if (itdev && device != itdev->last_device) {
		struct ata_device *adev = &ap->link.device[device];
		it821x_program(ap, adev, itdev->pio[adev->devno]);
		itdev->last_device = device;
	}
	ata_sff_dev_select(ap, device);
}


static unsigned int it821x_smart_qc_issue(struct ata_queued_cmd *qc)
{
	switch(qc->tf.command)
	{
		/* Commands the firmware supports */
		case ATA_CMD_READ:
		case ATA_CMD_READ_EXT:
		case ATA_CMD_WRITE:
		case ATA_CMD_WRITE_EXT:
		case ATA_CMD_PIO_READ:
		case ATA_CMD_PIO_READ_EXT:
		case ATA_CMD_PIO_WRITE:
		case ATA_CMD_PIO_WRITE_EXT:
		case ATA_CMD_READ_MULTI:
		case ATA_CMD_READ_MULTI_EXT:
		case ATA_CMD_WRITE_MULTI:
		case ATA_CMD_WRITE_MULTI_EXT:
		case ATA_CMD_ID_ATA:
		case ATA_CMD_INIT_DEV_PARAMS:
		case 0xFC:	/* Internal 'report rebuild state' */
		/* Arguably should just no-op this one */
		case ATA_CMD_SET_FEATURES:
			return ata_sff_qc_issue(qc);
	}
	printk(KERN_DEBUG "it821x: can't process command 0x%02X\n", qc->tf.command);
	return AC_ERR_DEV;
}


static unsigned int it821x_passthru_qc_issue(struct ata_queued_cmd *qc)
{
	it821x_passthru_dev_select(qc->ap, qc->dev->devno);
	return ata_sff_qc_issue(qc);
}


static int it821x_smart_set_mode(struct ata_link *link, struct ata_device **unused)
{
	struct ata_device *dev;

	ata_for_each_dev(dev, link, ENABLED) {
		/* We don't really care */
		dev->pio_mode = XFER_PIO_0;
		dev->dma_mode = XFER_MW_DMA_0;
		/* We do need the right mode information for DMA or PIO
		   and this comes from the current configuration flags */
		if (ata_id_has_dma(dev->id)) {
			ata_dev_printk(dev, KERN_INFO, "configured for DMA\n");
			dev->xfer_mode = XFER_MW_DMA_0;
			dev->xfer_shift = ATA_SHIFT_MWDMA;
			dev->flags &= ~ATA_DFLAG_PIO;
		} else {
			ata_dev_printk(dev, KERN_INFO, "configured for PIO\n");
			dev->xfer_mode = XFER_PIO_0;
			dev->xfer_shift = ATA_SHIFT_PIO;
			dev->flags |= ATA_DFLAG_PIO;
		}
	}
	return 0;
}


static void it821x_dev_config(struct ata_device *adev)
{
	unsigned char model_num[ATA_ID_PROD_LEN + 1];

	ata_id_c_string(adev->id, model_num, ATA_ID_PROD, sizeof(model_num));

	if (adev->max_sectors > 255)
		adev->max_sectors = 255;

	if (strstr(model_num, "Integrated Technology Express")) {
		/* RAID mode */
		ata_dev_printk(adev, KERN_INFO, "%sRAID%d volume",
			adev->id[147]?"Bootable ":"",
			adev->id[129]);
		if (adev->id[129] != 1)
			printk("(%dK stripe)", adev->id[146]);
		printk(".\n");
	}
	/* This is a controller firmware triggered funny, don't
	   report the drive faulty! */
	adev->horkage &= ~ATA_HORKAGE_DIAGNOSTIC;
	/* No HPA in 'smart' mode */
	adev->horkage |= ATA_HORKAGE_BROKEN_HPA;
}


static unsigned int it821x_read_id(struct ata_device *adev,
					struct ata_taskfile *tf, u16 *id)
{
	unsigned int err_mask;
	unsigned char model_num[ATA_ID_PROD_LEN + 1];

	err_mask = ata_do_dev_read_id(adev, tf, id);
	if (err_mask)
		return err_mask;
	ata_id_c_string(id, model_num, ATA_ID_PROD, sizeof(model_num));

	id[83] &= ~(1 << 12);	/* Cache flush is firmware handled */
	id[83] &= ~(1 << 13);	/* Ditto for LBA48 flushes */
	id[84] &= ~(1 << 6);	/* No FUA */
	id[85] &= ~(1 << 10);	/* No HPA */
	id[76] = 0;		/* No NCQ/AN etc */

	if (strstr(model_num, "Integrated Technology Express")) {
		/* Set feature bits the firmware neglects */
		id[49] |= 0x0300;	/* LBA, DMA */
		id[83] &= 0x7FFF;
		id[83] |= 0x4400;	/* Word 83 is valid and LBA48 */
		id[86] |= 0x0400;	/* LBA48 on */
		id[ATA_ID_MAJOR_VER] |= 0x1F;
		/* Clear the serial number because it's different each boot
		   which breaks validation on resume */
		memset(&id[ATA_ID_SERNO], 0x20, ATA_ID_SERNO_LEN);
	}
	return err_mask;
}


static int it821x_check_atapi_dma(struct ata_queued_cmd *qc)
{
	struct ata_port *ap = qc->ap;
	struct it821x_dev *itdev = ap->private_data;

	/* Only use dma for transfers to/from the media. */
	if (ata_qc_raw_nbytes(qc) < 2048)
		return -EOPNOTSUPP;

	/* No ATAPI DMA in smart mode */
	if (itdev->smart)
		return -EOPNOTSUPP;
	/* No ATAPI DMA on rev 10 */
	if (itdev->timing10)
		return -EOPNOTSUPP;
	/* Cool */
	return 0;
}


static void it821x_display_disk(int n, u8 *buf)
{
	unsigned char id[41];
	int mode = 0;
	char *mtype = "";
	char mbuf[8];
	char *cbl = "(40 wire cable)";

	static const char *types[5] = {
		"RAID0", "RAID1" "RAID 0+1", "JBOD", "DISK"
	};

	if (buf[52] > 4)	/* No Disk */
		return;

	ata_id_c_string((u16 *)buf, id, 0, 41); 

	if (buf[51]) {
		mode = ffs(buf[51]);
		mtype = "UDMA";
	} else if (buf[49]) {
		mode = ffs(buf[49]);
		mtype = "MWDMA";
	}

	if (buf[76])
		cbl = "";

	if (mode)
		snprintf(mbuf, 8, "%5s%d", mtype, mode - 1);
	else
		strcpy(mbuf, "PIO");
	if (buf[52] == 4)
		printk(KERN_INFO "%d: %-6s %-8s          %s %s\n",
				n, mbuf, types[buf[52]], id, cbl);
	else
		printk(KERN_INFO "%d: %-6s %-8s Volume: %1d %s %s\n",
				n, mbuf, types[buf[52]], buf[53], id, cbl);
	if (buf[125] < 100)
		printk(KERN_INFO "%d: Rebuilding: %d%%\n", n, buf[125]);
}


static u8 *it821x_firmware_command(struct ata_port *ap, u8 cmd, int len)
{
	u8 status;
	int n = 0;
	u16 *buf = kmalloc(len, GFP_KERNEL);
	if (buf == NULL) {
		printk(KERN_ERR "it821x_firmware_command: Out of memory\n");
		return NULL;
	}
	/* This isn't quite a normal ATA command as we are talking to the
	   firmware not the drives */
	ap->ctl |= ATA_NIEN;
	iowrite8(ap->ctl, ap->ioaddr.ctl_addr);
	ata_wait_idle(ap);
	iowrite8(ATA_DEVICE_OBS, ap->ioaddr.device_addr);
	iowrite8(cmd, ap->ioaddr.command_addr);
	udelay(1);
	/* This should be almost immediate but a little paranoia goes a long
	   way. */
	while(n++ < 10) {
		status = ioread8(ap->ioaddr.status_addr);
		if (status & ATA_ERR) {
			kfree(buf);
			printk(KERN_ERR "it821x_firmware_command: rejected\n");
			return NULL;
		}
		if (status & ATA_DRQ) {
			ioread16_rep(ap->ioaddr.data_addr, buf, len/2);
			return (u8 *)buf;
		}
		mdelay(1);
	}
	kfree(buf);
	printk(KERN_ERR "it821x_firmware_command: timeout\n");
	return NULL;
}


static void it821x_probe_firmware(struct ata_port *ap)
{
	u8 *buf;
	int i;

	/* This is a bit ugly as we can't just issue a task file to a device
	   as this is controller magic */

	buf = it821x_firmware_command(ap, 0xFA, 512);

	if (buf != NULL) {
		printk(KERN_INFO "pata_it821x: Firmware %02X/%02X/%02X%02X\n",
				buf[505],
				buf[506],
				buf[507],
				buf[508]);
		for (i = 0; i < 4; i++)
 			it821x_display_disk(i, buf + 128 * i);
		kfree(buf);
	}
}




static int it821x_port_start(struct ata_port *ap)
{
	struct pci_dev *pdev = to_pci_dev(ap->host->dev);
	struct it821x_dev *itdev;
	u8 conf;

	int ret = ata_sff_port_start(ap);
	if (ret < 0)
		return ret;

	itdev = devm_kzalloc(&pdev->dev, sizeof(struct it821x_dev), GFP_KERNEL);
	if (itdev == NULL)
		return -ENOMEM;
	ap->private_data = itdev;

	pci_read_config_byte(pdev, 0x50, &conf);

	if (conf & 1) {
		itdev->smart = 1;
		/* Long I/O's although allowed in LBA48 space cause the
		   onboard firmware to enter the twighlight zone */
		/* No ATAPI DMA in this mode either */
		if (ap->port_no == 0)
			it821x_probe_firmware(ap);
	}
	/* Pull the current clocks from 0x50 */
	if (conf & (1 << (1 + ap->port_no)))
		itdev->clock_mode = ATA_50;
	else
		itdev->clock_mode = ATA_66;

	itdev->want[0][1] = ATA_ANY;
	itdev->want[1][1] = ATA_ANY;
	itdev->last_device = -1;

	if (pdev->revision == 0x10) {
		itdev->timing10 = 1;
		/* Need to disable ATAPI DMA for this case */
		if (!itdev->smart)
			printk(KERN_WARNING DRV_NAME": Revision 0x10, workarounds activated.\n");
	}

	return 0;
}


static int it821x_rdc_cable(struct ata_port *ap)
{
	u16 r40;
	struct pci_dev *pdev = to_pci_dev(ap->host->dev);

	pci_read_config_word(pdev, 0x40, &r40);
	if (r40 & (1 << (2 + ap->port_no)))
		return ATA_CBL_PATA40;
	return ATA_CBL_PATA80;
}

static struct scsi_host_template it821x_sht = {
	ATA_BMDMA_SHT(DRV_NAME),
};

static struct ata_port_operations it821x_smart_port_ops = {
	.inherits	= &ata_bmdma_port_ops,

	.check_atapi_dma= it821x_check_atapi_dma,
	.qc_issue	= it821x_smart_qc_issue,

	.cable_detect	= ata_cable_80wire,
	.set_mode	= it821x_smart_set_mode,
	.dev_config	= it821x_dev_config,
	.read_id	= it821x_read_id,

	.port_start	= it821x_port_start,
};

static struct ata_port_operations it821x_passthru_port_ops = {
	.inherits	= &ata_bmdma_port_ops,

	.check_atapi_dma= it821x_check_atapi_dma,
	.sff_dev_select	= it821x_passthru_dev_select,
	.bmdma_start 	= it821x_passthru_bmdma_start,
	.bmdma_stop	= it821x_passthru_bmdma_stop,
	.qc_issue	= it821x_passthru_qc_issue,

	.cable_detect	= ata_cable_unknown,
	.set_piomode	= it821x_passthru_set_piomode,
	.set_dmamode	= it821x_passthru_set_dmamode,

	.port_start	= it821x_port_start,
};

static struct ata_port_operations it821x_rdc_port_ops = {
	.inherits	= &ata_bmdma_port_ops,

	.check_atapi_dma= it821x_check_atapi_dma,
	.sff_dev_select	= it821x_passthru_dev_select,
	.bmdma_start 	= it821x_passthru_bmdma_start,
	.bmdma_stop	= it821x_passthru_bmdma_stop,
	.qc_issue	= it821x_passthru_qc_issue,

	.cable_detect	= it821x_rdc_cable,
	.set_piomode	= it821x_passthru_set_piomode,
	.set_dmamode	= it821x_passthru_set_dmamode,

	.port_start	= it821x_port_start,
};

static void it821x_disable_raid(struct pci_dev *pdev)
{
	/* Neither the RDC nor the IT8211 */
	if (pdev->vendor != PCI_VENDOR_ID_ITE ||
			pdev->device != PCI_DEVICE_ID_ITE_8212)
			return;

	/* Reset local CPU, and set BIOS not ready */
	pci_write_config_byte(pdev, 0x5E, 0x01);

	/* Set to bypass mode, and reset PCI bus */
	pci_write_config_byte(pdev, 0x50, 0x00);
	pci_write_config_word(pdev, PCI_COMMAND,
			      PCI_COMMAND_PARITY | PCI_COMMAND_IO |
			      PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER);
	pci_write_config_word(pdev, 0x40, 0xA0F3);

	pci_write_config_dword(pdev,0x4C, 0x02040204);
	pci_write_config_byte(pdev, 0x42, 0x36);
	pci_write_config_byte(pdev, PCI_LATENCY_TIMER, 0x20);
}


static int it821x_init_one(struct pci_dev *pdev, const struct pci_device_id *id)
{
	u8 conf;

	static const struct ata_port_info info_smart = {
		.flags = ATA_FLAG_SLAVE_POSS,
		.pio_mask = 0x1f,
		.mwdma_mask = 0x07,
		.udma_mask = ATA_UDMA6,
		.port_ops = &it821x_smart_port_ops
	};
	static const struct ata_port_info info_passthru = {
		.flags = ATA_FLAG_SLAVE_POSS,
		.pio_mask = 0x1f,
		.mwdma_mask = 0x07,
		.udma_mask = ATA_UDMA6,
		.port_ops = &it821x_passthru_port_ops
	};
	static const struct ata_port_info info_rdc = {
		.flags = ATA_FLAG_SLAVE_POSS,
		.pio_mask = 0x1f,
		.mwdma_mask = 0x07,
		.udma_mask = ATA_UDMA6,
		.port_ops = &it821x_rdc_port_ops
	};
	static const struct ata_port_info info_rdc_11 = {
		.flags = ATA_FLAG_SLAVE_POSS,
		.pio_mask = 0x1f,
		.mwdma_mask = 0x07,
		/* No UDMA */
		.port_ops = &it821x_rdc_port_ops
	};

	const struct ata_port_info *ppi[] = { NULL, NULL };
	static char *mode[2] = { "pass through", "smart" };
	int rc;

	rc = pcim_enable_device(pdev);
	if (rc)
		return rc;
		
	if (pdev->vendor == PCI_VENDOR_ID_RDC) {
		/* Deal with Vortex86SX */
		if (pdev->revision == 0x11)
			ppi[0] = &info_rdc_11;
		else
			ppi[0] = &info_rdc;
	} else {
		/* Force the card into bypass mode if so requested */
		if (it8212_noraid) {
			printk(KERN_INFO DRV_NAME ": forcing bypass mode.\n");
			it821x_disable_raid(pdev);
		}
		pci_read_config_byte(pdev, 0x50, &conf);
		conf &= 1;

		printk(KERN_INFO DRV_NAME": controller in %s mode.\n",
								mode[conf]);
		if (conf == 0)
			ppi[0] = &info_passthru;
		else
			ppi[0] = &info_smart;
	}
	return ata_pci_sff_init_one(pdev, ppi, &it821x_sht, NULL);
}

#ifdef CONFIG_PM
static int it821x_reinit_one(struct pci_dev *pdev)
{
	struct ata_host *host = dev_get_drvdata(&pdev->dev);
	int rc;

	rc = ata_pci_device_do_resume(pdev);
	if (rc)
		return rc;
	/* Resume - turn raid back off if need be */
	if (it8212_noraid)
		it821x_disable_raid(pdev);
	ata_host_resume(host);
	return rc;
}
#endif

static const struct pci_device_id it821x[] = {
	{ PCI_VDEVICE(ITE, PCI_DEVICE_ID_ITE_8211), },
	{ PCI_VDEVICE(ITE, PCI_DEVICE_ID_ITE_8212), },
	{ PCI_VDEVICE(RDC, 0x1010), },

	{ },
};

static struct pci_driver it821x_pci_driver = {
	.name 		= DRV_NAME,
	.id_table	= it821x,
	.probe 		= it821x_init_one,
	.remove		= ata_pci_remove_one,
#ifdef CONFIG_PM
	.suspend	= ata_pci_device_suspend,
	.resume		= it821x_reinit_one,
#endif
};

static int __init it821x_init(void)
{
	return pci_register_driver(&it821x_pci_driver);
}

static void __exit it821x_exit(void)
{
	pci_unregister_driver(&it821x_pci_driver);
}

MODULE_AUTHOR("Alan Cox");
MODULE_DESCRIPTION("low-level driver for the IT8211/IT8212 IDE RAID controller");
MODULE_LICENSE("GPL");
MODULE_DEVICE_TABLE(pci, it821x);
MODULE_VERSION(DRV_VERSION);


module_param_named(noraid, it8212_noraid, int, S_IRUGO);
MODULE_PARM_DESC(noraid, "Force card into bypass mode");

module_init(it821x_init);
module_exit(it821x_exit);
