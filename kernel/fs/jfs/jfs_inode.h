
#ifndef	_H_JFS_INODE
#define _H_JFS_INODE

struct fid;

extern struct inode *ialloc(struct inode *, umode_t);
extern int jfs_fsync(struct file *, struct dentry *, int);
extern long jfs_ioctl(struct file *, unsigned int, unsigned long);
extern long jfs_compat_ioctl(struct file *, unsigned int, unsigned long);
extern struct inode *jfs_iget(struct super_block *, unsigned long);
extern int jfs_commit_inode(struct inode *, int);
extern int jfs_write_inode(struct inode*, int);
extern void jfs_delete_inode(struct inode *);
extern void jfs_dirty_inode(struct inode *);
extern void jfs_truncate(struct inode *);
extern void jfs_truncate_nolock(struct inode *, loff_t);
extern void jfs_free_zero_link(struct inode *);
extern struct dentry *jfs_get_parent(struct dentry *dentry);
extern void jfs_get_inode_flags(struct jfs_inode_info *);
extern struct dentry *jfs_fh_to_dentry(struct super_block *sb, struct fid *fid,
	int fh_len, int fh_type);
extern struct dentry *jfs_fh_to_parent(struct super_block *sb, struct fid *fid,
	int fh_len, int fh_type);
extern void jfs_set_inode_flags(struct inode *);
extern int jfs_get_block(struct inode *, sector_t, struct buffer_head *, int);

extern const struct address_space_operations jfs_aops;
extern const struct inode_operations jfs_dir_inode_operations;
extern const struct file_operations jfs_dir_operations;
extern const struct inode_operations jfs_file_inode_operations;
extern const struct file_operations jfs_file_operations;
extern const struct inode_operations jfs_symlink_inode_operations;
extern struct dentry_operations jfs_ci_dentry_operations;
#endif				/* _H_JFS_INODE */
