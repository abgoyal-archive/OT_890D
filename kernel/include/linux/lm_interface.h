

#ifndef __LM_INTERFACE_DOT_H__
#define __LM_INTERFACE_DOT_H__


typedef void (*lm_callback_t) (void *ptr, unsigned int type, void *data);


#define LM_MFLAG_SPECTATOR	0x00000001
#define LM_MFLAG_CONV_NODROP	0x00000002


#define LM_LSFLAG_LOCAL		0x00000001


#define LM_TYPE_RESERVED	0x00
#define LM_TYPE_NONDISK		0x01
#define LM_TYPE_INODE		0x02
#define LM_TYPE_RGRP		0x03
#define LM_TYPE_META		0x04
#define LM_TYPE_IOPEN		0x05
#define LM_TYPE_FLOCK		0x06
#define LM_TYPE_PLOCK		0x07
#define LM_TYPE_QUOTA		0x08
#define LM_TYPE_JOURNAL		0x09


#define LM_ST_UNLOCKED		0
#define LM_ST_EXCLUSIVE		1
#define LM_ST_DEFERRED		2
#define LM_ST_SHARED		3


#define LM_FLAG_TRY		0x00000001
#define LM_FLAG_TRY_1CB		0x00000002
#define LM_FLAG_NOEXP		0x00000004
#define LM_FLAG_ANY		0x00000008
#define LM_FLAG_PRIORITY	0x00000010


#define LM_OUT_ST_MASK		0x00000003
#define LM_OUT_CANCELED		0x00000008
#define LM_OUT_ASYNC		0x00000080
#define LM_OUT_ERROR		0x00000100


#define LM_CB_NEED_E		257
#define LM_CB_NEED_D		258
#define LM_CB_NEED_S		259
#define LM_CB_NEED_RECOVERY	260
#define LM_CB_ASYNC		262


#define LM_RD_GAVEUP		308
#define LM_RD_SUCCESS		309


struct lm_lockname {
	u64 ln_number;
	unsigned int ln_type;
};

#define lm_name_equal(name1, name2) \
	(((name1)->ln_number == (name2)->ln_number) && \
	 ((name1)->ln_type == (name2)->ln_type)) \

struct lm_async_cb {
	struct lm_lockname lc_name;
	int lc_ret;
};

struct lm_lockstruct;

struct lm_lockops {
	const char *lm_proto_name;

	/*
	 * Mount/Unmount
	 */

	int (*lm_mount) (char *table_name, char *host_data,
			 lm_callback_t cb, void *cb_data,
			 unsigned int min_lvb_size, int flags,
			 struct lm_lockstruct *lockstruct,
			 struct kobject *fskobj);

	void (*lm_others_may_mount) (void *lockspace);

	void (*lm_unmount) (void *lockspace);

	void (*lm_withdraw) (void *lockspace);

	/*
	 * Lock oriented operations
	 */

	int (*lm_get_lock) (void *lockspace, struct lm_lockname *name, void **lockp);

	void (*lm_put_lock) (void *lock);

	unsigned int (*lm_lock) (void *lock, unsigned int cur_state,
				 unsigned int req_state, unsigned int flags);

	unsigned int (*lm_unlock) (void *lock, unsigned int cur_state);

	void (*lm_cancel) (void *lock);

	int (*lm_hold_lvb) (void *lock, char **lvbp);
	void (*lm_unhold_lvb) (void *lock, char *lvb);

	/*
	 * Posix Lock oriented operations
	 */

	int (*lm_plock_get) (void *lockspace, struct lm_lockname *name,
			     struct file *file, struct file_lock *fl);

	int (*lm_plock) (void *lockspace, struct lm_lockname *name,
			 struct file *file, int cmd, struct file_lock *fl);

	int (*lm_punlock) (void *lockspace, struct lm_lockname *name,
			   struct file *file, struct file_lock *fl);

	/*
	 * Client oriented operations
	 */

	void (*lm_recovery_done) (void *lockspace, unsigned int jid,
				  unsigned int message);

	struct module *lm_owner;
};


struct lm_lockstruct {
	unsigned int ls_jid;
	unsigned int ls_first;
	unsigned int ls_lvb_size;
	void *ls_lockspace;
	const struct lm_lockops *ls_ops;
	int ls_flags;
};


int gfs2_register_lockproto(const struct lm_lockops *proto);
void gfs2_unregister_lockproto(const struct lm_lockops *proto);


int gfs2_mount_lockproto(char *proto_name, char *table_name, char *host_data,
			 lm_callback_t cb, void *cb_data,
			 unsigned int min_lvb_size, int flags,
			 struct lm_lockstruct *lockstruct,
			 struct kobject *fskobj);

void gfs2_unmount_lockproto(struct lm_lockstruct *lockstruct);

void gfs2_withdraw_lockproto(struct lm_lockstruct *lockstruct);

#endif /* __LM_INTERFACE_DOT_H__ */

