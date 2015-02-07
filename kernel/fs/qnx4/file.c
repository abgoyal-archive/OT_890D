

#include <linux/fs.h>
#include <linux/qnx4_fs.h>

const struct file_operations qnx4_file_operations =
{
	.llseek		= generic_file_llseek,
	.read		= do_sync_read,
	.aio_read	= generic_file_aio_read,
	.mmap		= generic_file_mmap,
	.splice_read	= generic_file_splice_read,
#ifdef CONFIG_QNX4FS_RW
	.write		= do_sync_write,
	.aio_write	= generic_file_aio_write,
	.fsync		= qnx4_sync_file,
#endif
};

const struct inode_operations qnx4_file_inode_operations =
{
#ifdef CONFIG_QNX4FS_RW
	.truncate	= qnx4_truncate,
#endif
};
