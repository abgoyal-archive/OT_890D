

#include "sysv.h"

const struct file_operations sysv_file_operations = {
	.llseek		= generic_file_llseek,
	.read		= do_sync_read,
	.aio_read	= generic_file_aio_read,
	.write		= do_sync_write,
	.aio_write	= generic_file_aio_write,
	.mmap		= generic_file_mmap,
	.fsync		= sysv_sync_file,
	.splice_read	= generic_file_splice_read,
};

const struct inode_operations sysv_file_inode_operations = {
	.truncate	= sysv_truncate,
	.getattr	= sysv_getattr,
};

int sysv_sync_file(struct file * file, struct dentry *dentry, int datasync)
{
	struct inode *inode = dentry->d_inode;
	int err;

	err = sync_mapping_buffers(inode->i_mapping);
	if (!(inode->i_state & I_DIRTY))
		return err;
	if (datasync && !(inode->i_state & I_DIRTY_DATASYNC))
		return err;
	
	err |= sysv_sync_inode(inode);
	return err ? -EIO : 0;
}
