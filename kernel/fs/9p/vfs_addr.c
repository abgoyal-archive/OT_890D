

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/inet.h>
#include <linux/pagemap.h>
#include <linux/idr.h>
#include <linux/sched.h>
#include <net/9p/9p.h>
#include <net/9p/client.h>

#include "v9fs.h"
#include "v9fs_vfs.h"


static int v9fs_vfs_readpage(struct file *filp, struct page *page)
{
	int retval;
	loff_t offset;
	char *buffer;

	P9_DPRINTK(P9_DEBUG_VFS, "\n");
	buffer = kmap(page);
	offset = page_offset(page);

	retval = v9fs_file_readn(filp, buffer, NULL, offset, PAGE_CACHE_SIZE);
	if (retval < 0)
		goto done;

	memset(buffer + retval, 0, PAGE_CACHE_SIZE - retval);
	flush_dcache_page(page);
	SetPageUptodate(page);
	retval = 0;

done:
	kunmap(page);
	unlock_page(page);
	return retval;
}

const struct address_space_operations v9fs_addr_operations = {
      .readpage = v9fs_vfs_readpage,
};
