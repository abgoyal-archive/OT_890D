

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/notifier.h>
#include <linux/platform_device.h>
#include <linux/jiffies.h>
#include <linux/stddef.h>
#include <acpi/acpi_bus.h>
#include <acpi/acpi_drivers.h>

#define ACPI_DOCK_DRIVER_DESCRIPTION "ACPI Dock Station Driver"

ACPI_MODULE_NAME("dock");
MODULE_AUTHOR("Kristen Carlson Accardi");
MODULE_DESCRIPTION(ACPI_DOCK_DRIVER_DESCRIPTION);
MODULE_LICENSE("GPL");

static int immediate_undock = 1;
module_param(immediate_undock, bool, 0644);
MODULE_PARM_DESC(immediate_undock, "1 (default) will cause the driver to "
	"undock immediately when the undock button is pressed, 0 will cause"
	" the driver to wait for userspace to write the undock sysfs file "
	" before undocking");

static struct atomic_notifier_head dock_notifier_list;
static char dock_device_name[] = "dock";

static const struct acpi_device_id dock_device_ids[] = {
	{"LNXDOCK", 0},
	{"", 0},
};
MODULE_DEVICE_TABLE(acpi, dock_device_ids);

struct dock_station {
	acpi_handle handle;
	unsigned long last_dock_time;
	u32 flags;
	spinlock_t dd_lock;
	struct mutex hp_lock;
	struct list_head dependent_devices;
	struct list_head hotplug_devices;

	struct list_head sibiling;
	struct platform_device *dock_device;
};
static LIST_HEAD(dock_stations);
static int dock_station_count;

struct dock_dependent_device {
	struct list_head list;
	struct list_head hotplug_list;
	acpi_handle handle;
	struct acpi_dock_ops *ops;
	void *context;
};

#define DOCK_DOCKING	0x00000001
#define DOCK_UNDOCKING  0x00000002
#define DOCK_IS_DOCK	0x00000010
#define DOCK_IS_ATA	0x00000020
#define DOCK_IS_BAT	0x00000040
#define DOCK_EVENT	3
#define UNDOCK_EVENT	2

static struct dock_dependent_device *
alloc_dock_dependent_device(acpi_handle handle)
{
	struct dock_dependent_device *dd;

	dd = kzalloc(sizeof(*dd), GFP_KERNEL);
	if (dd) {
		dd->handle = handle;
		INIT_LIST_HEAD(&dd->list);
		INIT_LIST_HEAD(&dd->hotplug_list);
	}
	return dd;
}

static void
add_dock_dependent_device(struct dock_station *ds,
			  struct dock_dependent_device *dd)
{
	spin_lock(&ds->dd_lock);
	list_add_tail(&dd->list, &ds->dependent_devices);
	spin_unlock(&ds->dd_lock);
}

static void
dock_add_hotplug_device(struct dock_station *ds,
			struct dock_dependent_device *dd)
{
	mutex_lock(&ds->hp_lock);
	list_add_tail(&dd->hotplug_list, &ds->hotplug_devices);
	mutex_unlock(&ds->hp_lock);
}

static void
dock_del_hotplug_device(struct dock_station *ds,
			struct dock_dependent_device *dd)
{
	mutex_lock(&ds->hp_lock);
	list_del(&dd->hotplug_list);
	mutex_unlock(&ds->hp_lock);
}

static struct dock_dependent_device *
find_dock_dependent_device(struct dock_station *ds, acpi_handle handle)
{
	struct dock_dependent_device *dd;

	spin_lock(&ds->dd_lock);
	list_for_each_entry(dd, &ds->dependent_devices, list) {
		if (handle == dd->handle) {
			spin_unlock(&ds->dd_lock);
			return dd;
		}
	}
	spin_unlock(&ds->dd_lock);
	return NULL;
}

static int is_dock(acpi_handle handle)
{
	acpi_status status;
	acpi_handle tmp;

	status = acpi_get_handle(handle, "_DCK", &tmp);
	if (ACPI_FAILURE(status))
		return 0;
	return 1;
}

static int is_ejectable(acpi_handle handle)
{
	acpi_status status;
	acpi_handle tmp;

	status = acpi_get_handle(handle, "_EJ0", &tmp);
	if (ACPI_FAILURE(status))
		return 0;
	return 1;
}

static int is_ata(acpi_handle handle)
{
	acpi_handle tmp;

	if ((ACPI_SUCCESS(acpi_get_handle(handle, "_GTF", &tmp))) ||
	   (ACPI_SUCCESS(acpi_get_handle(handle, "_GTM", &tmp))) ||
	   (ACPI_SUCCESS(acpi_get_handle(handle, "_STM", &tmp))) ||
	   (ACPI_SUCCESS(acpi_get_handle(handle, "_SDD", &tmp))))
		return 1;

	return 0;
}

static int is_battery(acpi_handle handle)
{
	struct acpi_device_info *info;
	struct acpi_buffer buffer = {ACPI_ALLOCATE_BUFFER, NULL};
	int ret = 1;

	if (!ACPI_SUCCESS(acpi_get_object_info(handle, &buffer)))
		return 0;
	info = buffer.pointer;
	if (!(info->valid & ACPI_VALID_HID))
		ret = 0;
	else
		ret = !strcmp("PNP0C0A", info->hardware_id.value);

	kfree(buffer.pointer);
	return ret;
}

static int is_ejectable_bay(acpi_handle handle)
{
	acpi_handle phandle;
	if (!is_ejectable(handle))
		return 0;
	if (is_battery(handle) || is_ata(handle))
		return 1;
	if (!acpi_get_parent(handle, &phandle) && is_ata(phandle))
		return 1;
	return 0;
}

int is_dock_device(acpi_handle handle)
{
	struct dock_station *dock_station;

	if (!dock_station_count)
		return 0;

	if (is_dock(handle))
		return 1;
	list_for_each_entry(dock_station, &dock_stations, sibiling) {
		if (find_dock_dependent_device(dock_station, handle))
			return 1;
	}

	return 0;
}

EXPORT_SYMBOL_GPL(is_dock_device);

static int dock_present(struct dock_station *ds)
{
	unsigned long long sta;
	acpi_status status;

	if (ds) {
		status = acpi_evaluate_integer(ds->handle, "_STA", NULL, &sta);
		if (ACPI_SUCCESS(status) && sta)
			return 1;
	}
	return 0;
}



static struct acpi_device * dock_create_acpi_device(acpi_handle handle)
{
	struct acpi_device *device = NULL;
	struct acpi_device *parent_device;
	acpi_handle parent;
	int ret;

	if (acpi_bus_get_device(handle, &device)) {
		/*
		 * no device created for this object,
		 * so we should create one.
		 */
		acpi_get_parent(handle, &parent);
		if (acpi_bus_get_device(parent, &parent_device))
			parent_device = NULL;

		ret = acpi_bus_add(&device, parent_device, handle,
			ACPI_BUS_TYPE_DEVICE);
		if (ret) {
			pr_debug("error adding bus, %x\n",
				-ret);
			return NULL;
		}
	}
	return device;
}

static void dock_remove_acpi_device(acpi_handle handle)
{
	struct acpi_device *device;
	int ret;

	if (!acpi_bus_get_device(handle, &device)) {
		ret = acpi_bus_trim(device, 1);
		if (ret)
			pr_debug("error removing bus, %x\n", -ret);
	}
}


static void hotplug_dock_devices(struct dock_station *ds, u32 event)
{
	struct dock_dependent_device *dd;

	mutex_lock(&ds->hp_lock);

	/*
	 * First call driver specific hotplug functions
	 */
	list_for_each_entry(dd, &ds->hotplug_devices, hotplug_list) {
		if (dd->ops && dd->ops->handler)
			dd->ops->handler(dd->handle, event, dd->context);
	}

	/*
	 * Now make sure that an acpi_device is created for each
	 * dependent device, or removed if this is an eject request.
	 * This will cause acpi_drivers to be stopped/started if they
	 * exist
	 */
	list_for_each_entry(dd, &ds->dependent_devices, list) {
		if (event == ACPI_NOTIFY_EJECT_REQUEST)
			dock_remove_acpi_device(dd->handle);
		else
			dock_create_acpi_device(dd->handle);
	}
	mutex_unlock(&ds->hp_lock);
}

static void dock_event(struct dock_station *ds, u32 event, int num)
{
	struct device *dev = &ds->dock_device->dev;
	char event_string[13];
	char *envp[] = { event_string, NULL };
	struct dock_dependent_device *dd;

	if (num == UNDOCK_EVENT)
		sprintf(event_string, "EVENT=undock");
	else
		sprintf(event_string, "EVENT=dock");

	/*
	 * Indicate that the status of the dock station has
	 * changed.
	 */
	if (num == DOCK_EVENT)
		kobject_uevent_env(&dev->kobj, KOBJ_CHANGE, envp);

	list_for_each_entry(dd, &ds->hotplug_devices, hotplug_list)
		if (dd->ops && dd->ops->uevent)
			dd->ops->uevent(dd->handle, event, dd->context);
	if (num != DOCK_EVENT)
		kobject_uevent_env(&dev->kobj, KOBJ_CHANGE, envp);
}

static void eject_dock(struct dock_station *ds)
{
	struct acpi_object_list arg_list;
	union acpi_object arg;
	acpi_status status;
	acpi_handle tmp;

	/* all dock devices should have _EJ0, but check anyway */
	status = acpi_get_handle(ds->handle, "_EJ0", &tmp);
	if (ACPI_FAILURE(status)) {
		pr_debug("No _EJ0 support for dock device\n");
		return;
	}

	arg_list.count = 1;
	arg_list.pointer = &arg;
	arg.type = ACPI_TYPE_INTEGER;
	arg.integer.value = 1;

	if (ACPI_FAILURE(acpi_evaluate_object(ds->handle, "_EJ0",
					      &arg_list, NULL)))
		pr_debug("Failed to evaluate _EJ0!\n");
}

static void handle_dock(struct dock_station *ds, int dock)
{
	acpi_status status;
	struct acpi_object_list arg_list;
	union acpi_object arg;
	struct acpi_buffer buffer = { ACPI_ALLOCATE_BUFFER, NULL };
	struct acpi_buffer name_buffer = { ACPI_ALLOCATE_BUFFER, NULL };

	acpi_get_name(ds->handle, ACPI_FULL_PATHNAME, &name_buffer);

	printk(KERN_INFO PREFIX "%s - %s\n",
		(char *)name_buffer.pointer, dock ? "docking" : "undocking");

	/* _DCK method has one argument */
	arg_list.count = 1;
	arg_list.pointer = &arg;
	arg.type = ACPI_TYPE_INTEGER;
	arg.integer.value = dock;
	status = acpi_evaluate_object(ds->handle, "_DCK", &arg_list, &buffer);
	if (ACPI_FAILURE(status) && status != AE_NOT_FOUND)
		ACPI_EXCEPTION((AE_INFO, status, "%s - failed to execute"
			" _DCK\n", (char *)name_buffer.pointer));

	kfree(buffer.pointer);
	kfree(name_buffer.pointer);
}

static inline void dock(struct dock_station *ds)
{
	handle_dock(ds, 1);
}

static inline void undock(struct dock_station *ds)
{
	handle_dock(ds, 0);
}

static inline void begin_dock(struct dock_station *ds)
{
	ds->flags |= DOCK_DOCKING;
}

static inline void complete_dock(struct dock_station *ds)
{
	ds->flags &= ~(DOCK_DOCKING);
	ds->last_dock_time = jiffies;
}

static inline void begin_undock(struct dock_station *ds)
{
	ds->flags |= DOCK_UNDOCKING;
}

static inline void complete_undock(struct dock_station *ds)
{
	ds->flags &= ~(DOCK_UNDOCKING);
}

static void dock_lock(struct dock_station *ds, int lock)
{
	struct acpi_object_list arg_list;
	union acpi_object arg;
	acpi_status status;

	arg_list.count = 1;
	arg_list.pointer = &arg;
	arg.type = ACPI_TYPE_INTEGER;
	arg.integer.value = !!lock;
	status = acpi_evaluate_object(ds->handle, "_LCK", &arg_list, NULL);
	if (ACPI_FAILURE(status) && status != AE_NOT_FOUND) {
		if (lock)
			printk(KERN_WARNING PREFIX "Locking device failed\n");
		else
			printk(KERN_WARNING PREFIX "Unlocking device failed\n");
	}
}

static int dock_in_progress(struct dock_station *ds)
{
	if ((ds->flags & DOCK_DOCKING) ||
	    time_before(jiffies, (ds->last_dock_time + HZ)))
		return 1;
	return 0;
}

int register_dock_notifier(struct notifier_block *nb)
{
	if (!dock_station_count)
		return -ENODEV;

	return atomic_notifier_chain_register(&dock_notifier_list, nb);
}

EXPORT_SYMBOL_GPL(register_dock_notifier);

void unregister_dock_notifier(struct notifier_block *nb)
{
	if (!dock_station_count)
		return;

	atomic_notifier_chain_unregister(&dock_notifier_list, nb);
}

EXPORT_SYMBOL_GPL(unregister_dock_notifier);

int
register_hotplug_dock_device(acpi_handle handle, struct acpi_dock_ops *ops,
			     void *context)
{
	struct dock_dependent_device *dd;
	struct dock_station *dock_station;
	int ret = -EINVAL;

	if (!dock_station_count)
		return -ENODEV;

	/*
	 * make sure this handle is for a device dependent on the dock,
	 * this would include the dock station itself
	 */
	list_for_each_entry(dock_station, &dock_stations, sibiling) {
		/*
		 * An ATA bay can be in a dock and itself can be ejected
		 * seperately, so there are two 'dock stations' which need the
		 * ops
		 */
		dd = find_dock_dependent_device(dock_station, handle);
		if (dd) {
			dd->ops = ops;
			dd->context = context;
			dock_add_hotplug_device(dock_station, dd);
			ret = 0;
		}
	}

	return ret;
}

EXPORT_SYMBOL_GPL(register_hotplug_dock_device);

void unregister_hotplug_dock_device(acpi_handle handle)
{
	struct dock_dependent_device *dd;
	struct dock_station *dock_station;

	if (!dock_station_count)
		return;

	list_for_each_entry(dock_station, &dock_stations, sibiling) {
		dd = find_dock_dependent_device(dock_station, handle);
		if (dd)
			dock_del_hotplug_device(dock_station, dd);
	}
}

EXPORT_SYMBOL_GPL(unregister_hotplug_dock_device);

static int handle_eject_request(struct dock_station *ds, u32 event)
{
	if (dock_in_progress(ds))
		return -EBUSY;

	/*
	 * here we need to generate the undock
	 * event prior to actually doing the undock
	 * so that the device struct still exists.
	 * Also, even send the dock event if the
	 * device is not present anymore
	 */
	dock_event(ds, event, UNDOCK_EVENT);

	hotplug_dock_devices(ds, ACPI_NOTIFY_EJECT_REQUEST);
	undock(ds);
	dock_lock(ds, 0);
	eject_dock(ds);
	if (dock_present(ds)) {
		printk(KERN_ERR PREFIX "Unable to undock!\n");
		return -EBUSY;
	}
	complete_undock(ds);
	return 0;
}

static void dock_notify(acpi_handle handle, u32 event, void *data)
{
	struct dock_station *ds = data;
	struct acpi_device *tmp;
	int surprise_removal = 0;

	/*
	 * According to acpi spec 3.0a, if a DEVICE_CHECK notification
	 * is sent and _DCK is present, it is assumed to mean an undock
	 * request.
	 */
	if ((ds->flags & DOCK_IS_DOCK) && event == ACPI_NOTIFY_DEVICE_CHECK)
		event = ACPI_NOTIFY_EJECT_REQUEST;

	/*
	 * dock station: BUS_CHECK - docked or surprise removal
	 *		 DEVICE_CHECK - undocked
	 * other device: BUS_CHECK/DEVICE_CHECK - added or surprise removal
	 *
	 * To simplify event handling, dock dependent device handler always
	 * get ACPI_NOTIFY_BUS_CHECK/ACPI_NOTIFY_DEVICE_CHECK for add and
	 * ACPI_NOTIFY_EJECT_REQUEST for removal
	 */
	switch (event) {
	case ACPI_NOTIFY_BUS_CHECK:
	case ACPI_NOTIFY_DEVICE_CHECK:
		if (!dock_in_progress(ds) && acpi_bus_get_device(ds->handle,
		   &tmp)) {
			begin_dock(ds);
			dock(ds);
			if (!dock_present(ds)) {
				printk(KERN_ERR PREFIX "Unable to dock!\n");
				complete_dock(ds);
				break;
			}
			atomic_notifier_call_chain(&dock_notifier_list,
						   event, NULL);
			hotplug_dock_devices(ds, event);
			complete_dock(ds);
			dock_event(ds, event, DOCK_EVENT);
			dock_lock(ds, 1);
			break;
		}
		if (dock_present(ds) || dock_in_progress(ds))
			break;
		/* This is a surprise removal */
		surprise_removal = 1;
		event = ACPI_NOTIFY_EJECT_REQUEST;
		/* Fall back */
	case ACPI_NOTIFY_EJECT_REQUEST:
		begin_undock(ds);
		if ((immediate_undock && !(ds->flags & DOCK_IS_ATA))
		   || surprise_removal)
			handle_eject_request(ds, event);
		else
			dock_event(ds, event, UNDOCK_EVENT);
		break;
	default:
		printk(KERN_ERR PREFIX "Unknown dock event %d\n", event);
	}
}

struct dock_data {
	acpi_handle handle;
	unsigned long event;
	struct dock_station *ds;
};

static void acpi_dock_deferred_cb(void *context)
{
	struct dock_data *data = (struct dock_data *)context;

	dock_notify(data->handle, data->event, data->ds);
	kfree(data);
}

static int acpi_dock_notifier_call(struct notifier_block *this,
	unsigned long event, void *data)
{
	struct dock_station *dock_station;
	acpi_handle handle = (acpi_handle)data;

	if (event != ACPI_NOTIFY_BUS_CHECK && event != ACPI_NOTIFY_DEVICE_CHECK
	   && event != ACPI_NOTIFY_EJECT_REQUEST)
		return 0;
	list_for_each_entry(dock_station, &dock_stations, sibiling) {
		if (dock_station->handle == handle) {
			struct dock_data *dock_data;

			dock_data = kmalloc(sizeof(*dock_data), GFP_KERNEL);
			if (!dock_data)
				return 0;
			dock_data->handle = handle;
			dock_data->event = event;
			dock_data->ds = dock_station;
			acpi_os_hotplug_execute(acpi_dock_deferred_cb,
				dock_data);
			return 0 ;
		}
	}
	return 0;
}

static struct notifier_block dock_acpi_notifier = {
	.notifier_call = acpi_dock_notifier_call,
};

static acpi_status
find_dock_devices(acpi_handle handle, u32 lvl, void *context, void **rv)
{
	acpi_status status;
	acpi_handle tmp, parent;
	struct dock_station *ds = context;
	struct dock_dependent_device *dd;

	status = acpi_bus_get_ejd(handle, &tmp);
	if (ACPI_FAILURE(status)) {
		/* try the parent device as well */
		status = acpi_get_parent(handle, &parent);
		if (ACPI_FAILURE(status))
			goto fdd_out;
		/* see if parent is dependent on dock */
		status = acpi_bus_get_ejd(parent, &tmp);
		if (ACPI_FAILURE(status))
			goto fdd_out;
	}

	if (tmp == ds->handle) {
		dd = alloc_dock_dependent_device(handle);
		if (dd)
			add_dock_dependent_device(ds, dd);
	}
fdd_out:
	return AE_OK;
}

static ssize_t show_docked(struct device *dev,
			   struct device_attribute *attr, char *buf)
{
	struct acpi_device *tmp;

	struct dock_station *dock_station = *((struct dock_station **)
		dev->platform_data);

	if (ACPI_SUCCESS(acpi_bus_get_device(dock_station->handle, &tmp)))
		return snprintf(buf, PAGE_SIZE, "1\n");
	return snprintf(buf, PAGE_SIZE, "0\n");
}
static DEVICE_ATTR(docked, S_IRUGO, show_docked, NULL);

static ssize_t show_flags(struct device *dev,
			  struct device_attribute *attr, char *buf)
{
	struct dock_station *dock_station = *((struct dock_station **)
		dev->platform_data);
	return snprintf(buf, PAGE_SIZE, "%d\n", dock_station->flags);

}
static DEVICE_ATTR(flags, S_IRUGO, show_flags, NULL);

static ssize_t write_undock(struct device *dev, struct device_attribute *attr,
			   const char *buf, size_t count)
{
	int ret;
	struct dock_station *dock_station = *((struct dock_station **)
		dev->platform_data);

	if (!count)
		return -EINVAL;

	begin_undock(dock_station);
	ret = handle_eject_request(dock_station, ACPI_NOTIFY_EJECT_REQUEST);
	return ret ? ret: count;
}
static DEVICE_ATTR(undock, S_IWUSR, NULL, write_undock);

static ssize_t show_dock_uid(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	unsigned long long lbuf;
	struct dock_station *dock_station = *((struct dock_station **)
		dev->platform_data);
	acpi_status status = acpi_evaluate_integer(dock_station->handle,
					"_UID", NULL, &lbuf);
	if (ACPI_FAILURE(status))
	    return 0;

	return snprintf(buf, PAGE_SIZE, "%llx\n", lbuf);
}
static DEVICE_ATTR(uid, S_IRUGO, show_dock_uid, NULL);

static ssize_t show_dock_type(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct dock_station *dock_station = *((struct dock_station **)
		dev->platform_data);
	char *type;

	if (dock_station->flags & DOCK_IS_DOCK)
		type = "dock_station";
	else if (dock_station->flags & DOCK_IS_ATA)
		type = "ata_bay";
	else if (dock_station->flags & DOCK_IS_BAT)
		type = "battery_bay";
	else
		type = "unknown";

	return snprintf(buf, PAGE_SIZE, "%s\n", type);
}
static DEVICE_ATTR(type, S_IRUGO, show_dock_type, NULL);

static int dock_add(acpi_handle handle)
{
	int ret;
	struct dock_dependent_device *dd;
	struct dock_station *dock_station;
	struct platform_device *dock_device;

	/* allocate & initialize the dock_station private data */
	dock_station = kzalloc(sizeof(*dock_station), GFP_KERNEL);
	if (!dock_station)
		return -ENOMEM;
	dock_station->handle = handle;
	dock_station->last_dock_time = jiffies - HZ;
	INIT_LIST_HEAD(&dock_station->dependent_devices);
	INIT_LIST_HEAD(&dock_station->hotplug_devices);
	INIT_LIST_HEAD(&dock_station->sibiling);
	spin_lock_init(&dock_station->dd_lock);
	mutex_init(&dock_station->hp_lock);
	ATOMIC_INIT_NOTIFIER_HEAD(&dock_notifier_list);

	/* initialize platform device stuff */
	dock_station->dock_device =
		platform_device_register_simple(dock_device_name,
			dock_station_count, NULL, 0);
	dock_device = dock_station->dock_device;
	if (IS_ERR(dock_device)) {
		kfree(dock_station);
		dock_station = NULL;
		return PTR_ERR(dock_device);
	}
	platform_device_add_data(dock_device, &dock_station,
		sizeof(struct dock_station *));

	/* we want the dock device to send uevents */
	dock_device->dev.uevent_suppress = 0;

	if (is_dock(handle))
		dock_station->flags |= DOCK_IS_DOCK;
	if (is_ata(handle))
		dock_station->flags |= DOCK_IS_ATA;
	if (is_battery(handle))
		dock_station->flags |= DOCK_IS_BAT;

	ret = device_create_file(&dock_device->dev, &dev_attr_docked);
	if (ret) {
		printk(KERN_ERR "Error %d adding sysfs file\n", ret);
		platform_device_unregister(dock_device);
		kfree(dock_station);
		dock_station = NULL;
		return ret;
	}
	ret = device_create_file(&dock_device->dev, &dev_attr_undock);
	if (ret) {
		printk(KERN_ERR "Error %d adding sysfs file\n", ret);
		device_remove_file(&dock_device->dev, &dev_attr_docked);
		platform_device_unregister(dock_device);
		kfree(dock_station);
		dock_station = NULL;
		return ret;
	}
	ret = device_create_file(&dock_device->dev, &dev_attr_uid);
	if (ret) {
		printk(KERN_ERR "Error %d adding sysfs file\n", ret);
		device_remove_file(&dock_device->dev, &dev_attr_docked);
		device_remove_file(&dock_device->dev, &dev_attr_undock);
		platform_device_unregister(dock_device);
		kfree(dock_station);
		dock_station = NULL;
		return ret;
	}
	ret = device_create_file(&dock_device->dev, &dev_attr_flags);
	if (ret) {
		printk(KERN_ERR "Error %d adding sysfs file\n", ret);
		device_remove_file(&dock_device->dev, &dev_attr_docked);
		device_remove_file(&dock_device->dev, &dev_attr_undock);
		device_remove_file(&dock_device->dev, &dev_attr_uid);
		platform_device_unregister(dock_device);
		kfree(dock_station);
		dock_station = NULL;
		return ret;
	}
	ret = device_create_file(&dock_device->dev, &dev_attr_type);
	if (ret)
		printk(KERN_ERR"Error %d adding sysfs file\n", ret);

	/* Find dependent devices */
	acpi_walk_namespace(ACPI_TYPE_DEVICE, ACPI_ROOT_OBJECT,
			    ACPI_UINT32_MAX, find_dock_devices, dock_station,
			    NULL);

	/* add the dock station as a device dependent on itself */
	dd = alloc_dock_dependent_device(handle);
	if (!dd) {
		kfree(dock_station);
		dock_station = NULL;
		ret = -ENOMEM;
		goto dock_add_err_unregister;
	}
	add_dock_dependent_device(dock_station, dd);

	dock_station_count++;
	list_add(&dock_station->sibiling, &dock_stations);
	return 0;

dock_add_err_unregister:
	device_remove_file(&dock_device->dev, &dev_attr_type);
	device_remove_file(&dock_device->dev, &dev_attr_docked);
	device_remove_file(&dock_device->dev, &dev_attr_undock);
	device_remove_file(&dock_device->dev, &dev_attr_uid);
	device_remove_file(&dock_device->dev, &dev_attr_flags);
	platform_device_unregister(dock_device);
	kfree(dock_station);
	dock_station = NULL;
	return ret;
}

static int dock_remove(struct dock_station *dock_station)
{
	struct dock_dependent_device *dd, *tmp;
	struct platform_device *dock_device = dock_station->dock_device;

	if (!dock_station_count)
		return 0;

	/* remove dependent devices */
	list_for_each_entry_safe(dd, tmp, &dock_station->dependent_devices,
				 list)
	    kfree(dd);

	/* cleanup sysfs */
	device_remove_file(&dock_device->dev, &dev_attr_type);
	device_remove_file(&dock_device->dev, &dev_attr_docked);
	device_remove_file(&dock_device->dev, &dev_attr_undock);
	device_remove_file(&dock_device->dev, &dev_attr_uid);
	device_remove_file(&dock_device->dev, &dev_attr_flags);
	platform_device_unregister(dock_device);

	/* free dock station memory */
	kfree(dock_station);
	dock_station = NULL;
	return 0;
}

static acpi_status
find_dock(acpi_handle handle, u32 lvl, void *context, void **rv)
{
	acpi_status status = AE_OK;

	if (is_dock(handle)) {
		if (dock_add(handle) >= 0) {
			status = AE_CTRL_TERMINATE;
		}
	}
	return status;
}

static acpi_status
find_bay(acpi_handle handle, u32 lvl, void *context, void **rv)
{
	/* If bay is a dock, it's already handled */
	if (is_ejectable_bay(handle) && !is_dock(handle))
		dock_add(handle);
	return AE_OK;
}

static int __init dock_init(void)
{
	if (acpi_disabled)
		return 0;

	/* look for a dock station */
	acpi_walk_namespace(ACPI_TYPE_DEVICE, ACPI_ROOT_OBJECT,
			    ACPI_UINT32_MAX, find_dock, NULL, NULL);

	/* look for bay */
	acpi_walk_namespace(ACPI_TYPE_DEVICE, ACPI_ROOT_OBJECT,
			ACPI_UINT32_MAX, find_bay, NULL, NULL);
	if (!dock_station_count) {
		printk(KERN_INFO PREFIX "No dock devices found.\n");
		return 0;
	}

	register_acpi_bus_notifier(&dock_acpi_notifier);
	printk(KERN_INFO PREFIX "%s: %d docks/bays found\n",
		ACPI_DOCK_DRIVER_DESCRIPTION, dock_station_count);
	return 0;
}

static void __exit dock_exit(void)
{
	struct dock_station *dock_station;

	unregister_acpi_bus_notifier(&dock_acpi_notifier);
	list_for_each_entry(dock_station, &dock_stations, sibiling)
		dock_remove(dock_station);
}

subsys_initcall(dock_init);
module_exit(dock_exit);
