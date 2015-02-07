

#ifndef __EAOPS_DOT_H__
#define __EAOPS_DOT_H__

struct gfs2_ea_request;
struct gfs2_inode;

struct gfs2_eattr_operations {
	int (*eo_get) (struct gfs2_inode *ip, struct gfs2_ea_request *er);
	int (*eo_set) (struct gfs2_inode *ip, struct gfs2_ea_request *er);
	int (*eo_remove) (struct gfs2_inode *ip, struct gfs2_ea_request *er);
	char *eo_name;
};

unsigned int gfs2_ea_name2type(const char *name, const char **truncated_name);

extern const struct gfs2_eattr_operations gfs2_system_eaops;

extern const struct gfs2_eattr_operations *gfs2_ea_ops[];

#endif /* __EAOPS_DOT_H__ */

