
#ifndef _LINUX_FS_NOTIFY_H
#define _LINUX_FS_NOTIFY_H


#include <linux/dnotify.h>
#include <linux/inotify.h>
#include <linux/audit.h>

static inline void fsnotify_d_instantiate(struct dentry *entry,
						struct inode *inode)
{
	inotify_d_instantiate(entry, inode);
}

static inline void fsnotify_d_move(struct dentry *entry)
{
	inotify_d_move(entry);
}

static inline void fsnotify_move(struct inode *old_dir, struct inode *new_dir,
				 const char *old_name, const char *new_name,
				 int isdir, struct inode *target, struct dentry *moved)
{
	struct inode *source = moved->d_inode;
	u32 cookie = inotify_get_cookie();

	if (old_dir == new_dir)
		inode_dir_notify(old_dir, DN_RENAME);
	else {
		inode_dir_notify(old_dir, DN_DELETE);
		inode_dir_notify(new_dir, DN_CREATE);
	}

	if (isdir)
		isdir = IN_ISDIR;
	inotify_inode_queue_event(old_dir, IN_MOVED_FROM|isdir,cookie,old_name,
				  source);
	inotify_inode_queue_event(new_dir, IN_MOVED_TO|isdir, cookie, new_name,
				  source);

	if (target) {
		inotify_inode_queue_event(target, IN_DELETE_SELF, 0, NULL, NULL);
		inotify_inode_is_dead(target);
	}

	if (source) {
		inotify_inode_queue_event(source, IN_MOVE_SELF, 0, NULL, NULL);
	}
	audit_inode_child(new_name, moved, new_dir);
}

static inline void fsnotify_nameremove(struct dentry *dentry, int isdir)
{
	if (isdir)
		isdir = IN_ISDIR;
	dnotify_parent(dentry, DN_DELETE);
	inotify_dentry_parent_queue_event(dentry, IN_DELETE|isdir, 0, dentry->d_name.name);
}

static inline void fsnotify_inoderemove(struct inode *inode)
{
	inotify_inode_queue_event(inode, IN_DELETE_SELF, 0, NULL, NULL);
	inotify_inode_is_dead(inode);
}

static inline void fsnotify_link_count(struct inode *inode)
{
	inotify_inode_queue_event(inode, IN_ATTRIB, 0, NULL, NULL);
}

static inline void fsnotify_create(struct inode *inode, struct dentry *dentry)
{
	inode_dir_notify(inode, DN_CREATE);
	inotify_inode_queue_event(inode, IN_CREATE, 0, dentry->d_name.name,
				  dentry->d_inode);
	audit_inode_child(dentry->d_name.name, dentry, inode);
}

static inline void fsnotify_link(struct inode *dir, struct inode *inode, struct dentry *new_dentry)
{
	inode_dir_notify(dir, DN_CREATE);
	inotify_inode_queue_event(dir, IN_CREATE, 0, new_dentry->d_name.name,
				  inode);
	fsnotify_link_count(inode);
	audit_inode_child(new_dentry->d_name.name, new_dentry, dir);
}

static inline void fsnotify_mkdir(struct inode *inode, struct dentry *dentry)
{
	inode_dir_notify(inode, DN_CREATE);
	inotify_inode_queue_event(inode, IN_CREATE | IN_ISDIR, 0, 
				  dentry->d_name.name, dentry->d_inode);
	audit_inode_child(dentry->d_name.name, dentry, inode);
}

static inline void fsnotify_access(struct dentry *dentry)
{
	struct inode *inode = dentry->d_inode;
	u32 mask = IN_ACCESS;

	if (S_ISDIR(inode->i_mode))
		mask |= IN_ISDIR;

	dnotify_parent(dentry, DN_ACCESS);
	inotify_dentry_parent_queue_event(dentry, mask, 0, dentry->d_name.name);
	inotify_inode_queue_event(inode, mask, 0, NULL, NULL);
}

static inline void fsnotify_modify(struct dentry *dentry)
{
	struct inode *inode = dentry->d_inode;
	u32 mask = IN_MODIFY;

	if (S_ISDIR(inode->i_mode))
		mask |= IN_ISDIR;

	dnotify_parent(dentry, DN_MODIFY);
	inotify_dentry_parent_queue_event(dentry, mask, 0, dentry->d_name.name);
	inotify_inode_queue_event(inode, mask, 0, NULL, NULL);
}

static inline void fsnotify_open(struct dentry *dentry)
{
	struct inode *inode = dentry->d_inode;
	u32 mask = IN_OPEN;

	if (S_ISDIR(inode->i_mode))
		mask |= IN_ISDIR;

	inotify_dentry_parent_queue_event(dentry, mask, 0, dentry->d_name.name);
	inotify_inode_queue_event(inode, mask, 0, NULL, NULL);
}

static inline void fsnotify_close(struct file *file)
{
	struct dentry *dentry = file->f_path.dentry;
	struct inode *inode = dentry->d_inode;
	const char *name = dentry->d_name.name;
	fmode_t mode = file->f_mode;
	u32 mask = (mode & FMODE_WRITE) ? IN_CLOSE_WRITE : IN_CLOSE_NOWRITE;

	if (S_ISDIR(inode->i_mode))
		mask |= IN_ISDIR;

	inotify_dentry_parent_queue_event(dentry, mask, 0, name);
	inotify_inode_queue_event(inode, mask, 0, NULL, NULL);
}

static inline void fsnotify_xattr(struct dentry *dentry)
{
	struct inode *inode = dentry->d_inode;
	u32 mask = IN_ATTRIB;

	if (S_ISDIR(inode->i_mode))
		mask |= IN_ISDIR;

	inotify_dentry_parent_queue_event(dentry, mask, 0, dentry->d_name.name);
	inotify_inode_queue_event(inode, mask, 0, NULL, NULL);
}

static inline void fsnotify_change(struct dentry *dentry, unsigned int ia_valid)
{
	struct inode *inode = dentry->d_inode;
	int dn_mask = 0;
	u32 in_mask = 0;

	if (ia_valid & ATTR_UID) {
		in_mask |= IN_ATTRIB;
		dn_mask |= DN_ATTRIB;
	}
	if (ia_valid & ATTR_GID) {
		in_mask |= IN_ATTRIB;
		dn_mask |= DN_ATTRIB;
	}
	if (ia_valid & ATTR_SIZE) {
		in_mask |= IN_MODIFY;
		dn_mask |= DN_MODIFY;
	}
	/* both times implies a utime(s) call */
	if ((ia_valid & (ATTR_ATIME | ATTR_MTIME)) == (ATTR_ATIME | ATTR_MTIME))
	{
		in_mask |= IN_ATTRIB;
		dn_mask |= DN_ATTRIB;
	} else if (ia_valid & ATTR_ATIME) {
		in_mask |= IN_ACCESS;
		dn_mask |= DN_ACCESS;
	} else if (ia_valid & ATTR_MTIME) {
		in_mask |= IN_MODIFY;
		dn_mask |= DN_MODIFY;
	}
	if (ia_valid & ATTR_MODE) {
		in_mask |= IN_ATTRIB;
		dn_mask |= DN_ATTRIB;
	}

	if (dn_mask)
		dnotify_parent(dentry, dn_mask);
	if (in_mask) {
		if (S_ISDIR(inode->i_mode))
			in_mask |= IN_ISDIR;
		inotify_inode_queue_event(inode, in_mask, 0, NULL, NULL);
		inotify_dentry_parent_queue_event(dentry, in_mask, 0,
						  dentry->d_name.name);
	}
}

#ifdef CONFIG_INOTIFY	/* inotify helpers */

static inline const char *fsnotify_oldname_init(const char *name)
{
	return kstrdup(name, GFP_KERNEL);
}

static inline void fsnotify_oldname_free(const char *old_name)
{
	kfree(old_name);
}

#else	/* CONFIG_INOTIFY */

static inline const char *fsnotify_oldname_init(const char *name)
{
	return NULL;
}

static inline void fsnotify_oldname_free(const char *old_name)
{
}

#endif	/* ! CONFIG_INOTIFY */

#endif	/* _LINUX_FS_NOTIFY_H */
