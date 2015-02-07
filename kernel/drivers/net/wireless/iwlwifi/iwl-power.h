
#ifndef __iwl_power_setting_h__
#define __iwl_power_setting_h__

#include <net/mac80211.h>
#include "iwl-commands.h"

struct iwl_priv;

enum {
	IWL_POWER_MODE_CAM, /* Continuously Aware Mode, always on */
	IWL_POWER_INDEX_1,
	IWL_POWER_INDEX_2,
	IWL_POWER_INDEX_3,
	IWL_POWER_INDEX_4,
	IWL_POWER_INDEX_5,
	IWL_POWER_AUTO,
	IWL_POWER_MAX = IWL_POWER_AUTO,
	IWL_POWER_AC,
	IWL_POWER_BATTERY,
};

enum {
	IWL_POWER_SYS_AUTO,
	IWL_POWER_SYS_AC,
	IWL_POWER_SYS_BATTERY,
};

#define IWL_POWER_LIMIT		0x08
#define IWL_POWER_MASK		0x0F
#define IWL_POWER_ENABLED	0x10

/* Power management (not Tx power) structures */

struct iwl_power_vec_entry {
	struct iwl_powertable_cmd cmd;
	u8 no_dtim;
};

struct iwl_power_mgr {
	spinlock_t lock;
	struct iwl_power_vec_entry pwr_range_0[IWL_POWER_MAX];
	struct iwl_power_vec_entry pwr_range_1[IWL_POWER_MAX];
	struct iwl_power_vec_entry pwr_range_2[IWL_POWER_MAX];
	u32 dtim_period;
	/* final power level that used to calculate final power command */
	u8 power_mode;
	u8 user_power_setting; /* set by user through mac80211 or sysfs */
	u8 system_power_setting; /* set by kernel system tools */
	u8 critical_power_setting; /* set if driver over heated */
	u8 is_battery_active; /* DC/AC power */
	u8 power_disabled; /* flag to disable using power saving level */
};

void iwl_setup_power_deferred_work(struct iwl_priv *priv);
void iwl_power_cancel_timeout(struct iwl_priv *priv);
int iwl_power_update_mode(struct iwl_priv *priv, bool force);
int iwl_power_disable_management(struct iwl_priv *priv, u32 ms);
int iwl_power_enable_management(struct iwl_priv *priv);
int iwl_power_set_user_mode(struct iwl_priv *priv, u16 mode);
int iwl_power_set_system_mode(struct iwl_priv *priv, u16 mode);
void iwl_power_initialize(struct iwl_priv *priv);
int iwl_power_temperature_change(struct iwl_priv *priv);

#endif  /* __iwl_power_setting_h__ */
