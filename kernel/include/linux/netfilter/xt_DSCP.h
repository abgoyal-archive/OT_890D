
#ifndef _XT_DSCP_TARGET_H
#define _XT_DSCP_TARGET_H
#include <linux/netfilter/xt_dscp.h>

/* target info */
struct xt_DSCP_info {
	u_int8_t dscp;
};

struct xt_tos_target_info {
	u_int8_t tos_value;
	u_int8_t tos_mask;
};

#endif /* _XT_DSCP_TARGET_H */
