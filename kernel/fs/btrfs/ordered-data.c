

#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/blkdev.h>
#include <linux/writeback.h>
#include <linux/pagevec.h>
#include "ctree.h"
#include "transaction.h"
#include "btrfs_inode.h"
#include "extent_io.h"

static u64 entry_end(struct btrfs_ordered_extent *entry)
{
	if (entry->file_offset + entry->len < entry->file_offset)
		return (u64)-1;
	return entry->file_offset + entry->len;
}

static struct rb_node *tree_insert(struct rb_root *root, u64 file_offset,
				   struct rb_node *node)
{
	struct rb_node **p = &root->rb_node;
	struct rb_node *parent = NULL;
	struct btrfs_ordered_extent *entry;

	while (*p) {
		parent = *p;
		entry = rb_entry(parent, struct btrfs_ordered_extent, rb_node);

		if (file_offset < entry->file_offset)
			p = &(*p)->rb_left;
		else if (file_offset >= entry_end(entry))
			p = &(*p)->rb_right;
		else
			return parent;
	}

	rb_link_node(node, parent, p);
	rb_insert_color(node, root);
	return NULL;
}

static struct rb_node *__tree_search(struct rb_root *root, u64 file_offset,
				     struct rb_node **prev_ret)
{
	struct rb_node *n = root->rb_node;
	struct rb_node *prev = NULL;
	struct rb_node *test;
	struct btrfs_ordered_extent *entry;
	struct btrfs_ordered_extent *prev_entry = NULL;

	while (n) {
		entry = rb_entry(n, struct btrfs_ordered_extent, rb_node);
		prev = n;
		prev_entry = entry;

		if (file_offset < entry->file_offset)
			n = n->rb_left;
		else if (file_offset >= entry_end(entry))
			n = n->rb_right;
		else
			return n;
	}
	if (!prev_ret)
		return NULL;

	while (prev && file_offset >= entry_end(prev_entry)) {
		test = rb_next(prev);
		if (!test)
			break;
		prev_entry = rb_entry(test, struct btrfs_ordered_extent,
				      rb_node);
		if (file_offset < entry_end(prev_entry))
			break;

		prev = test;
	}
	if (prev)
		prev_entry = rb_entry(prev, struct btrfs_ordered_extent,
				      rb_node);
	while (prev && file_offset < entry_end(prev_entry)) {
		test = rb_prev(prev);
		if (!test)
			break;
		prev_entry = rb_entry(test, struct btrfs_ordered_extent,
				      rb_node);
		prev = test;
	}
	*prev_ret = prev;
	return NULL;
}

static int offset_in_entry(struct btrfs_ordered_extent *entry, u64 file_offset)
{
	if (file_offset < entry->file_offset ||
	    entry->file_offset + entry->len <= file_offset)
		return 0;
	return 1;
}

static inline struct rb_node *tree_search(struct btrfs_ordered_inode_tree *tree,
					  u64 file_offset)
{
	struct rb_root *root = &tree->tree;
	struct rb_node *prev;
	struct rb_node *ret;
	struct btrfs_ordered_extent *entry;

	if (tree->last) {
		entry = rb_entry(tree->last, struct btrfs_ordered_extent,
				 rb_node);
		if (offset_in_entry(entry, file_offset))
			return tree->last;
	}
	ret = __tree_search(root, file_offset, &prev);
	if (!ret)
		ret = prev;
	if (ret)
		tree->last = ret;
	return ret;
}

int btrfs_add_ordered_extent(struct inode *inode, u64 file_offset,
			     u64 start, u64 len, u64 disk_len, int type)
{
	struct btrfs_ordered_inode_tree *tree;
	struct rb_node *node;
	struct btrfs_ordered_extent *entry;

	tree = &BTRFS_I(inode)->ordered_tree;
	entry = kzalloc(sizeof(*entry), GFP_NOFS);
	if (!entry)
		return -ENOMEM;

	mutex_lock(&tree->mutex);
	entry->file_offset = file_offset;
	entry->start = start;
	entry->len = len;
	entry->disk_len = disk_len;
	entry->inode = inode;
	if (type != BTRFS_ORDERED_IO_DONE && type != BTRFS_ORDERED_COMPLETE)
		set_bit(type, &entry->flags);

	/* one ref for the tree */
	atomic_set(&entry->refs, 1);
	init_waitqueue_head(&entry->wait);
	INIT_LIST_HEAD(&entry->list);
	INIT_LIST_HEAD(&entry->root_extent_list);

	node = tree_insert(&tree->tree, file_offset,
			   &entry->rb_node);
	BUG_ON(node);

	set_extent_ordered(&BTRFS_I(inode)->io_tree, file_offset,
			   entry_end(entry) - 1, GFP_NOFS);

	spin_lock(&BTRFS_I(inode)->root->fs_info->ordered_extent_lock);
	list_add_tail(&entry->root_extent_list,
		      &BTRFS_I(inode)->root->fs_info->ordered_extents);
	spin_unlock(&BTRFS_I(inode)->root->fs_info->ordered_extent_lock);

	mutex_unlock(&tree->mutex);
	BUG_ON(node);
	return 0;
}

int btrfs_add_ordered_sum(struct inode *inode,
			  struct btrfs_ordered_extent *entry,
			  struct btrfs_ordered_sum *sum)
{
	struct btrfs_ordered_inode_tree *tree;

	tree = &BTRFS_I(inode)->ordered_tree;
	mutex_lock(&tree->mutex);
	list_add_tail(&sum->list, &entry->list);
	mutex_unlock(&tree->mutex);
	return 0;
}

int btrfs_dec_test_ordered_pending(struct inode *inode,
				   u64 file_offset, u64 io_size)
{
	struct btrfs_ordered_inode_tree *tree;
	struct rb_node *node;
	struct btrfs_ordered_extent *entry;
	struct extent_io_tree *io_tree = &BTRFS_I(inode)->io_tree;
	int ret;

	tree = &BTRFS_I(inode)->ordered_tree;
	mutex_lock(&tree->mutex);
	clear_extent_ordered(io_tree, file_offset, file_offset + io_size - 1,
			     GFP_NOFS);
	node = tree_search(tree, file_offset);
	if (!node) {
		ret = 1;
		goto out;
	}

	entry = rb_entry(node, struct btrfs_ordered_extent, rb_node);
	if (!offset_in_entry(entry, file_offset)) {
		ret = 1;
		goto out;
	}

	ret = test_range_bit(io_tree, entry->file_offset,
			     entry->file_offset + entry->len - 1,
			     EXTENT_ORDERED, 0);
	if (ret == 0)
		ret = test_and_set_bit(BTRFS_ORDERED_IO_DONE, &entry->flags);
out:
	mutex_unlock(&tree->mutex);
	return ret == 0;
}

int btrfs_put_ordered_extent(struct btrfs_ordered_extent *entry)
{
	struct list_head *cur;
	struct btrfs_ordered_sum *sum;

	if (atomic_dec_and_test(&entry->refs)) {
		while (!list_empty(&entry->list)) {
			cur = entry->list.next;
			sum = list_entry(cur, struct btrfs_ordered_sum, list);
			list_del(&sum->list);
			kfree(sum);
		}
		kfree(entry);
	}
	return 0;
}

int btrfs_remove_ordered_extent(struct inode *inode,
				struct btrfs_ordered_extent *entry)
{
	struct btrfs_ordered_inode_tree *tree;
	struct rb_node *node;

	tree = &BTRFS_I(inode)->ordered_tree;
	mutex_lock(&tree->mutex);
	node = &entry->rb_node;
	rb_erase(node, &tree->tree);
	tree->last = NULL;
	set_bit(BTRFS_ORDERED_COMPLETE, &entry->flags);

	spin_lock(&BTRFS_I(inode)->root->fs_info->ordered_extent_lock);
	list_del_init(&entry->root_extent_list);
	spin_unlock(&BTRFS_I(inode)->root->fs_info->ordered_extent_lock);

	mutex_unlock(&tree->mutex);
	wake_up(&entry->wait);
	return 0;
}

int btrfs_wait_ordered_extents(struct btrfs_root *root, int nocow_only)
{
	struct list_head splice;
	struct list_head *cur;
	struct btrfs_ordered_extent *ordered;
	struct inode *inode;

	INIT_LIST_HEAD(&splice);

	spin_lock(&root->fs_info->ordered_extent_lock);
	list_splice_init(&root->fs_info->ordered_extents, &splice);
	while (!list_empty(&splice)) {
		cur = splice.next;
		ordered = list_entry(cur, struct btrfs_ordered_extent,
				     root_extent_list);
		if (nocow_only &&
		    !test_bit(BTRFS_ORDERED_NOCOW, &ordered->flags) &&
		    !test_bit(BTRFS_ORDERED_PREALLOC, &ordered->flags)) {
			list_move(&ordered->root_extent_list,
				  &root->fs_info->ordered_extents);
			cond_resched_lock(&root->fs_info->ordered_extent_lock);
			continue;
		}

		list_del_init(&ordered->root_extent_list);
		atomic_inc(&ordered->refs);

		/*
		 * the inode may be getting freed (in sys_unlink path).
		 */
		inode = igrab(ordered->inode);

		spin_unlock(&root->fs_info->ordered_extent_lock);

		if (inode) {
			btrfs_start_ordered_extent(inode, ordered, 1);
			btrfs_put_ordered_extent(ordered);
			iput(inode);
		} else {
			btrfs_put_ordered_extent(ordered);
		}

		spin_lock(&root->fs_info->ordered_extent_lock);
	}
	spin_unlock(&root->fs_info->ordered_extent_lock);
	return 0;
}

void btrfs_start_ordered_extent(struct inode *inode,
				       struct btrfs_ordered_extent *entry,
				       int wait)
{
	u64 start = entry->file_offset;
	u64 end = start + entry->len - 1;

	/*
	 * pages in the range can be dirty, clean or writeback.  We
	 * start IO on any dirty ones so the wait doesn't stall waiting
	 * for pdflush to find them
	 */
	btrfs_fdatawrite_range(inode->i_mapping, start, end, WB_SYNC_ALL);
	if (wait) {
		wait_event(entry->wait, test_bit(BTRFS_ORDERED_COMPLETE,
						 &entry->flags));
	}
}

int btrfs_wait_ordered_range(struct inode *inode, u64 start, u64 len)
{
	u64 end;
	u64 orig_end;
	u64 wait_end;
	struct btrfs_ordered_extent *ordered;

	if (start + len < start) {
		orig_end = INT_LIMIT(loff_t);
	} else {
		orig_end = start + len - 1;
		if (orig_end > INT_LIMIT(loff_t))
			orig_end = INT_LIMIT(loff_t);
	}
	wait_end = orig_end;
again:
	/* start IO across the range first to instantiate any delalloc
	 * extents
	 */
	btrfs_fdatawrite_range(inode->i_mapping, start, orig_end, WB_SYNC_NONE);

	/* The compression code will leave pages locked but return from
	 * writepage without setting the page writeback.  Starting again
	 * with WB_SYNC_ALL will end up waiting for the IO to actually start.
	 */
	btrfs_fdatawrite_range(inode->i_mapping, start, orig_end, WB_SYNC_ALL);

	btrfs_wait_on_page_writeback_range(inode->i_mapping,
					   start >> PAGE_CACHE_SHIFT,
					   orig_end >> PAGE_CACHE_SHIFT);

	end = orig_end;
	while (1) {
		ordered = btrfs_lookup_first_ordered_extent(inode, end);
		if (!ordered)
			break;
		if (ordered->file_offset > orig_end) {
			btrfs_put_ordered_extent(ordered);
			break;
		}
		if (ordered->file_offset + ordered->len < start) {
			btrfs_put_ordered_extent(ordered);
			break;
		}
		btrfs_start_ordered_extent(inode, ordered, 1);
		end = ordered->file_offset;
		btrfs_put_ordered_extent(ordered);
		if (end == 0 || end == start)
			break;
		end--;
	}
	if (test_range_bit(&BTRFS_I(inode)->io_tree, start, orig_end,
			   EXTENT_ORDERED | EXTENT_DELALLOC, 0)) {
		schedule_timeout(1);
		goto again;
	}
	return 0;
}

struct btrfs_ordered_extent *btrfs_lookup_ordered_extent(struct inode *inode,
							 u64 file_offset)
{
	struct btrfs_ordered_inode_tree *tree;
	struct rb_node *node;
	struct btrfs_ordered_extent *entry = NULL;

	tree = &BTRFS_I(inode)->ordered_tree;
	mutex_lock(&tree->mutex);
	node = tree_search(tree, file_offset);
	if (!node)
		goto out;

	entry = rb_entry(node, struct btrfs_ordered_extent, rb_node);
	if (!offset_in_entry(entry, file_offset))
		entry = NULL;
	if (entry)
		atomic_inc(&entry->refs);
out:
	mutex_unlock(&tree->mutex);
	return entry;
}

struct btrfs_ordered_extent *
btrfs_lookup_first_ordered_extent(struct inode *inode, u64 file_offset)
{
	struct btrfs_ordered_inode_tree *tree;
	struct rb_node *node;
	struct btrfs_ordered_extent *entry = NULL;

	tree = &BTRFS_I(inode)->ordered_tree;
	mutex_lock(&tree->mutex);
	node = tree_search(tree, file_offset);
	if (!node)
		goto out;

	entry = rb_entry(node, struct btrfs_ordered_extent, rb_node);
	atomic_inc(&entry->refs);
out:
	mutex_unlock(&tree->mutex);
	return entry;
}

int btrfs_ordered_update_i_size(struct inode *inode,
				struct btrfs_ordered_extent *ordered)
{
	struct btrfs_ordered_inode_tree *tree = &BTRFS_I(inode)->ordered_tree;
	struct extent_io_tree *io_tree = &BTRFS_I(inode)->io_tree;
	u64 disk_i_size;
	u64 new_i_size;
	u64 i_size_test;
	struct rb_node *node;
	struct btrfs_ordered_extent *test;

	mutex_lock(&tree->mutex);
	disk_i_size = BTRFS_I(inode)->disk_i_size;

	/*
	 * if the disk i_size is already at the inode->i_size, or
	 * this ordered extent is inside the disk i_size, we're done
	 */
	if (disk_i_size >= inode->i_size ||
	    ordered->file_offset + ordered->len <= disk_i_size) {
		goto out;
	}

	/*
	 * we can't update the disk_isize if there are delalloc bytes
	 * between disk_i_size and  this ordered extent
	 */
	if (test_range_bit(io_tree, disk_i_size,
			   ordered->file_offset + ordered->len - 1,
			   EXTENT_DELALLOC, 0)) {
		goto out;
	}
	/*
	 * walk backward from this ordered extent to disk_i_size.
	 * if we find an ordered extent then we can't update disk i_size
	 * yet
	 */
	node = &ordered->rb_node;
	while (1) {
		node = rb_prev(node);
		if (!node)
			break;
		test = rb_entry(node, struct btrfs_ordered_extent, rb_node);
		if (test->file_offset + test->len <= disk_i_size)
			break;
		if (test->file_offset >= inode->i_size)
			break;
		if (test->file_offset >= disk_i_size)
			goto out;
	}
	new_i_size = min_t(u64, entry_end(ordered), i_size_read(inode));

	/*
	 * at this point, we know we can safely update i_size to at least
	 * the offset from this ordered extent.  But, we need to
	 * walk forward and see if ios from higher up in the file have
	 * finished.
	 */
	node = rb_next(&ordered->rb_node);
	i_size_test = 0;
	if (node) {
		/*
		 * do we have an area where IO might have finished
		 * between our ordered extent and the next one.
		 */
		test = rb_entry(node, struct btrfs_ordered_extent, rb_node);
		if (test->file_offset > entry_end(ordered))
			i_size_test = test->file_offset;
	} else {
		i_size_test = i_size_read(inode);
	}

	/*
	 * i_size_test is the end of a region after this ordered
	 * extent where there are no ordered extents.  As long as there
	 * are no delalloc bytes in this area, it is safe to update
	 * disk_i_size to the end of the region.
	 */
	if (i_size_test > entry_end(ordered) &&
	    !test_range_bit(io_tree, entry_end(ordered), i_size_test - 1,
			   EXTENT_DELALLOC, 0)) {
		new_i_size = min_t(u64, i_size_test, i_size_read(inode));
	}
	BTRFS_I(inode)->disk_i_size = new_i_size;
out:
	mutex_unlock(&tree->mutex);
	return 0;
}

int btrfs_find_ordered_sum(struct inode *inode, u64 offset, u64 disk_bytenr,
			   u32 *sum)
{
	struct btrfs_ordered_sum *ordered_sum;
	struct btrfs_sector_sum *sector_sums;
	struct btrfs_ordered_extent *ordered;
	struct btrfs_ordered_inode_tree *tree = &BTRFS_I(inode)->ordered_tree;
	unsigned long num_sectors;
	unsigned long i;
	u32 sectorsize = BTRFS_I(inode)->root->sectorsize;
	int ret = 1;

	ordered = btrfs_lookup_ordered_extent(inode, offset);
	if (!ordered)
		return 1;

	mutex_lock(&tree->mutex);
	list_for_each_entry_reverse(ordered_sum, &ordered->list, list) {
		if (disk_bytenr >= ordered_sum->bytenr) {
			num_sectors = ordered_sum->len / sectorsize;
			sector_sums = ordered_sum->sums;
			for (i = 0; i < num_sectors; i++) {
				if (sector_sums[i].bytenr == disk_bytenr) {
					*sum = sector_sums[i].sum;
					ret = 0;
					goto out;
				}
			}
		}
	}
out:
	mutex_unlock(&tree->mutex);
	btrfs_put_ordered_extent(ordered);
	return ret;
}


int btrfs_fdatawrite_range(struct address_space *mapping, loff_t start,
			   loff_t end, int sync_mode)
{
	struct writeback_control wbc = {
		.sync_mode = sync_mode,
		.nr_to_write = mapping->nrpages * 2,
		.range_start = start,
		.range_end = end,
		.for_writepages = 1,
	};
	return btrfs_writepages(mapping, &wbc);
}

int btrfs_wait_on_page_writeback_range(struct address_space *mapping,
				       pgoff_t start, pgoff_t end)
{
	struct pagevec pvec;
	int nr_pages;
	int ret = 0;
	pgoff_t index;

	if (end < start)
		return 0;

	pagevec_init(&pvec, 0);
	index = start;
	while ((index <= end) &&
			(nr_pages = pagevec_lookup_tag(&pvec, mapping, &index,
			PAGECACHE_TAG_WRITEBACK,
			min(end - index, (pgoff_t)PAGEVEC_SIZE-1) + 1)) != 0) {
		unsigned i;

		for (i = 0; i < nr_pages; i++) {
			struct page *page = pvec.pages[i];

			/* until radix tree lookup accepts end_index */
			if (page->index > end)
				continue;

			wait_on_page_writeback(page);
			if (PageError(page))
				ret = -EIO;
		}
		pagevec_release(&pvec);
		cond_resched();
	}

	/* Check for outstanding write errors */
	if (test_and_clear_bit(AS_ENOSPC, &mapping->flags))
		ret = -ENOSPC;
	if (test_and_clear_bit(AS_EIO, &mapping->flags))
		ret = -EIO;

	return ret;
}
