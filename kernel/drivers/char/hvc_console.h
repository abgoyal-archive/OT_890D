

#ifndef HVC_CONSOLE_H
#define HVC_CONSOLE_H
#include <linux/kref.h>
#include <linux/tty.h>

#define MAX_NR_HVC_CONSOLES	16

#define HVC_ALLOC_TTY_ADAPTERS	8

struct hvc_struct {
	spinlock_t lock;
	int index;
	struct tty_struct *tty;
	int count;
	int do_wakeup;
	char *outbuf;
	int outbuf_size;
	int n_outbuf;
	uint32_t vtermno;
	struct hv_ops *ops;
	int irq_requested;
	int data;
	struct winsize ws;
	struct work_struct tty_resize;
	struct list_head next;
	struct kref kref; /* ref count & hvc_struct lifetime */
};

/* implemented by a low level driver */
struct hv_ops {
	int (*get_chars)(uint32_t vtermno, char *buf, int count);
	int (*put_chars)(uint32_t vtermno, const char *buf, int count);

	/* Callbacks for notification. Called in open, close and hangup */
	int (*notifier_add)(struct hvc_struct *hp, int irq);
	void (*notifier_del)(struct hvc_struct *hp, int irq);
	void (*notifier_hangup)(struct hvc_struct *hp, int irq);
};

/* Register a vterm and a slot index for use as a console (console_init) */
extern int hvc_instantiate(uint32_t vtermno, int index, struct hv_ops *ops);

/* register a vterm for hvc tty operation (module_init or hotplug add) */
extern struct hvc_struct * __devinit hvc_alloc(uint32_t vtermno, int data,
				struct hv_ops *ops, int outbuf_size);
/* remove a vterm from hvc tty operation (module_exit or hotplug remove) */
extern int hvc_remove(struct hvc_struct *hp);

/* data available */
int hvc_poll(struct hvc_struct *hp);
void hvc_kick(void);

/* Resize hvc tty terminal window */
extern void hvc_resize(struct hvc_struct *hp, struct winsize ws);

/* default notifier for irq based notification */
extern int notifier_add_irq(struct hvc_struct *hp, int data);
extern void notifier_del_irq(struct hvc_struct *hp, int data);
extern void notifier_hangup_irq(struct hvc_struct *hp, int data);


#if defined(CONFIG_XMON) && defined(CONFIG_SMP)
#include <asm/xmon.h>
#else
static inline int cpus_are_in_xmon(void)
{
	return 0;
}
#endif

#endif // HVC_CONSOLE_H
