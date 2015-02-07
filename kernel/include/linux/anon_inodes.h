

#ifndef _LINUX_ANON_INODES_H
#define _LINUX_ANON_INODES_H

int anon_inode_getfd(const char *name, const struct file_operations *fops,
		     void *priv, int flags);

#endif /* _LINUX_ANON_INODES_H */

