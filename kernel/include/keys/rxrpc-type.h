

#ifndef _KEYS_RXRPC_TYPE_H
#define _KEYS_RXRPC_TYPE_H

#include <linux/key.h>

extern struct key_type key_type_rxrpc;

extern struct key *rxrpc_get_null_key(const char *);

#endif /* _KEYS_RXRPC_TYPE_H */
