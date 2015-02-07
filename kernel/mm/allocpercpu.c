
#include <linux/mm.h>
#include <linux/module.h>

#ifndef cache_line_size
#define cache_line_size()	L1_CACHE_BYTES
#endif

static void percpu_depopulate(void *__pdata, int cpu)
{
	struct percpu_data *pdata = __percpu_disguise(__pdata);

	kfree(pdata->ptrs[cpu]);
	pdata->ptrs[cpu] = NULL;
}

static void __percpu_depopulate_mask(void *__pdata, cpumask_t *mask)
{
	int cpu;
	for_each_cpu_mask_nr(cpu, *mask)
		percpu_depopulate(__pdata, cpu);
}

#define percpu_depopulate_mask(__pdata, mask) \
	__percpu_depopulate_mask((__pdata), &(mask))

static void *percpu_populate(void *__pdata, size_t size, gfp_t gfp, int cpu)
{
	struct percpu_data *pdata = __percpu_disguise(__pdata);
	int node = cpu_to_node(cpu);

	/*
	 * We should make sure each CPU gets private memory.
	 */
	size = roundup(size, cache_line_size());

	BUG_ON(pdata->ptrs[cpu]);
	if (node_online(node))
		pdata->ptrs[cpu] = kmalloc_node(size, gfp|__GFP_ZERO, node);
	else
		pdata->ptrs[cpu] = kzalloc(size, gfp);
	return pdata->ptrs[cpu];
}

static int __percpu_populate_mask(void *__pdata, size_t size, gfp_t gfp,
				  cpumask_t *mask)
{
	cpumask_t populated;
	int cpu;

	cpus_clear(populated);
	for_each_cpu_mask_nr(cpu, *mask)
		if (unlikely(!percpu_populate(__pdata, size, gfp, cpu))) {
			__percpu_depopulate_mask(__pdata, &populated);
			return -ENOMEM;
		} else
			cpu_set(cpu, populated);
	return 0;
}

#define percpu_populate_mask(__pdata, size, gfp, mask) \
	__percpu_populate_mask((__pdata), (size), (gfp), &(mask))

void *__percpu_alloc_mask(size_t size, gfp_t gfp, cpumask_t *mask)
{
	/*
	 * We allocate whole cache lines to avoid false sharing
	 */
	size_t sz = roundup(nr_cpu_ids * sizeof(void *), cache_line_size());
	void *pdata = kzalloc(sz, gfp);
	void *__pdata = __percpu_disguise(pdata);

	if (unlikely(!pdata))
		return NULL;
	if (likely(!__percpu_populate_mask(__pdata, size, gfp, mask)))
		return __pdata;
	kfree(pdata);
	return NULL;
}
EXPORT_SYMBOL_GPL(__percpu_alloc_mask);

void percpu_free(void *__pdata)
{
	if (unlikely(!__pdata))
		return;
	__percpu_depopulate_mask(__pdata, &cpu_possible_map);
	kfree(__percpu_disguise(__pdata));
}
EXPORT_SYMBOL_GPL(percpu_free);
