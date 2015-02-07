

#ifndef OCFS2_ALLOC_H
#define OCFS2_ALLOC_H


#define OCFS2_MAX_XATTR_TREE_LEAF_SIZE 65536

struct ocfs2_extent_tree_operations;
struct ocfs2_extent_tree {
	struct ocfs2_extent_tree_operations	*et_ops;
	struct buffer_head			*et_root_bh;
	struct ocfs2_extent_list		*et_root_el;
	ocfs2_journal_access_func		et_root_journal_access;
	void					*et_object;
	unsigned int				et_max_leaf_clusters;
};

void ocfs2_init_dinode_extent_tree(struct ocfs2_extent_tree *et,
				   struct inode *inode,
				   struct buffer_head *bh);
void ocfs2_init_xattr_tree_extent_tree(struct ocfs2_extent_tree *et,
				       struct inode *inode,
				       struct buffer_head *bh);
struct ocfs2_xattr_value_buf;
void ocfs2_init_xattr_value_extent_tree(struct ocfs2_extent_tree *et,
					struct inode *inode,
					struct ocfs2_xattr_value_buf *vb);

int ocfs2_read_extent_block(struct inode *inode, u64 eb_blkno,
			    struct buffer_head **bh);

struct ocfs2_alloc_context;
int ocfs2_insert_extent(struct ocfs2_super *osb,
			handle_t *handle,
			struct inode *inode,
			struct ocfs2_extent_tree *et,
			u32 cpos,
			u64 start_blk,
			u32 new_clusters,
			u8 flags,
			struct ocfs2_alloc_context *meta_ac);

enum ocfs2_alloc_restarted {
	RESTART_NONE = 0,
	RESTART_TRANS,
	RESTART_META
};
int ocfs2_add_clusters_in_btree(struct ocfs2_super *osb,
				struct inode *inode,
				u32 *logical_offset,
				u32 clusters_to_add,
				int mark_unwritten,
				struct ocfs2_extent_tree *et,
				handle_t *handle,
				struct ocfs2_alloc_context *data_ac,
				struct ocfs2_alloc_context *meta_ac,
				enum ocfs2_alloc_restarted *reason_ret);
struct ocfs2_cached_dealloc_ctxt;
int ocfs2_mark_extent_written(struct inode *inode,
			      struct ocfs2_extent_tree *et,
			      handle_t *handle, u32 cpos, u32 len, u32 phys,
			      struct ocfs2_alloc_context *meta_ac,
			      struct ocfs2_cached_dealloc_ctxt *dealloc);
int ocfs2_remove_extent(struct inode *inode,
			struct ocfs2_extent_tree *et,
			u32 cpos, u32 len, handle_t *handle,
			struct ocfs2_alloc_context *meta_ac,
			struct ocfs2_cached_dealloc_ctxt *dealloc);
int ocfs2_remove_btree_range(struct inode *inode,
			     struct ocfs2_extent_tree *et,
			     u32 cpos, u32 phys_cpos, u32 len,
			     struct ocfs2_cached_dealloc_ctxt *dealloc);

int ocfs2_num_free_extents(struct ocfs2_super *osb,
			   struct inode *inode,
			   struct ocfs2_extent_tree *et);

static inline int ocfs2_extend_meta_needed(struct ocfs2_extent_list *root_el)
{
	/*
	 * Rather than do all the work of determining how much we need
	 * (involves a ton of reads and locks), just ask for the
	 * maximal limit.  That's a tree depth shift.  So, one block for
	 * level of the tree (current l_tree_depth), one block for the
	 * new tree_depth==0 extent_block, and one block at the new
	 * top-of-the tree.
	 */
	return le16_to_cpu(root_el->l_tree_depth) + 2;
}

void ocfs2_dinode_new_extent_list(struct inode *inode, struct ocfs2_dinode *di);
void ocfs2_set_inode_data_inline(struct inode *inode, struct ocfs2_dinode *di);
int ocfs2_convert_inline_data_to_extents(struct inode *inode,
					 struct buffer_head *di_bh);

int ocfs2_truncate_log_init(struct ocfs2_super *osb);
void ocfs2_truncate_log_shutdown(struct ocfs2_super *osb);
void ocfs2_schedule_truncate_log_flush(struct ocfs2_super *osb,
				       int cancel);
int ocfs2_flush_truncate_log(struct ocfs2_super *osb);
int ocfs2_begin_truncate_log_recovery(struct ocfs2_super *osb,
				      int slot_num,
				      struct ocfs2_dinode **tl_copy);
int ocfs2_complete_truncate_log_recovery(struct ocfs2_super *osb,
					 struct ocfs2_dinode *tl_copy);
int ocfs2_truncate_log_needs_flush(struct ocfs2_super *osb);
int ocfs2_truncate_log_append(struct ocfs2_super *osb,
			      handle_t *handle,
			      u64 start_blk,
			      unsigned int num_clusters);
int __ocfs2_flush_truncate_log(struct ocfs2_super *osb);

struct ocfs2_cached_dealloc_ctxt {
	struct ocfs2_per_slot_free_list		*c_first_suballocator;
	struct ocfs2_cached_block_free 		*c_global_allocator;
};
static inline void ocfs2_init_dealloc_ctxt(struct ocfs2_cached_dealloc_ctxt *c)
{
	c->c_first_suballocator = NULL;
	c->c_global_allocator = NULL;
}
int ocfs2_cache_cluster_dealloc(struct ocfs2_cached_dealloc_ctxt *ctxt,
				u64 blkno, unsigned int bit);
static inline int ocfs2_dealloc_has_cluster(struct ocfs2_cached_dealloc_ctxt *c)
{
	return c->c_global_allocator != NULL;
}
int ocfs2_run_deallocs(struct ocfs2_super *osb,
		       struct ocfs2_cached_dealloc_ctxt *ctxt);

struct ocfs2_truncate_context {
	struct ocfs2_cached_dealloc_ctxt tc_dealloc;
	int tc_ext_alloc_locked; /* is it cluster locked? */
	/* these get destroyed once it's passed to ocfs2_commit_truncate. */
	struct buffer_head *tc_last_eb_bh;
};

int ocfs2_zero_range_for_truncate(struct inode *inode, handle_t *handle,
				  u64 range_start, u64 range_end);
int ocfs2_prepare_truncate(struct ocfs2_super *osb,
			   struct inode *inode,
			   struct buffer_head *fe_bh,
			   struct ocfs2_truncate_context **tc);
int ocfs2_commit_truncate(struct ocfs2_super *osb,
			  struct inode *inode,
			  struct buffer_head *fe_bh,
			  struct ocfs2_truncate_context *tc);
int ocfs2_truncate_inline(struct inode *inode, struct buffer_head *di_bh,
			  unsigned int start, unsigned int end, int trunc);

int ocfs2_find_leaf(struct inode *inode, struct ocfs2_extent_list *root_el,
		    u32 cpos, struct buffer_head **leaf_bh);
int ocfs2_search_extent_list(struct ocfs2_extent_list *el, u32 v_cluster);

static inline unsigned int ocfs2_rec_clusters(struct ocfs2_extent_list *el,
					      struct ocfs2_extent_rec *rec)
{
	/*
	 * Cluster count in extent records is slightly different
	 * between interior nodes and leaf nodes. This is to support
	 * unwritten extents which need a flags field in leaf node
	 * records, thus shrinking the available space for a clusters
	 * field.
	 */
	if (el->l_tree_depth)
		return le32_to_cpu(rec->e_int_clusters);
	else
		return le16_to_cpu(rec->e_leaf_clusters);
}

static inline int ocfs2_is_empty_extent(struct ocfs2_extent_rec *rec)
{
	return !rec->e_leaf_clusters;
}

#endif /* OCFS2_ALLOC_H */
