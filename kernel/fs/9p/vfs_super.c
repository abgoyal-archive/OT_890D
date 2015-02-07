

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/inet.h>
#include <linux/pagemap.h>
#include <linux/seq_file.h>
#include <linux/mount.h>
#include <linux/idr.h>
#include <linux/sched.h>
#include <net/9p/9p.h>
#include <net/9p/client.h>

#include "v9fs.h"
#include "v9fs_vfs.h"
#include "fid.h"

static void v9fs_clear_inode(struct inode *);
static const struct super_operations v9fs_super_ops;


static void v9fs_clear_inode(struct inode *inode)
{
	filemap_fdatawrite(inode->i_mapping);
}


static int v9fs_set_super(struct super_block *s, void *data)
{
	s->s_fs_info = data;
	return set_anon_super(s, data);
}


static void
v9fs_fill_super(struct super_block *sb, struct v9fs_session_info *v9ses,
		int flags)
{
	sb->s_maxbytes = MAX_LFS_FILESIZE;
	sb->s_blocksize_bits = fls(v9ses->maxdata - 1);
	sb->s_blocksize = 1 << sb->s_blocksize_bits;
	sb->s_magic = V9FS_MAGIC;
	sb->s_op = &v9fs_super_ops;

	sb->s_flags = flags | MS_ACTIVE | MS_SYNCHRONOUS | MS_DIRSYNC |
	    MS_NOATIME;
}


static int v9fs_get_sb(struct file_system_type *fs_type, int flags,
		       const char *dev_name, void *data,
		       struct vfsmount *mnt)
{
	struct super_block *sb = NULL;
	struct inode *inode = NULL;
	struct dentry *root = NULL;
	struct v9fs_session_info *v9ses = NULL;
	struct p9_wstat *st = NULL;
	int mode = S_IRWXUGO | S_ISVTX;
	uid_t uid = current_fsuid();
	gid_t gid = current_fsgid();
	struct p9_fid *fid;
	int retval = 0;

	P9_DPRINTK(P9_DEBUG_VFS, " \n");

	st = NULL;
	v9ses = kzalloc(sizeof(struct v9fs_session_info), GFP_KERNEL);
	if (!v9ses)
		return -ENOMEM;

	fid = v9fs_session_init(v9ses, dev_name, data);
	if (IS_ERR(fid)) {
		retval = PTR_ERR(fid);
		goto close_session;
	}

	st = p9_client_stat(fid);
	if (IS_ERR(st)) {
		retval = PTR_ERR(st);
		goto clunk_fid;
	}

	sb = sget(fs_type, NULL, v9fs_set_super, v9ses);
	if (IS_ERR(sb)) {
		retval = PTR_ERR(sb);
		goto free_stat;
	}
	v9fs_fill_super(sb, v9ses, flags);

	inode = v9fs_get_inode(sb, S_IFDIR | mode);
	if (IS_ERR(inode)) {
		retval = PTR_ERR(inode);
		goto release_sb;
	}

	inode->i_uid = uid;
	inode->i_gid = gid;

	root = d_alloc_root(inode);
	if (!root) {
		retval = -ENOMEM;
		goto release_sb;
	}

	sb->s_root = root;
	root->d_inode->i_ino = v9fs_qid2ino(&st->qid);

	v9fs_stat2inode(st, root->d_inode, sb);

	v9fs_fid_add(root, fid);
	p9stat_free(st);
	kfree(st);

P9_DPRINTK(P9_DEBUG_VFS, " return simple set mount\n");
	return simple_set_mnt(mnt, sb);

release_sb:
	if (sb) {
		up_write(&sb->s_umount);
		deactivate_super(sb);
	}

free_stat:
	kfree(st);

clunk_fid:
	p9_client_clunk(fid);

close_session:
	v9fs_session_close(v9ses);
	kfree(v9ses);

	return retval;
}


static void v9fs_kill_super(struct super_block *s)
{
	struct v9fs_session_info *v9ses = s->s_fs_info;

	P9_DPRINTK(P9_DEBUG_VFS, " %p\n", s);

	v9fs_dentry_release(s->s_root);	/* clunk root */

	kill_anon_super(s);

	v9fs_session_close(v9ses);
	kfree(v9ses);
	P9_DPRINTK(P9_DEBUG_VFS, "exiting kill_super\n");
}


static int v9fs_show_options(struct seq_file *m, struct vfsmount *mnt)
{
	struct v9fs_session_info *v9ses = mnt->mnt_sb->s_fs_info;

	seq_printf(m, "%s", v9ses->options);
	return 0;
}

static void
v9fs_umount_begin(struct super_block *sb)
{
	struct v9fs_session_info *v9ses = sb->s_fs_info;

	v9fs_session_cancel(v9ses);
}

static const struct super_operations v9fs_super_ops = {
	.statfs = simple_statfs,
	.clear_inode = v9fs_clear_inode,
	.show_options = v9fs_show_options,
	.umount_begin = v9fs_umount_begin,
};

struct file_system_type v9fs_fs_type = {
	.name = "9p",
	.get_sb = v9fs_get_sb,
	.kill_sb = v9fs_kill_super,
	.owner = THIS_MODULE,
};
