


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

#include <net/mac80211.h>

#include "iwl-commands.h"
#include "iwl-dev.h"
#include "iwl-core.h"
#include "iwl-debug.h"
#include "iwl-eeprom.h"
#include "iwl-io.h"


/* 2.4 GHz */
const u8 iwl_eeprom_band_1[14] = {
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14
};

/* 5.2 GHz bands */
static const u8 iwl_eeprom_band_2[] = {	/* 4915-5080MHz */
	183, 184, 185, 187, 188, 189, 192, 196, 7, 8, 11, 12, 16
};

static const u8 iwl_eeprom_band_3[] = {	/* 5170-5320MHz */
	34, 36, 38, 40, 42, 44, 46, 48, 52, 56, 60, 64
};

static const u8 iwl_eeprom_band_4[] = {	/* 5500-5700MHz */
	100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140
};

static const u8 iwl_eeprom_band_5[] = {	/* 5725-5825MHz */
	145, 149, 153, 157, 161, 165
};

static const u8 iwl_eeprom_band_6[] = {       /* 2.4 FAT channel */
	1, 2, 3, 4, 5, 6, 7
};

static const u8 iwl_eeprom_band_7[] = {       /* 5.2 FAT channel */
	36, 44, 52, 60, 100, 108, 116, 124, 132, 149, 157
};


int iwlcore_eeprom_verify_signature(struct iwl_priv *priv)
{
	u32 gp = iwl_read32(priv, CSR_EEPROM_GP);
	if ((gp & CSR_EEPROM_GP_VALID_MSK) == CSR_EEPROM_GP_BAD_SIGNATURE) {
		IWL_ERROR("EEPROM not found, EEPROM_GP=0x%08x\n", gp);
		return -ENOENT;
	}
	return 0;
}
EXPORT_SYMBOL(iwlcore_eeprom_verify_signature);

int iwlcore_eeprom_acquire_semaphore(struct iwl_priv *priv)
{
	u16 count;
	int ret;

	for (count = 0; count < EEPROM_SEM_RETRY_LIMIT; count++) {
		/* Request semaphore */
		iwl_set_bit(priv, CSR_HW_IF_CONFIG_REG,
			    CSR_HW_IF_CONFIG_REG_BIT_EEPROM_OWN_SEM);

		/* See if we got it */
		ret = iwl_poll_direct_bit(priv, CSR_HW_IF_CONFIG_REG,
				CSR_HW_IF_CONFIG_REG_BIT_EEPROM_OWN_SEM,
				EEPROM_SEM_TIMEOUT);
		if (ret >= 0) {
			IWL_DEBUG_IO("Acquired semaphore after %d tries.\n",
				count+1);
			return ret;
		}
	}

	return ret;
}
EXPORT_SYMBOL(iwlcore_eeprom_acquire_semaphore);

void iwlcore_eeprom_release_semaphore(struct iwl_priv *priv)
{
	iwl_clear_bit(priv, CSR_HW_IF_CONFIG_REG,
		CSR_HW_IF_CONFIG_REG_BIT_EEPROM_OWN_SEM);

}
EXPORT_SYMBOL(iwlcore_eeprom_release_semaphore);

const u8 *iwlcore_eeprom_query_addr(const struct iwl_priv *priv, size_t offset)
{
	BUG_ON(offset >= priv->cfg->eeprom_size);
	return &priv->eeprom[offset];
}
EXPORT_SYMBOL(iwlcore_eeprom_query_addr);

int iwl_eeprom_init(struct iwl_priv *priv)
{
	u16 *e;
	u32 gp = iwl_read32(priv, CSR_EEPROM_GP);
	int sz = priv->cfg->eeprom_size;
	int ret;
	u16 addr;

	/* allocate eeprom */
	priv->eeprom = kzalloc(sz, GFP_KERNEL);
	if (!priv->eeprom) {
		ret = -ENOMEM;
		goto alloc_err;
	}
	e = (u16 *)priv->eeprom;

	ret = priv->cfg->ops->lib->eeprom_ops.verify_signature(priv);
	if (ret < 0) {
		IWL_ERROR("EEPROM not found, EEPROM_GP=0x%08x\n", gp);
		ret = -ENOENT;
		goto err;
	}

	/* Make sure driver (instead of uCode) is allowed to read EEPROM */
	ret = priv->cfg->ops->lib->eeprom_ops.acquire_semaphore(priv);
	if (ret < 0) {
		IWL_ERROR("Failed to acquire EEPROM semaphore.\n");
		ret = -ENOENT;
		goto err;
	}

	/* eeprom is an array of 16bit values */
	for (addr = 0; addr < sz; addr += sizeof(u16)) {
		u32 r;

		_iwl_write32(priv, CSR_EEPROM_REG,
			     CSR_EEPROM_REG_MSK_ADDR & (addr << 1));

		ret = iwl_poll_direct_bit(priv, CSR_EEPROM_REG,
					  CSR_EEPROM_REG_READ_VALID_MSK,
					  IWL_EEPROM_ACCESS_TIMEOUT);
		if (ret < 0) {
			IWL_ERROR("Time out reading EEPROM[%d]\n", addr);
			goto done;
		}
		r = _iwl_read_direct32(priv, CSR_EEPROM_REG);
		e[addr / 2] = le16_to_cpu((__force __le16)(r >> 16));
	}
	ret = 0;
done:
	priv->cfg->ops->lib->eeprom_ops.release_semaphore(priv);
err:
	if (ret)
		kfree(priv->eeprom);
alloc_err:
	return ret;
}
EXPORT_SYMBOL(iwl_eeprom_init);

void iwl_eeprom_free(struct iwl_priv *priv)
{
	kfree(priv->eeprom);
	priv->eeprom = NULL;
}
EXPORT_SYMBOL(iwl_eeprom_free);

int iwl_eeprom_check_version(struct iwl_priv *priv)
{
	u16 eeprom_ver;
	u16 calib_ver;

	eeprom_ver = iwl_eeprom_query16(priv, EEPROM_VERSION);
	calib_ver = priv->cfg->ops->lib->eeprom_ops.calib_version(priv);

	if (eeprom_ver < priv->cfg->eeprom_ver ||
	    calib_ver < priv->cfg->eeprom_calib_ver)
		goto err;

	return 0;
err:
	IWL_ERROR("Unsupported EEPROM VER=0x%x < 0x%x CALIB=0x%x < 0x%x\n",
		  eeprom_ver, priv->cfg->eeprom_ver,
		  calib_ver,  priv->cfg->eeprom_calib_ver);
	return -EINVAL;

}
EXPORT_SYMBOL(iwl_eeprom_check_version);

const u8 *iwl_eeprom_query_addr(const struct iwl_priv *priv, size_t offset)
{
	return priv->cfg->ops->lib->eeprom_ops.query_addr(priv, offset);
}
EXPORT_SYMBOL(iwl_eeprom_query_addr);

u16 iwl_eeprom_query16(const struct iwl_priv *priv, size_t offset)
{
	return (u16)priv->eeprom[offset] | ((u16)priv->eeprom[offset + 1] << 8);
}
EXPORT_SYMBOL(iwl_eeprom_query16);

void iwl_eeprom_get_mac(const struct iwl_priv *priv, u8 *mac)
{
	const u8 *addr = priv->cfg->ops->lib->eeprom_ops.query_addr(priv,
					EEPROM_MAC_ADDRESS);
	memcpy(mac, addr, ETH_ALEN);
}
EXPORT_SYMBOL(iwl_eeprom_get_mac);

static void iwl_init_band_reference(const struct iwl_priv *priv,
			int eep_band, int *eeprom_ch_count,
			const struct iwl_eeprom_channel **eeprom_ch_info,
			const u8 **eeprom_ch_index)
{
	u32 offset = priv->cfg->ops->lib->
			eeprom_ops.regulatory_bands[eep_band - 1];
	switch (eep_band) {
	case 1:		/* 2.4GHz band */
		*eeprom_ch_count = ARRAY_SIZE(iwl_eeprom_band_1);
		*eeprom_ch_info = (struct iwl_eeprom_channel *)
				iwl_eeprom_query_addr(priv, offset);
		*eeprom_ch_index = iwl_eeprom_band_1;
		break;
	case 2:		/* 4.9GHz band */
		*eeprom_ch_count = ARRAY_SIZE(iwl_eeprom_band_2);
		*eeprom_ch_info = (struct iwl_eeprom_channel *)
				iwl_eeprom_query_addr(priv, offset);
		*eeprom_ch_index = iwl_eeprom_band_2;
		break;
	case 3:		/* 5.2GHz band */
		*eeprom_ch_count = ARRAY_SIZE(iwl_eeprom_band_3);
		*eeprom_ch_info = (struct iwl_eeprom_channel *)
				iwl_eeprom_query_addr(priv, offset);
		*eeprom_ch_index = iwl_eeprom_band_3;
		break;
	case 4:		/* 5.5GHz band */
		*eeprom_ch_count = ARRAY_SIZE(iwl_eeprom_band_4);
		*eeprom_ch_info = (struct iwl_eeprom_channel *)
				iwl_eeprom_query_addr(priv, offset);
		*eeprom_ch_index = iwl_eeprom_band_4;
		break;
	case 5:		/* 5.7GHz band */
		*eeprom_ch_count = ARRAY_SIZE(iwl_eeprom_band_5);
		*eeprom_ch_info = (struct iwl_eeprom_channel *)
				iwl_eeprom_query_addr(priv, offset);
		*eeprom_ch_index = iwl_eeprom_band_5;
		break;
	case 6:		/* 2.4GHz FAT channels */
		*eeprom_ch_count = ARRAY_SIZE(iwl_eeprom_band_6);
		*eeprom_ch_info = (struct iwl_eeprom_channel *)
				iwl_eeprom_query_addr(priv, offset);
		*eeprom_ch_index = iwl_eeprom_band_6;
		break;
	case 7:		/* 5 GHz FAT channels */
		*eeprom_ch_count = ARRAY_SIZE(iwl_eeprom_band_7);
		*eeprom_ch_info = (struct iwl_eeprom_channel *)
				iwl_eeprom_query_addr(priv, offset);
		*eeprom_ch_index = iwl_eeprom_band_7;
		break;
	default:
		BUG();
		return;
	}
}

#define CHECK_AND_PRINT(x) ((eeprom_ch->flags & EEPROM_CHANNEL_##x) \
			    ? # x " " : "")

static int iwl_set_fat_chan_info(struct iwl_priv *priv,
			      enum ieee80211_band band, u16 channel,
			      const struct iwl_eeprom_channel *eeprom_ch,
			      u8 fat_extension_channel)
{
	struct iwl_channel_info *ch_info;

	ch_info = (struct iwl_channel_info *)
			iwl_get_channel_info(priv, band, channel);

	if (!is_channel_valid(ch_info))
		return -1;

	IWL_DEBUG_INFO("FAT Ch. %d [%sGHz] %s%s%s%s%s(0x%02x %ddBm):"
			" Ad-Hoc %ssupported\n",
			ch_info->channel,
			is_channel_a_band(ch_info) ?
			"5.2" : "2.4",
			CHECK_AND_PRINT(IBSS),
			CHECK_AND_PRINT(ACTIVE),
			CHECK_AND_PRINT(RADAR),
			CHECK_AND_PRINT(WIDE),
			CHECK_AND_PRINT(DFS),
			eeprom_ch->flags,
			eeprom_ch->max_power_avg,
			((eeprom_ch->flags & EEPROM_CHANNEL_IBSS)
			 && !(eeprom_ch->flags & EEPROM_CHANNEL_RADAR)) ?
			"" : "not ");

	ch_info->fat_eeprom = *eeprom_ch;
	ch_info->fat_max_power_avg = eeprom_ch->max_power_avg;
	ch_info->fat_curr_txpow = eeprom_ch->max_power_avg;
	ch_info->fat_min_power = 0;
	ch_info->fat_scan_power = eeprom_ch->max_power_avg;
	ch_info->fat_flags = eeprom_ch->flags;
	ch_info->fat_extension_channel = fat_extension_channel;

	return 0;
}

#define CHECK_AND_PRINT_I(x) ((eeprom_ch_info[ch].flags & EEPROM_CHANNEL_##x) \
			    ? # x " " : "")

int iwl_init_channel_map(struct iwl_priv *priv)
{
	int eeprom_ch_count = 0;
	const u8 *eeprom_ch_index = NULL;
	const struct iwl_eeprom_channel *eeprom_ch_info = NULL;
	int band, ch;
	struct iwl_channel_info *ch_info;

	if (priv->channel_count) {
		IWL_DEBUG_INFO("Channel map already initialized.\n");
		return 0;
	}

	IWL_DEBUG_INFO("Initializing regulatory info from EEPROM\n");

	priv->channel_count =
	    ARRAY_SIZE(iwl_eeprom_band_1) +
	    ARRAY_SIZE(iwl_eeprom_band_2) +
	    ARRAY_SIZE(iwl_eeprom_band_3) +
	    ARRAY_SIZE(iwl_eeprom_band_4) +
	    ARRAY_SIZE(iwl_eeprom_band_5);

	IWL_DEBUG_INFO("Parsing data for %d channels.\n", priv->channel_count);

	priv->channel_info = kzalloc(sizeof(struct iwl_channel_info) *
				     priv->channel_count, GFP_KERNEL);
	if (!priv->channel_info) {
		IWL_ERROR("Could not allocate channel_info\n");
		priv->channel_count = 0;
		return -ENOMEM;
	}

	ch_info = priv->channel_info;

	/* Loop through the 5 EEPROM bands adding them in order to the
	 * channel map we maintain (that contains additional information than
	 * what just in the EEPROM) */
	for (band = 1; band <= 5; band++) {

		iwl_init_band_reference(priv, band, &eeprom_ch_count,
					&eeprom_ch_info, &eeprom_ch_index);

		/* Loop through each band adding each of the channels */
		for (ch = 0; ch < eeprom_ch_count; ch++) {
			ch_info->channel = eeprom_ch_index[ch];
			ch_info->band = (band == 1) ? IEEE80211_BAND_2GHZ :
			    IEEE80211_BAND_5GHZ;

			/* permanently store EEPROM's channel regulatory flags
			 *   and max power in channel info database. */
			ch_info->eeprom = eeprom_ch_info[ch];

			/* Copy the run-time flags so they are there even on
			 * invalid channels */
			ch_info->flags = eeprom_ch_info[ch].flags;
			/* First write that fat is not enabled, and then enable
			 * one by one */
			ch_info->fat_extension_channel =
				(IEEE80211_CHAN_NO_FAT_ABOVE |
				 IEEE80211_CHAN_NO_FAT_BELOW);

			if (!(is_channel_valid(ch_info))) {
				IWL_DEBUG_INFO("Ch. %d Flags %x [%sGHz] - "
					       "No traffic\n",
					       ch_info->channel,
					       ch_info->flags,
					       is_channel_a_band(ch_info) ?
					       "5.2" : "2.4");
				ch_info++;
				continue;
			}

			/* Initialize regulatory-based run-time data */
			ch_info->max_power_avg = ch_info->curr_txpow =
			    eeprom_ch_info[ch].max_power_avg;
			ch_info->scan_power = eeprom_ch_info[ch].max_power_avg;
			ch_info->min_power = 0;

			IWL_DEBUG_INFO("Ch. %d [%sGHz] %s%s%s%s%s%s(0x%02x %ddBm):"
				       " Ad-Hoc %ssupported\n",
				       ch_info->channel,
				       is_channel_a_band(ch_info) ?
				       "5.2" : "2.4",
				       CHECK_AND_PRINT_I(VALID),
				       CHECK_AND_PRINT_I(IBSS),
				       CHECK_AND_PRINT_I(ACTIVE),
				       CHECK_AND_PRINT_I(RADAR),
				       CHECK_AND_PRINT_I(WIDE),
				       CHECK_AND_PRINT_I(DFS),
				       eeprom_ch_info[ch].flags,
				       eeprom_ch_info[ch].max_power_avg,
				       ((eeprom_ch_info[ch].
					 flags & EEPROM_CHANNEL_IBSS)
					&& !(eeprom_ch_info[ch].
					     flags & EEPROM_CHANNEL_RADAR))
				       ? "" : "not ");

			/* Set the user_txpower_limit to the highest power
			 * supported by any channel */
			if (eeprom_ch_info[ch].max_power_avg >
						priv->tx_power_user_lmt)
				priv->tx_power_user_lmt =
				    eeprom_ch_info[ch].max_power_avg;

			ch_info++;
		}
	}

	/* Two additional EEPROM bands for 2.4 and 5 GHz FAT channels */
	for (band = 6; band <= 7; band++) {
		enum ieee80211_band ieeeband;
		u8 fat_extension_chan;

		iwl_init_band_reference(priv, band, &eeprom_ch_count,
					&eeprom_ch_info, &eeprom_ch_index);

		/* EEPROM band 6 is 2.4, band 7 is 5 GHz */
		ieeeband =
			(band == 6) ? IEEE80211_BAND_2GHZ : IEEE80211_BAND_5GHZ;

		/* Loop through each band adding each of the channels */
		for (ch = 0; ch < eeprom_ch_count; ch++) {

			if ((band == 6) &&
				((eeprom_ch_index[ch] == 5) ||
				 (eeprom_ch_index[ch] == 6) ||
				 (eeprom_ch_index[ch] == 7)))
				/* both are allowed: above and below */
				fat_extension_chan = 0;
			else
				fat_extension_chan =
					IEEE80211_CHAN_NO_FAT_BELOW;

			/* Set up driver's info for lower half */
			iwl_set_fat_chan_info(priv, ieeeband,
						eeprom_ch_index[ch],
						&(eeprom_ch_info[ch]),
						fat_extension_chan);

			/* Set up driver's info for upper half */
			iwl_set_fat_chan_info(priv, ieeeband,
						(eeprom_ch_index[ch] + 4),
						&(eeprom_ch_info[ch]),
						IEEE80211_CHAN_NO_FAT_ABOVE);
		}
	}

	return 0;
}
EXPORT_SYMBOL(iwl_init_channel_map);

void iwl_free_channel_map(struct iwl_priv *priv)
{
	kfree(priv->channel_info);
	priv->channel_count = 0;
}

const struct iwl_channel_info *iwl_get_channel_info(const struct iwl_priv *priv,
					enum ieee80211_band band, u16 channel)
{
	int i;

	switch (band) {
	case IEEE80211_BAND_5GHZ:
		for (i = 14; i < priv->channel_count; i++) {
			if (priv->channel_info[i].channel == channel)
				return &priv->channel_info[i];
		}
		break;
	case IEEE80211_BAND_2GHZ:
		if (channel >= 1 && channel <= 14)
			return &priv->channel_info[channel - 1];
		break;
	default:
		BUG();
	}

	return NULL;
}
EXPORT_SYMBOL(iwl_get_channel_info);

