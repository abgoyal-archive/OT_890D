

#ifndef _ASM_X86_DS_H
#define _ASM_X86_DS_H


#include <linux/types.h>
#include <linux/init.h>
#include <linux/err.h>


#ifdef CONFIG_X86_DS

struct task_struct;
struct ds_context;
struct ds_tracer;
struct bts_tracer;
struct pebs_tracer;

typedef void (*bts_ovfl_callback_t)(struct bts_tracer *);
typedef void (*pebs_ovfl_callback_t)(struct pebs_tracer *);


enum ds_feature {
	dsf_bts = 0,
	dsf_bts_kernel,
#define BTS_KERNEL (1 << dsf_bts_kernel)
	/* trace kernel-mode branches */

	dsf_bts_user,
#define BTS_USER (1 << dsf_bts_user)
	/* trace user-mode branches */

	dsf_bts_overflow,
	dsf_bts_max,
	dsf_pebs = dsf_bts_max,

	dsf_pebs_max,
	dsf_ctl_max = dsf_pebs_max,
	dsf_bts_timestamps = dsf_ctl_max,
#define BTS_TIMESTAMPS (1 << dsf_bts_timestamps)
	/* add timestamps into BTS trace */

#define BTS_USER_FLAGS (BTS_KERNEL | BTS_USER | BTS_TIMESTAMPS)
};


extern struct bts_tracer *ds_request_bts(struct task_struct *task,
					 void *base, size_t size,
					 bts_ovfl_callback_t ovfl,
					 size_t th, unsigned int flags);
extern struct pebs_tracer *ds_request_pebs(struct task_struct *task,
					   void *base, size_t size,
					   pebs_ovfl_callback_t ovfl,
					   size_t th, unsigned int flags);

extern void ds_release_bts(struct bts_tracer *tracer);
extern void ds_suspend_bts(struct bts_tracer *tracer);
extern void ds_resume_bts(struct bts_tracer *tracer);
extern void ds_release_pebs(struct pebs_tracer *tracer);
extern void ds_suspend_pebs(struct pebs_tracer *tracer);
extern void ds_resume_pebs(struct pebs_tracer *tracer);


struct ds_trace {
	/* the number of bts/pebs records */
	size_t n;
	/* the size of a bts/pebs record in bytes */
	size_t size;
	/* pointers into the raw buffer:
	   - to the first entry */
	void *begin;
	/* - one beyond the last entry */
	void *end;
	/* - one beyond the newest entry */
	void *top;
	/* - the interrupt threshold */
	void *ith;
	/* flags given on ds_request() */
	unsigned int flags;
};

enum bts_qualifier {
	bts_invalid,
#define BTS_INVALID bts_invalid

	bts_branch,
#define BTS_BRANCH bts_branch

	bts_task_arrives,
#define BTS_TASK_ARRIVES bts_task_arrives

	bts_task_departs,
#define BTS_TASK_DEPARTS bts_task_departs

	bts_qual_bit_size = 4,
	bts_qual_max = (1 << bts_qual_bit_size),
};

struct bts_struct {
	__u64 qualifier;
	union {
		/* BTS_BRANCH */
		struct {
			__u64 from;
			__u64 to;
		} lbr;
		/* BTS_TASK_ARRIVES or BTS_TASK_DEPARTS */
		struct {
			__u64 jiffies;
			pid_t pid;
		} timestamp;
	} variant;
};


struct bts_trace {
	struct ds_trace ds;

	int (*read)(struct bts_tracer *tracer, const void *at,
		    struct bts_struct *out);
	int (*write)(struct bts_tracer *tracer, const struct bts_struct *in);
};


struct pebs_trace {
	struct ds_trace ds;

	/* the PEBS reset value */
	unsigned long long reset_value;
};


extern const struct bts_trace *ds_read_bts(struct bts_tracer *tracer);
extern const struct pebs_trace *ds_read_pebs(struct pebs_tracer *tracer);


extern int ds_reset_bts(struct bts_tracer *tracer);
extern int ds_reset_pebs(struct pebs_tracer *tracer);

extern int ds_set_pebs_reset(struct pebs_tracer *tracer, u64 value);

struct cpuinfo_x86;
extern void __cpuinit ds_init_intel(struct cpuinfo_x86 *);

extern void ds_switch_to(struct task_struct *prev, struct task_struct *next);

extern void ds_copy_thread(struct task_struct *tsk, struct task_struct *father);
extern void ds_exit_thread(struct task_struct *tsk);

#else /* CONFIG_X86_DS */

struct cpuinfo_x86;
static inline void __cpuinit ds_init_intel(struct cpuinfo_x86 *ignored) {}
static inline void ds_switch_to(struct task_struct *prev,
				struct task_struct *next) {}
static inline void ds_copy_thread(struct task_struct *tsk,
				  struct task_struct *father) {}
static inline void ds_exit_thread(struct task_struct *tsk) {}

#endif /* CONFIG_X86_DS */
#endif /* _ASM_X86_DS_H */
