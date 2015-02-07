
#include <linux/kernel.h>
#include <linux/oprofile.h>
#include <linux/profile.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/fs.h>
#include "op_impl.h"

#define PM_CR_BASE	0xff000084	/* 16-bit */
#define PM_CTR_BASE	0xff100004	/* 32-bit */

#define PMCR(n)		(PM_CR_BASE + ((n) * 0x04))
#define PMCTRH(n)	(PM_CTR_BASE + 0x00 + ((n) * 0x08))
#define PMCTRL(n)	(PM_CTR_BASE + 0x04 + ((n) * 0x08))

#define PMCR_PMM_MASK	0x0000003f

#define PMCR_CLKF	0x00000100
#define PMCR_PMCLR	0x00002000
#define PMCR_PMST	0x00004000
#define PMCR_PMEN	0x00008000

struct op_sh_model op_model_sh7750_ops;

#define NR_CNTRS	2

static struct sh7750_ppc_register_config {
	unsigned int ctrl;
	unsigned long cnt_hi;
	unsigned long cnt_lo;
} regcache[NR_CNTRS];


static int sh7750_timer_notify(struct pt_regs *regs)
{
	oprofile_add_sample(regs, 0);
	return 0;
}

static u64 sh7750_read_counter(int counter)
{
	return (u64)((u64)(__raw_readl(PMCTRH(counter)) & 0xffff) << 32) |
			   __raw_readl(PMCTRL(counter));
}

static inline int to_counter(struct file *file)
{
	const unsigned char *name = file->f_path.dentry->d_parent->d_name.name;

	return (int)simple_strtol(name, NULL, 10);
}

static ssize_t sh7750_read_count(struct file *file, char __user *buf,
				 size_t count, loff_t *ppos)
{
	int counter = to_counter(file);
	u64 val = sh7750_read_counter(counter);

	return oprofilefs_ulong_to_user((unsigned long)val, buf, count, ppos);
}

static ssize_t sh7750_write_count(struct file *file, const char __user *buf,
				  size_t count, loff_t *ppos)
{
	int counter = to_counter(file);
	unsigned long val;

	if (oprofilefs_ulong_from_user(&val, buf, count))
		return -EFAULT;

	/*
	 * Any write will clear the counter, although only 0 should be
	 * written for this purpose, as we do not support setting the
	 * counter to an arbitrary value.
	 */
	WARN_ON(val != 0);

	__raw_writew(__raw_readw(PMCR(counter)) | PMCR_PMCLR, PMCR(counter));

	return count;
}

static const struct file_operations count_fops = {
	.read		= sh7750_read_count,
	.write		= sh7750_write_count,
};

static int sh7750_ppc_create_files(struct super_block *sb, struct dentry *dir)
{
	return oprofilefs_create_file(sb, dir, "count", &count_fops);
}

static void sh7750_ppc_reg_setup(struct op_counter_config *ctr)
{
	unsigned int counters = op_model_sh7750_ops.num_counters;
	int i;

	for (i = 0; i < counters; i++) {
		regcache[i].ctrl	= 0;
		regcache[i].cnt_hi	= 0;
		regcache[i].cnt_lo	= 0;

		if (!ctr[i].enabled)
			continue;

		regcache[i].ctrl |= ctr[i].event | PMCR_PMEN | PMCR_PMST;
		regcache[i].cnt_hi = (unsigned long)((ctr->count >> 32) & 0xffff);
		regcache[i].cnt_lo = (unsigned long)(ctr->count & 0xffffffff);
	}
}

static void sh7750_ppc_cpu_setup(void *args)
{
	unsigned int counters = op_model_sh7750_ops.num_counters;
	int i;

	for (i = 0; i < counters; i++) {
		__raw_writew(0, PMCR(i));
		__raw_writel(regcache[i].cnt_hi, PMCTRH(i));
		__raw_writel(regcache[i].cnt_lo, PMCTRL(i));
	}
}

static void sh7750_ppc_cpu_start(void *args)
{
	unsigned int counters = op_model_sh7750_ops.num_counters;
	int i;

	for (i = 0; i < counters; i++)
		__raw_writew(regcache[i].ctrl, PMCR(i));
}

static void sh7750_ppc_cpu_stop(void *args)
{
	unsigned int counters = op_model_sh7750_ops.num_counters;
	int i;

	/* Disable the counters */
	for (i = 0; i < counters; i++)
		__raw_writew(__raw_readw(PMCR(i)) & ~PMCR_PMEN, PMCR(i));
}

static inline void sh7750_ppc_reset(void)
{
	unsigned int counters = op_model_sh7750_ops.num_counters;
	int i;

	/* Clear the counters */
	for (i = 0; i < counters; i++)
		__raw_writew(__raw_readw(PMCR(i)) | PMCR_PMCLR, PMCR(i));
}

static int sh7750_ppc_init(void)
{
	sh7750_ppc_reset();

	return register_timer_hook(sh7750_timer_notify);
}

static void sh7750_ppc_exit(void)
{
	unregister_timer_hook(sh7750_timer_notify);

	sh7750_ppc_reset();
}

struct op_sh_model op_model_sh7750_ops = {
	.cpu_type	= "sh/sh7750",
	.num_counters	= NR_CNTRS,
	.reg_setup	= sh7750_ppc_reg_setup,
	.cpu_setup	= sh7750_ppc_cpu_setup,
	.cpu_start	= sh7750_ppc_cpu_start,
	.cpu_stop	= sh7750_ppc_cpu_stop,
	.init		= sh7750_ppc_init,
	.exit		= sh7750_ppc_exit,
	.create_files	= sh7750_ppc_create_files,
};
