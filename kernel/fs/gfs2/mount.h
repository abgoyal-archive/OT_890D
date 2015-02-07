

#ifndef __MOUNT_DOT_H__
#define __MOUNT_DOT_H__

struct gfs2_sbd;

int gfs2_mount_args(struct gfs2_sbd *sdp, char *data_arg, int remount);

#endif /* __MOUNT_DOT_H__ */
