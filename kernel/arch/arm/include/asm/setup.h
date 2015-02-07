
#ifndef __ASMARM_SETUP_H
#define __ASMARM_SETUP_H

#include <asm/types.h>

#define COMMAND_LINE_SIZE 1024

/* The list ends with an ATAG_NONE node. */
#define ATAG_NONE	0x00000000

struct tag_header {
	__u32 size;
	__u32 tag;
};

/* The list must start with an ATAG_CORE node */
#define ATAG_CORE	0x54410001

struct tag_core {
	__u32 flags;		/* bit 0 = read-only */
	__u32 pagesize;
	__u32 rootdev;
};

/* it is allowed to have multiple ATAG_MEM nodes */
#define ATAG_MEM	0x54410002

struct tag_mem32 {
	__u32	size;
	__u32	start;	/* physical start address */
};

/* VGA text type displays */
#define ATAG_VIDEOTEXT	0x54410003

struct tag_videotext {
	__u8		x;
	__u8		y;
	__u16		video_page;
	__u8		video_mode;
	__u8		video_cols;
	__u16		video_ega_bx;
	__u8		video_lines;
	__u8		video_isvga;
	__u16		video_points;
};

/* describes how the ramdisk will be used in kernel */
#define ATAG_RAMDISK	0x54410004

struct tag_ramdisk {
	__u32 flags;	/* bit 0 = load, bit 1 = prompt */
	__u32 size;	/* decompressed ramdisk size in _kilo_ bytes */
	__u32 start;	/* starting block of floppy-based RAM disk image */
};

/* describes where the compressed ramdisk image lives (virtual address) */
#define ATAG_INITRD	0x54410005

/* describes where the compressed ramdisk image lives (physical address) */
#define ATAG_INITRD2	0x54420005

struct tag_initrd {
	__u32 start;	/* physical start address */
	__u32 size;	/* size of compressed ramdisk image in bytes */
};

/* board serial number. "64 bits should be enough for everybody" */
#define ATAG_SERIAL	0x54410006

struct tag_serialnr {
	__u32 low;
	__u32 high;
};

/* board revision */
#define ATAG_REVISION	0x54410007

struct tag_revision {
	__u32 rev;
};

#define ATAG_VIDEOLFB	0x54410008

struct tag_videolfb {
	__u16		lfb_width;
	__u16		lfb_height;
	__u16		lfb_depth;
	__u16		lfb_linelength;
	__u32		lfb_base;
	__u32		lfb_size;
	__u8		red_size;
	__u8		red_pos;
	__u8		green_size;
	__u8		green_pos;
	__u8		blue_size;
	__u8		blue_pos;
	__u8		rsvd_size;
	__u8		rsvd_pos;
};

/* command line: \0 terminated string */
#define ATAG_CMDLINE	0x54410009

struct tag_cmdline {
	char	cmdline[1];	/* this is the minimum size */
};

/* acorn RiscPC specific information */
#define ATAG_ACORN	0x41000101

struct tag_acorn {
	__u32 memc_control_reg;
	__u32 vram_pages;
	__u8 sounddefault;
	__u8 adfsdrives;
};

/* footbridge memory clock, see arch/arm/mach-footbridge/arch.c */
#define ATAG_MEMCLK	0x41000402

struct tag_memclk {
	__u32 fmemclk;
};

/* boot information */
#define ATAG_BOOT	0x41000802

struct tag_boot {
	u32 bootmode;
};

struct tag {
	struct tag_header hdr;
	union {
		struct tag_core		core;
		struct tag_mem32	mem;
		struct tag_videotext	videotext;
		struct tag_ramdisk	ramdisk;
		struct tag_initrd	initrd;
		struct tag_serialnr	serialnr;
		struct tag_revision	revision;
		struct tag_videolfb	videolfb;
		struct tag_cmdline	cmdline;

		/*
		 * Acorn specific
		 */
		struct tag_acorn	acorn;

		/*
		 * DC21285 specific
		 */
		struct tag_memclk	memclk;

		/*
		 * MTK specific
		 */
		struct tag_boot boot;			
	} u;
};

struct tagtable {
	__u32 tag;
	int (*parse)(const struct tag *);
};

#define tag_member_present(tag,member)				\
	((unsigned long)(&((struct tag *)0L)->member + 1)	\
		<= (tag)->hdr.size * 4)

#define tag_next(t)	((struct tag *)((__u32 *)(t) + (t)->hdr.size))
#define tag_size(type)	((sizeof(struct tag_header) + sizeof(struct type)) >> 2)

#define for_each_tag(t,base)		\
	for (t = base; t->hdr.size; t = tag_next(t))

#ifdef __KERNEL__

#define __tag __used __attribute__((__section__(".taglist.init")))
#define __tagtable(tag, fn) \
static struct tagtable __tagtable_##fn __tag = { tag, fn }

#ifdef CONFIG_ARCH_LH7A40X
# define NR_BANKS 16
#else
# define NR_BANKS 8
#endif

struct membank {
	unsigned long start;
	unsigned long size;
	int           node;
};

struct meminfo {
	int nr_banks;
	struct membank bank[NR_BANKS];
};

extern struct meminfo meminfo;

#define for_each_nodebank(iter,mi,no)			\
	for (iter = 0; iter < (mi)->nr_banks; iter++)	\
		if ((mi)->bank[iter].node == no)

#define bank_pfn_start(bank)	__phys_to_pfn((bank)->start)
#define bank_pfn_end(bank)	__phys_to_pfn((bank)->start + (bank)->size)
#define bank_pfn_size(bank)	((bank)->size >> PAGE_SHIFT)
#define bank_phys_start(bank)	(bank)->start
#define bank_phys_end(bank)	((bank)->start + (bank)->size)
#define bank_phys_size(bank)	(bank)->size

struct early_params {
	const char *arg;
	void (*fn)(char **p);
};

#define __early_param(name,fn)					\
static struct early_params __early_##fn __used			\
__attribute__((__section__(".early_param.init"))) = { name, fn }

#endif  /*  __KERNEL__  */

#endif
