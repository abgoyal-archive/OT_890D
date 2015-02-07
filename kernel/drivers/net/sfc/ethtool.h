

#ifndef EFX_ETHTOOL_H
#define EFX_ETHTOOL_H

#include "net_driver.h"


extern int efx_ethtool_get_settings(struct net_device *net_dev,
				    struct ethtool_cmd *ecmd);
extern int efx_ethtool_set_settings(struct net_device *net_dev,
				    struct ethtool_cmd *ecmd);

extern struct ethtool_ops efx_ethtool_ops;

#endif /* EFX_ETHTOOL_H */
