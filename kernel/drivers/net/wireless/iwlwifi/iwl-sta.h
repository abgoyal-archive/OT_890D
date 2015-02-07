
#ifndef __iwl_sta_h__
#define __iwl_sta_h__

#define HW_KEY_DYNAMIC 0
#define HW_KEY_DEFAULT 1

u8 iwl_find_station(struct iwl_priv *priv, const u8 *bssid);

int iwl_send_static_wepkey_cmd(struct iwl_priv *priv, u8 send_if_empty);
int iwl_remove_default_wep_key(struct iwl_priv *priv,
			       struct ieee80211_key_conf *key);
int iwl_set_default_wep_key(struct iwl_priv *priv,
			    struct ieee80211_key_conf *key);
int iwl_set_dynamic_key(struct iwl_priv *priv,
			struct ieee80211_key_conf *key, u8 sta_id);
int iwl_remove_dynamic_key(struct iwl_priv *priv,
			   struct ieee80211_key_conf *key, u8 sta_id);
void iwl_update_tkip_key(struct iwl_priv *priv,
			struct ieee80211_key_conf *keyconf,
			const u8 *addr, u32 iv32, u16 *phase1key);

int iwl_rxon_add_station(struct iwl_priv *priv, const u8 *addr, int is_ap);
int iwl_remove_station(struct iwl_priv *priv, const u8 *addr, int is_ap);
void iwl_clear_stations_table(struct iwl_priv *priv);
int iwl_get_sta_id(struct iwl_priv *priv, struct ieee80211_hdr *hdr);
int iwl_get_ra_sta_id(struct iwl_priv *priv, struct ieee80211_hdr *hdr);
u8 iwl_add_station_flags(struct iwl_priv *priv, const u8 *addr,
			int is_ap, u8 flags,
			struct ieee80211_sta_ht_cap *ht_info);
void iwl_sta_tx_modify_enable_tid(struct iwl_priv *priv, int sta_id, int tid);
int iwl_sta_rx_agg_start(struct iwl_priv *priv,
			 const u8 *addr, int tid, u16 ssn);
int iwl_sta_rx_agg_stop(struct iwl_priv *priv, const u8 *addr, int tid);
void iwl_update_ps_mode(struct iwl_priv *priv, u16 ps_bit, u8 *addr);
#endif /* __iwl_sta_h__ */
