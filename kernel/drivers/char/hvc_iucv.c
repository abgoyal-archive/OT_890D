
#define KMSG_COMPONENT		"hvc_iucv"
#define pr_fmt(fmt)		KMSG_COMPONENT ": " fmt

#include <linux/types.h>
#include <asm/ebcdic.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/mempool.h>
#include <linux/module.h>
#include <linux/tty.h>
#include <linux/wait.h>
#include <net/iucv/iucv.h>

#include "hvc_console.h"


/* General device driver settings */
#define HVC_IUCV_MAGIC		0xc9e4c3e5
#define MAX_HVC_IUCV_LINES	HVC_ALLOC_TTY_ADAPTERS
#define MEMPOOL_MIN_NR		(PAGE_SIZE / sizeof(struct iucv_tty_buffer)/4)

/* IUCV TTY message  */
#define MSG_VERSION		0x02	/* Message version */
#define MSG_TYPE_ERROR		0x01	/* Error message */
#define MSG_TYPE_TERMENV	0x02	/* Terminal environment variable */
#define MSG_TYPE_TERMIOS	0x04	/* Terminal IO struct update */
#define MSG_TYPE_WINSIZE	0x08	/* Terminal window size update */
#define MSG_TYPE_DATA		0x10	/* Terminal data */

struct iucv_tty_msg {
	u8	version;		/* Message version */
	u8	type;			/* Message type */
#define MSG_MAX_DATALEN		((u16)(~0))
	u16	datalen;		/* Payload length */
	u8	data[];			/* Payload buffer */
} __attribute__((packed));
#define MSG_SIZE(s)		((s) + offsetof(struct iucv_tty_msg, data))

enum iucv_state_t {
	IUCV_DISCONN	= 0,
	IUCV_CONNECTED	= 1,
	IUCV_SEVERED	= 2,
};

enum tty_state_t {
	TTY_CLOSED	= 0,
	TTY_OPENED	= 1,
};

struct hvc_iucv_private {
	struct hvc_struct	*hvc;		/* HVC struct reference */
	u8			srv_name[8];	/* IUCV service name (ebcdic) */
	unsigned char		is_console;	/* Linux console usage flag */
	enum iucv_state_t	iucv_state;	/* IUCV connection status */
	enum tty_state_t	tty_state;	/* TTY status */
	struct iucv_path	*path;		/* IUCV path pointer */
	spinlock_t		lock;		/* hvc_iucv_private lock */
#define SNDBUF_SIZE		(PAGE_SIZE)	/* must be < MSG_MAX_DATALEN */
	void			*sndbuf;	/* send buffer		  */
	size_t			sndbuf_len;	/* length of send buffer  */
#define QUEUE_SNDBUF_DELAY	(HZ / 25)
	struct delayed_work	sndbuf_work;	/* work: send iucv msg(s) */
	wait_queue_head_t	sndbuf_waitq;	/* wait for send completion */
	struct list_head	tty_outqueue;	/* outgoing IUCV messages */
	struct list_head	tty_inqueue;	/* incoming IUCV messages */
};

struct iucv_tty_buffer {
	struct list_head	list;	/* list pointer */
	struct iucv_message	msg;	/* store an IUCV message */
	size_t			offset;	/* data buffer offset */
	struct iucv_tty_msg	*mbuf;	/* buffer to store input/output data */
};

/* IUCV callback handler */
static	int hvc_iucv_path_pending(struct iucv_path *, u8[8], u8[16]);
static void hvc_iucv_path_severed(struct iucv_path *, u8[16]);
static void hvc_iucv_msg_pending(struct iucv_path *, struct iucv_message *);
static void hvc_iucv_msg_complete(struct iucv_path *, struct iucv_message *);


/* Kernel module parameter: use one terminal device as default */
static unsigned long hvc_iucv_devices = 1;

/* Array of allocated hvc iucv tty lines... */
static struct hvc_iucv_private *hvc_iucv_table[MAX_HVC_IUCV_LINES];
#define IUCV_HVC_CON_IDX	(0)

/* Kmem cache and mempool for iucv_tty_buffer elements */
static struct kmem_cache *hvc_iucv_buffer_cache;
static mempool_t *hvc_iucv_mempool;

/* IUCV handler callback functions */
static struct iucv_handler hvc_iucv_handler = {
	.path_pending  = hvc_iucv_path_pending,
	.path_severed  = hvc_iucv_path_severed,
	.message_complete = hvc_iucv_msg_complete,
	.message_pending  = hvc_iucv_msg_pending,
};


struct hvc_iucv_private *hvc_iucv_get_private(uint32_t num)
{
	if ((num < HVC_IUCV_MAGIC) || (num - HVC_IUCV_MAGIC > hvc_iucv_devices))
		return NULL;
	return hvc_iucv_table[num - HVC_IUCV_MAGIC];
}

static struct iucv_tty_buffer *alloc_tty_buffer(size_t size, gfp_t flags)
{
	struct iucv_tty_buffer *bufp;

	bufp = mempool_alloc(hvc_iucv_mempool, flags);
	if (!bufp)
		return NULL;
	memset(bufp, 0, sizeof(*bufp));

	if (size > 0) {
		bufp->msg.length = MSG_SIZE(size);
		bufp->mbuf = kmalloc(bufp->msg.length, flags);
		if (!bufp->mbuf) {
			mempool_free(bufp, hvc_iucv_mempool);
			return NULL;
		}
		bufp->mbuf->version = MSG_VERSION;
		bufp->mbuf->type    = MSG_TYPE_DATA;
		bufp->mbuf->datalen = (u16) size;
	}
	return bufp;
}

static void destroy_tty_buffer(struct iucv_tty_buffer *bufp)
{
	kfree(bufp->mbuf);
	mempool_free(bufp, hvc_iucv_mempool);
}

static void destroy_tty_buffer_list(struct list_head *list)
{
	struct iucv_tty_buffer *ent, *next;

	list_for_each_entry_safe(ent, next, list, list) {
		list_del(&ent->list);
		destroy_tty_buffer(ent);
	}
}

static int hvc_iucv_write(struct hvc_iucv_private *priv,
			  char *buf, int count, int *has_more_data)
{
	struct iucv_tty_buffer *rb;
	int written;
	int rc;

	/* immediately return if there is no IUCV connection */
	if (priv->iucv_state == IUCV_DISCONN)
		return 0;

	/* if the IUCV path has been severed, return -EPIPE to inform the
	 * HVC layer to hang up the tty device. */
	if (priv->iucv_state == IUCV_SEVERED)
		return -EPIPE;

	/* check if there are pending messages */
	if (list_empty(&priv->tty_inqueue))
		return 0;

	/* receive an iucv message and flip data to the tty (ldisc) */
	rb = list_first_entry(&priv->tty_inqueue, struct iucv_tty_buffer, list);

	written = 0;
	if (!rb->mbuf) { /* message not yet received ... */
		/* allocate mem to store msg data; if no memory is available
		 * then leave the buffer on the list and re-try later */
		rb->mbuf = kmalloc(rb->msg.length, GFP_ATOMIC);
		if (!rb->mbuf)
			return -ENOMEM;

		rc = __iucv_message_receive(priv->path, &rb->msg, 0,
					    rb->mbuf, rb->msg.length, NULL);
		switch (rc) {
		case 0: /* Successful	    */
			break;
		case 2:	/* No message found */
		case 9: /* Message purged   */
			break;
		default:
			written = -EIO;
		}
		/* remove buffer if an error has occured or received data
		 * is not correct */
		if (rc || (rb->mbuf->version != MSG_VERSION) ||
			  (rb->msg.length    != MSG_SIZE(rb->mbuf->datalen)))
			goto out_remove_buffer;
	}

	switch (rb->mbuf->type) {
	case MSG_TYPE_DATA:
		written = min_t(int, rb->mbuf->datalen - rb->offset, count);
		memcpy(buf, rb->mbuf->data + rb->offset, written);
		if (written < (rb->mbuf->datalen - rb->offset)) {
			rb->offset += written;
			*has_more_data = 1;
			goto out_written;
		}
		break;

	case MSG_TYPE_WINSIZE:
		if (rb->mbuf->datalen != sizeof(struct winsize))
			break;
		hvc_resize(priv->hvc, *((struct winsize *) rb->mbuf->data));
		break;

	case MSG_TYPE_ERROR:	/* ignored ... */
	case MSG_TYPE_TERMENV:	/* ignored ... */
	case MSG_TYPE_TERMIOS:	/* ignored ... */
		break;
	}

out_remove_buffer:
	list_del(&rb->list);
	destroy_tty_buffer(rb);
	*has_more_data = !list_empty(&priv->tty_inqueue);

out_written:
	return written;
}

static int hvc_iucv_get_chars(uint32_t vtermno, char *buf, int count)
{
	struct hvc_iucv_private *priv = hvc_iucv_get_private(vtermno);
	int written;
	int has_more_data;

	if (count <= 0)
		return 0;

	if (!priv)
		return -ENODEV;

	spin_lock(&priv->lock);
	has_more_data = 0;
	written = hvc_iucv_write(priv, buf, count, &has_more_data);
	spin_unlock(&priv->lock);

	/* if there are still messages on the queue... schedule another run */
	if (has_more_data)
		hvc_kick();

	return written;
}

static int hvc_iucv_queue(struct hvc_iucv_private *priv, const char *buf,
			  int count)
{
	size_t len;

	if (priv->iucv_state == IUCV_DISCONN)
		return count;			/* ignore data */

	if (priv->iucv_state == IUCV_SEVERED)
		return -EPIPE;

	len = min_t(size_t, count, SNDBUF_SIZE - priv->sndbuf_len);
	if (!len)
		return 0;

	memcpy(priv->sndbuf + priv->sndbuf_len, buf, len);
	priv->sndbuf_len += len;

	if (priv->iucv_state == IUCV_CONNECTED)
		schedule_delayed_work(&priv->sndbuf_work, QUEUE_SNDBUF_DELAY);

	return len;
}

static int hvc_iucv_send(struct hvc_iucv_private *priv)
{
	struct iucv_tty_buffer *sb;
	int rc, len;

	if (priv->iucv_state == IUCV_SEVERED)
		return -EPIPE;

	if (priv->iucv_state == IUCV_DISCONN)
		return -EIO;

	if (!priv->sndbuf_len)
		return 0;

	/* allocate internal buffer to store msg data and also compute total
	 * message length */
	sb = alloc_tty_buffer(priv->sndbuf_len, GFP_ATOMIC);
	if (!sb)
		return -ENOMEM;

	memcpy(sb->mbuf->data, priv->sndbuf, priv->sndbuf_len);
	sb->mbuf->datalen = (u16) priv->sndbuf_len;
	sb->msg.length = MSG_SIZE(sb->mbuf->datalen);

	list_add_tail(&sb->list, &priv->tty_outqueue);

	rc = __iucv_message_send(priv->path, &sb->msg, 0, 0,
				 (void *) sb->mbuf, sb->msg.length);
	if (rc) {
		/* drop the message here; however we might want to handle
		 * 0x03 (msg limit reached) by trying again... */
		list_del(&sb->list);
		destroy_tty_buffer(sb);
	}
	len = priv->sndbuf_len;
	priv->sndbuf_len = 0;

	return len;
}

static void hvc_iucv_sndbuf_work(struct work_struct *work)
{
	struct hvc_iucv_private *priv;

	priv = container_of(work, struct hvc_iucv_private, sndbuf_work.work);
	if (!priv)
		return;

	spin_lock_bh(&priv->lock);
	hvc_iucv_send(priv);
	spin_unlock_bh(&priv->lock);
}

static int hvc_iucv_put_chars(uint32_t vtermno, const char *buf, int count)
{
	struct hvc_iucv_private *priv = hvc_iucv_get_private(vtermno);
	int queued;

	if (count <= 0)
		return 0;

	if (!priv)
		return -ENODEV;

	spin_lock(&priv->lock);
	queued = hvc_iucv_queue(priv, buf, count);
	spin_unlock(&priv->lock);

	return queued;
}

static int hvc_iucv_notifier_add(struct hvc_struct *hp, int id)
{
	struct hvc_iucv_private *priv;

	priv = hvc_iucv_get_private(id);
	if (!priv)
		return 0;

	spin_lock_bh(&priv->lock);
	priv->tty_state = TTY_OPENED;
	spin_unlock_bh(&priv->lock);

	return 0;
}

static void hvc_iucv_cleanup(struct hvc_iucv_private *priv)
{
	destroy_tty_buffer_list(&priv->tty_outqueue);
	destroy_tty_buffer_list(&priv->tty_inqueue);

	priv->tty_state = TTY_CLOSED;
	priv->iucv_state = IUCV_DISCONN;

	priv->sndbuf_len = 0;
}

static inline int tty_outqueue_empty(struct hvc_iucv_private *priv)
{
	int rc;

	spin_lock_bh(&priv->lock);
	rc = list_empty(&priv->tty_outqueue);
	spin_unlock_bh(&priv->lock);

	return rc;
}

static void flush_sndbuf_sync(struct hvc_iucv_private *priv)
{
	int sync_wait;

	cancel_delayed_work_sync(&priv->sndbuf_work);

	spin_lock_bh(&priv->lock);
	hvc_iucv_send(priv);		/* force sending buffered data */
	sync_wait = !list_empty(&priv->tty_outqueue); /* anything queued ? */
	spin_unlock_bh(&priv->lock);

	if (sync_wait)
		wait_event_timeout(priv->sndbuf_waitq,
				   tty_outqueue_empty(priv), HZ);
}

static void hvc_iucv_notifier_hangup(struct hvc_struct *hp, int id)
{
	struct hvc_iucv_private *priv;

	priv = hvc_iucv_get_private(id);
	if (!priv)
		return;

	flush_sndbuf_sync(priv);

	spin_lock_bh(&priv->lock);
	/* NOTE: If the hangup was scheduled by ourself (from the iucv
	 *	 path_servered callback [IUCV_SEVERED]), we have to clean up
	 *	 our structure and to set state to TTY_CLOSED.
	 *	 If the tty was hung up otherwise (e.g. vhangup()), then we
	 *	 ignore this hangup and keep an established IUCV path open...
	 *	 (...the reason is that we are not able to connect back to the
	 *	 client if we disconnect on hang up) */
	priv->tty_state = TTY_CLOSED;

	if (priv->iucv_state == IUCV_SEVERED)
		hvc_iucv_cleanup(priv);
	spin_unlock_bh(&priv->lock);
}

static void hvc_iucv_notifier_del(struct hvc_struct *hp, int id)
{
	struct hvc_iucv_private *priv;
	struct iucv_path	*path;

	priv = hvc_iucv_get_private(id);
	if (!priv)
		return;

	flush_sndbuf_sync(priv);

	spin_lock_bh(&priv->lock);
	path = priv->path;		/* save reference to IUCV path */
	priv->path = NULL;
	hvc_iucv_cleanup(priv);
	spin_unlock_bh(&priv->lock);

	/* sever IUCV path outside of priv->lock due to lock ordering of:
	 * priv->lock <--> iucv_table_lock */
	if (path) {
		iucv_path_sever(path, NULL);
		iucv_path_free(path);
	}
}

static	int hvc_iucv_path_pending(struct iucv_path *path,
				  u8 ipvmid[8], u8 ipuser[16])
{
	struct hvc_iucv_private *priv;
	u8 nuser_data[16];
	int i, rc;

	priv = NULL;
	for (i = 0; i < hvc_iucv_devices; i++)
		if (hvc_iucv_table[i] &&
		    (0 == memcmp(hvc_iucv_table[i]->srv_name, ipuser, 8))) {
			priv = hvc_iucv_table[i];
			break;
		}
	if (!priv)
		return -ENODEV;

	spin_lock(&priv->lock);

	/* If the terminal is already connected or being severed, then sever
	 * this path to enforce that there is only ONE established communication
	 * path per terminal. */
	if (priv->iucv_state != IUCV_DISCONN) {
		iucv_path_sever(path, ipuser);
		iucv_path_free(path);
		goto out_path_handled;
	}

	/* accept path */
	memcpy(nuser_data, ipuser + 8, 8);  /* remote service (for af_iucv) */
	memcpy(nuser_data + 8, ipuser, 8);  /* local service  (for af_iucv) */
	path->msglim = 0xffff;		    /* IUCV MSGLIMIT */
	path->flags &= ~IUCV_IPRMDATA;	    /* TODO: use IUCV_IPRMDATA */
	rc = iucv_path_accept(path, &hvc_iucv_handler, nuser_data, priv);
	if (rc) {
		iucv_path_sever(path, ipuser);
		iucv_path_free(path);
		goto out_path_handled;
	}
	priv->path = path;
	priv->iucv_state = IUCV_CONNECTED;

	/* flush buffered output data... */
	schedule_delayed_work(&priv->sndbuf_work, 5);

out_path_handled:
	spin_unlock(&priv->lock);
	return 0;
}

static void hvc_iucv_path_severed(struct iucv_path *path, u8 ipuser[16])
{
	struct hvc_iucv_private *priv = path->private;

	spin_lock(&priv->lock);
	priv->iucv_state = IUCV_SEVERED;

	/* If the tty has not yet been opened, clean up the hvc_iucv_private
	 * structure to allow re-connects.
	 * This is also done for our console device because console hangups
	 * are handled specially and no notifier is called by HVC.
	 * The tty session is active (TTY_OPEN) and ready for re-connects...
	 *
	 * If it has been opened, let get_chars() return -EPIPE to signal the
	 * HVC layer to hang up the tty.
	 * If so, we need to wake up the HVC thread to call get_chars()...
	 */
	priv->path = NULL;
	if (priv->tty_state == TTY_CLOSED)
		hvc_iucv_cleanup(priv);
	else
		if (priv->is_console) {
			hvc_iucv_cleanup(priv);
			priv->tty_state = TTY_OPENED;
		} else
			hvc_kick();
	spin_unlock(&priv->lock);

	/* finally sever path (outside of priv->lock due to lock ordering) */
	iucv_path_sever(path, ipuser);
	iucv_path_free(path);
}

static void hvc_iucv_msg_pending(struct iucv_path *path,
				 struct iucv_message *msg)
{
	struct hvc_iucv_private *priv = path->private;
	struct iucv_tty_buffer *rb;

	/* reject messages that exceed max size of iucv_tty_msg->datalen */
	if (msg->length > MSG_SIZE(MSG_MAX_DATALEN)) {
		iucv_message_reject(path, msg);
		return;
	}

	spin_lock(&priv->lock);

	/* reject messages if tty has not yet been opened */
	if (priv->tty_state == TTY_CLOSED) {
		iucv_message_reject(path, msg);
		goto unlock_return;
	}

	/* allocate tty buffer to save iucv msg only */
	rb = alloc_tty_buffer(0, GFP_ATOMIC);
	if (!rb) {
		iucv_message_reject(path, msg);
		goto unlock_return;	/* -ENOMEM */
	}
	rb->msg = *msg;

	list_add_tail(&rb->list, &priv->tty_inqueue);

	hvc_kick();	/* wake up hvc thread */

unlock_return:
	spin_unlock(&priv->lock);
}

static void hvc_iucv_msg_complete(struct iucv_path *path,
				  struct iucv_message *msg)
{
	struct hvc_iucv_private *priv = path->private;
	struct iucv_tty_buffer	*ent, *next;
	LIST_HEAD(list_remove);

	spin_lock(&priv->lock);
	list_for_each_entry_safe(ent, next, &priv->tty_outqueue, list)
		if (ent->msg.id == msg->id) {
			list_move(&ent->list, &list_remove);
			break;
		}
	wake_up(&priv->sndbuf_waitq);
	spin_unlock(&priv->lock);
	destroy_tty_buffer_list(&list_remove);
}


/* HVC operations */
static struct hv_ops hvc_iucv_ops = {
	.get_chars = hvc_iucv_get_chars,
	.put_chars = hvc_iucv_put_chars,
	.notifier_add = hvc_iucv_notifier_add,
	.notifier_del = hvc_iucv_notifier_del,
	.notifier_hangup = hvc_iucv_notifier_hangup,
};

static int __init hvc_iucv_alloc(int id, unsigned int is_console)
{
	struct hvc_iucv_private *priv;
	char name[9];
	int rc;

	priv = kzalloc(sizeof(struct hvc_iucv_private), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	spin_lock_init(&priv->lock);
	INIT_LIST_HEAD(&priv->tty_outqueue);
	INIT_LIST_HEAD(&priv->tty_inqueue);
	INIT_DELAYED_WORK(&priv->sndbuf_work, hvc_iucv_sndbuf_work);
	init_waitqueue_head(&priv->sndbuf_waitq);

	priv->sndbuf = (void *) get_zeroed_page(GFP_KERNEL);
	if (!priv->sndbuf) {
		kfree(priv);
		return -ENOMEM;
	}

	/* set console flag */
	priv->is_console = is_console;

	/* finally allocate hvc */
	priv->hvc = hvc_alloc(HVC_IUCV_MAGIC + id, /*		  PAGE_SIZE */
			      HVC_IUCV_MAGIC + id, &hvc_iucv_ops, 256);
	if (IS_ERR(priv->hvc)) {
		rc = PTR_ERR(priv->hvc);
		free_page((unsigned long) priv->sndbuf);
		kfree(priv);
		return rc;
	}

	/* notify HVC thread instead of using polling */
	priv->hvc->irq_requested = 1;

	/* setup iucv related information */
	snprintf(name, 9, "lnxhvc%-2d", id);
	memcpy(priv->srv_name, name, 8);
	ASCEBC(priv->srv_name, 8);

	hvc_iucv_table[id] = priv;
	return 0;
}

static int __init hvc_iucv_init(void)
{
	int rc;
	unsigned int i;

	if (!MACHINE_IS_VM) {
		pr_info("The z/VM IUCV HVC device driver cannot "
			   "be used without z/VM\n");
		return -ENODEV;
	}

	if (!hvc_iucv_devices)
		return -ENODEV;

	if (hvc_iucv_devices > MAX_HVC_IUCV_LINES)
		return -EINVAL;

	hvc_iucv_buffer_cache = kmem_cache_create(KMSG_COMPONENT,
					   sizeof(struct iucv_tty_buffer),
					   0, 0, NULL);
	if (!hvc_iucv_buffer_cache) {
		pr_err("Allocating memory failed with reason code=%d\n", 1);
		return -ENOMEM;
	}

	hvc_iucv_mempool = mempool_create_slab_pool(MEMPOOL_MIN_NR,
						    hvc_iucv_buffer_cache);
	if (!hvc_iucv_mempool) {
		pr_err("Allocating memory failed with reason code=%d\n", 2);
		kmem_cache_destroy(hvc_iucv_buffer_cache);
		return -ENOMEM;
	}

	/* register the first terminal device as console
	 * (must be done before allocating hvc terminal devices) */
	rc = hvc_instantiate(HVC_IUCV_MAGIC, IUCV_HVC_CON_IDX, &hvc_iucv_ops);
	if (rc) {
		pr_err("Registering HVC terminal device as "
		       "Linux console failed\n");
		goto out_error_memory;
	}

	/* allocate hvc_iucv_private structs */
	for (i = 0; i < hvc_iucv_devices; i++) {
		rc = hvc_iucv_alloc(i, (i == IUCV_HVC_CON_IDX) ? 1 : 0);
		if (rc) {
			pr_err("Creating a new HVC terminal device "
				"failed with error code=%d\n", rc);
			goto out_error_hvc;
		}
	}

	/* register IUCV callback handler */
	rc = iucv_register(&hvc_iucv_handler, 0);
	if (rc) {
		pr_err("Registering IUCV handlers failed with error code=%d\n",
			rc);
		goto out_error_iucv;
	}

	return 0;

out_error_iucv:
	iucv_unregister(&hvc_iucv_handler, 0);
out_error_hvc:
	for (i = 0; i < hvc_iucv_devices; i++)
		if (hvc_iucv_table[i]) {
			if (hvc_iucv_table[i]->hvc)
				hvc_remove(hvc_iucv_table[i]->hvc);
			kfree(hvc_iucv_table[i]);
		}
out_error_memory:
	mempool_destroy(hvc_iucv_mempool);
	kmem_cache_destroy(hvc_iucv_buffer_cache);
	return rc;
}

static	int __init hvc_iucv_config(char *val)
{
	 return strict_strtoul(val, 10, &hvc_iucv_devices);
}


device_initcall(hvc_iucv_init);
__setup("hvc_iucv=", hvc_iucv_config);
