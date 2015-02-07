

#include "hfs_fs.h"
#include "btree.h"

void hfs_cat_build_key(struct super_block *sb, btree_key *key, u32 parent, struct qstr *name)
{
	key->cat.reserved = 0;
	key->cat.ParID = cpu_to_be32(parent);
	if (name) {
		hfs_asc2mac(sb, &key->cat.CName, name);
		key->key_len = 6 + key->cat.CName.len;
	} else {
		memset(&key->cat.CName, 0, sizeof(struct hfs_name));
		key->key_len = 6;
	}
}

static int hfs_cat_build_record(hfs_cat_rec *rec, u32 cnid, struct inode *inode)
{
	__be32 mtime = hfs_mtime();

	memset(rec, 0, sizeof(*rec));
	if (S_ISDIR(inode->i_mode)) {
		rec->type = HFS_CDR_DIR;
		rec->dir.DirID = cpu_to_be32(cnid);
		rec->dir.CrDat = mtime;
		rec->dir.MdDat = mtime;
		rec->dir.BkDat = 0;
		rec->dir.UsrInfo.frView = cpu_to_be16(0xff);
		return sizeof(struct hfs_cat_dir);
	} else {
		/* init some fields for the file record */
		rec->type = HFS_CDR_FIL;
		rec->file.Flags = HFS_FIL_USED | HFS_FIL_THD;
		if (!(inode->i_mode & S_IWUSR))
			rec->file.Flags |= HFS_FIL_LOCK;
		rec->file.FlNum = cpu_to_be32(cnid);
		rec->file.CrDat = mtime;
		rec->file.MdDat = mtime;
		rec->file.BkDat = 0;
		rec->file.UsrWds.fdType = HFS_SB(inode->i_sb)->s_type;
		rec->file.UsrWds.fdCreator = HFS_SB(inode->i_sb)->s_creator;
		return sizeof(struct hfs_cat_file);
	}
}

static int hfs_cat_build_thread(struct super_block *sb,
				hfs_cat_rec *rec, int type,
				u32 parentid, struct qstr *name)
{
	rec->type = type;
	memset(rec->thread.reserved, 0, sizeof(rec->thread.reserved));
	rec->thread.ParID = cpu_to_be32(parentid);
	hfs_asc2mac(sb, &rec->thread.CName, name);
	return sizeof(struct hfs_cat_thread);
}

int hfs_cat_create(u32 cnid, struct inode *dir, struct qstr *str, struct inode *inode)
{
	struct hfs_find_data fd;
	struct super_block *sb;
	union hfs_cat_rec entry;
	int entry_size;
	int err;

	dprint(DBG_CAT_MOD, "create_cat: %s,%u(%d)\n", str->name, cnid, inode->i_nlink);
	if (dir->i_size >= HFS_MAX_VALENCE)
		return -ENOSPC;

	sb = dir->i_sb;
	hfs_find_init(HFS_SB(sb)->cat_tree, &fd);

	hfs_cat_build_key(sb, fd.search_key, cnid, NULL);
	entry_size = hfs_cat_build_thread(sb, &entry, S_ISDIR(inode->i_mode) ?
			HFS_CDR_THD : HFS_CDR_FTH,
			dir->i_ino, str);
	err = hfs_brec_find(&fd);
	if (err != -ENOENT) {
		if (!err)
			err = -EEXIST;
		goto err2;
	}
	err = hfs_brec_insert(&fd, &entry, entry_size);
	if (err)
		goto err2;

	hfs_cat_build_key(sb, fd.search_key, dir->i_ino, str);
	entry_size = hfs_cat_build_record(&entry, cnid, inode);
	err = hfs_brec_find(&fd);
	if (err != -ENOENT) {
		/* panic? */
		if (!err)
			err = -EEXIST;
		goto err1;
	}
	err = hfs_brec_insert(&fd, &entry, entry_size);
	if (err)
		goto err1;

	dir->i_size++;
	dir->i_mtime = dir->i_ctime = CURRENT_TIME_SEC;
	mark_inode_dirty(dir);
	hfs_find_exit(&fd);
	return 0;

err1:
	hfs_cat_build_key(sb, fd.search_key, cnid, NULL);
	if (!hfs_brec_find(&fd))
		hfs_brec_remove(&fd);
err2:
	hfs_find_exit(&fd);
	return err;
}

int hfs_cat_keycmp(const btree_key *key1, const btree_key *key2)
{
	int retval;

	retval = be32_to_cpu(key1->cat.ParID) - be32_to_cpu(key2->cat.ParID);
	if (!retval)
		retval = hfs_strcmp(key1->cat.CName.name, key1->cat.CName.len,
				    key2->cat.CName.name, key2->cat.CName.len);

	return retval;
}

/* Try to get a catalog entry for given catalog id */
// move to read_super???
int hfs_cat_find_brec(struct super_block *sb, u32 cnid,
		      struct hfs_find_data *fd)
{
	hfs_cat_rec rec;
	int res, len, type;

	hfs_cat_build_key(sb, fd->search_key, cnid, NULL);
	res = hfs_brec_read(fd, &rec, sizeof(rec));
	if (res)
		return res;

	type = rec.type;
	if (type != HFS_CDR_THD && type != HFS_CDR_FTH) {
		printk(KERN_ERR "hfs: found bad thread record in catalog\n");
		return -EIO;
	}

	fd->search_key->cat.ParID = rec.thread.ParID;
	len = fd->search_key->cat.CName.len = rec.thread.CName.len;
	if (len > HFS_NAMELEN) {
		printk(KERN_ERR "hfs: bad catalog namelength\n");
		return -EIO;
	}
	memcpy(fd->search_key->cat.CName.name, rec.thread.CName.name, len);
	return hfs_brec_find(fd);
}


int hfs_cat_delete(u32 cnid, struct inode *dir, struct qstr *str)
{
	struct super_block *sb;
	struct hfs_find_data fd;
	struct list_head *pos;
	int res, type;

	dprint(DBG_CAT_MOD, "delete_cat: %s,%u\n", str ? str->name : NULL, cnid);
	sb = dir->i_sb;
	hfs_find_init(HFS_SB(sb)->cat_tree, &fd);

	hfs_cat_build_key(sb, fd.search_key, dir->i_ino, str);
	res = hfs_brec_find(&fd);
	if (res)
		goto out;

	type = hfs_bnode_read_u8(fd.bnode, fd.entryoffset);
	if (type == HFS_CDR_FIL) {
		struct hfs_cat_file file;
		hfs_bnode_read(fd.bnode, &file, fd.entryoffset, sizeof(file));
		if (be32_to_cpu(file.FlNum) == cnid) {
#if 0
			hfs_free_fork(sb, &file, HFS_FK_DATA);
#endif
			hfs_free_fork(sb, &file, HFS_FK_RSRC);
		}
	}

	list_for_each(pos, &HFS_I(dir)->open_dir_list) {
		struct hfs_readdir_data *rd =
			list_entry(pos, struct hfs_readdir_data, list);
		if (fd.tree->keycmp(fd.search_key, (void *)&rd->key) < 0)
			rd->file->f_pos--;
	}

	res = hfs_brec_remove(&fd);
	if (res)
		goto out;

	hfs_cat_build_key(sb, fd.search_key, cnid, NULL);
	res = hfs_brec_find(&fd);
	if (!res) {
		res = hfs_brec_remove(&fd);
		if (res)
			goto out;
	}

	dir->i_size--;
	dir->i_mtime = dir->i_ctime = CURRENT_TIME_SEC;
	mark_inode_dirty(dir);
	res = 0;
out:
	hfs_find_exit(&fd);

	return res;
}

int hfs_cat_move(u32 cnid, struct inode *src_dir, struct qstr *src_name,
		 struct inode *dst_dir, struct qstr *dst_name)
{
	struct super_block *sb;
	struct hfs_find_data src_fd, dst_fd;
	union hfs_cat_rec entry;
	int entry_size, type;
	int err;

	dprint(DBG_CAT_MOD, "rename_cat: %u - %lu,%s - %lu,%s\n", cnid, src_dir->i_ino, src_name->name,
		dst_dir->i_ino, dst_name->name);
	sb = src_dir->i_sb;
	hfs_find_init(HFS_SB(sb)->cat_tree, &src_fd);
	dst_fd = src_fd;

	/* find the old dir entry and read the data */
	hfs_cat_build_key(sb, src_fd.search_key, src_dir->i_ino, src_name);
	err = hfs_brec_find(&src_fd);
	if (err)
		goto out;

	hfs_bnode_read(src_fd.bnode, &entry, src_fd.entryoffset,
			    src_fd.entrylength);

	/* create new dir entry with the data from the old entry */
	hfs_cat_build_key(sb, dst_fd.search_key, dst_dir->i_ino, dst_name);
	err = hfs_brec_find(&dst_fd);
	if (err != -ENOENT) {
		if (!err)
			err = -EEXIST;
		goto out;
	}

	err = hfs_brec_insert(&dst_fd, &entry, src_fd.entrylength);
	if (err)
		goto out;
	dst_dir->i_size++;
	dst_dir->i_mtime = dst_dir->i_ctime = CURRENT_TIME_SEC;
	mark_inode_dirty(dst_dir);

	/* finally remove the old entry */
	hfs_cat_build_key(sb, src_fd.search_key, src_dir->i_ino, src_name);
	err = hfs_brec_find(&src_fd);
	if (err)
		goto out;
	err = hfs_brec_remove(&src_fd);
	if (err)
		goto out;
	src_dir->i_size--;
	src_dir->i_mtime = src_dir->i_ctime = CURRENT_TIME_SEC;
	mark_inode_dirty(src_dir);

	type = entry.type;
	if (type == HFS_CDR_FIL && !(entry.file.Flags & HFS_FIL_THD))
		goto out;

	/* remove old thread entry */
	hfs_cat_build_key(sb, src_fd.search_key, cnid, NULL);
	err = hfs_brec_find(&src_fd);
	if (err)
		goto out;
	err = hfs_brec_remove(&src_fd);
	if (err)
		goto out;

	/* create new thread entry */
	hfs_cat_build_key(sb, dst_fd.search_key, cnid, NULL);
	entry_size = hfs_cat_build_thread(sb, &entry, type == HFS_CDR_FIL ? HFS_CDR_FTH : HFS_CDR_THD,
					dst_dir->i_ino, dst_name);
	err = hfs_brec_find(&dst_fd);
	if (err != -ENOENT) {
		if (!err)
			err = -EEXIST;
		goto out;
	}
	err = hfs_brec_insert(&dst_fd, &entry, entry_size);
out:
	hfs_bnode_put(dst_fd.bnode);
	hfs_find_exit(&src_fd);
	return err;
}
