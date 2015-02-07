
#ifndef __USBHID_H
#define __USBHID_H



#include <linux/types.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/timer.h>
#include <linux/wait.h>
#include <linux/workqueue.h>
#include <linux/input.h>

/*  API provided by hid-core.c for USB HID drivers */
int usbhid_wait_io(struct hid_device* hid);
void usbhid_close(struct hid_device *hid);
int usbhid_open(struct hid_device *hid);
void usbhid_init_reports(struct hid_device *hid);
void usbhid_submit_report(struct hid_device *hid, struct hid_report *report, unsigned char dir);

/* iofl flags */
#define HID_CTRL_RUNNING	1
#define HID_OUT_RUNNING		2
#define HID_IN_RUNNING		3
#define HID_RESET_PENDING	4
#define HID_SUSPENDED		5
#define HID_CLEAR_HALT		6
#define HID_DISCONNECTED	7
#define HID_STARTED		8


struct usbhid_device {
	struct hid_device *hid;						/* pointer to corresponding HID dev */

	struct usb_interface *intf;                                     /* USB interface */
	int ifnum;                                                      /* USB interface number */

	unsigned int bufsize;                                           /* URB buffer size */

	struct urb *urbin;                                              /* Input URB */
	char *inbuf;                                                    /* Input buffer */
	dma_addr_t inbuf_dma;                                           /* Input buffer dma */
	spinlock_t inlock;                                              /* Input fifo spinlock */

	struct urb *urbctrl;                                            /* Control URB */
	struct usb_ctrlrequest *cr;                                     /* Control request struct */
	dma_addr_t cr_dma;                                              /* Control request struct dma */
	struct hid_control_fifo ctrl[HID_CONTROL_FIFO_SIZE];  		/* Control fifo */
	unsigned char ctrlhead, ctrltail;                               /* Control fifo head & tail */
	char *ctrlbuf;                                                  /* Control buffer */
	dma_addr_t ctrlbuf_dma;                                         /* Control buffer dma */
	spinlock_t ctrllock;                                            /* Control fifo spinlock */

	struct urb *urbout;                                             /* Output URB */
	struct hid_output_fifo out[HID_CONTROL_FIFO_SIZE];              /* Output pipe fifo */
	unsigned char outhead, outtail;                                 /* Output pipe fifo head & tail */
	char *outbuf;                                                   /* Output buffer */
	dma_addr_t outbuf_dma;                                          /* Output buffer dma */
	spinlock_t outlock;                                             /* Output fifo spinlock */

	unsigned long iofl;                                             /* I/O flags (CTRL_RUNNING, OUT_RUNNING) */
	struct timer_list io_retry;                                     /* Retry timer */
	unsigned long stop_retry;                                       /* Time to give up, in jiffies */
	unsigned int retry_delay;                                       /* Delay length in ms */
	struct work_struct reset_work;                                  /* Task context for resets */
	wait_queue_head_t wait;						/* For sleeping */
};

#define	hid_to_usb_dev(hid_dev) \
	container_of(hid_dev->dev.parent->parent, struct usb_device, dev)

#endif

