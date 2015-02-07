

#ifndef __LINUX_NET_DSA_H
#define __LINUX_NET_DSA_H

#define DSA_MAX_PORTS	12

struct dsa_platform_data {
	/*
	 * Reference to a Linux network interface that connects
	 * to the switch chip.
	 */
	struct device	*netdev;

	/*
	 * How to access the switch configuration registers, and
	 * the names of the switch ports (use "cpu" to designate
	 * the switch port that the cpu is connected to).
	 */
	struct device	*mii_bus;
	int		sw_addr;
	char		*port_names[DSA_MAX_PORTS];
};

extern bool dsa_uses_dsa_tags(void *dsa_ptr);
extern bool dsa_uses_trailer_tags(void *dsa_ptr);


#endif
