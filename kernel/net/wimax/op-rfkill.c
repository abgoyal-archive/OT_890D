

#include <net/wimax.h>
#include <net/genetlink.h>
#include <linux/wimax.h>
#include <linux/security.h>
#include <linux/rfkill.h>
#include <linux/input.h>
#include "wimax-internal.h"

#define D_SUBMODULE op_rfkill
#include "debug-levels.h"

#if defined(CONFIG_RFKILL) || defined(CONFIG_RFKILL_MODULE)


void wimax_report_rfkill_hw(struct wimax_dev *wimax_dev,
			    enum wimax_rf_state state)
{
	int result;
	struct device *dev = wimax_dev_to_dev(wimax_dev);
	enum wimax_st wimax_state;
	enum rfkill_state rfkill_state;

	d_fnstart(3, dev, "(wimax_dev %p state %u)\n", wimax_dev, state);
	BUG_ON(state == WIMAX_RF_QUERY);
	BUG_ON(state != WIMAX_RF_ON && state != WIMAX_RF_OFF);

	mutex_lock(&wimax_dev->mutex);
	result = wimax_dev_is_ready(wimax_dev);
	if (result < 0)
		goto error_not_ready;

	if (state != wimax_dev->rf_hw) {
		wimax_dev->rf_hw = state;
		rfkill_state = state == WIMAX_RF_ON ?
			RFKILL_STATE_OFF : RFKILL_STATE_ON;
		if (wimax_dev->rf_hw == WIMAX_RF_ON
		    && wimax_dev->rf_sw == WIMAX_RF_ON)
			wimax_state = WIMAX_ST_READY;
		else
			wimax_state = WIMAX_ST_RADIO_OFF;
		__wimax_state_change(wimax_dev, wimax_state);
		input_report_key(wimax_dev->rfkill_input, KEY_WIMAX,
				 rfkill_state);
	}
error_not_ready:
	mutex_unlock(&wimax_dev->mutex);
	d_fnend(3, dev, "(wimax_dev %p state %u) = void [%d]\n",
		wimax_dev, state, result);
}
EXPORT_SYMBOL_GPL(wimax_report_rfkill_hw);


void wimax_report_rfkill_sw(struct wimax_dev *wimax_dev,
			    enum wimax_rf_state state)
{
	int result;
	struct device *dev = wimax_dev_to_dev(wimax_dev);
	enum wimax_st wimax_state;

	d_fnstart(3, dev, "(wimax_dev %p state %u)\n", wimax_dev, state);
	BUG_ON(state == WIMAX_RF_QUERY);
	BUG_ON(state != WIMAX_RF_ON && state != WIMAX_RF_OFF);

	mutex_lock(&wimax_dev->mutex);
	result = wimax_dev_is_ready(wimax_dev);
	if (result < 0)
		goto error_not_ready;

	if (state != wimax_dev->rf_sw) {
		wimax_dev->rf_sw = state;
		if (wimax_dev->rf_hw == WIMAX_RF_ON
		    && wimax_dev->rf_sw == WIMAX_RF_ON)
			wimax_state = WIMAX_ST_READY;
		else
			wimax_state = WIMAX_ST_RADIO_OFF;
		__wimax_state_change(wimax_dev, wimax_state);
	}
error_not_ready:
	mutex_unlock(&wimax_dev->mutex);
	d_fnend(3, dev, "(wimax_dev %p state %u) = void [%d]\n",
		wimax_dev, state, result);
}
EXPORT_SYMBOL_GPL(wimax_report_rfkill_sw);


static
int __wimax_rf_toggle_radio(struct wimax_dev *wimax_dev,
			    enum wimax_rf_state state)
{
	int result = 0;
	struct device *dev = wimax_dev_to_dev(wimax_dev);
	enum wimax_st wimax_state;

	might_sleep();
	d_fnstart(3, dev, "(wimax_dev %p state %u)\n", wimax_dev, state);
	if (wimax_dev->rf_sw == state)
		goto out_no_change;
	if (wimax_dev->op_rfkill_sw_toggle != NULL)
		result = wimax_dev->op_rfkill_sw_toggle(wimax_dev, state);
	else if (state == WIMAX_RF_OFF)	/* No op? can't turn off */
		result = -ENXIO;
	else				/* No op? can turn on */
		result = 0;		/* should never happen tho */
	if (result >= 0) {
		result = 0;
		wimax_dev->rf_sw = state;
		wimax_state = state == WIMAX_RF_ON ?
			WIMAX_ST_READY : WIMAX_ST_RADIO_OFF;
		__wimax_state_change(wimax_dev, wimax_state);
	}
out_no_change:
	d_fnend(3, dev, "(wimax_dev %p state %u) = %d\n",
		wimax_dev, state, result);
	return result;
}


static
int wimax_rfkill_toggle_radio(void *data, enum rfkill_state state)
{
	int result;
	struct wimax_dev *wimax_dev = data;
	struct device *dev = wimax_dev_to_dev(wimax_dev);
	enum wimax_rf_state rf_state;

	d_fnstart(3, dev, "(wimax_dev %p state %u)\n", wimax_dev, state);
	switch (state) {
	case RFKILL_STATE_ON:
		rf_state = WIMAX_RF_OFF;
		break;
	case RFKILL_STATE_OFF:
		rf_state = WIMAX_RF_ON;
		break;
	default:
		BUG();
	}
	mutex_lock(&wimax_dev->mutex);
	if (wimax_dev->state <= __WIMAX_ST_QUIESCING)
		result = 0;	/* just pretend it didn't happen */
	else
		result = __wimax_rf_toggle_radio(wimax_dev, rf_state);
	mutex_unlock(&wimax_dev->mutex);
	d_fnend(3, dev, "(wimax_dev %p state %u) = %d\n",
		wimax_dev, state, result);
	return result;
}


int wimax_rfkill(struct wimax_dev *wimax_dev, enum wimax_rf_state state)
{
	int result;
	struct device *dev = wimax_dev_to_dev(wimax_dev);

	d_fnstart(3, dev, "(wimax_dev %p state %u)\n", wimax_dev, state);
	mutex_lock(&wimax_dev->mutex);
	result = wimax_dev_is_ready(wimax_dev);
	if (result < 0)
		goto error_not_ready;
	switch (state) {
	case WIMAX_RF_ON:
	case WIMAX_RF_OFF:
		result = __wimax_rf_toggle_radio(wimax_dev, state);
		if (result < 0)
			goto error;
		break;
	case WIMAX_RF_QUERY:
		break;
	default:
		result = -EINVAL;
		goto error;
	}
	result = wimax_dev->rf_sw << 1 | wimax_dev->rf_hw;
error:
error_not_ready:
	mutex_unlock(&wimax_dev->mutex);
	d_fnend(3, dev, "(wimax_dev %p state %u) = %d\n",
		wimax_dev, state, result);
	return result;
}
EXPORT_SYMBOL(wimax_rfkill);


int wimax_rfkill_add(struct wimax_dev *wimax_dev)
{
	int result;
	struct rfkill *rfkill;
	struct input_dev *input_dev;
	struct device *dev = wimax_dev_to_dev(wimax_dev);

	d_fnstart(3, dev, "(wimax_dev %p)\n", wimax_dev);
	/* Initialize RF Kill */
	result = -ENOMEM;
	rfkill = rfkill_allocate(dev, RFKILL_TYPE_WIMAX);
	if (rfkill == NULL)
		goto error_rfkill_allocate;
	wimax_dev->rfkill = rfkill;

	rfkill->name = wimax_dev->name;
	rfkill->state = RFKILL_STATE_OFF;
	rfkill->data = wimax_dev;
	rfkill->toggle_radio = wimax_rfkill_toggle_radio;
	rfkill->user_claim_unsupported = 1;

	/* Initialize the input device for the hw key */
	input_dev = input_allocate_device();
	if (input_dev == NULL)
		goto error_input_allocate;
	wimax_dev->rfkill_input = input_dev;
	d_printf(1, dev, "rfkill %p input %p\n", rfkill, input_dev);

	input_dev->name = wimax_dev->name;
	/* FIXME: get a real device bus ID and stuff? do we care? */
	input_dev->id.bustype = BUS_HOST;
	input_dev->id.vendor = 0xffff;
	input_dev->evbit[0] = BIT(EV_KEY);
	set_bit(KEY_WIMAX, input_dev->keybit);

	/* Register both */
	result = input_register_device(wimax_dev->rfkill_input);
	if (result < 0)
		goto error_input_register;
	result = rfkill_register(wimax_dev->rfkill);
	if (result < 0)
		goto error_rfkill_register;

	/* If there is no SW toggle op, SW RFKill is always on */
	if (wimax_dev->op_rfkill_sw_toggle == NULL)
		wimax_dev->rf_sw = WIMAX_RF_ON;

	d_fnend(3, dev, "(wimax_dev %p) = 0\n", wimax_dev);
	return 0;

	/* if rfkill_register() suceeds, can't use rfkill_free() any
	 * more, only rfkill_unregister() [it owns the refcount]; with
	 * the input device we have the same issue--hence the if. */
error_rfkill_register:
	input_unregister_device(wimax_dev->rfkill_input);
	wimax_dev->rfkill_input = NULL;
error_input_register:
	if (wimax_dev->rfkill_input)
		input_free_device(wimax_dev->rfkill_input);
error_input_allocate:
	rfkill_free(wimax_dev->rfkill);
error_rfkill_allocate:
	d_fnend(3, dev, "(wimax_dev %p) = %d\n", wimax_dev, result);
	return result;
}


void wimax_rfkill_rm(struct wimax_dev *wimax_dev)
{
	struct device *dev = wimax_dev_to_dev(wimax_dev);
	d_fnstart(3, dev, "(wimax_dev %p)\n", wimax_dev);
	rfkill_unregister(wimax_dev->rfkill);	/* frees */
	input_unregister_device(wimax_dev->rfkill_input);
	d_fnend(3, dev, "(wimax_dev %p)\n", wimax_dev);
}


#else /* #ifdef CONFIG_RFKILL */

void wimax_report_rfkill_hw(struct wimax_dev *wimax_dev,
			    enum wimax_rf_state state)
{
}
EXPORT_SYMBOL_GPL(wimax_report_rfkill_hw);

void wimax_report_rfkill_sw(struct wimax_dev *wimax_dev,
			    enum wimax_rf_state state)
{
}
EXPORT_SYMBOL_GPL(wimax_report_rfkill_sw);

int wimax_rfkill(struct wimax_dev *wimax_dev,
		 enum wimax_rf_state state)
{
	return WIMAX_RF_ON << 1 | WIMAX_RF_ON;
}
EXPORT_SYMBOL_GPL(wimax_rfkill);

int wimax_rfkill_add(struct wimax_dev *wimax_dev)
{
	return 0;
}

void wimax_rfkill_rm(struct wimax_dev *wimax_dev)
{
}

#endif /* #ifdef CONFIG_RFKILL */



static const
struct nla_policy wimax_gnl_rfkill_policy[WIMAX_GNL_ATTR_MAX + 1] = {
	[WIMAX_GNL_RFKILL_IFIDX] = {
		.type = NLA_U32,
	},
	[WIMAX_GNL_RFKILL_STATE] = {
		.type = NLA_U32		/* enum wimax_rf_state */
	},
};


static
int wimax_gnl_doit_rfkill(struct sk_buff *skb, struct genl_info *info)
{
	int result, ifindex;
	struct wimax_dev *wimax_dev;
	struct device *dev;
	enum wimax_rf_state new_state;

	d_fnstart(3, NULL, "(skb %p info %p)\n", skb, info);
	result = -ENODEV;
	if (info->attrs[WIMAX_GNL_RFKILL_IFIDX] == NULL) {
		printk(KERN_ERR "WIMAX_GNL_OP_RFKILL: can't find IFIDX "
			"attribute\n");
		goto error_no_wimax_dev;
	}
	ifindex = nla_get_u32(info->attrs[WIMAX_GNL_RFKILL_IFIDX]);
	wimax_dev = wimax_dev_get_by_genl_info(info, ifindex);
	if (wimax_dev == NULL)
		goto error_no_wimax_dev;
	dev = wimax_dev_to_dev(wimax_dev);
	result = -EINVAL;
	if (info->attrs[WIMAX_GNL_RFKILL_STATE] == NULL) {
		dev_err(dev, "WIMAX_GNL_RFKILL: can't find RFKILL_STATE "
			"attribute\n");
		goto error_no_pid;
	}
	new_state = nla_get_u32(info->attrs[WIMAX_GNL_RFKILL_STATE]);

	/* Execute the operation and send the result back to user space */
	result = wimax_rfkill(wimax_dev, new_state);
error_no_pid:
	dev_put(wimax_dev->net_dev);
error_no_wimax_dev:
	d_fnend(3, NULL, "(skb %p info %p) = %d\n", skb, info, result);
	return result;
}


struct genl_ops wimax_gnl_rfkill = {
	.cmd = WIMAX_GNL_OP_RFKILL,
	.flags = GENL_ADMIN_PERM,
	.policy = wimax_gnl_rfkill_policy,
	.doit = wimax_gnl_doit_rfkill,
	.dumpit = NULL,
};

