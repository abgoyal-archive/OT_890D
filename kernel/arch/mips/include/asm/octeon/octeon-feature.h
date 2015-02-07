


#ifndef __OCTEON_FEATURE_H__
#define __OCTEON_FEATURE_H__

enum octeon_feature {
	/*
	 * Octeon models in the CN5XXX family and higher support
	 * atomic add instructions to memory (saa/saad).
	 */
	OCTEON_FEATURE_SAAD,
	/* Does this Octeon support the ZIP offload engine? */
	OCTEON_FEATURE_ZIP,
	/* Does this Octeon support crypto acceleration using COP2? */
	OCTEON_FEATURE_CRYPTO,
	/* Does this Octeon support PCI express? */
	OCTEON_FEATURE_PCIE,
	/* Some Octeon models support internal memory for storing
	 * cryptographic keys */
	OCTEON_FEATURE_KEY_MEMORY,
	/* Octeon has a LED controller for banks of external LEDs */
	OCTEON_FEATURE_LED_CONTROLLER,
	/* Octeon has a trace buffer */
	OCTEON_FEATURE_TRA,
	/* Octeon has a management port */
	OCTEON_FEATURE_MGMT_PORT,
	/* Octeon has a raid unit */
	OCTEON_FEATURE_RAID,
	/* Octeon has a builtin USB */
	OCTEON_FEATURE_USB,
};

static inline int cvmx_fuse_read(int fuse);

static inline int octeon_has_feature(enum octeon_feature feature)
{
	switch (feature) {
	case OCTEON_FEATURE_SAAD:
		return !OCTEON_IS_MODEL(OCTEON_CN3XXX);

	case OCTEON_FEATURE_ZIP:
		if (OCTEON_IS_MODEL(OCTEON_CN30XX)
		    || OCTEON_IS_MODEL(OCTEON_CN50XX)
		    || OCTEON_IS_MODEL(OCTEON_CN52XX))
			return 0;
		else if (OCTEON_IS_MODEL(OCTEON_CN38XX_PASS1))
			return 1;
		else
			return !cvmx_fuse_read(121);

	case OCTEON_FEATURE_CRYPTO:
		return !cvmx_fuse_read(90);

	case OCTEON_FEATURE_PCIE:
		return OCTEON_IS_MODEL(OCTEON_CN56XX)
			|| OCTEON_IS_MODEL(OCTEON_CN52XX);

	case OCTEON_FEATURE_KEY_MEMORY:
	case OCTEON_FEATURE_LED_CONTROLLER:
		return OCTEON_IS_MODEL(OCTEON_CN38XX)
			|| OCTEON_IS_MODEL(OCTEON_CN58XX)
			|| OCTEON_IS_MODEL(OCTEON_CN56XX);
	case OCTEON_FEATURE_TRA:
		return !(OCTEON_IS_MODEL(OCTEON_CN30XX)
			 || OCTEON_IS_MODEL(OCTEON_CN50XX));
	case OCTEON_FEATURE_MGMT_PORT:
		return OCTEON_IS_MODEL(OCTEON_CN56XX)
			|| OCTEON_IS_MODEL(OCTEON_CN52XX);
	case OCTEON_FEATURE_RAID:
		return OCTEON_IS_MODEL(OCTEON_CN56XX)
			|| OCTEON_IS_MODEL(OCTEON_CN52XX);
	case OCTEON_FEATURE_USB:
		return !(OCTEON_IS_MODEL(OCTEON_CN38XX)
			 || OCTEON_IS_MODEL(OCTEON_CN58XX));
	}
	return 0;
}

#endif /* __OCTEON_FEATURE_H__ */
