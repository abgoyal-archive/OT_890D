

#ifndef OCFS2_UPTODATE_H
#define OCFS2_UPTODATE_H

int __init init_ocfs2_uptodate_cache(void);
void exit_ocfs2_uptodate_cache(void);

void ocfs2_metadata_cache_init(struct inode *inode);
void ocfs2_metadata_cache_purge(struct inode *inode);

int ocfs2_buffer_uptodate(struct inode *inode,
			  struct buffer_head *bh);
void ocfs2_set_buffer_uptodate(struct inode *inode,
			       struct buffer_head *bh);
void ocfs2_set_new_buffer_uptodate(struct inode *inode,
				   struct buffer_head *bh);
void ocfs2_remove_from_cache(struct inode *inode,
			     struct buffer_head *bh);
void ocfs2_remove_xattr_clusters_from_cache(struct inode *inode,
					    sector_t block,
					    u32 c_len);
int ocfs2_buffer_read_ahead(struct inode *inode,
			    struct buffer_head *bh);

#endif /* OCFS2_UPTODATE_H */
