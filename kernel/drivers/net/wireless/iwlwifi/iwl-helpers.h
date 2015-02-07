

#ifndef __iwl_helpers_h__
#define __iwl_helpers_h__

#include <linux/ctype.h>

#define IWL_MASK(lo, hi) ((1 << (hi)) | ((1 << (hi)) - (1 << (lo))))


static inline struct ieee80211_conf *ieee80211_get_hw_conf(
	struct ieee80211_hw *hw)
{
	return &hw->conf;
}

static inline int iwl_check_bits(unsigned long field, unsigned long mask)
{
	return ((field & mask) == mask) ? 1 : 0;
}

static inline unsigned long elapsed_jiffies(unsigned long start,
					    unsigned long end)
{
	if (end >= start)
		return end - start;

	return end + (MAX_JIFFY_OFFSET - start) + 1;
}

static inline int iwl_queue_inc_wrap(int index, int n_bd)
{
	return ++index & (n_bd - 1);
}

static inline int iwl_queue_dec_wrap(int index, int n_bd)
{
	return --index & (n_bd - 1);
}

/* TODO: Move fw_desc functions to iwl-pci.ko */
static inline void iwl_free_fw_desc(struct pci_dev *pci_dev,
				    struct fw_desc *desc)
{
	if (desc->v_addr)
		pci_free_consistent(pci_dev, desc->len,
				    desc->v_addr, desc->p_addr);
	desc->v_addr = NULL;
	desc->len = 0;
}

static inline int iwl_alloc_fw_desc(struct pci_dev *pci_dev,
				    struct fw_desc *desc)
{
	desc->v_addr = pci_alloc_consistent(pci_dev, desc->len, &desc->p_addr);
	return (desc->v_addr != NULL) ? 0 : -ENOMEM;
}

#endif				/* __iwl_helpers_h__ */
