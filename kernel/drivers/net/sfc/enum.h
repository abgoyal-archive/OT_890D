

#ifndef EFX_ENUM_H
#define EFX_ENUM_H

/* Please keep in order and up-to-date w.r.t the following two #defines */
enum efx_loopback_mode {
	LOOPBACK_NONE = 0,
	LOOPBACK_GMAC = 1,
	LOOPBACK_XGMII = 2,
	LOOPBACK_XGXS = 3,
	LOOPBACK_XAUI = 4,
	LOOPBACK_GPHY = 5,
	LOOPBACK_PHYXS = 6,
	LOOPBACK_PCS = 7,
	LOOPBACK_PMAPMD = 8,
	LOOPBACK_NETWORK = 9,
	LOOPBACK_MAX
};

#define LOOPBACK_TEST_MAX LOOPBACK_PMAPMD

extern const char *efx_loopback_mode_names[];
#define LOOPBACK_MODE_NAME(mode)			\
	STRING_TABLE_LOOKUP(mode, efx_loopback_mode)
#define LOOPBACK_MODE(efx)				\
	LOOPBACK_MODE_NAME(efx->loopback_mode)

/* These loopbacks occur within the controller */
#define LOOPBACKS_INTERNAL ((1 << LOOPBACK_GMAC) |     \
			    (1 << LOOPBACK_XGMII)|     \
			    (1 << LOOPBACK_XGXS) |     \
			    (1 << LOOPBACK_XAUI))

#define LOOPBACK_MASK(_efx)			\
	(1 << (_efx)->loopback_mode)

#define LOOPBACK_INTERNAL(_efx)				\
	(!!(LOOPBACKS_INTERNAL & LOOPBACK_MASK(_efx)))

#define LOOPBACK_CHANGED(_from, _to, _mask)				\
	(!!((LOOPBACK_MASK(_from) ^ LOOPBACK_MASK(_to)) & (_mask)))

#define LOOPBACK_OUT_OF(_from, _to, _mask)				\
	((LOOPBACK_MASK(_from) & (_mask)) && !(LOOPBACK_MASK(_to) & (_mask)))

/*****************************************************************************/

enum reset_type {
	RESET_TYPE_NONE = -1,
	RESET_TYPE_INVISIBLE = 0,
	RESET_TYPE_ALL = 1,
	RESET_TYPE_WORLD = 2,
	RESET_TYPE_DISABLE = 3,
	RESET_TYPE_MAX_METHOD,
	RESET_TYPE_TX_WATCHDOG,
	RESET_TYPE_INT_ERROR,
	RESET_TYPE_RX_RECOVERY,
	RESET_TYPE_RX_DESC_FETCH,
	RESET_TYPE_TX_DESC_FETCH,
	RESET_TYPE_TX_SKIP,
	RESET_TYPE_MAX,
};

#endif /* EFX_ENUM_H */
