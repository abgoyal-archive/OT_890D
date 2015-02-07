

#include "core.h"
#include "dbg.h"
#include "addr.h"
#include "zone.h"
#include "cluster.h"
#include "net.h"

u32 tipc_get_addr(void)
{
	return tipc_own_addr;
}


int tipc_addr_domain_valid(u32 addr)
{
	u32 n = tipc_node(addr);
	u32 c = tipc_cluster(addr);
	u32 z = tipc_zone(addr);
	u32 max_nodes = tipc_max_nodes;

	if (is_slave(addr))
		max_nodes = LOWEST_SLAVE + tipc_max_slaves;
	if (n > max_nodes)
		return 0;
	if (c > tipc_max_clusters)
		return 0;
	if (z > tipc_max_zones)
		return 0;

	if (n && (!z || !c))
		return 0;
	if (c && !z)
		return 0;
	return 1;
}


int tipc_addr_node_valid(u32 addr)
{
	return (tipc_addr_domain_valid(addr) && tipc_node(addr));
}

