
#if !defined(__AEE_IPANIC_H__)
#define __AEE_IPANIC_H__

#include <linux/autoconf.h>
#include <linux/kallsyms.h>


#define IPANIC_OOPS_BLOCK_COUNT 64

#define AEE_IPANIC_MAGIC 0xaee0dead
#define AEE_IPANIC_PHDR_VERSION   0x02
#define AEE_IPANIC_DATALENGTH_MAX (64 * 1024)

struct ipanic_header {
	u32 magic;

	u32 version;

	u32 console_offset;
	u32 console_length;

	u32 oops_header_offset;
	u32 oops_header_length;

	u32 oops_detail_offset;
	u32 oops_detail_length;
};

#define PANICLOG_BACKTRACE_NUM 4
#define PANICLOG_SOURCE_LENGTH 16

#define IPANIC_OOPS_HEADER_BACKTRACE_LENGTH 3072
#define IPANIC_OOPS_HEADER_PROCESS_NAME_LENGTH 256

struct ipanic_oops_header 
{
	char backtrace[IPANIC_OOPS_HEADER_BACKTRACE_LENGTH];
	char process_path[IPANIC_OOPS_HEADER_PROCESS_NAME_LENGTH];
};

struct ipanic_oops
{
	struct ipanic_oops_header header;

	char *detail;
	int detail_len;

	char *console;
	int console_len;
};

struct ipanic_data {
	struct mtd_info		*mtd;
	struct ipanic_header	curr;
	void			*bounce;
	u32 blk_offset[IPANIC_OOPS_BLOCK_COUNT];

	struct proc_dir_entry	*oops;
};

struct ipanic_oops *ipanic_oops_copy(void);

void ipanic_oops_free(struct ipanic_oops *oops, int erase);

#endif
