
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/jhash.h>
#include <linux/list.h>
#include <linux/rcupdate.h>
#include <linux/marker.h>
#include <linux/err.h>
#include <linux/slab.h>

extern struct marker __start___markers[];
extern struct marker __stop___markers[];

/* Set to 1 to enable marker debug output */
static const int marker_debug;

static DEFINE_MUTEX(markers_mutex);

#define MARKER_HASH_BITS 6
#define MARKER_TABLE_SIZE (1 << MARKER_HASH_BITS)
static struct hlist_head marker_table[MARKER_TABLE_SIZE];

struct marker_entry {
	struct hlist_node hlist;
	char *format;
			/* Probe wrapper */
	void (*call)(const struct marker *mdata, void *call_private, ...);
	struct marker_probe_closure single;
	struct marker_probe_closure *multi;
	int refcount;	/* Number of times armed. 0 if disarmed. */
	struct rcu_head rcu;
	void *oldptr;
	int rcu_pending;
	unsigned char ptype:1;
	unsigned char format_allocated:1;
	char name[0];	/* Contains name'\0'format'\0' */
};

notrace void __mark_empty_function(void *probe_private, void *call_private,
	const char *fmt, va_list *args)
{
}
EXPORT_SYMBOL_GPL(__mark_empty_function);

notrace void marker_probe_cb(const struct marker *mdata,
		void *call_private, ...)
{
	va_list args;
	char ptype;

	/*
	 * rcu_read_lock_sched does two things : disabling preemption to make
	 * sure the teardown of the callbacks can be done correctly when they
	 * are in modules and they insure RCU read coherency.
	 */
	rcu_read_lock_sched_notrace();
	ptype = mdata->ptype;
	if (likely(!ptype)) {
		marker_probe_func *func;
		/* Must read the ptype before ptr. They are not data dependant,
		 * so we put an explicit smp_rmb() here. */
		smp_rmb();
		func = mdata->single.func;
		/* Must read the ptr before private data. They are not data
		 * dependant, so we put an explicit smp_rmb() here. */
		smp_rmb();
		va_start(args, call_private);
		func(mdata->single.probe_private, call_private, mdata->format,
			&args);
		va_end(args);
	} else {
		struct marker_probe_closure *multi;
		int i;
		/*
		 * Read mdata->ptype before mdata->multi.
		 */
		smp_rmb();
		multi = mdata->multi;
		/*
		 * multi points to an array, therefore accessing the array
		 * depends on reading multi. However, even in this case,
		 * we must insure that the pointer is read _before_ the array
		 * data. Same as rcu_dereference, but we need a full smp_rmb()
		 * in the fast path, so put the explicit barrier here.
		 */
		smp_read_barrier_depends();
		for (i = 0; multi[i].func; i++) {
			va_start(args, call_private);
			multi[i].func(multi[i].probe_private, call_private,
				mdata->format, &args);
			va_end(args);
		}
	}
	rcu_read_unlock_sched_notrace();
}
EXPORT_SYMBOL_GPL(marker_probe_cb);

static notrace void marker_probe_cb_noarg(const struct marker *mdata,
		void *call_private, ...)
{
	va_list args;	/* not initialized */
	char ptype;

	rcu_read_lock_sched_notrace();
	ptype = mdata->ptype;
	if (likely(!ptype)) {
		marker_probe_func *func;
		/* Must read the ptype before ptr. They are not data dependant,
		 * so we put an explicit smp_rmb() here. */
		smp_rmb();
		func = mdata->single.func;
		/* Must read the ptr before private data. They are not data
		 * dependant, so we put an explicit smp_rmb() here. */
		smp_rmb();
		func(mdata->single.probe_private, call_private, mdata->format,
			&args);
	} else {
		struct marker_probe_closure *multi;
		int i;
		/*
		 * Read mdata->ptype before mdata->multi.
		 */
		smp_rmb();
		multi = mdata->multi;
		/*
		 * multi points to an array, therefore accessing the array
		 * depends on reading multi. However, even in this case,
		 * we must insure that the pointer is read _before_ the array
		 * data. Same as rcu_dereference, but we need a full smp_rmb()
		 * in the fast path, so put the explicit barrier here.
		 */
		smp_read_barrier_depends();
		for (i = 0; multi[i].func; i++)
			multi[i].func(multi[i].probe_private, call_private,
				mdata->format, &args);
	}
	rcu_read_unlock_sched_notrace();
}

static void free_old_closure(struct rcu_head *head)
{
	struct marker_entry *entry = container_of(head,
		struct marker_entry, rcu);
	kfree(entry->oldptr);
	/* Make sure we free the data before setting the pending flag to 0 */
	smp_wmb();
	entry->rcu_pending = 0;
}

static void debug_print_probes(struct marker_entry *entry)
{
	int i;

	if (!marker_debug)
		return;

	if (!entry->ptype) {
		printk(KERN_DEBUG "Single probe : %p %p\n",
			entry->single.func,
			entry->single.probe_private);
	} else {
		for (i = 0; entry->multi[i].func; i++)
			printk(KERN_DEBUG "Multi probe %d : %p %p\n", i,
				entry->multi[i].func,
				entry->multi[i].probe_private);
	}
}

static struct marker_probe_closure *
marker_entry_add_probe(struct marker_entry *entry,
		marker_probe_func *probe, void *probe_private)
{
	int nr_probes = 0;
	struct marker_probe_closure *old, *new;

	WARN_ON(!probe);

	debug_print_probes(entry);
	old = entry->multi;
	if (!entry->ptype) {
		if (entry->single.func == probe &&
				entry->single.probe_private == probe_private)
			return ERR_PTR(-EBUSY);
		if (entry->single.func == __mark_empty_function) {
			/* 0 -> 1 probes */
			entry->single.func = probe;
			entry->single.probe_private = probe_private;
			entry->refcount = 1;
			entry->ptype = 0;
			debug_print_probes(entry);
			return NULL;
		} else {
			/* 1 -> 2 probes */
			nr_probes = 1;
			old = NULL;
		}
	} else {
		/* (N -> N+1), (N != 0, 1) probes */
		for (nr_probes = 0; old[nr_probes].func; nr_probes++)
			if (old[nr_probes].func == probe
					&& old[nr_probes].probe_private
						== probe_private)
				return ERR_PTR(-EBUSY);
	}
	/* + 2 : one for new probe, one for NULL func */
	new = kzalloc((nr_probes + 2) * sizeof(struct marker_probe_closure),
			GFP_KERNEL);
	if (new == NULL)
		return ERR_PTR(-ENOMEM);
	if (!old)
		new[0] = entry->single;
	else
		memcpy(new, old,
			nr_probes * sizeof(struct marker_probe_closure));
	new[nr_probes].func = probe;
	new[nr_probes].probe_private = probe_private;
	entry->refcount = nr_probes + 1;
	entry->multi = new;
	entry->ptype = 1;
	debug_print_probes(entry);
	return old;
}

static struct marker_probe_closure *
marker_entry_remove_probe(struct marker_entry *entry,
		marker_probe_func *probe, void *probe_private)
{
	int nr_probes = 0, nr_del = 0, i;
	struct marker_probe_closure *old, *new;

	old = entry->multi;

	debug_print_probes(entry);
	if (!entry->ptype) {
		/* 0 -> N is an error */
		WARN_ON(entry->single.func == __mark_empty_function);
		/* 1 -> 0 probes */
		WARN_ON(probe && entry->single.func != probe);
		WARN_ON(entry->single.probe_private != probe_private);
		entry->single.func = __mark_empty_function;
		entry->refcount = 0;
		entry->ptype = 0;
		debug_print_probes(entry);
		return NULL;
	} else {
		/* (N -> M), (N > 1, M >= 0) probes */
		for (nr_probes = 0; old[nr_probes].func; nr_probes++) {
			if ((!probe || old[nr_probes].func == probe)
					&& old[nr_probes].probe_private
						== probe_private)
				nr_del++;
		}
	}

	if (nr_probes - nr_del == 0) {
		/* N -> 0, (N > 1) */
		entry->single.func = __mark_empty_function;
		entry->refcount = 0;
		entry->ptype = 0;
	} else if (nr_probes - nr_del == 1) {
		/* N -> 1, (N > 1) */
		for (i = 0; old[i].func; i++)
			if ((probe && old[i].func != probe) ||
					old[i].probe_private != probe_private)
				entry->single = old[i];
		entry->refcount = 1;
		entry->ptype = 0;
	} else {
		int j = 0;
		/* N -> M, (N > 1, M > 1) */
		/* + 1 for NULL */
		new = kzalloc((nr_probes - nr_del + 1)
			* sizeof(struct marker_probe_closure), GFP_KERNEL);
		if (new == NULL)
			return ERR_PTR(-ENOMEM);
		for (i = 0; old[i].func; i++)
			if ((probe && old[i].func != probe) ||
					old[i].probe_private != probe_private)
				new[j++] = old[i];
		entry->refcount = nr_probes - nr_del;
		entry->ptype = 1;
		entry->multi = new;
	}
	debug_print_probes(entry);
	return old;
}

static struct marker_entry *get_marker(const char *name)
{
	struct hlist_head *head;
	struct hlist_node *node;
	struct marker_entry *e;
	u32 hash = jhash(name, strlen(name), 0);

	head = &marker_table[hash & ((1 << MARKER_HASH_BITS)-1)];
	hlist_for_each_entry(e, node, head, hlist) {
		if (!strcmp(name, e->name))
			return e;
	}
	return NULL;
}

static struct marker_entry *add_marker(const char *name, const char *format)
{
	struct hlist_head *head;
	struct hlist_node *node;
	struct marker_entry *e;
	size_t name_len = strlen(name) + 1;
	size_t format_len = 0;
	u32 hash = jhash(name, name_len-1, 0);

	if (format)
		format_len = strlen(format) + 1;
	head = &marker_table[hash & ((1 << MARKER_HASH_BITS)-1)];
	hlist_for_each_entry(e, node, head, hlist) {
		if (!strcmp(name, e->name)) {
			printk(KERN_NOTICE
				"Marker %s busy\n", name);
			return ERR_PTR(-EBUSY);	/* Already there */
		}
	}
	/*
	 * Using kmalloc here to allocate a variable length element. Could
	 * cause some memory fragmentation if overused.
	 */
	e = kmalloc(sizeof(struct marker_entry) + name_len + format_len,
			GFP_KERNEL);
	if (!e)
		return ERR_PTR(-ENOMEM);
	memcpy(&e->name[0], name, name_len);
	if (format) {
		e->format = &e->name[name_len];
		memcpy(e->format, format, format_len);
		if (strcmp(e->format, MARK_NOARGS) == 0)
			e->call = marker_probe_cb_noarg;
		else
			e->call = marker_probe_cb;
		trace_mark(core_marker_format, "name %s format %s",
				e->name, e->format);
	} else {
		e->format = NULL;
		e->call = marker_probe_cb;
	}
	e->single.func = __mark_empty_function;
	e->single.probe_private = NULL;
	e->multi = NULL;
	e->ptype = 0;
	e->format_allocated = 0;
	e->refcount = 0;
	e->rcu_pending = 0;
	hlist_add_head(&e->hlist, head);
	return e;
}

static int remove_marker(const char *name)
{
	struct hlist_head *head;
	struct hlist_node *node;
	struct marker_entry *e;
	int found = 0;
	size_t len = strlen(name) + 1;
	u32 hash = jhash(name, len-1, 0);

	head = &marker_table[hash & ((1 << MARKER_HASH_BITS)-1)];
	hlist_for_each_entry(e, node, head, hlist) {
		if (!strcmp(name, e->name)) {
			found = 1;
			break;
		}
	}
	if (!found)
		return -ENOENT;
	if (e->single.func != __mark_empty_function)
		return -EBUSY;
	hlist_del(&e->hlist);
	if (e->format_allocated)
		kfree(e->format);
	/* Make sure the call_rcu has been executed */
	if (e->rcu_pending)
		rcu_barrier_sched();
	kfree(e);
	return 0;
}

static int marker_set_format(struct marker_entry *entry, const char *format)
{
	entry->format = kstrdup(format, GFP_KERNEL);
	if (!entry->format)
		return -ENOMEM;
	entry->format_allocated = 1;

	trace_mark(core_marker_format, "name %s format %s",
			entry->name, entry->format);
	return 0;
}

static int set_marker(struct marker_entry *entry, struct marker *elem,
		int active)
{
	int ret = 0;
	WARN_ON(strcmp(entry->name, elem->name) != 0);

	if (entry->format) {
		if (strcmp(entry->format, elem->format) != 0) {
			printk(KERN_NOTICE
				"Format mismatch for probe %s "
				"(%s), marker (%s)\n",
				entry->name,
				entry->format,
				elem->format);
			return -EPERM;
		}
	} else {
		ret = marker_set_format(entry, elem->format);
		if (ret)
			return ret;
	}

	/*
	 * probe_cb setup (statically known) is done here. It is
	 * asynchronous with the rest of execution, therefore we only
	 * pass from a "safe" callback (with argument) to an "unsafe"
	 * callback (does not set arguments).
	 */
	elem->call = entry->call;
	/*
	 * Sanity check :
	 * We only update the single probe private data when the ptr is
	 * set to a _non_ single probe! (0 -> 1 and N -> 1, N != 1)
	 */
	WARN_ON(elem->single.func != __mark_empty_function
		&& elem->single.probe_private != entry->single.probe_private
		&& !elem->ptype);
	elem->single.probe_private = entry->single.probe_private;
	/*
	 * Make sure the private data is valid when we update the
	 * single probe ptr.
	 */
	smp_wmb();
	elem->single.func = entry->single.func;
	/*
	 * We also make sure that the new probe callbacks array is consistent
	 * before setting a pointer to it.
	 */
	rcu_assign_pointer(elem->multi, entry->multi);
	/*
	 * Update the function or multi probe array pointer before setting the
	 * ptype.
	 */
	smp_wmb();
	elem->ptype = entry->ptype;

	if (elem->tp_name && (active ^ elem->state)) {
		WARN_ON(!elem->tp_cb);
		/*
		 * It is ok to directly call the probe registration because type
		 * checking has been done in the __trace_mark_tp() macro.
		 */

		if (active) {
			/*
			 * try_module_get should always succeed because we hold
			 * lock_module() to get the tp_cb address.
			 */
			ret = try_module_get(__module_text_address(
				(unsigned long)elem->tp_cb));
			BUG_ON(!ret);
			ret = tracepoint_probe_register_noupdate(
				elem->tp_name,
				elem->tp_cb);
		} else {
			ret = tracepoint_probe_unregister_noupdate(
				elem->tp_name,
				elem->tp_cb);
			/*
			 * tracepoint_probe_update_all() must be called
			 * before the module containing tp_cb is unloaded.
			 */
			module_put(__module_text_address(
				(unsigned long)elem->tp_cb));
		}
	}
	elem->state = active;

	return ret;
}

static void disable_marker(struct marker *elem)
{
	int ret;

	/* leave "call" as is. It is known statically. */
	if (elem->tp_name && elem->state) {
		WARN_ON(!elem->tp_cb);
		/*
		 * It is ok to directly call the probe registration because type
		 * checking has been done in the __trace_mark_tp() macro.
		 */
		ret = tracepoint_probe_unregister_noupdate(elem->tp_name,
			elem->tp_cb);
		WARN_ON(ret);
		/*
		 * tracepoint_probe_update_all() must be called
		 * before the module containing tp_cb is unloaded.
		 */
		module_put(__module_text_address((unsigned long)elem->tp_cb));
	}
	elem->state = 0;
	elem->single.func = __mark_empty_function;
	/* Update the function before setting the ptype */
	smp_wmb();
	elem->ptype = 0;	/* single probe */
	/*
	 * Leave the private data and id there, because removal is racy and
	 * should be done only after an RCU period. These are never used until
	 * the next initialization anyway.
	 */
}

void marker_update_probe_range(struct marker *begin,
	struct marker *end)
{
	struct marker *iter;
	struct marker_entry *mark_entry;

	mutex_lock(&markers_mutex);
	for (iter = begin; iter < end; iter++) {
		mark_entry = get_marker(iter->name);
		if (mark_entry) {
			set_marker(mark_entry, iter, !!mark_entry->refcount);
			/*
			 * ignore error, continue
			 */
		} else {
			disable_marker(iter);
		}
	}
	mutex_unlock(&markers_mutex);
}

static void marker_update_probes(void)
{
	/* Core kernel markers */
	marker_update_probe_range(__start___markers, __stop___markers);
	/* Markers in modules. */
	module_update_markers();
	tracepoint_probe_update_all();
}

int marker_probe_register(const char *name, const char *format,
			marker_probe_func *probe, void *probe_private)
{
	struct marker_entry *entry;
	int ret = 0;
	struct marker_probe_closure *old;

	mutex_lock(&markers_mutex);
	entry = get_marker(name);
	if (!entry) {
		entry = add_marker(name, format);
		if (IS_ERR(entry))
			ret = PTR_ERR(entry);
	} else if (format) {
		if (!entry->format)
			ret = marker_set_format(entry, format);
		else if (strcmp(entry->format, format))
			ret = -EPERM;
	}
	if (ret)
		goto end;

	/*
	 * If we detect that a call_rcu is pending for this marker,
	 * make sure it's executed now.
	 */
	if (entry->rcu_pending)
		rcu_barrier_sched();
	old = marker_entry_add_probe(entry, probe, probe_private);
	if (IS_ERR(old)) {
		ret = PTR_ERR(old);
		goto end;
	}
	mutex_unlock(&markers_mutex);
	marker_update_probes();
	mutex_lock(&markers_mutex);
	entry = get_marker(name);
	if (!entry)
		goto end;
	if (entry->rcu_pending)
		rcu_barrier_sched();
	entry->oldptr = old;
	entry->rcu_pending = 1;
	/* write rcu_pending before calling the RCU callback */
	smp_wmb();
	call_rcu_sched(&entry->rcu, free_old_closure);
end:
	mutex_unlock(&markers_mutex);
	return ret;
}
EXPORT_SYMBOL_GPL(marker_probe_register);

int marker_probe_unregister(const char *name,
	marker_probe_func *probe, void *probe_private)
{
	struct marker_entry *entry;
	struct marker_probe_closure *old;
	int ret = -ENOENT;

	mutex_lock(&markers_mutex);
	entry = get_marker(name);
	if (!entry)
		goto end;
	if (entry->rcu_pending)
		rcu_barrier_sched();
	old = marker_entry_remove_probe(entry, probe, probe_private);
	mutex_unlock(&markers_mutex);
	marker_update_probes();
	mutex_lock(&markers_mutex);
	entry = get_marker(name);
	if (!entry)
		goto end;
	if (entry->rcu_pending)
		rcu_barrier_sched();
	entry->oldptr = old;
	entry->rcu_pending = 1;
	/* write rcu_pending before calling the RCU callback */
	smp_wmb();
	call_rcu_sched(&entry->rcu, free_old_closure);
	remove_marker(name);	/* Ignore busy error message */
	ret = 0;
end:
	mutex_unlock(&markers_mutex);
	return ret;
}
EXPORT_SYMBOL_GPL(marker_probe_unregister);

static struct marker_entry *
get_marker_from_private_data(marker_probe_func *probe, void *probe_private)
{
	struct marker_entry *entry;
	unsigned int i;
	struct hlist_head *head;
	struct hlist_node *node;

	for (i = 0; i < MARKER_TABLE_SIZE; i++) {
		head = &marker_table[i];
		hlist_for_each_entry(entry, node, head, hlist) {
			if (!entry->ptype) {
				if (entry->single.func == probe
						&& entry->single.probe_private
						== probe_private)
					return entry;
			} else {
				struct marker_probe_closure *closure;
				closure = entry->multi;
				for (i = 0; closure[i].func; i++) {
					if (closure[i].func == probe &&
							closure[i].probe_private
							== probe_private)
						return entry;
				}
			}
		}
	}
	return NULL;
}

int marker_probe_unregister_private_data(marker_probe_func *probe,
		void *probe_private)
{
	struct marker_entry *entry;
	int ret = 0;
	struct marker_probe_closure *old;

	mutex_lock(&markers_mutex);
	entry = get_marker_from_private_data(probe, probe_private);
	if (!entry) {
		ret = -ENOENT;
		goto end;
	}
	if (entry->rcu_pending)
		rcu_barrier_sched();
	old = marker_entry_remove_probe(entry, NULL, probe_private);
	mutex_unlock(&markers_mutex);
	marker_update_probes();
	mutex_lock(&markers_mutex);
	entry = get_marker_from_private_data(probe, probe_private);
	if (!entry)
		goto end;
	if (entry->rcu_pending)
		rcu_barrier_sched();
	entry->oldptr = old;
	entry->rcu_pending = 1;
	/* write rcu_pending before calling the RCU callback */
	smp_wmb();
	call_rcu_sched(&entry->rcu, free_old_closure);
	remove_marker(entry->name);	/* Ignore busy error message */
end:
	mutex_unlock(&markers_mutex);
	return ret;
}
EXPORT_SYMBOL_GPL(marker_probe_unregister_private_data);

void *marker_get_private_data(const char *name, marker_probe_func *probe,
		int num)
{
	struct hlist_head *head;
	struct hlist_node *node;
	struct marker_entry *e;
	size_t name_len = strlen(name) + 1;
	u32 hash = jhash(name, name_len-1, 0);
	int i;

	head = &marker_table[hash & ((1 << MARKER_HASH_BITS)-1)];
	hlist_for_each_entry(e, node, head, hlist) {
		if (!strcmp(name, e->name)) {
			if (!e->ptype) {
				if (num == 0 && e->single.func == probe)
					return e->single.probe_private;
			} else {
				struct marker_probe_closure *closure;
				int match = 0;
				closure = e->multi;
				for (i = 0; closure[i].func; i++) {
					if (closure[i].func != probe)
						continue;
					if (match++ == num)
						return closure[i].probe_private;
				}
			}
			break;
		}
	}
	return ERR_PTR(-ENOENT);
}
EXPORT_SYMBOL_GPL(marker_get_private_data);

#ifdef CONFIG_MODULES

int marker_module_notify(struct notifier_block *self,
			 unsigned long val, void *data)
{
	struct module *mod = data;

	switch (val) {
	case MODULE_STATE_COMING:
		marker_update_probe_range(mod->markers,
			mod->markers + mod->num_markers);
		break;
	case MODULE_STATE_GOING:
		marker_update_probe_range(mod->markers,
			mod->markers + mod->num_markers);
		break;
	}
	return 0;
}

struct notifier_block marker_module_nb = {
	.notifier_call = marker_module_notify,
	.priority = 0,
};

static int init_markers(void)
{
	return register_module_notifier(&marker_module_nb);
}
__initcall(init_markers);

#endif /* CONFIG_MODULES */
