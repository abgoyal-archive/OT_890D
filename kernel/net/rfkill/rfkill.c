

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/workqueue.h>
#include <linux/capability.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/rfkill.h>

/* Get declaration of rfkill_switch_all() to shut up sparse. */
#include "rfkill-input.h"


MODULE_AUTHOR("Ivo van Doorn <IvDoorn@gmail.com>");
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("RF switch support");
MODULE_LICENSE("GPL");

static LIST_HEAD(rfkill_list);	/* list of registered rf switches */
static DEFINE_MUTEX(rfkill_global_mutex);

static unsigned int rfkill_default_state = RFKILL_STATE_UNBLOCKED;
module_param_named(default_state, rfkill_default_state, uint, 0444);
MODULE_PARM_DESC(default_state,
		 "Default initial state for all radio types, 0 = radio off");

struct rfkill_gsw_state {
	enum rfkill_state current_state;
	enum rfkill_state default_state;
};

static struct rfkill_gsw_state rfkill_global_states[RFKILL_TYPE_MAX];
static unsigned long rfkill_states_lockdflt[BITS_TO_LONGS(RFKILL_TYPE_MAX)];
static bool rfkill_epo_lock_active;


#ifdef CONFIG_RFKILL_LEDS
static void rfkill_led_trigger(struct rfkill *rfkill,
			       enum rfkill_state state)
{
	struct led_trigger *led = &rfkill->led_trigger;

	if (!led->name)
		return;
	if (state != RFKILL_STATE_UNBLOCKED)
		led_trigger_event(led, LED_OFF);
	else
		led_trigger_event(led, LED_FULL);
}

static void rfkill_led_trigger_activate(struct led_classdev *led)
{
	struct rfkill *rfkill = container_of(led->trigger,
			struct rfkill, led_trigger);

	rfkill_led_trigger(rfkill, rfkill->state);
}
#endif /* CONFIG_RFKILL_LEDS */

static void rfkill_uevent(struct rfkill *rfkill)
{
	kobject_uevent(&rfkill->dev.kobj, KOBJ_CHANGE);
}

static void update_rfkill_state(struct rfkill *rfkill)
{
	enum rfkill_state newstate, oldstate;

	if (rfkill->get_state) {
		mutex_lock(&rfkill->mutex);
		if (!rfkill->get_state(rfkill->data, &newstate)) {
			oldstate = rfkill->state;
			rfkill->state = newstate;
			if (oldstate != newstate)
				rfkill_uevent(rfkill);
		}
		mutex_unlock(&rfkill->mutex);
	}
}

static int rfkill_toggle_radio(struct rfkill *rfkill,
				enum rfkill_state state,
				int force)
{
	int retval = 0;
	enum rfkill_state oldstate, newstate;

	if (unlikely(rfkill->dev.power.power_state.event & PM_EVENT_SLEEP))
		return -EBUSY;

	oldstate = rfkill->state;

	if (rfkill->get_state && !force &&
	    !rfkill->get_state(rfkill->data, &newstate))
		rfkill->state = newstate;

	switch (state) {
	case RFKILL_STATE_HARD_BLOCKED:
		/* typically happens when refreshing hardware state,
		 * such as on resume */
		state = RFKILL_STATE_SOFT_BLOCKED;
		break;
	case RFKILL_STATE_UNBLOCKED:
		/* force can't override this, only rfkill_force_state() can */
		if (rfkill->state == RFKILL_STATE_HARD_BLOCKED)
			return -EPERM;
		break;
	case RFKILL_STATE_SOFT_BLOCKED:
		/* nothing to do, we want to give drivers the hint to double
		 * BLOCK even a transmitter that is already in state
		 * RFKILL_STATE_HARD_BLOCKED */
		break;
	default:
		WARN(1, KERN_WARNING
			"rfkill: illegal state %d passed as parameter "
			"to rfkill_toggle_radio\n", state);
		return -EINVAL;
	}

	if (force || state != rfkill->state) {
		retval = rfkill->toggle_radio(rfkill->data, state);
		/* never allow a HARD->SOFT downgrade! */
		if (!retval && rfkill->state != RFKILL_STATE_HARD_BLOCKED)
			rfkill->state = state;
	}

	if (force || rfkill->state != oldstate)
		rfkill_uevent(rfkill);

	return retval;
}

static void __rfkill_switch_all(const enum rfkill_type type,
				const enum rfkill_state state)
{
	struct rfkill *rfkill;

	if (WARN((state >= RFKILL_STATE_MAX || type >= RFKILL_TYPE_MAX),
			KERN_WARNING
			"rfkill: illegal state %d or type %d "
			"passed as parameter to __rfkill_switch_all\n",
			state, type))
		return;

	rfkill_global_states[type].current_state = state;
	list_for_each_entry(rfkill, &rfkill_list, node) {
		if ((!rfkill->user_claim) && (rfkill->type == type)) {
			mutex_lock(&rfkill->mutex);
			rfkill_toggle_radio(rfkill, state, 0);
			mutex_unlock(&rfkill->mutex);
		}
	}
}

void rfkill_switch_all(enum rfkill_type type, enum rfkill_state state)
{
	mutex_lock(&rfkill_global_mutex);
	if (!rfkill_epo_lock_active)
		__rfkill_switch_all(type, state);
	mutex_unlock(&rfkill_global_mutex);
}
EXPORT_SYMBOL(rfkill_switch_all);

void rfkill_epo(void)
{
	struct rfkill *rfkill;
	int i;

	mutex_lock(&rfkill_global_mutex);

	rfkill_epo_lock_active = true;
	list_for_each_entry(rfkill, &rfkill_list, node) {
		mutex_lock(&rfkill->mutex);
		rfkill_toggle_radio(rfkill, RFKILL_STATE_SOFT_BLOCKED, 1);
		mutex_unlock(&rfkill->mutex);
	}
	for (i = 0; i < RFKILL_TYPE_MAX; i++) {
		rfkill_global_states[i].default_state =
				rfkill_global_states[i].current_state;
		rfkill_global_states[i].current_state =
				RFKILL_STATE_SOFT_BLOCKED;
	}
	mutex_unlock(&rfkill_global_mutex);
}
EXPORT_SYMBOL_GPL(rfkill_epo);

void rfkill_restore_states(void)
{
	int i;

	mutex_lock(&rfkill_global_mutex);

	rfkill_epo_lock_active = false;
	for (i = 0; i < RFKILL_TYPE_MAX; i++)
		__rfkill_switch_all(i, rfkill_global_states[i].default_state);
	mutex_unlock(&rfkill_global_mutex);
}
EXPORT_SYMBOL_GPL(rfkill_restore_states);

void rfkill_remove_epo_lock(void)
{
	mutex_lock(&rfkill_global_mutex);
	rfkill_epo_lock_active = false;
	mutex_unlock(&rfkill_global_mutex);
}
EXPORT_SYMBOL_GPL(rfkill_remove_epo_lock);

bool rfkill_is_epo_lock_active(void)
{
	return rfkill_epo_lock_active;
}
EXPORT_SYMBOL_GPL(rfkill_is_epo_lock_active);

enum rfkill_state rfkill_get_global_state(const enum rfkill_type type)
{
	return rfkill_global_states[type].current_state;
}
EXPORT_SYMBOL_GPL(rfkill_get_global_state);

int rfkill_force_state(struct rfkill *rfkill, enum rfkill_state state)
{
	enum rfkill_state oldstate;

	BUG_ON(!rfkill);
	if (WARN((state >= RFKILL_STATE_MAX),
			KERN_WARNING
			"rfkill: illegal state %d passed as parameter "
			"to rfkill_force_state\n", state))
		return -EINVAL;

	mutex_lock(&rfkill->mutex);

	oldstate = rfkill->state;
	rfkill->state = state;

	if (state != oldstate)
		rfkill_uevent(rfkill);

	mutex_unlock(&rfkill->mutex);

	return 0;
}
EXPORT_SYMBOL(rfkill_force_state);

static ssize_t rfkill_name_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct rfkill *rfkill = to_rfkill(dev);

	return sprintf(buf, "%s\n", rfkill->name);
}

static const char *rfkill_get_type_str(enum rfkill_type type)
{
	switch (type) {
	case RFKILL_TYPE_WLAN:
		return "wlan";
	case RFKILL_TYPE_BLUETOOTH:
		return "bluetooth";
	case RFKILL_TYPE_UWB:
		return "ultrawideband";
	case RFKILL_TYPE_WIMAX:
		return "wimax";
	case RFKILL_TYPE_WWAN:
		return "wwan";
	default:
		BUG();
	}
}

static ssize_t rfkill_type_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct rfkill *rfkill = to_rfkill(dev);

	return sprintf(buf, "%s\n", rfkill_get_type_str(rfkill->type));
}

static ssize_t rfkill_state_show(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	struct rfkill *rfkill = to_rfkill(dev);

	update_rfkill_state(rfkill);
	return sprintf(buf, "%d\n", rfkill->state);
}

static ssize_t rfkill_state_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct rfkill *rfkill = to_rfkill(dev);
	unsigned long state;
	int error;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	error = strict_strtoul(buf, 0, &state);
	if (error)
		return error;

	/* RFKILL_STATE_HARD_BLOCKED is illegal here... */
	if (state != RFKILL_STATE_UNBLOCKED &&
	    state != RFKILL_STATE_SOFT_BLOCKED)
		return -EINVAL;

	error = mutex_lock_killable(&rfkill->mutex);
	if (error)
		return error;

	if (!rfkill_epo_lock_active)
		error = rfkill_toggle_radio(rfkill, state, 0);
	else
		error = -EPERM;

	mutex_unlock(&rfkill->mutex);

	return error ? error : count;
}

static ssize_t rfkill_claim_show(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	struct rfkill *rfkill = to_rfkill(dev);

	return sprintf(buf, "%d\n", rfkill->user_claim);
}

static ssize_t rfkill_claim_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct rfkill *rfkill = to_rfkill(dev);
	unsigned long claim_tmp;
	bool claim;
	int error;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (rfkill->user_claim_unsupported)
		return -EOPNOTSUPP;

	error = strict_strtoul(buf, 0, &claim_tmp);
	if (error)
		return error;
	claim = !!claim_tmp;

	/*
	 * Take the global lock to make sure the kernel is not in
	 * the middle of rfkill_switch_all
	 */
	error = mutex_lock_killable(&rfkill_global_mutex);
	if (error)
		return error;

	if (rfkill->user_claim != claim) {
		if (!claim && !rfkill_epo_lock_active) {
			mutex_lock(&rfkill->mutex);
			rfkill_toggle_radio(rfkill,
					rfkill_global_states[rfkill->type].current_state,
					0);
			mutex_unlock(&rfkill->mutex);
		}
		rfkill->user_claim = claim;
	}

	mutex_unlock(&rfkill_global_mutex);

	return error ? error : count;
}

static struct device_attribute rfkill_dev_attrs[] = {
	__ATTR(name, S_IRUGO, rfkill_name_show, NULL),
	__ATTR(type, S_IRUGO, rfkill_type_show, NULL),
	__ATTR(state, S_IRUGO|S_IWUSR, rfkill_state_show, rfkill_state_store),
	__ATTR(claim, S_IRUGO|S_IWUSR, rfkill_claim_show, rfkill_claim_store),
	__ATTR_NULL
};

static void rfkill_release(struct device *dev)
{
	struct rfkill *rfkill = to_rfkill(dev);

	kfree(rfkill);
	module_put(THIS_MODULE);
}

#ifdef CONFIG_RFKILL_PM
static int rfkill_suspend(struct device *dev, pm_message_t state)
{
	struct rfkill *rfkill = to_rfkill(dev);

	/* mark class device as suspended */
	if (dev->power.power_state.event != state.event)
		dev->power.power_state = state;

	/* store state for the resume handler */
	rfkill->state_for_resume = rfkill->state;

	return 0;
}

static int rfkill_resume(struct device *dev)
{
	struct rfkill *rfkill = to_rfkill(dev);
	enum rfkill_state newstate;

	if (dev->power.power_state.event != PM_EVENT_ON) {
		mutex_lock(&rfkill->mutex);

		dev->power.power_state.event = PM_EVENT_ON;

		/*
		 * rfkill->state could have been modified before we got
		 * called, and won't be updated by rfkill_toggle_radio()
		 * in force mode.  Sync it FIRST.
		 */
		if (rfkill->get_state &&
		    !rfkill->get_state(rfkill->data, &newstate))
			rfkill->state = newstate;

		/*
		 * If we are under EPO, kick transmitter offline,
		 * otherwise restore to pre-suspend state.
		 *
		 * Issue a notification in any case
		 */
		rfkill_toggle_radio(rfkill,
				rfkill_epo_lock_active ?
					RFKILL_STATE_SOFT_BLOCKED :
					rfkill->state_for_resume,
				1);

		mutex_unlock(&rfkill->mutex);
	}

	return 0;
}
#else
#define rfkill_suspend NULL
#define rfkill_resume NULL
#endif

static int rfkill_dev_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	struct rfkill *rfkill = to_rfkill(dev);
	int error;

	error = add_uevent_var(env, "RFKILL_NAME=%s", rfkill->name);
	if (error)
		return error;
	error = add_uevent_var(env, "RFKILL_TYPE=%s",
				rfkill_get_type_str(rfkill->type));
	if (error)
		return error;
	error = add_uevent_var(env, "RFKILL_STATE=%d", rfkill->state);
	return error;
}

static struct class rfkill_class = {
	.name		= "rfkill",
	.dev_release	= rfkill_release,
	.dev_attrs	= rfkill_dev_attrs,
	.suspend	= rfkill_suspend,
	.resume		= rfkill_resume,
	.dev_uevent	= rfkill_dev_uevent,
};

static int rfkill_check_duplicity(const struct rfkill *rfkill)
{
	struct rfkill *p;
	unsigned long seen[BITS_TO_LONGS(RFKILL_TYPE_MAX)];

	memset(seen, 0, sizeof(seen));

	list_for_each_entry(p, &rfkill_list, node) {
		if (WARN((p == rfkill), KERN_WARNING
				"rfkill: illegal attempt to register "
				"an already registered rfkill struct\n"))
			return -EEXIST;
		set_bit(p->type, seen);
	}

	/* 0: first switch of its kind */
	return (test_bit(rfkill->type, seen)) ? 1 : 0;
}

static int rfkill_add_switch(struct rfkill *rfkill)
{
	int error;

	mutex_lock(&rfkill_global_mutex);

	error = rfkill_check_duplicity(rfkill);
	if (error < 0)
		goto unlock_out;

	if (!error) {
		/* lock default after first use */
		set_bit(rfkill->type, rfkill_states_lockdflt);
		rfkill_global_states[rfkill->type].current_state =
			rfkill_global_states[rfkill->type].default_state;
	}

	rfkill_toggle_radio(rfkill,
			    rfkill_global_states[rfkill->type].current_state,
			    0);

	list_add_tail(&rfkill->node, &rfkill_list);

	error = 0;
unlock_out:
	mutex_unlock(&rfkill_global_mutex);

	return error;
}

static void rfkill_remove_switch(struct rfkill *rfkill)
{
	mutex_lock(&rfkill_global_mutex);
	list_del_init(&rfkill->node);
	mutex_unlock(&rfkill_global_mutex);

	mutex_lock(&rfkill->mutex);
	rfkill_toggle_radio(rfkill, RFKILL_STATE_SOFT_BLOCKED, 1);
	mutex_unlock(&rfkill->mutex);
}

struct rfkill * __must_check rfkill_allocate(struct device *parent,
					     enum rfkill_type type)
{
	struct rfkill *rfkill;
	struct device *dev;

	if (WARN((type >= RFKILL_TYPE_MAX),
			KERN_WARNING
			"rfkill: illegal type %d passed as parameter "
			"to rfkill_allocate\n", type))
		return NULL;

	rfkill = kzalloc(sizeof(struct rfkill), GFP_KERNEL);
	if (!rfkill)
		return NULL;

	mutex_init(&rfkill->mutex);
	INIT_LIST_HEAD(&rfkill->node);
	rfkill->type = type;

	dev = &rfkill->dev;
	dev->class = &rfkill_class;
	dev->parent = parent;
	device_initialize(dev);

	__module_get(THIS_MODULE);

	return rfkill;
}
EXPORT_SYMBOL(rfkill_allocate);

void rfkill_free(struct rfkill *rfkill)
{
	if (rfkill)
		put_device(&rfkill->dev);
}
EXPORT_SYMBOL(rfkill_free);

static void rfkill_led_trigger_register(struct rfkill *rfkill)
{
#ifdef CONFIG_RFKILL_LEDS
	int error;

	if (!rfkill->led_trigger.name)
		rfkill->led_trigger.name = dev_name(&rfkill->dev);
	if (!rfkill->led_trigger.activate)
		rfkill->led_trigger.activate = rfkill_led_trigger_activate;
	error = led_trigger_register(&rfkill->led_trigger);
	if (error)
		rfkill->led_trigger.name = NULL;
#endif /* CONFIG_RFKILL_LEDS */
}

static void rfkill_led_trigger_unregister(struct rfkill *rfkill)
{
#ifdef CONFIG_RFKILL_LEDS
	if (rfkill->led_trigger.name) {
		led_trigger_unregister(&rfkill->led_trigger);
		rfkill->led_trigger.name = NULL;
	}
#endif
}

int __must_check rfkill_register(struct rfkill *rfkill)
{
	static atomic_t rfkill_no = ATOMIC_INIT(0);
	struct device *dev = &rfkill->dev;
	int error;

	if (WARN((!rfkill || !rfkill->toggle_radio ||
			rfkill->type >= RFKILL_TYPE_MAX ||
			rfkill->state >= RFKILL_STATE_MAX),
			KERN_WARNING
			"rfkill: attempt to register a "
			"badly initialized rfkill struct\n"))
		return -EINVAL;

	dev_set_name(dev, "rfkill%ld", (long)atomic_inc_return(&rfkill_no) - 1);

	rfkill_led_trigger_register(rfkill);

	error = rfkill_add_switch(rfkill);
	if (error) {
		rfkill_led_trigger_unregister(rfkill);
		return error;
	}

	error = device_add(dev);
	if (error) {
		rfkill_remove_switch(rfkill);
		rfkill_led_trigger_unregister(rfkill);
		return error;
	}

	return 0;
}
EXPORT_SYMBOL(rfkill_register);

void rfkill_unregister(struct rfkill *rfkill)
{
	BUG_ON(!rfkill);
	device_del(&rfkill->dev);
	rfkill_remove_switch(rfkill);
	rfkill_led_trigger_unregister(rfkill);
	put_device(&rfkill->dev);
}
EXPORT_SYMBOL(rfkill_unregister);

int rfkill_set_default(enum rfkill_type type, enum rfkill_state state)
{
	int error;

	if (WARN((type >= RFKILL_TYPE_MAX ||
			(state != RFKILL_STATE_SOFT_BLOCKED &&
			 state != RFKILL_STATE_UNBLOCKED)),
			KERN_WARNING
			"rfkill: illegal state %d or type %d passed as "
			"parameter to rfkill_set_default\n", state, type))
		return -EINVAL;

	mutex_lock(&rfkill_global_mutex);

	if (!test_and_set_bit(type, rfkill_states_lockdflt)) {
		rfkill_global_states[type].default_state = state;
		rfkill_global_states[type].current_state = state;
		error = 0;
	} else
		error = -EPERM;

	mutex_unlock(&rfkill_global_mutex);
	return error;
}
EXPORT_SYMBOL_GPL(rfkill_set_default);

static int __init rfkill_init(void)
{
	int error;
	int i;

	/* RFKILL_STATE_HARD_BLOCKED is illegal here... */
	if (rfkill_default_state != RFKILL_STATE_SOFT_BLOCKED &&
	    rfkill_default_state != RFKILL_STATE_UNBLOCKED)
		return -EINVAL;

	for (i = 0; i < RFKILL_TYPE_MAX; i++)
		rfkill_global_states[i].default_state = rfkill_default_state;

	error = class_register(&rfkill_class);
	if (error) {
		printk(KERN_ERR "rfkill: unable to register rfkill class\n");
		return error;
	}

	return 0;
}

static void __exit rfkill_exit(void)
{
	class_unregister(&rfkill_class);
}

subsys_initcall(rfkill_init);
module_exit(rfkill_exit);
