
#ifndef _ASM_X86_UV_BIOS_H
#define _ASM_X86_UV_BIOS_H


#include <linux/rtc.h>

enum uv_bios_cmd {
	UV_BIOS_COMMON,
	UV_BIOS_GET_SN_INFO,
	UV_BIOS_FREQ_BASE,
	UV_BIOS_WATCHLIST_ALLOC,
	UV_BIOS_WATCHLIST_FREE,
	UV_BIOS_MEMPROTECT,
	UV_BIOS_GET_PARTITION_ADDR
};

enum {
	BIOS_STATUS_MORE_PASSES		=  1,
	BIOS_STATUS_SUCCESS		=  0,
	BIOS_STATUS_UNIMPLEMENTED	= -ENOSYS,
	BIOS_STATUS_EINVAL		= -EINVAL,
	BIOS_STATUS_UNAVAIL		= -EBUSY
};

struct uv_systab {
	char signature[4];	/* must be "UVST" */
	u32 revision;		/* distinguish different firmware revs */
	u64 function;		/* BIOS runtime callback function ptr */
};

enum {
	BIOS_FREQ_BASE_PLATFORM = 0,
	BIOS_FREQ_BASE_INTERVAL_TIMER = 1,
	BIOS_FREQ_BASE_REALTIME_CLOCK = 2
};

union partition_info_u {
	u64	val;
	struct {
		u64	hub_version	:  8,
			partition_id	: 16,
			coherence_id	: 16,
			region_size	: 24;
	};
};

union uv_watchlist_u {
	u64	val;
	struct {
		u64	blade	: 16,
			size	: 32,
			filler	: 16;
	};
};

enum uv_memprotect {
	UV_MEMPROT_RESTRICT_ACCESS,
	UV_MEMPROT_ALLOW_AMO,
	UV_MEMPROT_ALLOW_RW
};

extern s64 uv_bios_call(enum uv_bios_cmd, u64, u64, u64, u64, u64);
extern s64 uv_bios_call_irqsave(enum uv_bios_cmd, u64, u64, u64, u64, u64);
extern s64 uv_bios_call_reentrant(enum uv_bios_cmd, u64, u64, u64, u64, u64);

extern s64 uv_bios_get_sn_info(int, int *, long *, long *, long *);
extern s64 uv_bios_freq_base(u64, u64 *);
extern int uv_bios_mq_watchlist_alloc(int, unsigned long, unsigned int,
					unsigned long *);
extern int uv_bios_mq_watchlist_free(int, int);
extern s64 uv_bios_change_memprotect(u64, u64, enum uv_memprotect);
extern s64 uv_bios_reserved_page_pa(u64, u64 *, u64 *, u64 *);

extern void uv_bios_init(void);

extern unsigned long sn_rtc_cycles_per_second;
extern int uv_type;
extern long sn_partition_id;
extern long sn_coherency_id;
extern long sn_region_size;
#define partition_coherence_id()	(sn_coherency_id)

extern struct kobject *sgi_uv_kobj;	/* /sys/firmware/sgi_uv */

#endif /* _ASM_X86_UV_BIOS_H */