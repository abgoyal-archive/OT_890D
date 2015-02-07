

#include <linux/pm_qos_params.h>
#include <linux/sched.h>
#include <linux/smp_lock.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/init.h>

#include <linux/uaccess.h>

struct requirement_list {
	struct list_head list;
	union {
		s32 value;
		s32 usec;
		s32 kbps;
	};
	char *name;
};

static s32 max_compare(s32 v1, s32 v2);
static s32 min_compare(s32 v1, s32 v2);

struct pm_qos_object {
	struct requirement_list requirements;
	struct blocking_notifier_head *notifiers;
	struct miscdevice pm_qos_power_miscdev;
	char *name;
	s32 default_value;
	atomic_t target_value;
	s32 (*comparitor)(s32, s32);
};

static struct pm_qos_object null_pm_qos;
static BLOCKING_NOTIFIER_HEAD(cpu_dma_lat_notifier);
static struct pm_qos_object cpu_dma_pm_qos = {
	.requirements = {LIST_HEAD_INIT(cpu_dma_pm_qos.requirements.list)},
	.notifiers = &cpu_dma_lat_notifier,
	.name = "cpu_dma_latency",
	.default_value = 2000 * USEC_PER_SEC,
	.target_value = ATOMIC_INIT(2000 * USEC_PER_SEC),
	.comparitor = min_compare
};

static BLOCKING_NOTIFIER_HEAD(network_lat_notifier);
static struct pm_qos_object network_lat_pm_qos = {
	.requirements = {LIST_HEAD_INIT(network_lat_pm_qos.requirements.list)},
	.notifiers = &network_lat_notifier,
	.name = "network_latency",
	.default_value = 2000 * USEC_PER_SEC,
	.target_value = ATOMIC_INIT(2000 * USEC_PER_SEC),
	.comparitor = min_compare
};


static BLOCKING_NOTIFIER_HEAD(network_throughput_notifier);
static struct pm_qos_object network_throughput_pm_qos = {
	.requirements =
		{LIST_HEAD_INIT(network_throughput_pm_qos.requirements.list)},
	.notifiers = &network_throughput_notifier,
	.name = "network_throughput",
	.default_value = 0,
	.target_value = ATOMIC_INIT(0),
	.comparitor = max_compare
};


static struct pm_qos_object *pm_qos_array[] = {
	&null_pm_qos,
	&cpu_dma_pm_qos,
	&network_lat_pm_qos,
	&network_throughput_pm_qos
};

static DEFINE_SPINLOCK(pm_qos_lock);

static ssize_t pm_qos_power_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *f_pos);
static int pm_qos_power_open(struct inode *inode, struct file *filp);
static int pm_qos_power_release(struct inode *inode, struct file *filp);

static const struct file_operations pm_qos_power_fops = {
	.write = pm_qos_power_write,
	.open = pm_qos_power_open,
	.release = pm_qos_power_release,
};

/* static helper functions */
static s32 max_compare(s32 v1, s32 v2)
{
	return max(v1, v2);
}

static s32 min_compare(s32 v1, s32 v2)
{
	return min(v1, v2);
}


static void update_target(int target)
{
	s32 extreme_value;
	struct requirement_list *node;
	unsigned long flags;
	int call_notifier = 0;

	spin_lock_irqsave(&pm_qos_lock, flags);
	extreme_value = pm_qos_array[target]->default_value;
	list_for_each_entry(node,
			&pm_qos_array[target]->requirements.list, list) {
		extreme_value = pm_qos_array[target]->comparitor(
				extreme_value, node->value);
	}
	if (atomic_read(&pm_qos_array[target]->target_value) != extreme_value) {
		call_notifier = 1;
		atomic_set(&pm_qos_array[target]->target_value, extreme_value);
		pr_debug(KERN_ERR "new target for qos %d is %d\n", target,
			atomic_read(&pm_qos_array[target]->target_value));
	}
	spin_unlock_irqrestore(&pm_qos_lock, flags);

	if (call_notifier)
		blocking_notifier_call_chain(pm_qos_array[target]->notifiers,
			(unsigned long) extreme_value, NULL);
}

static int register_pm_qos_misc(struct pm_qos_object *qos)
{
	qos->pm_qos_power_miscdev.minor = MISC_DYNAMIC_MINOR;
	qos->pm_qos_power_miscdev.name = qos->name;
	qos->pm_qos_power_miscdev.fops = &pm_qos_power_fops;

	return misc_register(&qos->pm_qos_power_miscdev);
}

static int find_pm_qos_object_by_minor(int minor)
{
	int pm_qos_class;

	for (pm_qos_class = 0;
		pm_qos_class < PM_QOS_NUM_CLASSES; pm_qos_class++) {
		if (minor ==
			pm_qos_array[pm_qos_class]->pm_qos_power_miscdev.minor)
			return pm_qos_class;
	}
	return -1;
}

int pm_qos_requirement(int pm_qos_class)
{
	return atomic_read(&pm_qos_array[pm_qos_class]->target_value);
}
EXPORT_SYMBOL_GPL(pm_qos_requirement);

int pm_qos_add_requirement(int pm_qos_class, char *name, s32 value)
{
	struct requirement_list *dep;
	unsigned long flags;

	dep = kzalloc(sizeof(struct requirement_list), GFP_KERNEL);
	if (dep) {
		if (value == PM_QOS_DEFAULT_VALUE)
			dep->value = pm_qos_array[pm_qos_class]->default_value;
		else
			dep->value = value;
		dep->name = kstrdup(name, GFP_KERNEL);
		if (!dep->name)
			goto cleanup;

		spin_lock_irqsave(&pm_qos_lock, flags);
		list_add(&dep->list,
			&pm_qos_array[pm_qos_class]->requirements.list);
		spin_unlock_irqrestore(&pm_qos_lock, flags);
		update_target(pm_qos_class);

		return 0;
	}

cleanup:
	kfree(dep);
	return -ENOMEM;
}
EXPORT_SYMBOL_GPL(pm_qos_add_requirement);

int pm_qos_update_requirement(int pm_qos_class, char *name, s32 new_value)
{
	unsigned long flags;
	struct requirement_list *node;
	int pending_update = 0;

	spin_lock_irqsave(&pm_qos_lock, flags);
	list_for_each_entry(node,
		&pm_qos_array[pm_qos_class]->requirements.list, list) {
		if (strcmp(node->name, name) == 0) {
			if (new_value == PM_QOS_DEFAULT_VALUE)
				node->value =
				pm_qos_array[pm_qos_class]->default_value;
			else
				node->value = new_value;
			pending_update = 1;
			break;
		}
	}
	spin_unlock_irqrestore(&pm_qos_lock, flags);
	if (pending_update)
		update_target(pm_qos_class);

	return 0;
}
EXPORT_SYMBOL_GPL(pm_qos_update_requirement);

void pm_qos_remove_requirement(int pm_qos_class, char *name)
{
	unsigned long flags;
	struct requirement_list *node;
	int pending_update = 0;

	spin_lock_irqsave(&pm_qos_lock, flags);
	list_for_each_entry(node,
		&pm_qos_array[pm_qos_class]->requirements.list, list) {
		if (strcmp(node->name, name) == 0) {
			kfree(node->name);
			list_del(&node->list);
			kfree(node);
			pending_update = 1;
			break;
		}
	}
	spin_unlock_irqrestore(&pm_qos_lock, flags);
	if (pending_update)
		update_target(pm_qos_class);
}
EXPORT_SYMBOL_GPL(pm_qos_remove_requirement);

 int pm_qos_add_notifier(int pm_qos_class, struct notifier_block *notifier)
{
	int retval;

	retval = blocking_notifier_chain_register(
			pm_qos_array[pm_qos_class]->notifiers, notifier);

	return retval;
}
EXPORT_SYMBOL_GPL(pm_qos_add_notifier);

int pm_qos_remove_notifier(int pm_qos_class, struct notifier_block *notifier)
{
	int retval;

	retval = blocking_notifier_chain_unregister(
			pm_qos_array[pm_qos_class]->notifiers, notifier);

	return retval;
}
EXPORT_SYMBOL_GPL(pm_qos_remove_notifier);

#define PID_NAME_LEN sizeof("process_1234567890")
static char name[PID_NAME_LEN];

static int pm_qos_power_open(struct inode *inode, struct file *filp)
{
	int ret;
	long pm_qos_class;

	lock_kernel();
	pm_qos_class = find_pm_qos_object_by_minor(iminor(inode));
	if (pm_qos_class >= 0) {
		filp->private_data = (void *)pm_qos_class;
		sprintf(name, "process_%d", current->pid);
		ret = pm_qos_add_requirement(pm_qos_class, name,
					PM_QOS_DEFAULT_VALUE);
		if (ret >= 0) {
			unlock_kernel();
			return 0;
		}
	}
	unlock_kernel();

	return -EPERM;
}

static int pm_qos_power_release(struct inode *inode, struct file *filp)
{
	int pm_qos_class;

	pm_qos_class = (long)filp->private_data;
	sprintf(name, "process_%d", current->pid);
	pm_qos_remove_requirement(pm_qos_class, name);

	return 0;
}

static ssize_t pm_qos_power_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *f_pos)
{
	s32 value;
	int pm_qos_class;

	pm_qos_class = (long)filp->private_data;
	if (count != sizeof(s32))
		return -EINVAL;
	if (copy_from_user(&value, buf, sizeof(s32)))
		return -EFAULT;
	sprintf(name, "process_%d", current->pid);
	pm_qos_update_requirement(pm_qos_class, name, value);

	return  sizeof(s32);
}


static int __init pm_qos_power_init(void)
{
	int ret = 0;

	ret = register_pm_qos_misc(&cpu_dma_pm_qos);
	if (ret < 0) {
		printk(KERN_ERR "pm_qos_param: cpu_dma_latency setup failed\n");
		return ret;
	}
	ret = register_pm_qos_misc(&network_lat_pm_qos);
	if (ret < 0) {
		printk(KERN_ERR "pm_qos_param: network_latency setup failed\n");
		return ret;
	}
	ret = register_pm_qos_misc(&network_throughput_pm_qos);
	if (ret < 0)
		printk(KERN_ERR
			"pm_qos_param: network_throughput setup failed\n");

	return ret;
}

late_initcall(pm_qos_power_init);
