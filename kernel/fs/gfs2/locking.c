

#include <linux/module.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/kmod.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/lm_interface.h>

struct lmh_wrapper {
	struct list_head lw_list;
	const struct lm_lockops *lw_ops;
};

static int nolock_mount(char *table_name, char *host_data,
			lm_callback_t cb, void *cb_data,
			unsigned int min_lvb_size, int flags,
			struct lm_lockstruct *lockstruct,
			struct kobject *fskobj);


static const struct lm_lockops nolock_ops = {
	.lm_proto_name = "lock_nolock",
	.lm_mount = nolock_mount,
};

static struct lmh_wrapper nolock_proto  = {
	.lw_list = LIST_HEAD_INIT(nolock_proto.lw_list),
	.lw_ops = &nolock_ops,
};

static LIST_HEAD(lmh_list);
static DEFINE_MUTEX(lmh_lock);

static int nolock_mount(char *table_name, char *host_data,
			lm_callback_t cb, void *cb_data,
			unsigned int min_lvb_size, int flags,
			struct lm_lockstruct *lockstruct,
			struct kobject *fskobj)
{
	char *c;
	unsigned int jid;

	c = strstr(host_data, "jid=");
	if (!c)
		jid = 0;
	else {
		c += 4;
		sscanf(c, "%u", &jid);
	}

	lockstruct->ls_jid = jid;
	lockstruct->ls_first = 1;
	lockstruct->ls_lvb_size = min_lvb_size;
	lockstruct->ls_ops = &nolock_ops;
	lockstruct->ls_flags = LM_LSFLAG_LOCAL;

	return 0;
}


int gfs2_register_lockproto(const struct lm_lockops *proto)
{
	struct lmh_wrapper *lw;

	mutex_lock(&lmh_lock);

	list_for_each_entry(lw, &lmh_list, lw_list) {
		if (!strcmp(lw->lw_ops->lm_proto_name, proto->lm_proto_name)) {
			mutex_unlock(&lmh_lock);
			printk(KERN_INFO "GFS2: protocol %s already exists\n",
			       proto->lm_proto_name);
			return -EEXIST;
		}
	}

	lw = kzalloc(sizeof(struct lmh_wrapper), GFP_KERNEL);
	if (!lw) {
		mutex_unlock(&lmh_lock);
		return -ENOMEM;
	}

	lw->lw_ops = proto;
	list_add(&lw->lw_list, &lmh_list);

	mutex_unlock(&lmh_lock);

	return 0;
}


void gfs2_unregister_lockproto(const struct lm_lockops *proto)
{
	struct lmh_wrapper *lw;

	mutex_lock(&lmh_lock);

	list_for_each_entry(lw, &lmh_list, lw_list) {
		if (!strcmp(lw->lw_ops->lm_proto_name, proto->lm_proto_name)) {
			list_del(&lw->lw_list);
			mutex_unlock(&lmh_lock);
			kfree(lw);
			return;
		}
	}

	mutex_unlock(&lmh_lock);

	printk(KERN_WARNING "GFS2: can't unregister lock protocol %s\n",
	       proto->lm_proto_name);
}


int gfs2_mount_lockproto(char *proto_name, char *table_name, char *host_data,
			 lm_callback_t cb, void *cb_data,
			 unsigned int min_lvb_size, int flags,
			 struct lm_lockstruct *lockstruct,
			 struct kobject *fskobj)
{
	struct lmh_wrapper *lw = NULL;
	int try = 0;
	int error, found;


retry:
	mutex_lock(&lmh_lock);

	if (list_empty(&nolock_proto.lw_list))
		list_add(&nolock_proto.lw_list, &lmh_list);

	found = 0;
	list_for_each_entry(lw, &lmh_list, lw_list) {
		if (!strcmp(lw->lw_ops->lm_proto_name, proto_name)) {
			found = 1;
			break;
		}
	}

	if (!found) {
		if (!try && capable(CAP_SYS_MODULE)) {
			try = 1;
			mutex_unlock(&lmh_lock);
			request_module(proto_name);
			goto retry;
		}
		printk(KERN_INFO "GFS2: can't find protocol %s\n", proto_name);
		error = -ENOENT;
		goto out;
	}

	if (lw->lw_ops->lm_owner &&
	    !try_module_get(lw->lw_ops->lm_owner)) {
		try = 0;
		mutex_unlock(&lmh_lock);
		msleep(1000);
		goto retry;
	}

	error = lw->lw_ops->lm_mount(table_name, host_data, cb, cb_data,
				     min_lvb_size, flags, lockstruct, fskobj);
	if (error)
		module_put(lw->lw_ops->lm_owner);
out:
	mutex_unlock(&lmh_lock);
	return error;
}

void gfs2_unmount_lockproto(struct lm_lockstruct *lockstruct)
{
	mutex_lock(&lmh_lock);
	if (lockstruct->ls_ops->lm_unmount)
		lockstruct->ls_ops->lm_unmount(lockstruct->ls_lockspace);
	if (lockstruct->ls_ops->lm_owner)
		module_put(lockstruct->ls_ops->lm_owner);
	mutex_unlock(&lmh_lock);
}


void gfs2_withdraw_lockproto(struct lm_lockstruct *lockstruct)
{
	mutex_lock(&lmh_lock);
	lockstruct->ls_ops->lm_withdraw(lockstruct->ls_lockspace);
	if (lockstruct->ls_ops->lm_owner)
		module_put(lockstruct->ls_ops->lm_owner);
	mutex_unlock(&lmh_lock);
}

EXPORT_SYMBOL_GPL(gfs2_register_lockproto);
EXPORT_SYMBOL_GPL(gfs2_unregister_lockproto);

