

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/inet.h>
#include <linux/idr.h>
#include <net/9p/9p.h>
#include <net/9p/client.h>

#include "v9fs.h"
#include "v9fs_vfs.h"
#include "fid.h"


static inline int dt_type(struct p9_wstat *mistat)
{
	unsigned long perm = mistat->mode;
	int rettype = DT_REG;

	if (perm & P9_DMDIR)
		rettype = DT_DIR;
	if (perm & P9_DMSYMLINK)
		rettype = DT_LNK;

	return rettype;
}


static int v9fs_dir_readdir(struct file *filp, void *dirent, filldir_t filldir)
{
	int over;
	struct p9_wstat st;
	int err;
	struct p9_fid *fid;
	int buflen;
	char *statbuf;
	int n, i = 0;

	P9_DPRINTK(P9_DEBUG_VFS, "name %s\n", filp->f_path.dentry->d_name.name);
	fid = filp->private_data;

	buflen = fid->clnt->msize - P9_IOHDRSZ;
	statbuf = kmalloc(buflen, GFP_KERNEL);
	if (!statbuf)
		return -ENOMEM;

	while (1) {
		err = v9fs_file_readn(filp, statbuf, NULL, buflen,
								fid->rdir_fpos);
		if (err <= 0)
			break;

		n = err;
		while (i < n) {
			err = p9stat_read(statbuf + i, buflen-i, &st,
							fid->clnt->dotu);
			if (err) {
				P9_DPRINTK(P9_DEBUG_VFS, "returned %d\n", err);
				err = -EIO;
				p9stat_free(&st);
				goto free_and_exit;
			}

			i += st.size+2;
			fid->rdir_fpos += st.size+2;

			over = filldir(dirent, st.name, strlen(st.name),
			    filp->f_pos, v9fs_qid2ino(&st.qid), dt_type(&st));

			filp->f_pos += st.size+2;

			p9stat_free(&st);

			if (over) {
				err = 0;
				goto free_and_exit;
			}
		}
	}

free_and_exit:
	kfree(statbuf);
	return err;
}



int v9fs_dir_release(struct inode *inode, struct file *filp)
{
	struct p9_fid *fid;

	fid = filp->private_data;
	P9_DPRINTK(P9_DEBUG_VFS,
			"inode: %p filp: %p fid: %d\n", inode, filp, fid->fid);
	filemap_write_and_wait(inode->i_mapping);
	p9_client_clunk(fid);
	return 0;
}

const struct file_operations v9fs_dir_operations = {
	.read = generic_read_dir,
	.llseek = generic_file_llseek,
	.readdir = v9fs_dir_readdir,
	.open = v9fs_file_open,
	.release = v9fs_dir_release,
};
