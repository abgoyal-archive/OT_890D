
#ifndef SQUASHFS_FS_SB
#define SQUASHFS_FS_SB

#include "squashfs_fs.h"

struct squashfs_cache {
	char			*name;
	int			entries;
	int			next_blk;
	int			num_waiters;
	int			unused;
	int			block_size;
	int			pages;
	spinlock_t		lock;
	wait_queue_head_t	wait_queue;
	struct squashfs_cache_entry *entry;
};

struct squashfs_cache_entry {
	u64			block;
	int			length;
	int			refcount;
	u64			next_index;
	int			pending;
	int			error;
	int			num_waiters;
	wait_queue_head_t	wait_queue;
	struct squashfs_cache	*cache;
	void			**data;
};

struct squashfs_sb_info {
	int			devblksize;
	int			devblksize_log2;
	struct squashfs_cache	*block_cache;
	struct squashfs_cache	*fragment_cache;
	struct squashfs_cache	*read_page;
	int			next_meta_index;
	__le64			*id_table;
	__le64			*fragment_index;
	unsigned int		*fragment_index_2;
	struct mutex		read_data_mutex;
	struct mutex		meta_index_mutex;
	struct meta_index	*meta_index;
	z_stream		stream;
	__le64			*inode_lookup_table;
	u64			inode_table;
	u64			directory_table;
	unsigned int		block_size;
	unsigned short		block_log;
	long long		bytes_used;
	unsigned int		inodes;
};
#endif
