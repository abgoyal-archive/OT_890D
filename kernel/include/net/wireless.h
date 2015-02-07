
#ifndef __NET_WIRELESS_H
#define __NET_WIRELESS_H


#include <linux/netdevice.h>
#include <linux/debugfs.h>
#include <linux/list.h>
#include <linux/ieee80211.h>
#include <net/cfg80211.h>

enum ieee80211_band {
	IEEE80211_BAND_2GHZ,
	IEEE80211_BAND_5GHZ,

	/* keep last */
	IEEE80211_NUM_BANDS
};

enum ieee80211_channel_flags {
	IEEE80211_CHAN_DISABLED		= 1<<0,
	IEEE80211_CHAN_PASSIVE_SCAN	= 1<<1,
	IEEE80211_CHAN_NO_IBSS		= 1<<2,
	IEEE80211_CHAN_RADAR		= 1<<3,
	IEEE80211_CHAN_NO_FAT_ABOVE	= 1<<4,
	IEEE80211_CHAN_NO_FAT_BELOW	= 1<<5,
};

struct ieee80211_channel {
	enum ieee80211_band band;
	u16 center_freq;
	u8 max_bandwidth;
	u16 hw_value;
	u32 flags;
	int max_antenna_gain;
	int max_power;
	u32 orig_flags;
	int orig_mag, orig_mpwr;
};

enum ieee80211_rate_flags {
	IEEE80211_RATE_SHORT_PREAMBLE	= 1<<0,
	IEEE80211_RATE_MANDATORY_A	= 1<<1,
	IEEE80211_RATE_MANDATORY_B	= 1<<2,
	IEEE80211_RATE_MANDATORY_G	= 1<<3,
	IEEE80211_RATE_ERP_G		= 1<<4,
};

struct ieee80211_rate {
	u32 flags;
	u16 bitrate;
	u16 hw_value, hw_value_short;
};

struct ieee80211_sta_ht_cap {
	u16 cap; /* use IEEE80211_HT_CAP_ */
	bool ht_supported;
	u8 ampdu_factor;
	u8 ampdu_density;
	struct ieee80211_mcs_info mcs;
};

struct ieee80211_supported_band {
	struct ieee80211_channel *channels;
	struct ieee80211_rate *bitrates;
	enum ieee80211_band band;
	int n_channels;
	int n_bitrates;
	struct ieee80211_sta_ht_cap ht_cap;
};

struct wiphy {
	/* assign these fields before you register the wiphy */

	/* permanent MAC address */
	u8 perm_addr[ETH_ALEN];

	/* Supported interface modes, OR together BIT(NL80211_IFTYPE_...) */
	u16 interface_modes;

	bool fw_handles_regulatory;

	/* If multiple wiphys are registered and you're handed e.g.
	 * a regular netdev with assigned ieee80211_ptr, you won't
	 * know whether it points to a wiphy your driver has registered
	 * or not. Assign this to something global to your driver to
	 * help determine whether you own this wiphy or not. */
	void *privid;

	struct ieee80211_supported_band *bands[IEEE80211_NUM_BANDS];

	/* Lets us get back the wiphy on the callback */
	int (*reg_notifier)(struct wiphy *wiphy, enum reg_set_by setby);

	/* fields below are read-only, assigned by cfg80211 */

	/* the item in /sys/class/ieee80211/ points to this,
	 * you need use set_wiphy_dev() (see below) */
	struct device dev;

	/* dir in debugfs: ieee80211/<wiphyname> */
	struct dentry *debugfsdir;

	char priv[0] __attribute__((__aligned__(NETDEV_ALIGN)));
};

struct wireless_dev {
	struct wiphy *wiphy;
	enum nl80211_iftype iftype;

	/* private to the generic wireless code */
	struct list_head list;
	struct net_device *netdev;
};

static inline void *wiphy_priv(struct wiphy *wiphy)
{
	BUG_ON(!wiphy);
	return &wiphy->priv;
}

static inline void set_wiphy_dev(struct wiphy *wiphy, struct device *dev)
{
	wiphy->dev.parent = dev;
}

static inline struct device *wiphy_dev(struct wiphy *wiphy)
{
	return wiphy->dev.parent;
}

static inline const char *wiphy_name(struct wiphy *wiphy)
{
	return dev_name(&wiphy->dev);
}

static inline void *wdev_priv(struct wireless_dev *wdev)
{
	BUG_ON(!wdev);
	return wiphy_priv(wdev->wiphy);
}

struct wiphy *wiphy_new(struct cfg80211_ops *ops, int sizeof_priv);

extern int wiphy_register(struct wiphy *wiphy);

extern void wiphy_unregister(struct wiphy *wiphy);

extern void wiphy_free(struct wiphy *wiphy);

extern int ieee80211_channel_to_frequency(int chan);

extern int ieee80211_frequency_to_channel(int freq);

extern struct ieee80211_channel *__ieee80211_get_channel(struct wiphy *wiphy,
							 int freq);
static inline struct ieee80211_channel *
ieee80211_get_channel(struct wiphy *wiphy, int freq)
{
	return __ieee80211_get_channel(wiphy, freq);
}

struct ieee80211_rate *
ieee80211_get_response_rate(struct ieee80211_supported_band *sband,
			    u64 basic_rates, int bitrate);

extern void regulatory_hint(struct wiphy *wiphy, const char *alpha2);

extern void regulatory_hint_11d(struct wiphy *wiphy,
				u8 *country_ie,
				u8 country_ie_len);
#endif /* __NET_WIRELESS_H */
