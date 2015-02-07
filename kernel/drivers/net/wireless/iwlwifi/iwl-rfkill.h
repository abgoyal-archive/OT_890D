
#ifndef __iwl_rf_kill_h__
#define __iwl_rf_kill_h__

struct iwl_priv;

#include <linux/rfkill.h>

#ifdef CONFIG_IWLWIFI_RFKILL

void iwl_rfkill_set_hw_state(struct iwl_priv *priv);
void iwl_rfkill_unregister(struct iwl_priv *priv);
int iwl_rfkill_init(struct iwl_priv *priv);
#else
static inline void iwl_rfkill_set_hw_state(struct iwl_priv *priv) {}
static inline void iwl_rfkill_unregister(struct iwl_priv *priv) {}
static inline int iwl_rfkill_init(struct iwl_priv *priv) { return 0; }
#endif



#endif  /* __iwl_rf_kill_h__ */
