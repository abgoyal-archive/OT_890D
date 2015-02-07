



extern struct file_system_type v9fs_fs_type;
extern const struct address_space_operations v9fs_addr_operations;
extern const struct file_operations v9fs_file_operations;
extern const struct file_operations v9fs_dir_operations;
extern struct dentry_operations v9fs_dentry_operations;
extern struct dentry_operations v9fs_cached_dentry_operations;

struct inode *v9fs_get_inode(struct super_block *sb, int mode);
ino_t v9fs_qid2ino(struct p9_qid *qid);
void v9fs_stat2inode(struct p9_wstat *, struct inode *, struct super_block *);
int v9fs_dir_release(struct inode *inode, struct file *filp);
int v9fs_file_open(struct inode *inode, struct file *file);
void v9fs_inode2stat(struct inode *inode, struct p9_wstat *stat);
void v9fs_dentry_release(struct dentry *);
int v9fs_uflags2omode(int uflags, int extended);

ssize_t v9fs_file_readn(struct file *, char *, char __user *, u32, u64);
