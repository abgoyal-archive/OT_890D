

#ifndef __LINUX_USB_SERIAL_IPAQ_H
#define __LINUX_USB_SERIAL_IPAQ_H


struct ipaq_packet {
	char			*data;
	size_t			len;
	size_t			written;
	struct list_head	list;
};

struct ipaq_private {
	int			active;
	int			queue_len;
	int			free_len;
	struct list_head	queue;
	struct list_head	freelist;
};

#define URBDATA_SIZE		4096
#define URBDATA_QUEUE_MAX	(64 * 1024)
#define PACKET_SIZE		256

#endif
