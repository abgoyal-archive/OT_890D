

#include <linux/key.h>
#include <linux/sysctl.h>
#include "internal.h"

ctl_table key_sysctls[] = {
	{
		.ctl_name = CTL_UNNUMBERED,
		.procname = "maxkeys",
		.data = &key_quota_maxkeys,
		.maxlen = sizeof(unsigned),
		.mode = 0644,
		.proc_handler = &proc_dointvec,
	},
	{
		.ctl_name = CTL_UNNUMBERED,
		.procname = "maxbytes",
		.data = &key_quota_maxbytes,
		.maxlen = sizeof(unsigned),
		.mode = 0644,
		.proc_handler = &proc_dointvec,
	},
	{
		.ctl_name = CTL_UNNUMBERED,
		.procname = "root_maxkeys",
		.data = &key_quota_root_maxkeys,
		.maxlen = sizeof(unsigned),
		.mode = 0644,
		.proc_handler = &proc_dointvec,
	},
	{
		.ctl_name = CTL_UNNUMBERED,
		.procname = "root_maxbytes",
		.data = &key_quota_root_maxbytes,
		.maxlen = sizeof(unsigned),
		.mode = 0644,
		.proc_handler = &proc_dointvec,
	},
	{ .ctl_name = 0 }
};
