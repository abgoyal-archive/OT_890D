

#ifndef __CONNECTOR_H
#define __CONNECTOR_H

#include <linux/types.h>

#define CN_IDX_CONNECTOR		0xffffffff
#define CN_VAL_CONNECTOR		0xffffffff

#define CN_IDX_PROC			0x1
#define CN_VAL_PROC			0x1
#define CN_IDX_CIFS			0x2
#define CN_VAL_CIFS                     0x1
#define CN_W1_IDX			0x3	/* w1 communication */
#define CN_W1_VAL			0x1
#define CN_IDX_V86D			0x4
#define CN_VAL_V86D_UVESAFB		0x1
#define CN_IDX_BB			0x5	/* BlackBoard, from the TSP GPL sampling framework */

#define CN_NETLINK_USERS		6

#define CONNECTOR_MAX_MSG_SIZE		16384


struct cb_id {
	__u32 idx;
	__u32 val;
};

struct cn_msg {
	struct cb_id id;

	__u32 seq;
	__u32 ack;

	__u16 len;		/* Length of the following data */
	__u16 flags;
	__u8 data[0];
};

struct cn_notify_req {
	__u32 first;
	__u32 range;
};

struct cn_ctl_msg {
	__u32 idx_notify_num;
	__u32 val_notify_num;
	__u32 group;
	__u32 len;
	__u8 data[0];
};

#ifdef __KERNEL__

#include <asm/atomic.h>

#include <linux/list.h>
#include <linux/workqueue.h>

#include <net/sock.h>

#define CN_CBQ_NAMELEN		32

struct cn_queue_dev {
	atomic_t refcnt;
	unsigned char name[CN_CBQ_NAMELEN];

	struct workqueue_struct *cn_queue;

	struct list_head queue_list;
	spinlock_t queue_lock;

	struct sock *nls;
};

struct cn_callback_id {
	unsigned char name[CN_CBQ_NAMELEN];
	struct cb_id id;
};

struct cn_callback_data {
	void (*destruct_data) (void *);
	void *ddata;
	
	void *callback_priv;
	void (*callback) (void *);

	void *free;
};

struct cn_callback_entry {
	struct list_head callback_entry;
	struct work_struct work;
	struct cn_queue_dev *pdev;

	struct cn_callback_id id;
	struct cn_callback_data data;

	u32 seq, group;
};

struct cn_ctl_entry {
	struct list_head notify_entry;
	struct cn_ctl_msg *msg;
};

struct cn_dev {
	struct cb_id id;

	u32 seq, groups;
	struct sock *nls;
	void (*input) (struct sk_buff *skb);

	struct cn_queue_dev *cbdev;
};

int cn_add_callback(struct cb_id *, char *, void (*callback) (void *));
void cn_del_callback(struct cb_id *);
int cn_netlink_send(struct cn_msg *, u32, gfp_t);

int cn_queue_add_callback(struct cn_queue_dev *dev, char *name, struct cb_id *id, void (*callback)(void *));
void cn_queue_del_callback(struct cn_queue_dev *dev, struct cb_id *id);

struct cn_queue_dev *cn_queue_alloc_dev(char *name, struct sock *);
void cn_queue_free_dev(struct cn_queue_dev *dev);

int cn_cb_equal(struct cb_id *, struct cb_id *);

void cn_queue_wrapper(struct work_struct *work);

#endif				/* __KERNEL__ */
#endif				/* __CONNECTOR_H */
