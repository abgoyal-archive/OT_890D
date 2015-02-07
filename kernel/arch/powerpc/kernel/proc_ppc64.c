

#include <linux/init.h>
#include <linux/mm.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/kernel.h>

#include <asm/machdep.h>
#include <asm/vdso_datapage.h>
#include <asm/rtas.h>
#include <asm/uaccess.h>
#include <asm/prom.h>

static loff_t  page_map_seek( struct file *file, loff_t off, int whence);
static ssize_t page_map_read( struct file *file, char __user *buf, size_t nbytes,
			      loff_t *ppos);
static int     page_map_mmap( struct file *file, struct vm_area_struct *vma );

static const struct file_operations page_map_fops = {
	.llseek	= page_map_seek,
	.read	= page_map_read,
	.mmap	= page_map_mmap
};

static int __init proc_ppc64_create(void)
{
	struct proc_dir_entry *root;

	root = proc_mkdir("ppc64", NULL);
	if (!root)
		return 1;

	if (!of_find_node_by_path("/rtas"))
		return 0;

	if (!proc_mkdir("rtas", root))
		return 1;

	if (!proc_symlink("rtas", NULL, "ppc64/rtas"))
		return 1;

	return 0;
}
core_initcall(proc_ppc64_create);

static int __init proc_ppc64_init(void)
{
	struct proc_dir_entry *pde;

	pde = proc_create_data("ppc64/systemcfg", S_IFREG|S_IRUGO, NULL,
			       &page_map_fops, vdso_data);
	if (!pde)
		return 1;
	pde->size = PAGE_SIZE;

	return 0;
}
__initcall(proc_ppc64_init);

static loff_t page_map_seek( struct file *file, loff_t off, int whence)
{
	loff_t new;
	struct proc_dir_entry *dp = PDE(file->f_path.dentry->d_inode);

	switch(whence) {
	case 0:
		new = off;
		break;
	case 1:
		new = file->f_pos + off;
		break;
	case 2:
		new = dp->size + off;
		break;
	default:
		return -EINVAL;
	}
	if ( new < 0 || new > dp->size )
		return -EINVAL;
	return (file->f_pos = new);
}

static ssize_t page_map_read( struct file *file, char __user *buf, size_t nbytes,
			      loff_t *ppos)
{
	struct proc_dir_entry *dp = PDE(file->f_path.dentry->d_inode);
	return simple_read_from_buffer(buf, nbytes, ppos, dp->data, dp->size);
}

static int page_map_mmap( struct file *file, struct vm_area_struct *vma )
{
	struct proc_dir_entry *dp = PDE(file->f_path.dentry->d_inode);

	if ((vma->vm_end - vma->vm_start) > dp->size)
		return -EINVAL;

	remap_pfn_range(vma, vma->vm_start, __pa(dp->data) >> PAGE_SHIFT,
						dp->size, vma->vm_page_prot);
	return 0;
}

