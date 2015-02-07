
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/stat.h>
#include <asm/page.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm/rm9k-ocd.h>

#include <excite.h>

static int excite_unit_id_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%06x", unit_id);
	return 0;
}

static int excite_unit_id_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, excite_unit_id_proc_show, NULL);
}

static const struct file_operations excite_unit_id_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= excite_unit_id_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int
excite_bootrom_read(char *page, char **start, off_t off, int count,
		  int *eof, void *data)
{
	void __iomem * src;

	if (off >= EXCITE_SIZE_BOOTROM) {
		*eof = 1;
		return 0;
	}

	if ((off + count) > EXCITE_SIZE_BOOTROM)
		count = EXCITE_SIZE_BOOTROM - off;

	src = ioremap(EXCITE_PHYS_BOOTROM + off, count);
	if (src) {
		memcpy_fromio(page, src, count);
		iounmap(src);
		*start = page;
	} else {
		count = -ENOMEM;
	}

	return count;
}

void excite_procfs_init(void)
{
	/* Create & populate /proc/excite */
	struct proc_dir_entry * const pdir = proc_mkdir("excite", NULL);
	if (pdir) {
		struct proc_dir_entry * e;

		e = proc_create("unit_id", S_IRUGO, pdir,
				&excite_unit_id_proc_fops);
		if (e) e->size = 6;

		e = create_proc_read_entry("bootrom", S_IRUGO, pdir,
					   excite_bootrom_read, NULL);
		if (e) e->size = EXCITE_SIZE_BOOTROM;
	}
}
