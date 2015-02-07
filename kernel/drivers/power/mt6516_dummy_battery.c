

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <mach/hardware.h>

#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/interrupt.h>

#include <linux/power_supply.h>





struct mt6516_ac_data {
    struct power_supply psy;
};

struct mt6516_battery_data {
    struct power_supply psy;
};

static enum power_supply_property mt6516_ac_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static enum power_supply_property mt6516_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_CAPACITY,
};

static int mt6516_ac_get_property(struct power_supply *psy,
			enum power_supply_property psp,
			union power_supply_propval *val)
{
    int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:		
		val->intval = 1;
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static int mt6516_battery_get_property(struct power_supply *psy,
				 enum power_supply_property psp,
				 union power_supply_propval *val)
{
    int ret = 0; 	
	
	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = POWER_SUPPLY_STATUS_FULL;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = POWER_SUPPLY_HEALTH_GOOD;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = 1;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = 100;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

// mt6516_ac_data initialization
static struct mt6516_ac_data mt6516_ac_main = {
    .psy = {
        .name = "ac",
        .type = POWER_SUPPLY_TYPE_MAINS,
        .properties = mt6516_ac_props,
        .num_properties = ARRAY_SIZE(mt6516_ac_props),
        .get_property = mt6516_ac_get_property,                
    },
};

// mt6516_battery_data initialization
static struct mt6516_battery_data mt6516_battery_main = {
    .psy = {
        .name = "battery",
        .type = POWER_SUPPLY_TYPE_BATTERY,
        .properties = mt6516_battery_props,
        .num_properties = ARRAY_SIZE(mt6516_battery_props),
        .get_property = mt6516_battery_get_property,                
    },
};


static int __init BAT_mod_init(void)
{
    int ret;

    printk("********* mt6516 BAT_probe *********\n");

    ret = power_supply_register(NULL, &mt6516_ac_main.psy);
    if (ret)
        return ret;
    
    ret = power_supply_register(NULL, &mt6516_battery_main.psy);
    
    return ret;
}

static void __exit BAT_mod_exit(void)
{
    power_supply_unregister(&mt6516_ac_main.psy);
    power_supply_unregister(&mt6516_battery_main.psy);
}

module_init(BAT_mod_init);
module_exit(BAT_mod_exit);

MODULE_AUTHOR("James Lo <James.lo@mediatek.com>");
MODULE_DESCRIPTION("mt6516 Battery Driver");
MODULE_LICENSE("GPL");

