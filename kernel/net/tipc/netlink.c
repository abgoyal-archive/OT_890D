

#include "core.h"
#include "config.h"
#include <net/genetlink.h>

static int handle_cmd(struct sk_buff *skb, struct genl_info *info)
{
	struct sk_buff *rep_buf;
	struct nlmsghdr *rep_nlh;
	struct nlmsghdr *req_nlh = info->nlhdr;
	struct tipc_genlmsghdr *req_userhdr = info->userhdr;
	int hdr_space = NLMSG_SPACE(GENL_HDRLEN + TIPC_GENL_HDRLEN);
	u16 cmd;

	if ((req_userhdr->cmd & 0xC000) && (!capable(CAP_NET_ADMIN)))
		cmd = TIPC_CMD_NOT_NET_ADMIN;
	else
		cmd = req_userhdr->cmd;

	rep_buf = tipc_cfg_do_cmd(req_userhdr->dest, cmd,
			NLMSG_DATA(req_nlh) + GENL_HDRLEN + TIPC_GENL_HDRLEN,
			NLMSG_PAYLOAD(req_nlh, GENL_HDRLEN + TIPC_GENL_HDRLEN),
			hdr_space);

	if (rep_buf) {
		skb_push(rep_buf, hdr_space);
		rep_nlh = nlmsg_hdr(rep_buf);
		memcpy(rep_nlh, req_nlh, hdr_space);
		rep_nlh->nlmsg_len = rep_buf->len;
		genlmsg_unicast(rep_buf, NETLINK_CB(skb).pid);
	}

	return 0;
}

static struct genl_family family = {
	.id		= GENL_ID_GENERATE,
	.name		= TIPC_GENL_NAME,
	.version	= TIPC_GENL_VERSION,
	.hdrsize	= TIPC_GENL_HDRLEN,
	.maxattr	= 0,
};

static struct genl_ops ops = {
	.cmd		= TIPC_GENL_CMD,
	.doit		= handle_cmd,
};

static int family_registered = 0;

int tipc_netlink_start(void)
{


	if (genl_register_family(&family))
		goto err;

	family_registered = 1;

	if (genl_register_ops(&family, &ops))
		goto err_unregister;

	return 0;

 err_unregister:
	genl_unregister_family(&family);
	family_registered = 0;
 err:
	err("Failed to register netlink interface\n");
	return -EFAULT;
}

void tipc_netlink_stop(void)
{
	if (family_registered) {
		genl_unregister_family(&family);
		family_registered = 0;
	}
}
