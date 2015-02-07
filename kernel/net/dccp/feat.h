
#ifndef _DCCP_FEAT_H
#define _DCCP_FEAT_H
#include <linux/types.h>
#include "dccp.h"

/* Ack Ratio takes 2-byte integer values (11.3) */
#define DCCPF_ACK_RATIO_MAX	0xFFFF
/* Wmin=32 and Wmax=2^46-1 from 7.5.2 */
#define DCCPF_SEQ_WMIN		32
#define DCCPF_SEQ_WMAX		0x3FFFFFFFFFFFull
/* Maximum number of SP values that fit in a single (Confirm) option */
#define DCCP_FEAT_MAX_SP_VALS	(DCCP_SINGLE_OPT_MAXLEN - 2)

enum dccp_feat_type {
	FEAT_AT_RX   = 1,	/* located at RX side of half-connection  */
	FEAT_AT_TX   = 2,	/* located at TX side of half-connection  */
	FEAT_SP      = 4,	/* server-priority reconciliation (6.3.1) */
	FEAT_NN	     = 8,	/* non-negotiable reconciliation (6.3.2)  */
	FEAT_UNKNOWN = 0xFF	/* not understood or invalid feature	  */
};

enum dccp_feat_state {
	FEAT_DEFAULT = 0,	/* using default values from 6.4 */
	FEAT_INITIALISING,	/* feature is being initialised  */
	FEAT_CHANGING,		/* Change sent but not confirmed yet */
	FEAT_UNSTABLE,		/* local modification in state CHANGING */
	FEAT_STABLE		/* both ends (think they) agree */
};

typedef union {
	u64 nn;
	struct {
		u8	*vec;
		u8	len;
	}   sp;
} dccp_feat_val;

struct dccp_feat_entry {
	dccp_feat_val           val;
	enum dccp_feat_state    state:8;
	u8                      feat_num;

	bool			needs_mandatory,
				needs_confirm,
				empty_confirm,
				is_local;

	struct list_head	node;
};

static inline u8 dccp_feat_genopt(struct dccp_feat_entry *entry)
{
	if (entry->needs_confirm)
		return entry->is_local ? DCCPO_CONFIRM_L : DCCPO_CONFIRM_R;
	return entry->is_local ? DCCPO_CHANGE_L : DCCPO_CHANGE_R;
}

struct ccid_dependency {
	u8	dependent_feat;
	bool	is_local:1,
		is_mandatory:1;
	u8	val;
};

#ifdef CONFIG_IP_DCCP_DEBUG
extern const char *dccp_feat_typename(const u8 type);
extern const char *dccp_feat_name(const u8 feat);

static inline void dccp_feat_debug(const u8 type, const u8 feat, const u8 val)
{
	dccp_pr_debug("%s(%s (%d), %d)\n", dccp_feat_typename(type),
					   dccp_feat_name(feat), feat, val);
}
#else
#define dccp_feat_debug(type, feat, val)
#endif /* CONFIG_IP_DCCP_DEBUG */

extern int  dccp_feat_register_sp(struct sock *sk, u8 feat, u8 is_local,
				  u8 const *list, u8 len);
extern int  dccp_feat_register_nn(struct sock *sk, u8 feat, u64 val);
extern int  dccp_feat_parse_options(struct sock *, struct dccp_request_sock *,
				    u8 mand, u8 opt, u8 feat, u8 *val, u8 len);
extern int  dccp_feat_clone_list(struct list_head const *, struct list_head *);
extern int  dccp_feat_init(struct sock *sk);

#define DCCP_OPTVAL_MAXLEN	6

extern void dccp_encode_value_var(const u64 value, u8 *to, const u8 len);
extern u64  dccp_decode_value_var(const u8 *bf, const u8 len);

extern int  dccp_insert_option_mandatory(struct sk_buff *skb);
extern int  dccp_insert_fn_opt(struct sk_buff *skb, u8 type, u8 feat,
			       u8 *val, u8 len, bool repeat_first);
#endif /* _DCCP_FEAT_H */
