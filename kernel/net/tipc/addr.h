

#ifndef _TIPC_ADDR_H
#define _TIPC_ADDR_H

static inline u32 own_node(void)
{
	return tipc_node(tipc_own_addr);
}

static inline u32 own_cluster(void)
{
	return tipc_cluster(tipc_own_addr);
}

static inline u32 own_zone(void)
{
	return tipc_zone(tipc_own_addr);
}

static inline int in_own_cluster(u32 addr)
{
	return !((addr ^ tipc_own_addr) >> 12);
}

static inline int is_slave(u32 addr)
{
	return addr & 0x800;
}

static inline int may_route(u32 addr)
{
	return(addr ^ tipc_own_addr) >> 11;
}

static inline int in_scope(u32 domain, u32 addr)
{
	if (!domain || (domain == addr))
		return 1;
	if (domain == (addr & 0xfffff000u)) /* domain <Z.C.0> */
		return 1;
	if (domain == (addr & 0xff000000u)) /* domain <Z.0.0> */
		return 1;
	return 0;
}


static inline int addr_scope(u32 domain)
{
	if (likely(!domain))
		return TIPC_ZONE_SCOPE;
	if (tipc_node(domain))
		return TIPC_NODE_SCOPE;
	if (tipc_cluster(domain))
		return TIPC_CLUSTER_SCOPE;
	return TIPC_ZONE_SCOPE;
}


static inline int addr_domain(int sc)
{
	if (likely(sc == TIPC_NODE_SCOPE))
		return tipc_own_addr;
	if (sc == TIPC_CLUSTER_SCOPE)
		return tipc_addr(tipc_zone(tipc_own_addr),
				 tipc_cluster(tipc_own_addr), 0);
	return tipc_addr(tipc_zone(tipc_own_addr), 0, 0);
}

static inline char *addr_string_fill(char *string, u32 addr)
{
	snprintf(string, 16, "<%u.%u.%u>",
		 tipc_zone(addr), tipc_cluster(addr), tipc_node(addr));
	return string;
}

int tipc_addr_domain_valid(u32);
int tipc_addr_node_valid(u32 addr);

#endif
