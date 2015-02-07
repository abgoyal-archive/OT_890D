

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/time.h>
#include <linux/pagemap.h>
#include <linux/highmem.h>
#include <linux/crc32.h>
#include <linux/jffs2.h>
#include <linux/xattr.h>
#include <linux/mtd/mtd.h>
#include <linux/security.h>
#include "nodelist.h"

/* ---- Initial Security Label Attachment -------------- */
int jffs2_init_security(struct inode *inode, struct inode *dir)
{
	int rc;
	size_t len;
	void *value;
	char *name;

	rc = security_inode_init_security(inode, dir, &name, &value, &len);
	if (rc) {
		if (rc == -EOPNOTSUPP)
			return 0;
		return rc;
	}
	rc = do_jffs2_setxattr(inode, JFFS2_XPREFIX_SECURITY, name, value, len, 0);

	kfree(name);
	kfree(value);
	return rc;
}

/* ---- XATTR Handler for "security.*" ----------------- */
static int jffs2_security_getxattr(struct inode *inode, const char *name,
				   void *buffer, size_t size)
{
	if (!strcmp(name, ""))
		return -EINVAL;

	return do_jffs2_getxattr(inode, JFFS2_XPREFIX_SECURITY, name, buffer, size);
}

static int jffs2_security_setxattr(struct inode *inode, const char *name, const void *buffer,
				   size_t size, int flags)
{
	if (!strcmp(name, ""))
		return -EINVAL;

	return do_jffs2_setxattr(inode, JFFS2_XPREFIX_SECURITY, name, buffer, size, flags);
}

static size_t jffs2_security_listxattr(struct inode *inode, char *list, size_t list_size,
				       const char *name, size_t name_len)
{
	size_t retlen = XATTR_SECURITY_PREFIX_LEN + name_len + 1;

	if (list && retlen <= list_size) {
		strcpy(list, XATTR_SECURITY_PREFIX);
		strcpy(list + XATTR_SECURITY_PREFIX_LEN, name);
	}

	return retlen;
}

struct xattr_handler jffs2_security_xattr_handler = {
	.prefix = XATTR_SECURITY_PREFIX,
	.list = jffs2_security_listxattr,
	.set = jffs2_security_setxattr,
	.get = jffs2_security_getxattr
};
