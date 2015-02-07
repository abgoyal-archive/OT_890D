

#ifndef EFX_TX_H
#define EFX_TX_H

#include "net_driver.h"

int efx_probe_tx_queue(struct efx_tx_queue *tx_queue);
void efx_remove_tx_queue(struct efx_tx_queue *tx_queue);
void efx_init_tx_queue(struct efx_tx_queue *tx_queue);
void efx_fini_tx_queue(struct efx_tx_queue *tx_queue);

int efx_hard_start_xmit(struct sk_buff *skb, struct net_device *net_dev);
void efx_release_tx_buffers(struct efx_tx_queue *tx_queue);

#endif /* EFX_TX_H */
