

#include <linux/kref.h>
#include <linux/module.h>

void kref_set(struct kref *kref, int num)
{
	atomic_set(&kref->refcount, num);
	smp_mb();
}

void kref_init(struct kref *kref)
{
	kref_set(kref, 1);
}

void kref_get(struct kref *kref)
{
	WARN_ON(!atomic_read(&kref->refcount));
	atomic_inc(&kref->refcount);
	smp_mb__after_atomic_inc();
}

int kref_put(struct kref *kref, void (*release)(struct kref *kref))
{
	WARN_ON(release == NULL);
	WARN_ON(release == (void (*)(struct kref *))kfree);

	if (atomic_dec_and_test(&kref->refcount)) {
		release(kref);
		return 1;
	}
	return 0;
}

EXPORT_SYMBOL(kref_set);
EXPORT_SYMBOL(kref_init);
EXPORT_SYMBOL(kref_get);
EXPORT_SYMBOL(kref_put);
