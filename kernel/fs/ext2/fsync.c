

#include "ext2.h"
#include <linux/buffer_head.h>		/* for sync_mapping_buffers() */



int ext2_sync_file(struct file *file, struct dentry *dentry, int datasync)
{
	struct inode *inode = dentry->d_inode;
	int err;
	int ret;

	ret = sync_mapping_buffers(inode->i_mapping);
	if (!(inode->i_state & I_DIRTY))
		return ret;
	if (datasync && !(inode->i_state & I_DIRTY_DATASYNC))
		return ret;

	err = ext2_sync_inode(inode);
	if (ret == 0)
		ret = err;
	return ret;
}
