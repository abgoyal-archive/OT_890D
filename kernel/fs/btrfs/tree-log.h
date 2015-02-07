

#ifndef __TREE_LOG_
#define __TREE_LOG_

int btrfs_sync_log(struct btrfs_trans_handle *trans,
		   struct btrfs_root *root);
int btrfs_free_log(struct btrfs_trans_handle *trans, struct btrfs_root *root);
int btrfs_log_dentry(struct btrfs_trans_handle *trans,
		    struct btrfs_root *root, struct dentry *dentry);
int btrfs_recover_log_trees(struct btrfs_root *tree_root);
int btrfs_log_dentry_safe(struct btrfs_trans_handle *trans,
			  struct btrfs_root *root, struct dentry *dentry);
int btrfs_log_inode(struct btrfs_trans_handle *trans,
		    struct btrfs_root *root, struct inode *inode,
		    int inode_only);
int btrfs_del_dir_entries_in_log(struct btrfs_trans_handle *trans,
				 struct btrfs_root *root,
				 const char *name, int name_len,
				 struct inode *dir, u64 index);
int btrfs_del_inode_ref_in_log(struct btrfs_trans_handle *trans,
			       struct btrfs_root *root,
			       const char *name, int name_len,
			       struct inode *inode, u64 dirid);
#endif
