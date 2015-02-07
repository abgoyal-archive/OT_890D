

#ifndef WACOM_H
#define WACOM_H
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/usb/input.h>
#include <asm/unaligned.h>

#define DRIVER_VERSION "v1.49"
#define DRIVER_AUTHOR "Vojtech Pavlik <vojtech@ucw.cz>"
#define DRIVER_DESC "USB Wacom Graphire and Wacom Intuos tablet driver"
#define DRIVER_LICENSE "GPL"

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE(DRIVER_LICENSE);

#define USB_VENDOR_ID_WACOM	0x056a

struct wacom {
	dma_addr_t data_dma;
	struct input_dev *dev;
	struct usb_device *usbdev;
	struct usb_interface *intf;
	struct urb *irq;
	struct wacom_wac *wacom_wac;
	struct mutex lock;
	unsigned int open:1;
	char phys[32];
};

struct wacom_combo {
	struct wacom *wacom;
	struct urb *urb;
};

extern int wacom_wac_irq(struct wacom_wac * wacom_wac, void * wcombo);
extern void wacom_report_abs(void *wcombo, unsigned int abs_type, int abs_data);
extern void wacom_report_rel(void *wcombo, unsigned int rel_type, int rel_data);
extern void wacom_report_key(void *wcombo, unsigned int key_type, int key_data);
extern void wacom_input_event(void *wcombo, unsigned int type, unsigned int code, int value);
extern void wacom_input_sync(void *wcombo);
extern void wacom_init_input_dev(struct input_dev *input_dev, struct wacom_wac *wacom_wac);
extern void input_dev_g4(struct input_dev *input_dev, struct wacom_wac *wacom_wac);
extern void input_dev_g(struct input_dev *input_dev, struct wacom_wac *wacom_wac);
extern void input_dev_i3s(struct input_dev *input_dev, struct wacom_wac *wacom_wac);
extern void input_dev_i3(struct input_dev *input_dev, struct wacom_wac *wacom_wac);
extern void input_dev_i(struct input_dev *input_dev, struct wacom_wac *wacom_wac);
extern void input_dev_pl(struct input_dev *input_dev, struct wacom_wac *wacom_wac);
extern void input_dev_pt(struct input_dev *input_dev, struct wacom_wac *wacom_wac);
extern void input_dev_mo(struct input_dev *input_dev, struct wacom_wac *wacom_wac);
extern void input_dev_bee(struct input_dev *input_dev, struct wacom_wac *wacom_wac);
extern __u16 wacom_le16_to_cpu(unsigned char *data);
extern __u16 wacom_be16_to_cpu(unsigned char *data);
extern struct wacom_features *get_wacom_feature(const struct usb_device_id *id);
extern const struct usb_device_id *get_device_table(void);

#endif
