

#include <linux/types.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/ide.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/ata_platform.h>
#include <linux/platform_device.h>
#include <linux/io.h>

static void __devinit plat_ide_setup_ports(hw_regs_t *hw,
					   void __iomem *base,
					   void __iomem *ctrl,
					   struct pata_platform_info *pdata,
					   int irq)
{
	unsigned long port = (unsigned long)base;
	int i;

	hw->io_ports.data_addr = port;

	port += (1 << pdata->ioport_shift);
	for (i = 1; i <= 7;
	     i++, port += (1 << pdata->ioport_shift))
		hw->io_ports_array[i] = port;

	hw->io_ports.ctl_addr = (unsigned long)ctrl;

	hw->irq = irq;

	hw->chipset = ide_generic;
}

static const struct ide_port_info platform_ide_port_info = {
	.host_flags		= IDE_HFLAG_NO_DMA,
};

static int __devinit plat_ide_probe(struct platform_device *pdev)
{
	struct resource *res_base, *res_alt, *res_irq;
	void __iomem *base, *alt_base;
	struct pata_platform_info *pdata;
	struct ide_host *host;
	int ret = 0, mmio = 0;
	hw_regs_t hw, *hws[] = { &hw, NULL, NULL, NULL };
	struct ide_port_info d = platform_ide_port_info;

	pdata = pdev->dev.platform_data;

	/* get a pointer to the register memory */
	res_base = platform_get_resource(pdev, IORESOURCE_IO, 0);
	res_alt = platform_get_resource(pdev, IORESOURCE_IO, 1);

	if (!res_base || !res_alt) {
		res_base = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		res_alt = platform_get_resource(pdev, IORESOURCE_MEM, 1);
		if (!res_base || !res_alt) {
			ret = -ENOMEM;
			goto out;
		}
		mmio = 1;
	}

	res_irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res_irq) {
		ret = -EINVAL;
		goto out;
	}

	if (mmio) {
		base = devm_ioremap(&pdev->dev,
			res_base->start, res_base->end - res_base->start + 1);
		alt_base = devm_ioremap(&pdev->dev,
			res_alt->start, res_alt->end - res_alt->start + 1);
	} else {
		base = devm_ioport_map(&pdev->dev,
			res_base->start, res_base->end - res_base->start + 1);
		alt_base = devm_ioport_map(&pdev->dev,
			res_alt->start, res_alt->end - res_alt->start + 1);
	}

	memset(&hw, 0, sizeof(hw));
	plat_ide_setup_ports(&hw, base, alt_base, pdata, res_irq->start);
	hw.dev = &pdev->dev;

	if (mmio)
		d.host_flags |= IDE_HFLAG_MMIO;

	ret = ide_host_add(&d, hws, &host);
	if (ret)
		goto out;

	platform_set_drvdata(pdev, host);

	return 0;

out:
	return ret;
}

static int __devexit plat_ide_remove(struct platform_device *pdev)
{
	struct ide_host *host = pdev->dev.driver_data;

	ide_host_remove(host);

	return 0;
}

static struct platform_driver platform_ide_driver = {
	.driver = {
		.name = "pata_platform",
		.owner = THIS_MODULE,
	},
	.probe = plat_ide_probe,
	.remove = __devexit_p(plat_ide_remove),
};

static int __init platform_ide_init(void)
{
	return platform_driver_register(&platform_ide_driver);
}

static void __exit platform_ide_exit(void)
{
	platform_driver_unregister(&platform_ide_driver);
}

MODULE_DESCRIPTION("Platform IDE driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:pata_platform");

module_init(platform_ide_init);
module_exit(platform_ide_exit);
