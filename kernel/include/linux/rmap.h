
#ifndef _LINUX_RMAP_H
#define _LINUX_RMAP_H

#include <linux/list.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/spinlock.h>
#include <linux/memcontrol.h>

struct anon_vma {
	spinlock_t lock;	/* Serialize access to vma list */
	/*
	 * NOTE: the LSB of the head.next is set by
	 * mm_take_all_locks() _after_ taking the above lock. So the
	 * head must only be read/written after taking the above lock
	 * to be sure to see a valid next pointer. The LSB bit itself
	 * is serialized by a system wide lock only visible to
	 * mm_take_all_locks() (mm_all_locks_mutex).
	 */
	struct list_head head;	/* List of private "related" vmas */
};

#ifdef CONFIG_MMU

static inline void anon_vma_lock(struct vm_area_struct *vma)
{
	struct anon_vma *anon_vma = vma->anon_vma;
	if (anon_vma)
		spin_lock(&anon_vma->lock);
}

static inline void anon_vma_unlock(struct vm_area_struct *vma)
{
	struct anon_vma *anon_vma = vma->anon_vma;
	if (anon_vma)
		spin_unlock(&anon_vma->lock);
}

void anon_vma_init(void);	/* create anon_vma_cachep */
int  anon_vma_prepare(struct vm_area_struct *);
void __anon_vma_merge(struct vm_area_struct *, struct vm_area_struct *);
void anon_vma_unlink(struct vm_area_struct *);
void anon_vma_link(struct vm_area_struct *);
void __anon_vma_link(struct vm_area_struct *);

void page_add_anon_rmap(struct page *, struct vm_area_struct *, unsigned long);
void page_add_new_anon_rmap(struct page *, struct vm_area_struct *, unsigned long);
void page_add_file_rmap(struct page *);
void page_remove_rmap(struct page *);

#ifdef CONFIG_DEBUG_VM
void page_dup_rmap(struct page *page, struct vm_area_struct *vma, unsigned long address);
#else
static inline void page_dup_rmap(struct page *page, struct vm_area_struct *vma, unsigned long address)
{
	atomic_inc(&page->_mapcount);
}
#endif

int page_referenced(struct page *, int is_locked, struct mem_cgroup *cnt);
int try_to_unmap(struct page *, int ignore_refs);

pte_t *page_check_address(struct page *, struct mm_struct *,
				unsigned long, spinlock_t **, int);

unsigned long page_address_in_vma(struct page *, struct vm_area_struct *);

int page_mkclean(struct page *);

#ifdef CONFIG_UNEVICTABLE_LRU
int try_to_munlock(struct page *);
#else
static inline int try_to_munlock(struct page *page)
{
	return 0;	/* a.k.a. SWAP_SUCCESS */
}
#endif

#else	/* !CONFIG_MMU */

#define anon_vma_init()		do {} while (0)
#define anon_vma_prepare(vma)	(0)
#define anon_vma_link(vma)	do {} while (0)

#define page_referenced(page,l,cnt) TestClearPageReferenced(page)
#define try_to_unmap(page, refs) SWAP_FAIL

static inline int page_mkclean(struct page *page)
{
	return 0;
}


#endif	/* CONFIG_MMU */

#define SWAP_SUCCESS	0
#define SWAP_AGAIN	1
#define SWAP_FAIL	2
#define SWAP_MLOCK	3

#endif	/* _LINUX_RMAP_H */
