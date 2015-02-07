

struct super_block;
struct linux_binprm;

#ifdef CONFIG_BLOCK
extern struct super_block *blockdev_superblock;
extern void __init bdev_cache_init(void);

static inline int sb_is_blkdev_sb(struct super_block *sb)
{
	return sb == blockdev_superblock;
}

#else
static inline void bdev_cache_init(void)
{
}

static inline int sb_is_blkdev_sb(struct super_block *sb)
{
	return 0;
}
#endif

extern void __init chrdev_init(void);

extern void check_unsafe_exec(struct linux_binprm *, struct files_struct *);

extern int copy_mount_options(const void __user *, unsigned long *);

extern void free_vfsmnt(struct vfsmount *);
extern struct vfsmount *alloc_vfsmnt(const char *);
extern struct vfsmount *__lookup_mnt(struct vfsmount *, struct dentry *, int);
extern void mnt_set_mountpoint(struct vfsmount *, struct dentry *,
				struct vfsmount *);
extern void release_mounts(struct list_head *);
extern void umount_tree(struct vfsmount *, int, struct list_head *);
extern struct vfsmount *copy_tree(struct vfsmount *, struct dentry *, int);

extern void __init mnt_init(void);
