

#include "udfdecl.h"

#include <linux/fs.h>

static int udf_fsync_inode(struct inode *, int);


int udf_fsync_file(struct file *file, struct dentry *dentry, int datasync)
{
	struct inode *inode = dentry->d_inode;

	return udf_fsync_inode(inode, datasync);
}

static int udf_fsync_inode(struct inode *inode, int datasync)
{
	int err;

	err = sync_mapping_buffers(inode->i_mapping);
	if (!(inode->i_state & I_DIRTY))
		return err;
	if (datasync && !(inode->i_state & I_DIRTY_DATASYNC))
		return err;

	err |= udf_sync_inode(inode);

	return err ? -EIO : 0;
}
