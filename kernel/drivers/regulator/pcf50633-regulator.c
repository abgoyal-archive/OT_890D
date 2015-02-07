

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/platform_device.h>

#include <linux/mfd/pcf50633/core.h>
#include <linux/mfd/pcf50633/pmic.h>

#define PCF50633_REGULATOR(_name, _id) 		\
	{					\
		.name = _name, 			\
		.id = _id,			\
		.ops = &pcf50633_regulator_ops,	\
		.type = REGULATOR_VOLTAGE, 	\
		.owner = THIS_MODULE, 		\
	}

static const u8 pcf50633_regulator_registers[PCF50633_NUM_REGULATORS] = {
	[PCF50633_REGULATOR_AUTO]	= PCF50633_REG_AUTOOUT,
	[PCF50633_REGULATOR_DOWN1]	= PCF50633_REG_DOWN1OUT,
	[PCF50633_REGULATOR_DOWN2]	= PCF50633_REG_DOWN2OUT,
	[PCF50633_REGULATOR_MEMLDO]	= PCF50633_REG_MEMLDOOUT,
	[PCF50633_REGULATOR_LDO1]	= PCF50633_REG_LDO1OUT,
	[PCF50633_REGULATOR_LDO2]	= PCF50633_REG_LDO2OUT,
	[PCF50633_REGULATOR_LDO3]	= PCF50633_REG_LDO3OUT,
	[PCF50633_REGULATOR_LDO4]	= PCF50633_REG_LDO4OUT,
	[PCF50633_REGULATOR_LDO5]	= PCF50633_REG_LDO5OUT,
	[PCF50633_REGULATOR_LDO6]	= PCF50633_REG_LDO6OUT,
	[PCF50633_REGULATOR_HCLDO]	= PCF50633_REG_HCLDOOUT,
};

/* Bits from voltage value */
static u8 auto_voltage_bits(unsigned int millivolts)
{
	if (millivolts < 1800)
		return 0;
	if (millivolts > 3800)
		return 0xff;

	millivolts -= 625;

	return millivolts / 25;
}

static u8 down_voltage_bits(unsigned int millivolts)
{
	if (millivolts < 625)
		return 0;
	else if (millivolts > 3000)
		return 0xff;

	millivolts -= 625;

	return millivolts / 25;
}

static u8 ldo_voltage_bits(unsigned int millivolts)
{
	if (millivolts < 900)
		return 0;
	else if (millivolts > 3600)
		return 0x1f;

	millivolts -= 900;
	return millivolts / 100;
}

/* Obtain voltage value from bits */
static unsigned int auto_voltage_value(u8 bits)
{
	if (bits < 0x2f)
		return 0;

	return 625 + (bits * 25);
}


static unsigned int down_voltage_value(u8 bits)
{
	return 625 + (bits * 25);
}


static unsigned int ldo_voltage_value(u8 bits)
{
	bits &= 0x1f;

	return 900 + (bits * 100);
}

static int pcf50633_regulator_set_voltage(struct regulator_dev *rdev,
						int min_uV, int max_uV)
{
	struct pcf50633 *pcf;
	int regulator_id, millivolts;
	u8 volt_bits, regnr;

	pcf = rdev_get_drvdata(rdev);

	regulator_id = rdev_get_id(rdev);
	if (regulator_id >= PCF50633_NUM_REGULATORS)
		return -EINVAL;

	millivolts = min_uV / 1000;

	regnr = pcf50633_regulator_registers[regulator_id];

	switch (regulator_id) {
	case PCF50633_REGULATOR_AUTO:
		volt_bits = auto_voltage_bits(millivolts);
		break;
	case PCF50633_REGULATOR_DOWN1:
		volt_bits = down_voltage_bits(millivolts);
		break;
	case PCF50633_REGULATOR_DOWN2:
		volt_bits = down_voltage_bits(millivolts);
		break;
	case PCF50633_REGULATOR_LDO1:
	case PCF50633_REGULATOR_LDO2:
	case PCF50633_REGULATOR_LDO3:
	case PCF50633_REGULATOR_LDO4:
	case PCF50633_REGULATOR_LDO5:
	case PCF50633_REGULATOR_LDO6:
	case PCF50633_REGULATOR_HCLDO:
		volt_bits = ldo_voltage_bits(millivolts);
		break;
	default:
		return -EINVAL;
	}

	return pcf50633_reg_write(pcf, regnr, volt_bits);
}

static int pcf50633_regulator_get_voltage(struct regulator_dev *rdev)
{
	struct pcf50633 *pcf;
	int regulator_id, millivolts, volt_bits;
	u8 regnr;

	pcf = rdev_get_drvdata(rdev);;

	regulator_id = rdev_get_id(rdev);
	if (regulator_id >= PCF50633_NUM_REGULATORS)
		return -EINVAL;

	regnr = pcf50633_regulator_registers[regulator_id];

	volt_bits = pcf50633_reg_read(pcf, regnr);
	if (volt_bits < 0)
		return -1;

	switch (regulator_id) {
	case PCF50633_REGULATOR_AUTO:
		millivolts = auto_voltage_value(volt_bits);
		break;
	case PCF50633_REGULATOR_DOWN1:
		millivolts = down_voltage_value(volt_bits);
		break;
	case PCF50633_REGULATOR_DOWN2:
		millivolts = down_voltage_value(volt_bits);
		break;
	case PCF50633_REGULATOR_LDO1:
	case PCF50633_REGULATOR_LDO2:
	case PCF50633_REGULATOR_LDO3:
	case PCF50633_REGULATOR_LDO4:
	case PCF50633_REGULATOR_LDO5:
	case PCF50633_REGULATOR_LDO6:
	case PCF50633_REGULATOR_HCLDO:
		millivolts = ldo_voltage_value(volt_bits);
		break;
	default:
		return -EINVAL;
	}

	return millivolts * 1000;
}

static int pcf50633_regulator_enable(struct regulator_dev *rdev)
{
	struct pcf50633 *pcf = rdev_get_drvdata(rdev);
	int regulator_id;
	u8 regnr;

	regulator_id = rdev_get_id(rdev);
	if (regulator_id >= PCF50633_NUM_REGULATORS)
		return -EINVAL;

	/* The *ENA register is always one after the *OUT register */
	regnr = pcf50633_regulator_registers[regulator_id] + 1;

	return pcf50633_reg_set_bit_mask(pcf, regnr, PCF50633_REGULATOR_ON,
						       PCF50633_REGULATOR_ON);
}

static int pcf50633_regulator_disable(struct regulator_dev *rdev)
{
	struct pcf50633 *pcf = rdev_get_drvdata(rdev);
	int regulator_id;
	u8 regnr;

	regulator_id = rdev_get_id(rdev);
	if (regulator_id >= PCF50633_NUM_REGULATORS)
		return -EINVAL;

	/* the *ENA register is always one after the *OUT register */
	regnr = pcf50633_regulator_registers[regulator_id] + 1;

	return pcf50633_reg_set_bit_mask(pcf, regnr,
					PCF50633_REGULATOR_ON, 0);
}

static int pcf50633_regulator_is_enabled(struct regulator_dev *rdev)
{
	struct pcf50633 *pcf = rdev_get_drvdata(rdev);
	int regulator_id = rdev_get_id(rdev);
	u8 regnr;

	regulator_id = rdev_get_id(rdev);
	if (regulator_id >= PCF50633_NUM_REGULATORS)
		return -EINVAL;

	/* the *ENA register is always one after the *OUT register */
	regnr = pcf50633_regulator_registers[regulator_id] + 1;

	return pcf50633_reg_read(pcf, regnr) & PCF50633_REGULATOR_ON;
}

static struct regulator_ops pcf50633_regulator_ops = {
	.set_voltage = pcf50633_regulator_set_voltage,
	.get_voltage = pcf50633_regulator_get_voltage,
	.enable = pcf50633_regulator_enable,
	.disable = pcf50633_regulator_disable,
	.is_enabled = pcf50633_regulator_is_enabled,
};

static struct regulator_desc regulators[] = {
	[PCF50633_REGULATOR_AUTO] =
		PCF50633_REGULATOR("auto", PCF50633_REGULATOR_AUTO),
	[PCF50633_REGULATOR_DOWN1] =
		PCF50633_REGULATOR("down1", PCF50633_REGULATOR_DOWN1),
	[PCF50633_REGULATOR_DOWN2] =
		PCF50633_REGULATOR("down2", PCF50633_REGULATOR_DOWN2),
	[PCF50633_REGULATOR_LDO1] =
		PCF50633_REGULATOR("ldo1", PCF50633_REGULATOR_LDO1),
	[PCF50633_REGULATOR_LDO2] =
		PCF50633_REGULATOR("ldo2", PCF50633_REGULATOR_LDO2),
	[PCF50633_REGULATOR_LDO3] =
		PCF50633_REGULATOR("ldo3", PCF50633_REGULATOR_LDO3),
	[PCF50633_REGULATOR_LDO4] =
		PCF50633_REGULATOR("ldo4", PCF50633_REGULATOR_LDO4),
	[PCF50633_REGULATOR_LDO5] =
		PCF50633_REGULATOR("ldo5", PCF50633_REGULATOR_LDO5),
	[PCF50633_REGULATOR_LDO6] =
		PCF50633_REGULATOR("ldo6", PCF50633_REGULATOR_LDO6),
	[PCF50633_REGULATOR_HCLDO] =
		PCF50633_REGULATOR("hcldo", PCF50633_REGULATOR_HCLDO),
	[PCF50633_REGULATOR_MEMLDO] =
		PCF50633_REGULATOR("memldo", PCF50633_REGULATOR_MEMLDO),
};

static int __devinit pcf50633_regulator_probe(struct platform_device *pdev)
{
	struct regulator_dev *rdev;
	struct pcf50633 *pcf;

	/* Already set by core driver */
	pcf = platform_get_drvdata(pdev);

	rdev = regulator_register(&regulators[pdev->id], &pdev->dev, pcf);
	if (IS_ERR(rdev))
		return PTR_ERR(rdev);

	if (pcf->pdata->regulator_registered)
		pcf->pdata->regulator_registered(pcf, pdev->id);

	return 0;
}

static int __devexit pcf50633_regulator_remove(struct platform_device *pdev)
{
	struct regulator_dev *rdev = platform_get_drvdata(pdev);

	regulator_unregister(rdev);

	return 0;
}

static struct platform_driver pcf50633_regulator_driver = {
	.driver = {
		.name = "pcf50633-regltr",
	},
	.probe = pcf50633_regulator_probe,
	.remove = __devexit_p(pcf50633_regulator_remove),
};

static int __init pcf50633_regulator_init(void)
{
	return platform_driver_register(&pcf50633_regulator_driver);
}
module_init(pcf50633_regulator_init);

static void __exit pcf50633_regulator_exit(void)
{
	platform_driver_unregister(&pcf50633_regulator_driver);
}
module_exit(pcf50633_regulator_exit);

MODULE_AUTHOR("Balaji Rao <balajirrao@openmoko.org>");
MODULE_DESCRIPTION("PCF50633 regulator driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:pcf50633-regulator");
