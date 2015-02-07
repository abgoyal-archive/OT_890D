

#ifndef __iwl_3945_rs_h__
#define __iwl_3945_rs_h__

struct iwl3945_rate_info {
	u8 plcp;		/* uCode API:  IWL_RATE_6M_PLCP, etc. */
	u8 ieee;		/* MAC header:  IWL_RATE_6M_IEEE, etc. */
	u8 prev_ieee;		/* previous rate in IEEE speeds */
	u8 next_ieee;		/* next rate in IEEE speeds */
	u8 prev_rs;		/* previous rate used in rs algo */
	u8 next_rs;		/* next rate used in rs algo */
	u8 prev_rs_tgg;		/* previous rate used in TGG rs algo */
	u8 next_rs_tgg;		/* next rate used in TGG rs algo */
	u8 table_rs_index;	/* index in rate scale table cmd */
	u8 prev_table_rs;	/* prev in rate table cmd */
};

enum {
	IWL_RATE_1M_INDEX = 0,
	IWL_RATE_2M_INDEX,
	IWL_RATE_5M_INDEX,
	IWL_RATE_11M_INDEX,
	IWL_RATE_6M_INDEX,
	IWL_RATE_9M_INDEX,
	IWL_RATE_12M_INDEX,
	IWL_RATE_18M_INDEX,
	IWL_RATE_24M_INDEX,
	IWL_RATE_36M_INDEX,
	IWL_RATE_48M_INDEX,
	IWL_RATE_54M_INDEX,
	IWL_RATE_COUNT,
	IWL_RATE_INVM_INDEX,
	IWL_RATE_INVALID = IWL_RATE_INVM_INDEX
};

enum {
	IWL_RATE_6M_INDEX_TABLE = 0,
	IWL_RATE_9M_INDEX_TABLE,
	IWL_RATE_12M_INDEX_TABLE,
	IWL_RATE_18M_INDEX_TABLE,
	IWL_RATE_24M_INDEX_TABLE,
	IWL_RATE_36M_INDEX_TABLE,
	IWL_RATE_48M_INDEX_TABLE,
	IWL_RATE_54M_INDEX_TABLE,
	IWL_RATE_1M_INDEX_TABLE,
	IWL_RATE_2M_INDEX_TABLE,
	IWL_RATE_5M_INDEX_TABLE,
	IWL_RATE_11M_INDEX_TABLE,
	IWL_RATE_INVM_INDEX_TABLE = IWL_RATE_INVM_INDEX,
};

enum {
	IWL_FIRST_OFDM_RATE = IWL_RATE_6M_INDEX,
	IWL_LAST_OFDM_RATE = IWL_RATE_54M_INDEX,
	IWL_FIRST_CCK_RATE = IWL_RATE_1M_INDEX,
	IWL_LAST_CCK_RATE = IWL_RATE_11M_INDEX,
};

/* #define vs. enum to keep from defaulting to 'large integer' */
#define	IWL_RATE_6M_MASK   (1 << IWL_RATE_6M_INDEX)
#define	IWL_RATE_9M_MASK   (1 << IWL_RATE_9M_INDEX)
#define	IWL_RATE_12M_MASK  (1 << IWL_RATE_12M_INDEX)
#define	IWL_RATE_18M_MASK  (1 << IWL_RATE_18M_INDEX)
#define	IWL_RATE_24M_MASK  (1 << IWL_RATE_24M_INDEX)
#define	IWL_RATE_36M_MASK  (1 << IWL_RATE_36M_INDEX)
#define	IWL_RATE_48M_MASK  (1 << IWL_RATE_48M_INDEX)
#define	IWL_RATE_54M_MASK  (1 << IWL_RATE_54M_INDEX)
#define	IWL_RATE_1M_MASK   (1 << IWL_RATE_1M_INDEX)
#define	IWL_RATE_2M_MASK   (1 << IWL_RATE_2M_INDEX)
#define	IWL_RATE_5M_MASK   (1 << IWL_RATE_5M_INDEX)
#define	IWL_RATE_11M_MASK  (1 << IWL_RATE_11M_INDEX)

/* 3945 uCode API values for (legacy) bit rates, both OFDM and CCK */
enum {
	IWL_RATE_6M_PLCP = 13,
	IWL_RATE_9M_PLCP = 15,
	IWL_RATE_12M_PLCP = 5,
	IWL_RATE_18M_PLCP = 7,
	IWL_RATE_24M_PLCP = 9,
	IWL_RATE_36M_PLCP = 11,
	IWL_RATE_48M_PLCP = 1,
	IWL_RATE_54M_PLCP = 3,
	IWL_RATE_1M_PLCP = 10,
	IWL_RATE_2M_PLCP = 20,
	IWL_RATE_5M_PLCP = 55,
	IWL_RATE_11M_PLCP = 110,
};

/* MAC header values for bit rates */
enum {
	IWL_RATE_6M_IEEE = 12,
	IWL_RATE_9M_IEEE = 18,
	IWL_RATE_12M_IEEE = 24,
	IWL_RATE_18M_IEEE = 36,
	IWL_RATE_24M_IEEE = 48,
	IWL_RATE_36M_IEEE = 72,
	IWL_RATE_48M_IEEE = 96,
	IWL_RATE_54M_IEEE = 108,
	IWL_RATE_1M_IEEE = 2,
	IWL_RATE_2M_IEEE = 4,
	IWL_RATE_5M_IEEE = 11,
	IWL_RATE_11M_IEEE = 22,
};

#define IWL_CCK_BASIC_RATES_MASK    \
       (IWL_RATE_1M_MASK          | \
	IWL_RATE_2M_MASK)

#define IWL_CCK_RATES_MASK          \
       (IWL_BASIC_RATES_MASK      | \
	IWL_RATE_5M_MASK          | \
	IWL_RATE_11M_MASK)

#define IWL_OFDM_BASIC_RATES_MASK   \
	(IWL_RATE_6M_MASK         | \
	IWL_RATE_12M_MASK         | \
	IWL_RATE_24M_MASK)

#define IWL_OFDM_RATES_MASK         \
       (IWL_OFDM_BASIC_RATES_MASK | \
	IWL_RATE_9M_MASK          | \
	IWL_RATE_18M_MASK         | \
	IWL_RATE_36M_MASK         | \
	IWL_RATE_48M_MASK         | \
	IWL_RATE_54M_MASK)

#define IWL_BASIC_RATES_MASK         \
	(IWL_OFDM_BASIC_RATES_MASK | \
	 IWL_CCK_BASIC_RATES_MASK)

#define IWL_RATES_MASK ((1 << IWL_RATE_COUNT) - 1)

#define IWL_INV_TPT    -1

#define IWL_MIN_RSSI_VAL                 -100
#define IWL_MAX_RSSI_VAL                    0

extern const struct iwl3945_rate_info iwl3945_rates[IWL_RATE_COUNT];

static inline u8 iwl3945_get_prev_ieee_rate(u8 rate_index)
{
	u8 rate = iwl3945_rates[rate_index].prev_ieee;

	if (rate == IWL_RATE_INVALID)
		rate = rate_index;
	return rate;
}

extern void iwl3945_rate_scale_init(struct ieee80211_hw *hw, s32 sta_id);

extern int iwl3945_rate_control_register(void);

extern void iwl3945_rate_control_unregister(void);

#endif
