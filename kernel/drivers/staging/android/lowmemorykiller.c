

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/oom.h>
#include <linux/sched.h>

static int lowmem_shrink(int nr_to_scan, gfp_t gfp_mask);

static struct shrinker lowmem_shrinker = {
	.shrink = lowmem_shrink,
	.seeks = DEFAULT_SEEKS * 16
};
static uint32_t lowmem_debug_level = 2;
static int lowmem_adj[6] = {
	0,
	1,
	6,
	12,
};
static int lowmem_adj_size = 4;
static size_t lowmem_minfree[6] = {
	3*512, // 6MB
	2*1024, // 8MB
	4*1024, // 16MB
	16*1024, // 64MB
};
static int lowmem_minfree_size = 4;

#define lowmem_print(level, x...) do { if(lowmem_debug_level >= (level)) printk(x); } while(0)

module_param_named(cost, lowmem_shrinker.seeks, int, S_IRUGO | S_IWUSR);
module_param_array_named(adj, lowmem_adj, int, &lowmem_adj_size, S_IRUGO | S_IWUSR);
module_param_array_named(minfree, lowmem_minfree, uint, &lowmem_minfree_size, S_IRUGO | S_IWUSR);
module_param_named(debug_level, lowmem_debug_level, uint, S_IRUGO | S_IWUSR);

static int lowmem_shrink(int nr_to_scan, gfp_t gfp_mask)
{
	struct task_struct *p;
	struct task_struct *selected = NULL;
	int rem = 0;
	int tasksize;
	int i;
	int min_adj = OOM_ADJUST_MAX + 1;
	int selected_tasksize = 0;
	int array_size = ARRAY_SIZE(lowmem_adj);
	int other_free = global_page_state(NR_FREE_PAGES);
	int other_file = global_page_state(NR_FILE_PAGES);
#ifdef HAVE_1G_DRAM
	unsigned long long ns;
	unsigned int usec_rem, secs;
	unsigned long total_mem, cache_mem, buffer_mem, free_mem;
	struct sysinfo ii;
#endif
	if(lowmem_adj_size < array_size)
		array_size = lowmem_adj_size;
	if(lowmem_minfree_size < array_size)
		array_size = lowmem_minfree_size;
	for(i = 0; i < array_size; i++) {
		if (other_free < lowmem_minfree[i] &&
		    other_file < lowmem_minfree[i]) {
			min_adj = lowmem_adj[i];
			break;
		}
	}
	if(nr_to_scan > 0)
		lowmem_print(3, "lowmem_shrink %d, %x, ofree %d %d, ma %d\n", nr_to_scan, gfp_mask, other_free, other_file, min_adj);
	rem = global_page_state(NR_ACTIVE_ANON) +
		global_page_state(NR_ACTIVE_FILE) +
		global_page_state(NR_INACTIVE_ANON) +
		global_page_state(NR_INACTIVE_FILE);
	if (nr_to_scan <= 0 || min_adj == OOM_ADJUST_MAX + 1) {
		lowmem_print(5, "lowmem_shrink %d, %x, return %d\n", nr_to_scan, gfp_mask, rem);
		return rem;
	}

	read_lock(&tasklist_lock);
	for_each_process(p) {
		if (p->oomkilladj < min_adj || !p->mm)
			continue;
		tasksize = get_mm_rss(p->mm);
		if (tasksize <= 0)
			continue;
		if (selected) {
			if (p->oomkilladj < selected->oomkilladj)
				continue;
			if (p->oomkilladj == selected->oomkilladj &&
			    tasksize <= selected_tasksize)
				continue;
		}
		selected = p;
		selected_tasksize = tasksize;
		lowmem_print(2, "select %d (%s), adj %d, size %d, to kill\n",
		             p->pid, p->comm, p->oomkilladj, tasksize);
	}
	if(selected != NULL) {
#ifndef HAVE_1G_DRAM
		lowmem_print(1, "send sigkill to %d (%s), adj %d, size %d\n",
		             selected->pid, selected->comm,
		             selected->oomkilladj, selected_tasksize);
#else
		ns = sched_clock() + 500;
		do_div(ns, 1000); //ns now is usec
		usec_rem = do_div(ns, 1000000ULL);
		secs = (unsigned long)ns;

		#define K(x) ((x) << (PAGE_SHIFT - 10))
		printk("[MEMPROF][%05lu.%-6lu]:sigkill:%d(%s):adj:%d:size:%d\n",
			     secs, usec_rem,
		             selected->pid, selected->comm,
		             selected->oomkilladj, K(selected_tasksize));

		#define total_swapcache_pages 0UL
		si_meminfo(&ii);
		cache_mem = global_page_state(NR_FILE_PAGES) - total_swapcache_pages - ii.bufferram;
		total_mem = K(ii.totalram);
		free_mem = K(ii.freeram);
		buffer_mem = K(ii.bufferram);
		cache_mem = K(cache_mem);
		printk("[MEMPROF][%05lu.%-6lu]:Used(kB):%lu:Cached:%lu:Buffer:%lu:Free:%lu\n", 
			secs, usec_rem, total_mem-free_mem-buffer_mem-cache_mem, 
			cache_mem, buffer_mem, free_mem);
#endif
		force_sig(SIGKILL, selected);
		rem -= selected_tasksize;
	}
	lowmem_print(4, "lowmem_shrink %d, %x, return %d\n", nr_to_scan, gfp_mask, rem);
	read_unlock(&tasklist_lock);
	return rem;
}

static int __init lowmem_init(void)
{
	register_shrinker(&lowmem_shrinker);
	return 0;
}

static void __exit lowmem_exit(void)
{
	unregister_shrinker(&lowmem_shrinker);
}

module_init(lowmem_init);
module_exit(lowmem_exit);

MODULE_LICENSE("GPL");

