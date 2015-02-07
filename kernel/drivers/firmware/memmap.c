

#include <linux/string.h>
#include <linux/firmware-map.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/bootmem.h>


struct firmware_map_entry {
	resource_size_t		start;	/* start of the memory range */
	resource_size_t		end;	/* end of the memory range (incl.) */
	const char		*type;	/* type of the memory range */
	struct list_head	list;	/* entry for the linked list */
	struct kobject		kobj;   /* kobject for each entry */
};

static ssize_t memmap_attr_show(struct kobject *kobj,
				struct attribute *attr, char *buf);
static ssize_t start_show(struct firmware_map_entry *entry, char *buf);
static ssize_t end_show(struct firmware_map_entry *entry, char *buf);
static ssize_t type_show(struct firmware_map_entry *entry, char *buf);


struct memmap_attribute {
	struct attribute attr;
	ssize_t (*show)(struct firmware_map_entry *entry, char *buf);
};

static struct memmap_attribute memmap_start_attr = __ATTR_RO(start);
static struct memmap_attribute memmap_end_attr   = __ATTR_RO(end);
static struct memmap_attribute memmap_type_attr  = __ATTR_RO(type);

static struct attribute *def_attrs[] = {
	&memmap_start_attr.attr,
	&memmap_end_attr.attr,
	&memmap_type_attr.attr,
	NULL
};

static struct sysfs_ops memmap_attr_ops = {
	.show = memmap_attr_show,
};

static struct kobj_type memmap_ktype = {
	.sysfs_ops	= &memmap_attr_ops,
	.default_attrs	= def_attrs,
};


static LIST_HEAD(map_entries);

static int firmware_map_add_entry(resource_size_t start, resource_size_t end,
				  const char *type,
				  struct firmware_map_entry *entry)
{
	BUG_ON(start > end);

	entry->start = start;
	entry->end = end;
	entry->type = type;
	INIT_LIST_HEAD(&entry->list);
	kobject_init(&entry->kobj, &memmap_ktype);

	list_add_tail(&entry->list, &map_entries);

	return 0;
}

int firmware_map_add(resource_size_t start, resource_size_t end,
		     const char *type)
{
	struct firmware_map_entry *entry;

	entry = kmalloc(sizeof(struct firmware_map_entry), GFP_ATOMIC);
	if (!entry)
		return -ENOMEM;

	return firmware_map_add_entry(start, end, type, entry);
}

int __init firmware_map_add_early(resource_size_t start, resource_size_t end,
				  const char *type)
{
	struct firmware_map_entry *entry;

	entry = alloc_bootmem_low(sizeof(struct firmware_map_entry));
	if (WARN_ON(!entry))
		return -ENOMEM;

	return firmware_map_add_entry(start, end, type, entry);
}


static ssize_t start_show(struct firmware_map_entry *entry, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "0x%llx\n",
		(unsigned long long)entry->start);
}

static ssize_t end_show(struct firmware_map_entry *entry, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "0x%llx\n",
		(unsigned long long)entry->end);
}

static ssize_t type_show(struct firmware_map_entry *entry, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", entry->type);
}

#define to_memmap_attr(_attr) container_of(_attr, struct memmap_attribute, attr)
#define to_memmap_entry(obj) container_of(obj, struct firmware_map_entry, kobj)

static ssize_t memmap_attr_show(struct kobject *kobj,
				struct attribute *attr, char *buf)
{
	struct firmware_map_entry *entry = to_memmap_entry(kobj);
	struct memmap_attribute *memmap_attr = to_memmap_attr(attr);

	return memmap_attr->show(entry, buf);
}

static int __init memmap_init(void)
{
	int i = 0;
	struct firmware_map_entry *entry;
	struct kset *memmap_kset;

	memmap_kset = kset_create_and_add("memmap", NULL, firmware_kobj);
	if (WARN_ON(!memmap_kset))
		return -ENOMEM;

	list_for_each_entry(entry, &map_entries, list) {
		entry->kobj.kset = memmap_kset;
		if (kobject_add(&entry->kobj, NULL, "%d", i++))
			kobject_put(&entry->kobj);
	}

	return 0;
}
late_initcall(memmap_init);

