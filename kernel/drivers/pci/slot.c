

#include <linux/kobject.h>
#include <linux/pci.h>
#include <linux/err.h>
#include "pci.h"

struct kset *pci_slots_kset;
EXPORT_SYMBOL_GPL(pci_slots_kset);

static ssize_t pci_slot_attr_show(struct kobject *kobj,
					struct attribute *attr, char *buf)
{
	struct pci_slot *slot = to_pci_slot(kobj);
	struct pci_slot_attribute *attribute = to_pci_slot_attr(attr);
	return attribute->show ? attribute->show(slot, buf) : -EIO;
}

static ssize_t pci_slot_attr_store(struct kobject *kobj,
			struct attribute *attr, const char *buf, size_t len)
{
	struct pci_slot *slot = to_pci_slot(kobj);
	struct pci_slot_attribute *attribute = to_pci_slot_attr(attr);
	return attribute->store ? attribute->store(slot, buf, len) : -EIO;
}

static struct sysfs_ops pci_slot_sysfs_ops = {
	.show = pci_slot_attr_show,
	.store = pci_slot_attr_store,
};

static ssize_t address_read_file(struct pci_slot *slot, char *buf)
{
	if (slot->number == 0xff)
		return sprintf(buf, "%04x:%02x\n",
				pci_domain_nr(slot->bus),
				slot->bus->number);
	else
		return sprintf(buf, "%04x:%02x:%02x\n",
				pci_domain_nr(slot->bus),
				slot->bus->number,
				slot->number);
}

static void pci_slot_release(struct kobject *kobj)
{
	struct pci_dev *dev;
	struct pci_slot *slot = to_pci_slot(kobj);

	pr_debug("%s: releasing pci_slot on %x:%d\n", __func__,
		 slot->bus->number, slot->number);

	list_for_each_entry(dev, &slot->bus->devices, bus_list)
		if (PCI_SLOT(dev->devfn) == slot->number)
			dev->slot = NULL;

	list_del(&slot->list);

	kfree(slot);
}

static struct pci_slot_attribute pci_slot_attr_address =
	__ATTR(address, (S_IFREG | S_IRUGO), address_read_file, NULL);

static struct attribute *pci_slot_default_attrs[] = {
	&pci_slot_attr_address.attr,
	NULL,
};

static struct kobj_type pci_slot_ktype = {
	.sysfs_ops = &pci_slot_sysfs_ops,
	.release = &pci_slot_release,
	.default_attrs = pci_slot_default_attrs,
};

static char *make_slot_name(const char *name)
{
	char *new_name;
	int len, max, dup;

	new_name = kstrdup(name, GFP_KERNEL);
	if (!new_name)
		return NULL;

	/*
	 * Make sure we hit the realloc case the first time through the
	 * loop.  'len' will be strlen(name) + 3 at that point which is
	 * enough space for "name-X" and the trailing NUL.
	 */
	len = strlen(name) + 2;
	max = 1;
	dup = 1;

	for (;;) {
		struct kobject *dup_slot;
		dup_slot = kset_find_obj(pci_slots_kset, new_name);
		if (!dup_slot)
			break;
		kobject_put(dup_slot);
		if (dup == max) {
			len++;
			max *= 10;
			kfree(new_name);
			new_name = kmalloc(len, GFP_KERNEL);
			if (!new_name)
				break;
		}
		sprintf(new_name, "%s-%d", name, dup++);
	}

	return new_name;
}

static int rename_slot(struct pci_slot *slot, const char *name)
{
	int result = 0;
	char *slot_name;

	if (strcmp(pci_slot_name(slot), name) == 0)
		return result;

	slot_name = make_slot_name(name);
	if (!slot_name)
		return -ENOMEM;

	result = kobject_rename(&slot->kobj, slot_name);
	kfree(slot_name);

	return result;
}

static struct pci_slot *get_slot(struct pci_bus *parent, int slot_nr)
{
	struct pci_slot *slot;
	/*
	 * We already hold pci_bus_sem so don't worry
	 */
	list_for_each_entry(slot, &parent->slots, list)
		if (slot->number == slot_nr) {
			kobject_get(&slot->kobj);
			return slot;
		}

	return NULL;
}

struct pci_slot *pci_create_slot(struct pci_bus *parent, int slot_nr,
				 const char *name,
				 struct hotplug_slot *hotplug)
{
	struct pci_dev *dev;
	struct pci_slot *slot;
	int err = 0;
	char *slot_name = NULL;

	down_write(&pci_bus_sem);

	if (slot_nr == -1)
		goto placeholder;

	/*
	 * Hotplug drivers are allowed to rename an existing slot,
	 * but only if not already claimed.
	 */
	slot = get_slot(parent, slot_nr);
	if (slot) {
		if (hotplug) {
			if ((err = slot->hotplug ? -EBUSY : 0)
			     || (err = rename_slot(slot, name))) {
				kobject_put(&slot->kobj);
				slot = NULL;
				goto err;
			}
		}
		goto out;
	}

placeholder:
	slot = kzalloc(sizeof(*slot), GFP_KERNEL);
	if (!slot) {
		err = -ENOMEM;
		goto err;
	}

	slot->bus = parent;
	slot->number = slot_nr;

	slot->kobj.kset = pci_slots_kset;

	slot_name = make_slot_name(name);
	if (!slot_name) {
		err = -ENOMEM;
		goto err;
	}

	err = kobject_init_and_add(&slot->kobj, &pci_slot_ktype, NULL,
				   "%s", slot_name);
	if (err)
		goto err;

	INIT_LIST_HEAD(&slot->list);
	list_add(&slot->list, &parent->slots);

	list_for_each_entry(dev, &parent->devices, bus_list)
		if (PCI_SLOT(dev->devfn) == slot_nr)
			dev->slot = slot;

	/* Don't care if debug printk has a -1 for slot_nr */
	pr_debug("%s: created pci_slot on %04x:%02x:%02x\n",
		 __func__, pci_domain_nr(parent), parent->number, slot_nr);

out:
	kfree(slot_name);
	up_write(&pci_bus_sem);
	return slot;
err:
	kfree(slot);
	slot = ERR_PTR(err);
	goto out;
}
EXPORT_SYMBOL_GPL(pci_create_slot);

void pci_renumber_slot(struct pci_slot *slot, int slot_nr)
{
	struct pci_slot *tmp;

	down_write(&pci_bus_sem);

	list_for_each_entry(tmp, &slot->bus->slots, list) {
		WARN_ON(tmp->number == slot_nr);
		goto out;
	}

	slot->number = slot_nr;
out:
	up_write(&pci_bus_sem);
}
EXPORT_SYMBOL_GPL(pci_renumber_slot);

void pci_destroy_slot(struct pci_slot *slot)
{
	pr_debug("%s: dec refcount to %d on %04x:%02x:%02x\n", __func__,
		 atomic_read(&slot->kobj.kref.refcount) - 1,
		 pci_domain_nr(slot->bus), slot->bus->number, slot->number);

	down_write(&pci_bus_sem);
	kobject_put(&slot->kobj);
	up_write(&pci_bus_sem);
}
EXPORT_SYMBOL_GPL(pci_destroy_slot);

static int pci_slot_init(void)
{
	struct kset *pci_bus_kset;

	pci_bus_kset = bus_get_kset(&pci_bus_type);
	pci_slots_kset = kset_create_and_add("slots", NULL,
						&pci_bus_kset->kobj);
	if (!pci_slots_kset) {
		printk(KERN_ERR "PCI: Slot initialization failure\n");
		return -ENOMEM;
	}
	return 0;
}

subsys_initcall(pci_slot_init);
