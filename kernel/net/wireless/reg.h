
#ifndef __NET_WIRELESS_REG_H
#define __NET_WIRELESS_REG_H

bool is_world_regdom(const char *alpha2);
bool reg_is_valid_request(const char *alpha2);

void reg_device_remove(struct wiphy *wiphy);

int regulatory_init(void);
void regulatory_exit(void);

int set_regdom(const struct ieee80211_regdomain *rd);

enum environment_cap {
	ENVIRON_ANY,
	ENVIRON_INDOOR,
	ENVIRON_OUTDOOR,
};


extern int __regulatory_hint(struct wiphy *wiphy, enum reg_set_by set_by,
			     const char *alpha2, u32 country_ie_checksum,
			     enum environment_cap country_ie_env);

#endif  /* __NET_WIRELESS_REG_H */
