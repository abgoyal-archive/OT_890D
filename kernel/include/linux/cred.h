

#ifndef _LINUX_CRED_H
#define _LINUX_CRED_H

#include <linux/capability.h>
#include <linux/key.h>
#include <asm/atomic.h>

struct user_struct;
struct cred;
struct inode;

#define NGROUPS_SMALL		32
#define NGROUPS_PER_BLOCK	((unsigned int)(PAGE_SIZE / sizeof(gid_t)))

struct group_info {
	atomic_t	usage;
	int		ngroups;
	int		nblocks;
	gid_t		small_block[NGROUPS_SMALL];
	gid_t		*blocks[0];
};

static inline struct group_info *get_group_info(struct group_info *gi)
{
	atomic_inc(&gi->usage);
	return gi;
}

#define put_group_info(group_info)			\
do {							\
	if (atomic_dec_and_test(&(group_info)->usage))	\
		groups_free(group_info);		\
} while (0)

extern struct group_info *groups_alloc(int);
extern struct group_info init_groups;
extern void groups_free(struct group_info *);
extern int set_current_groups(struct group_info *);
extern int set_groups(struct cred *, struct group_info *);
extern int groups_search(const struct group_info *, gid_t);

/* access the groups "array" with this macro */
#define GROUP_AT(gi, i) \
	((gi)->blocks[(i) / NGROUPS_PER_BLOCK][(i) % NGROUPS_PER_BLOCK])

extern int in_group_p(gid_t);
extern int in_egroup_p(gid_t);

#ifdef CONFIG_KEYS
struct thread_group_cred {
	atomic_t	usage;
	pid_t		tgid;			/* thread group process ID */
	spinlock_t	lock;
	struct key	*session_keyring;	/* keyring inherited over fork */
	struct key	*process_keyring;	/* keyring private to this process */
	struct rcu_head	rcu;			/* RCU deletion hook */
};
#endif

struct cred {
	atomic_t	usage;
	uid_t		uid;		/* real UID of the task */
	gid_t		gid;		/* real GID of the task */
	uid_t		suid;		/* saved UID of the task */
	gid_t		sgid;		/* saved GID of the task */
	uid_t		euid;		/* effective UID of the task */
	gid_t		egid;		/* effective GID of the task */
	uid_t		fsuid;		/* UID for VFS ops */
	gid_t		fsgid;		/* GID for VFS ops */
	unsigned	securebits;	/* SUID-less security management */
	kernel_cap_t	cap_inheritable; /* caps our children can inherit */
	kernel_cap_t	cap_permitted;	/* caps we're permitted */
	kernel_cap_t	cap_effective;	/* caps we can actually use */
	kernel_cap_t	cap_bset;	/* capability bounding set */
#ifdef CONFIG_KEYS
	unsigned char	jit_keyring;	/* default keyring to attach requested
					 * keys to */
	struct key	*thread_keyring; /* keyring private to this thread */
	struct key	*request_key_auth; /* assumed request_key authority */
	struct thread_group_cred *tgcred; /* thread-group shared credentials */
#endif
#ifdef CONFIG_SECURITY
	void		*security;	/* subjective LSM security */
#endif
	struct user_struct *user;	/* real user ID subscription */
	struct group_info *group_info;	/* supplementary groups for euid/fsgid */
	struct rcu_head	rcu;		/* RCU deletion hook */
};

extern void __put_cred(struct cred *);
extern int copy_creds(struct task_struct *, unsigned long);
extern struct cred *prepare_creds(void);
extern struct cred *prepare_exec_creds(void);
extern struct cred *prepare_usermodehelper_creds(void);
extern int commit_creds(struct cred *);
extern void abort_creds(struct cred *);
extern const struct cred *override_creds(const struct cred *);
extern void revert_creds(const struct cred *);
extern struct cred *prepare_kernel_cred(struct task_struct *);
extern int change_create_files_as(struct cred *, struct inode *);
extern int set_security_override(struct cred *, u32);
extern int set_security_override_from_ctx(struct cred *, const char *);
extern int set_create_files_as(struct cred *, struct inode *);
extern void __init cred_init(void);

static inline struct cred *get_new_cred(struct cred *cred)
{
	atomic_inc(&cred->usage);
	return cred;
}

static inline const struct cred *get_cred(const struct cred *cred)
{
	return get_new_cred((struct cred *) cred);
}

static inline void put_cred(const struct cred *_cred)
{
	struct cred *cred = (struct cred *) _cred;

	BUG_ON(atomic_read(&(cred)->usage) <= 0);
	if (atomic_dec_and_test(&(cred)->usage))
		__put_cred(cred);
}

#define current_cred() \
	(current->cred)

#define __task_cred(task) \
	((const struct cred *)(rcu_dereference((task)->real_cred)))

#define get_task_cred(task)				\
({							\
	struct cred *__cred;				\
	rcu_read_lock();				\
	__cred = (struct cred *) __task_cred((task));	\
	get_cred(__cred);				\
	rcu_read_unlock();				\
	__cred;						\
})

#define get_current_cred()				\
	(get_cred(current_cred()))

#define get_current_user()				\
({							\
	struct user_struct *__u;			\
	struct cred *__cred;				\
	__cred = (struct cred *) current_cred();	\
	__u = get_uid(__cred->user);			\
	__u;						\
})

#define get_current_groups()				\
({							\
	struct group_info *__groups;			\
	struct cred *__cred;				\
	__cred = (struct cred *) current_cred();	\
	__groups = get_group_info(__cred->group_info);	\
	__groups;					\
})

#define task_cred_xxx(task, xxx)			\
({							\
	__typeof__(((struct cred *)NULL)->xxx) ___val;	\
	rcu_read_lock();				\
	___val = __task_cred((task))->xxx;		\
	rcu_read_unlock();				\
	___val;						\
})

#define task_uid(task)		(task_cred_xxx((task), uid))
#define task_euid(task)		(task_cred_xxx((task), euid))

#define current_cred_xxx(xxx)			\
({						\
	current->cred->xxx;			\
})

#define current_uid()		(current_cred_xxx(uid))
#define current_gid()		(current_cred_xxx(gid))
#define current_euid()		(current_cred_xxx(euid))
#define current_egid()		(current_cred_xxx(egid))
#define current_suid()		(current_cred_xxx(suid))
#define current_sgid()		(current_cred_xxx(sgid))
#define current_fsuid() 	(current_cred_xxx(fsuid))
#define current_fsgid() 	(current_cred_xxx(fsgid))
#define current_cap()		(current_cred_xxx(cap_effective))
#define current_user()		(current_cred_xxx(user))
#define current_user_ns()	(current_cred_xxx(user)->user_ns)
#define current_security()	(current_cred_xxx(security))

#define current_uid_gid(_uid, _gid)		\
do {						\
	const struct cred *__cred;		\
	__cred = current_cred();		\
	*(_uid) = __cred->uid;			\
	*(_gid) = __cred->gid;			\
} while(0)

#define current_euid_egid(_euid, _egid)		\
do {						\
	const struct cred *__cred;		\
	__cred = current_cred();		\
	*(_euid) = __cred->euid;		\
	*(_egid) = __cred->egid;		\
} while(0)

#define current_fsuid_fsgid(_fsuid, _fsgid)	\
do {						\
	const struct cred *__cred;		\
	__cred = current_cred();		\
	*(_fsuid) = __cred->fsuid;		\
	*(_fsgid) = __cred->fsgid;		\
} while(0)

#endif /* _LINUX_CRED_H */
