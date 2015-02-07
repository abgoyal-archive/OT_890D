
#ifndef _LINUX_VIRTIO_CONSOLE_H
#define _LINUX_VIRTIO_CONSOLE_H
#include <linux/types.h>
#include <linux/virtio_config.h>

/* The ID for virtio console */
#define VIRTIO_ID_CONSOLE	3

/* Feature bits */
#define VIRTIO_CONSOLE_F_SIZE	0	/* Does host provide console size? */

struct virtio_console_config {
	/* colums of the screens */
	__u16 cols;
	/* rows of the screens */
	__u16 rows;
} __attribute__((packed));


#ifdef __KERNEL__
int __init virtio_cons_early_init(int (*put_chars)(u32, const char *, int));
#endif /* __KERNEL__ */

#endif /* _LINUX_VIRTIO_CONSOLE_H */
