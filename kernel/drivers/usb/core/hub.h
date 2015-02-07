
#ifndef __LINUX_HUB_H
#define __LINUX_HUB_H


#include <linux/list.h>
#include <linux/workqueue.h>
#include <linux/compiler.h>	/* likely()/unlikely() */


#define USB_RT_HUB	(USB_TYPE_CLASS | USB_RECIP_DEVICE)
#define USB_RT_PORT	(USB_TYPE_CLASS | USB_RECIP_OTHER)

#define HUB_CLEAR_TT_BUFFER	8
#define HUB_RESET_TT		9
#define HUB_GET_TT_STATE	10
#define HUB_STOP_TT		11

#define C_HUB_LOCAL_POWER	0
#define C_HUB_OVER_CURRENT	1

#define USB_PORT_FEAT_CONNECTION	0
#define USB_PORT_FEAT_ENABLE		1
#define USB_PORT_FEAT_SUSPEND		2	/* L2 suspend */
#define USB_PORT_FEAT_OVER_CURRENT	3
#define USB_PORT_FEAT_RESET		4
#define USB_PORT_FEAT_L1		5	/* L1 suspend */
#define USB_PORT_FEAT_POWER		8
#define USB_PORT_FEAT_LOWSPEED		9
#define USB_PORT_FEAT_HIGHSPEED		10
#define USB_PORT_FEAT_C_CONNECTION	16
#define USB_PORT_FEAT_C_ENABLE		17
#define USB_PORT_FEAT_C_SUSPEND		18
#define USB_PORT_FEAT_C_OVER_CURRENT	19
#define USB_PORT_FEAT_C_RESET		20
#define USB_PORT_FEAT_TEST              21
#define USB_PORT_FEAT_INDICATOR         22
#define USB_PORT_FEAT_C_PORT_L1         23

struct usb_port_status {
	__le16 wPortStatus;
	__le16 wPortChange;
} __attribute__ ((packed));

#define USB_PORT_STAT_CONNECTION	0x0001
#define USB_PORT_STAT_ENABLE		0x0002
#define USB_PORT_STAT_SUSPEND		0x0004
#define USB_PORT_STAT_OVERCURRENT	0x0008
#define USB_PORT_STAT_RESET		0x0010
#define USB_PORT_STAT_L1		0x0020
/* bits 6 to 7 are reserved */
#define USB_PORT_STAT_POWER		0x0100
#define USB_PORT_STAT_LOW_SPEED		0x0200
#define USB_PORT_STAT_HIGH_SPEED        0x0400
#define USB_PORT_STAT_TEST              0x0800
#define USB_PORT_STAT_INDICATOR         0x1000
/* bits 13 to 15 are reserved */

#define USB_PORT_STAT_C_CONNECTION	0x0001
#define USB_PORT_STAT_C_ENABLE		0x0002
#define USB_PORT_STAT_C_SUSPEND		0x0004
#define USB_PORT_STAT_C_OVERCURRENT	0x0008
#define USB_PORT_STAT_C_RESET		0x0010
#define USB_PORT_STAT_C_L1		0x0020

#define HUB_CHAR_LPSM		0x0003 /* D1 .. D0 */
#define HUB_CHAR_COMPOUND	0x0004 /* D2       */
#define HUB_CHAR_OCPM		0x0018 /* D4 .. D3 */
#define HUB_CHAR_TTTT           0x0060 /* D6 .. D5 */
#define HUB_CHAR_PORTIND        0x0080 /* D7       */

struct usb_hub_status {
	__le16 wHubStatus;
	__le16 wHubChange;
} __attribute__ ((packed));

#define HUB_STATUS_LOCAL_POWER	0x0001
#define HUB_STATUS_OVERCURRENT	0x0002
#define HUB_CHANGE_LOCAL_POWER	0x0001
#define HUB_CHANGE_OVERCURRENT	0x0002



#define USB_DT_HUB			(USB_TYPE_CLASS | 0x09)
#define USB_DT_HUB_NONVAR_SIZE		7

struct usb_hub_descriptor {
	__u8  bDescLength;
	__u8  bDescriptorType;
	__u8  bNbrPorts;
	__le16 wHubCharacteristics;
	__u8  bPwrOn2PwrGood;
	__u8  bHubContrCurrent;
		/* add 1 bit for hub status change; round to bytes */
	__u8  DeviceRemovable[(USB_MAXCHILDREN + 1 + 7) / 8];
	__u8  PortPwrCtrlMask[(USB_MAXCHILDREN + 1 + 7) / 8];
} __attribute__ ((packed));


/* port indicator status selectors, tables 11-7 and 11-25 */
#define HUB_LED_AUTO	0
#define HUB_LED_AMBER	1
#define HUB_LED_GREEN	2
#define HUB_LED_OFF	3

enum hub_led_mode {
	INDICATOR_AUTO = 0,
	INDICATOR_CYCLE,
	/* software blinks for attention:  software, hardware, reserved */
	INDICATOR_GREEN_BLINK, INDICATOR_GREEN_BLINK_OFF,
	INDICATOR_AMBER_BLINK, INDICATOR_AMBER_BLINK_OFF,
	INDICATOR_ALT_BLINK, INDICATOR_ALT_BLINK_OFF
} __attribute__ ((packed));

struct usb_device;

/* Transaction Translator Think Times, in bits */
#define HUB_TTTT_8_BITS		0x00
#define HUB_TTTT_16_BITS	0x20
#define HUB_TTTT_24_BITS	0x40
#define HUB_TTTT_32_BITS	0x60

struct usb_tt {
	struct usb_device	*hub;	/* upstream highspeed hub */
	int			multi;	/* true means one TT per port */
	unsigned		think_time;	/* think time in ns */

	/* for control/bulk error recovery (CLEAR_TT_BUFFER) */
	spinlock_t		lock;
	struct list_head	clear_list;	/* of usb_tt_clear */
	struct work_struct			kevent;
};

struct usb_tt_clear {
	struct list_head	clear_list;
	unsigned		tt;
	u16			devinfo;
};

extern void usb_hub_tt_clear_buffer(struct usb_device *dev, int pipe);
extern void usb_ep0_reinit(struct usb_device *);

#endif /* __LINUX_HUB_H */
