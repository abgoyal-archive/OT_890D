

#ifndef ISCSI_IBFT_H
#define ISCSI_IBFT_H

struct ibft_table_header {
	char signature[4];
	u32 length;
	u8 revision;
	u8 checksum;
	char oem_id[6];
	char oem_table_id[8];
	char reserved[24];
} __attribute__((__packed__));

extern struct ibft_table_header *ibft_addr;

#ifdef CONFIG_ISCSI_IBFT_FIND
extern void __init reserve_ibft_region(void);
#else
static inline void reserve_ibft_region(void) { }
#endif

#endif /* ISCSI_IBFT_H */
