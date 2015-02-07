

#ifndef _LINUX_FIREWIRE_CDEV_H
#define _LINUX_FIREWIRE_CDEV_H

#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/firewire-constants.h>

#define FW_CDEV_EVENT_BUS_RESET		0x00
#define FW_CDEV_EVENT_RESPONSE		0x01
#define FW_CDEV_EVENT_REQUEST		0x02
#define FW_CDEV_EVENT_ISO_INTERRUPT	0x03

struct fw_cdev_event_common {
	__u64 closure;
	__u32 type;
};

struct fw_cdev_event_bus_reset {
	__u64 closure;
	__u32 type;
	__u32 node_id;
	__u32 local_node_id;
	__u32 bm_node_id;
	__u32 irm_node_id;
	__u32 root_node_id;
	__u32 generation;
};

struct fw_cdev_event_response {
	__u64 closure;
	__u32 type;
	__u32 rcode;
	__u32 length;
	__u32 data[0];
};

struct fw_cdev_event_request {
	__u64 closure;
	__u32 type;
	__u32 tcode;
	__u64 offset;
	__u32 handle;
	__u32 length;
	__u32 data[0];
};

struct fw_cdev_event_iso_interrupt {
	__u64 closure;
	__u32 type;
	__u32 cycle;
	__u32 header_length;
	__u32 header[0];
};

union fw_cdev_event {
	struct fw_cdev_event_common common;
	struct fw_cdev_event_bus_reset bus_reset;
	struct fw_cdev_event_response response;
	struct fw_cdev_event_request request;
	struct fw_cdev_event_iso_interrupt iso_interrupt;
};

#define FW_CDEV_IOC_GET_INFO		_IOWR('#', 0x00, struct fw_cdev_get_info)
#define FW_CDEV_IOC_SEND_REQUEST	_IOW('#', 0x01, struct fw_cdev_send_request)
#define FW_CDEV_IOC_ALLOCATE		_IOWR('#', 0x02, struct fw_cdev_allocate)
#define FW_CDEV_IOC_DEALLOCATE		_IOW('#', 0x03, struct fw_cdev_deallocate)
#define FW_CDEV_IOC_SEND_RESPONSE	_IOW('#', 0x04, struct fw_cdev_send_response)
#define FW_CDEV_IOC_INITIATE_BUS_RESET	_IOW('#', 0x05, struct fw_cdev_initiate_bus_reset)
#define FW_CDEV_IOC_ADD_DESCRIPTOR	_IOWR('#', 0x06, struct fw_cdev_add_descriptor)
#define FW_CDEV_IOC_REMOVE_DESCRIPTOR	_IOW('#', 0x07, struct fw_cdev_remove_descriptor)

#define FW_CDEV_IOC_CREATE_ISO_CONTEXT	_IOWR('#', 0x08, struct fw_cdev_create_iso_context)
#define FW_CDEV_IOC_QUEUE_ISO		_IOWR('#', 0x09, struct fw_cdev_queue_iso)
#define FW_CDEV_IOC_START_ISO		_IOW('#', 0x0a, struct fw_cdev_start_iso)
#define FW_CDEV_IOC_STOP_ISO		_IOW('#', 0x0b, struct fw_cdev_stop_iso)
#define FW_CDEV_IOC_GET_CYCLE_TIMER	_IOR('#', 0x0c, struct fw_cdev_get_cycle_timer)

#define FW_CDEV_VERSION		1

struct fw_cdev_get_info {
	__u32 version;
	__u32 rom_length;
	__u64 rom;
	__u64 bus_reset;
	__u64 bus_reset_closure;
	__u32 card;
};

struct fw_cdev_send_request {
	__u32 tcode;
	__u32 length;
	__u64 offset;
	__u64 closure;
	__u64 data;
	__u32 generation;
};

struct fw_cdev_send_response {
	__u32 rcode;
	__u32 length;
	__u64 data;
	__u32 handle;
};

struct fw_cdev_allocate {
	__u64 offset;
	__u64 closure;
	__u32 length;
	__u32 handle;
};

struct fw_cdev_deallocate {
	__u32 handle;
};

#define FW_CDEV_LONG_RESET	0
#define FW_CDEV_SHORT_RESET	1

struct fw_cdev_initiate_bus_reset {
	__u32 type;	/* FW_CDEV_SHORT_RESET or FW_CDEV_LONG_RESET */
};

struct fw_cdev_add_descriptor {
	__u32 immediate;
	__u32 key;
	__u64 data;
	__u32 length;
	__u32 handle;
};

struct fw_cdev_remove_descriptor {
	__u32 handle;
};

#define FW_CDEV_ISO_CONTEXT_TRANSMIT	0
#define FW_CDEV_ISO_CONTEXT_RECEIVE	1

struct fw_cdev_create_iso_context {
	__u32 type;
	__u32 header_size;
	__u32 channel;
	__u32 speed;
	__u64 closure;
	__u32 handle;
};

#define FW_CDEV_ISO_PAYLOAD_LENGTH(v)	(v)
#define FW_CDEV_ISO_INTERRUPT		(1 << 16)
#define FW_CDEV_ISO_SKIP		(1 << 17)
#define FW_CDEV_ISO_SYNC		(1 << 17)
#define FW_CDEV_ISO_TAG(v)		((v) << 18)
#define FW_CDEV_ISO_SY(v)		((v) << 20)
#define FW_CDEV_ISO_HEADER_LENGTH(v)	((v) << 24)

struct fw_cdev_iso_packet {
	__u32 control;
	__u32 header[0];
};

struct fw_cdev_queue_iso {
	__u64 packets;
	__u64 data;
	__u32 size;
	__u32 handle;
};

#define FW_CDEV_ISO_CONTEXT_MATCH_TAG0		 1
#define FW_CDEV_ISO_CONTEXT_MATCH_TAG1		 2
#define FW_CDEV_ISO_CONTEXT_MATCH_TAG2		 4
#define FW_CDEV_ISO_CONTEXT_MATCH_TAG3		 8
#define FW_CDEV_ISO_CONTEXT_MATCH_ALL_TAGS	15

struct fw_cdev_start_iso {
	__s32 cycle;
	__u32 sync;
	__u32 tags;
	__u32 handle;
};

struct fw_cdev_stop_iso {
	__u32 handle;
};

struct fw_cdev_get_cycle_timer {
	__u64 local_time;
	__u32 cycle_timer;
};

#endif /* _LINUX_FIREWIRE_CDEV_H */
