
#ifndef __RFKILL_H
#define __RFKILL_H


#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/device.h>
#include <linux/leds.h>

enum rfkill_type {
	RFKILL_TYPE_WLAN ,
	RFKILL_TYPE_BLUETOOTH,
	RFKILL_TYPE_UWB,
	RFKILL_TYPE_WIMAX,
	RFKILL_TYPE_WWAN,
	RFKILL_TYPE_MAX,
};

enum rfkill_state {
	RFKILL_STATE_SOFT_BLOCKED = 0,	/* Radio output blocked */
	RFKILL_STATE_UNBLOCKED    = 1,	/* Radio output allowed */
	RFKILL_STATE_HARD_BLOCKED = 2,	/* Output blocked, non-overrideable */
	RFKILL_STATE_MAX,		/* marker for last valid state */
};

#define RFKILL_STATE_OFF RFKILL_STATE_SOFT_BLOCKED
#define RFKILL_STATE_ON  RFKILL_STATE_UNBLOCKED

struct rfkill {
	const char *name;
	enum rfkill_type type;

	bool user_claim_unsupported;
	bool user_claim;

	/* the mutex serializes callbacks and also protects
	 * the state */
	struct mutex mutex;
	enum rfkill_state state;
	void *data;
	int (*toggle_radio)(void *data, enum rfkill_state state);
	int (*get_state)(void *data, enum rfkill_state *state);

#ifdef CONFIG_RFKILL_LEDS
	struct led_trigger led_trigger;
#endif

	struct device dev;
	struct list_head node;
	enum rfkill_state state_for_resume;
};
#define to_rfkill(d)	container_of(d, struct rfkill, dev)

struct rfkill * __must_check rfkill_allocate(struct device *parent,
					     enum rfkill_type type);
void rfkill_free(struct rfkill *rfkill);
int __must_check rfkill_register(struct rfkill *rfkill);
void rfkill_unregister(struct rfkill *rfkill);

int rfkill_force_state(struct rfkill *rfkill, enum rfkill_state state);
int rfkill_set_default(enum rfkill_type type, enum rfkill_state state);

static inline enum rfkill_state rfkill_state_complement(enum rfkill_state state)
{
	return (state == RFKILL_STATE_UNBLOCKED) ?
		RFKILL_STATE_SOFT_BLOCKED : RFKILL_STATE_UNBLOCKED;
}

static inline char *rfkill_get_led_name(struct rfkill *rfkill)
{
#ifdef CONFIG_RFKILL_LEDS
	return (char *)(rfkill->led_trigger.name);
#else
	return NULL;
#endif
}

#endif /* RFKILL_H */
