

#ifndef IWL3945_LEDS_H
#define IWL3945_LEDS_H

struct iwl3945_priv;

#ifdef CONFIG_IWL3945_LEDS
#define IWL_LED_SOLID 11
#define IWL_LED_NAME_LEN 31
#define IWL_DEF_LED_INTRVL __constant_cpu_to_le32(1000)

#define IWL_LED_ACTIVITY       (0<<1)
#define IWL_LED_LINK           (1<<1)

enum led_type {
	IWL_LED_TRG_TX,
	IWL_LED_TRG_RX,
	IWL_LED_TRG_ASSOC,
	IWL_LED_TRG_RADIO,
	IWL_LED_TRG_MAX,
};

#include <linux/leds.h>

struct iwl3945_led {
	struct iwl3945_priv *priv;
	struct led_classdev led_dev;
	char name[32];

	int (*led_on) (struct iwl3945_priv *priv, int led_id);
	int (*led_off) (struct iwl3945_priv *priv, int led_id);
	int (*led_pattern) (struct iwl3945_priv *priv, int led_id,
			    unsigned int idx);

	enum led_type type;
	unsigned int registered;
};

extern int iwl3945_led_register(struct iwl3945_priv *priv);
extern void iwl3945_led_unregister(struct iwl3945_priv *priv);
extern void iwl3945_led_background(struct iwl3945_priv *priv);

#else
static inline int iwl3945_led_register(struct iwl3945_priv *priv) { return 0; }
static inline void iwl3945_led_unregister(struct iwl3945_priv *priv) {}
static inline void iwl3945_led_background(struct iwl3945_priv *priv) {}
#endif /* CONFIG_IWL3945_LEDS */

#endif /* IWL3945_LEDS_H */
