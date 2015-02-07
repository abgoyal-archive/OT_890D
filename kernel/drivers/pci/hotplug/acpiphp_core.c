

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/pci_hotplug.h>
#include <linux/slab.h>
#include <linux/smp.h>
#include "acpiphp.h"

#define MY_NAME	"acpiphp"

/* name size which is used for entries in pcihpfs */
#define SLOT_NAME_SIZE  21              /* {_SUN} */

static int debug;
int acpiphp_debug;

/* local variables */
static int num_slots;
static struct acpiphp_attention_info *attention_info;

#define DRIVER_VERSION	"0.5"
#define DRIVER_AUTHOR	"Greg Kroah-Hartman <gregkh@us.ibm.com>, Takayoshi Kochi <t-kochi@bq.jp.nec.com>, Matthew Wilcox <willy@hp.com>"
#define DRIVER_DESC	"ACPI Hot Plug PCI Controller Driver"

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");
MODULE_PARM_DESC(debug, "Debugging mode enabled or not");
module_param(debug, bool, 0644);

/* export the attention callback registration methods */
EXPORT_SYMBOL_GPL(acpiphp_register_attention);
EXPORT_SYMBOL_GPL(acpiphp_unregister_attention);

static int enable_slot		(struct hotplug_slot *slot);
static int disable_slot		(struct hotplug_slot *slot);
static int set_attention_status (struct hotplug_slot *slot, u8 value);
static int get_power_status	(struct hotplug_slot *slot, u8 *value);
static int get_attention_status (struct hotplug_slot *slot, u8 *value);
static int get_latch_status	(struct hotplug_slot *slot, u8 *value);
static int get_adapter_status	(struct hotplug_slot *slot, u8 *value);

static struct hotplug_slot_ops acpi_hotplug_slot_ops = {
	.owner			= THIS_MODULE,
	.enable_slot		= enable_slot,
	.disable_slot		= disable_slot,
	.set_attention_status	= set_attention_status,
	.get_power_status	= get_power_status,
	.get_attention_status	= get_attention_status,
	.get_latch_status	= get_latch_status,
	.get_adapter_status	= get_adapter_status,
};

int acpiphp_register_attention(struct acpiphp_attention_info *info)
{
	int retval = -EINVAL;

	if (info && info->owner && info->set_attn &&
			info->get_attn && !attention_info) {
		retval = 0;
		attention_info = info;
	}
	return retval;
}


int acpiphp_unregister_attention(struct acpiphp_attention_info *info)
{
	int retval = -EINVAL;

	if (info && attention_info == info) {
		attention_info = NULL;
		retval = 0;
	}
	return retval;
}


static int enable_slot(struct hotplug_slot *hotplug_slot)
{
	struct slot *slot = hotplug_slot->private;

	dbg("%s - physical_slot = %s\n", __func__, slot_name(slot));

	/* enable the specified slot */
	return acpiphp_enable_slot(slot->acpi_slot);
}


static int disable_slot(struct hotplug_slot *hotplug_slot)
{
	struct slot *slot = hotplug_slot->private;
	int retval;

	dbg("%s - physical_slot = %s\n", __func__, slot_name(slot));

	/* disable the specified slot */
	retval = acpiphp_disable_slot(slot->acpi_slot);
	if (!retval)
		retval = acpiphp_eject_slot(slot->acpi_slot);
	return retval;
}


 static int set_attention_status(struct hotplug_slot *hotplug_slot, u8 status)
 {
	int retval = -ENODEV;

	dbg("%s - physical_slot = %s\n", __func__, hotplug_slot_name(hotplug_slot));
 
	if (attention_info && try_module_get(attention_info->owner)) {
		retval = attention_info->set_attn(hotplug_slot, status);
		module_put(attention_info->owner);
	} else
		attention_info = NULL;
	return retval;
 }
 

static int get_power_status(struct hotplug_slot *hotplug_slot, u8 *value)
{
	struct slot *slot = hotplug_slot->private;

	dbg("%s - physical_slot = %s\n", __func__, slot_name(slot));

	*value = acpiphp_get_power_status(slot->acpi_slot);

	return 0;
}


static int get_attention_status(struct hotplug_slot *hotplug_slot, u8 *value)
{
	int retval = -EINVAL;

	dbg("%s - physical_slot = %s\n", __func__, hotplug_slot_name(hotplug_slot));

	if (attention_info && try_module_get(attention_info->owner)) {
		retval = attention_info->get_attn(hotplug_slot, value);
		module_put(attention_info->owner);
	} else
		attention_info = NULL;
	return retval;
}


static int get_latch_status(struct hotplug_slot *hotplug_slot, u8 *value)
{
	struct slot *slot = hotplug_slot->private;

	dbg("%s - physical_slot = %s\n", __func__, slot_name(slot));

	*value = acpiphp_get_latch_status(slot->acpi_slot);

	return 0;
}


static int get_adapter_status(struct hotplug_slot *hotplug_slot, u8 *value)
{
	struct slot *slot = hotplug_slot->private;

	dbg("%s - physical_slot = %s\n", __func__, slot_name(slot));

	*value = acpiphp_get_adapter_status(slot->acpi_slot);

	return 0;
}

static int __init init_acpi(void)
{
	int retval;

	/* initialize internal data structure etc. */
	retval = acpiphp_glue_init();

	/* read initial number of slots */
	if (!retval) {
		num_slots = acpiphp_get_num_slots();
		if (num_slots == 0) {
			acpiphp_glue_exit();
			retval = -ENODEV;
		}
	}

	return retval;
}

static void release_slot(struct hotplug_slot *hotplug_slot)
{
	struct slot *slot = hotplug_slot->private;

	dbg("%s - physical_slot = %s\n", __func__, slot_name(slot));

	kfree(slot->hotplug_slot);
	kfree(slot);
}

/* callback routine to initialize 'struct slot' for each slot */
int acpiphp_register_hotplug_slot(struct acpiphp_slot *acpiphp_slot)
{
	struct slot *slot;
	int retval = -ENOMEM;
	char name[SLOT_NAME_SIZE];

	slot = kzalloc(sizeof(*slot), GFP_KERNEL);
	if (!slot)
		goto error;

	slot->hotplug_slot = kzalloc(sizeof(*slot->hotplug_slot), GFP_KERNEL);
	if (!slot->hotplug_slot)
		goto error_slot;

	slot->hotplug_slot->info = &slot->info;

	slot->hotplug_slot->private = slot;
	slot->hotplug_slot->release = &release_slot;
	slot->hotplug_slot->ops = &acpi_hotplug_slot_ops;

	slot->acpi_slot = acpiphp_slot;
	slot->hotplug_slot->info->power_status = acpiphp_get_power_status(slot->acpi_slot);
	slot->hotplug_slot->info->attention_status = 0;
	slot->hotplug_slot->info->latch_status = acpiphp_get_latch_status(slot->acpi_slot);
	slot->hotplug_slot->info->adapter_status = acpiphp_get_adapter_status(slot->acpi_slot);
	slot->hotplug_slot->info->max_bus_speed = PCI_SPEED_UNKNOWN;
	slot->hotplug_slot->info->cur_bus_speed = PCI_SPEED_UNKNOWN;

	acpiphp_slot->slot = slot;
	snprintf(name, SLOT_NAME_SIZE, "%llu", slot->acpi_slot->sun);

	retval = pci_hp_register(slot->hotplug_slot,
					acpiphp_slot->bridge->pci_bus,
					acpiphp_slot->device,
					name);
	if (retval == -EBUSY)
		goto error_hpslot;
	if (retval) {
		err("pci_hp_register failed with error %d\n", retval);
		goto error_hpslot;
 	}

	info("Slot [%s] registered\n", slot_name(slot));

	return 0;
error_hpslot:
	kfree(slot->hotplug_slot);
error_slot:
	kfree(slot);
error:
	return retval;
}


void acpiphp_unregister_hotplug_slot(struct acpiphp_slot *acpiphp_slot)
{
	struct slot *slot = acpiphp_slot->slot;
	int retval = 0;

	info("Slot [%s] unregistered\n", slot_name(slot));

	retval = pci_hp_deregister(slot->hotplug_slot);
	if (retval)
		err("pci_hp_deregister failed with error %d\n", retval);
}


static int __init acpiphp_init(void)
{
	info(DRIVER_DESC " version: " DRIVER_VERSION "\n");

	if (acpi_pci_disabled)
		return 0;

	acpiphp_debug = debug;

	/* read all the ACPI info from the system */
	return init_acpi();
}


static void __exit acpiphp_exit(void)
{
	if (acpi_pci_disabled)
		return;

	/* deallocate internal data structures etc. */
	acpiphp_glue_exit();
}

module_init(acpiphp_init);
module_exit(acpiphp_exit);
