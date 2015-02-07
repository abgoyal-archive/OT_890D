

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/usb/input.h>
#include <linux/hid.h>
#include <linux/mutex.h>

#define USB_VENDOR_ID_APPLE		0x05ac

/* MacbookAir, aka wellspring */
#define USB_DEVICE_ID_APPLE_WELLSPRING_ANSI	0x0223
#define USB_DEVICE_ID_APPLE_WELLSPRING_ISO	0x0224
#define USB_DEVICE_ID_APPLE_WELLSPRING_JIS	0x0225
/* MacbookProPenryn, aka wellspring2 */
#define USB_DEVICE_ID_APPLE_WELLSPRING2_ANSI	0x0230
#define USB_DEVICE_ID_APPLE_WELLSPRING2_ISO	0x0231
#define USB_DEVICE_ID_APPLE_WELLSPRING2_JIS	0x0232

#define BCM5974_DEVICE(prod) {					\
	.match_flags = (USB_DEVICE_ID_MATCH_DEVICE |		\
			USB_DEVICE_ID_MATCH_INT_CLASS |		\
			USB_DEVICE_ID_MATCH_INT_PROTOCOL),	\
	.idVendor = USB_VENDOR_ID_APPLE,			\
	.idProduct = (prod),					\
	.bInterfaceClass = USB_INTERFACE_CLASS_HID,		\
	.bInterfaceProtocol = USB_INTERFACE_PROTOCOL_MOUSE	\
}

/* table of devices that work with this driver */
static const struct usb_device_id bcm5974_table[] = {
	/* MacbookAir1.1 */
	BCM5974_DEVICE(USB_DEVICE_ID_APPLE_WELLSPRING_ANSI),
	BCM5974_DEVICE(USB_DEVICE_ID_APPLE_WELLSPRING_ISO),
	BCM5974_DEVICE(USB_DEVICE_ID_APPLE_WELLSPRING_JIS),
	/* MacbookProPenryn */
	BCM5974_DEVICE(USB_DEVICE_ID_APPLE_WELLSPRING2_ANSI),
	BCM5974_DEVICE(USB_DEVICE_ID_APPLE_WELLSPRING2_ISO),
	BCM5974_DEVICE(USB_DEVICE_ID_APPLE_WELLSPRING2_JIS),
	/* Terminating entry */
	{}
};
MODULE_DEVICE_TABLE(usb, bcm5974_table);

MODULE_AUTHOR("Henrik Rydberg");
MODULE_DESCRIPTION("Apple USB BCM5974 multitouch driver");
MODULE_LICENSE("GPL");

#define dprintk(level, format, a...)\
	{ if (debug >= level) printk(KERN_DEBUG format, ##a); }

static int debug = 1;
module_param(debug, int, 0644);
MODULE_PARM_DESC(debug, "Activate debugging output");

/* button data structure */
struct bt_data {
	u8 unknown1;		/* constant */
	u8 button;		/* left button */
	u8 rel_x;		/* relative x coordinate */
	u8 rel_y;		/* relative y coordinate */
};

/* trackpad header structure */
struct tp_header {
	u8 unknown1[16];	/* constants, timers, etc */
	u8 fingers;		/* number of fingers on trackpad */
	u8 unknown2[9];		/* constants, timers, etc */
};

/* trackpad finger structure */
struct tp_finger {
	__le16 origin;		/* zero when switching track finger */
	__le16 abs_x;		/* absolute x coodinate */
	__le16 abs_y;		/* absolute y coodinate */
	__le16 rel_x;		/* relative x coodinate */
	__le16 rel_y;		/* relative y coodinate */
	__le16 size_major;	/* finger size, major axis? */
	__le16 size_minor;	/* finger size, minor axis? */
	__le16 orientation;	/* 16384 when point, else 15 bit angle */
	__le16 force_major;	/* trackpad force, major axis? */
	__le16 force_minor;	/* trackpad force, minor axis? */
	__le16 unused[3];	/* zeros */
	__le16 multi;		/* one finger: varies, more fingers: constant */
};

/* trackpad data structure, empirically at least ten fingers */
struct tp_data {
	struct tp_header header;
	struct tp_finger finger[16];
};

/* device-specific parameters */
struct bcm5974_param {
	int dim;		/* logical dimension */
	int fuzz;		/* logical noise value */
	int devmin;		/* device minimum reading */
	int devmax;		/* device maximum reading */
};

/* device-specific configuration */
struct bcm5974_config {
	int ansi, iso, jis;	/* the product id of this device */
	int bt_ep;		/* the endpoint of the button interface */
	int bt_datalen;		/* data length of the button interface */
	int tp_ep;		/* the endpoint of the trackpad interface */
	int tp_datalen;		/* data length of the trackpad interface */
	struct bcm5974_param p;	/* finger pressure limits */
	struct bcm5974_param w;	/* finger width limits */
	struct bcm5974_param x;	/* horizontal limits */
	struct bcm5974_param y;	/* vertical limits */
};

/* logical device structure */
struct bcm5974 {
	char phys[64];
	struct usb_device *udev;	/* usb device */
	struct usb_interface *intf;	/* our interface */
	struct input_dev *input;	/* input dev */
	struct bcm5974_config cfg;	/* device configuration */
	struct mutex pm_mutex;		/* serialize access to open/suspend */
	int opened;			/* 1: opened, 0: closed */
	struct urb *bt_urb;		/* button usb request block */
	struct bt_data *bt_data;	/* button transferred data */
	struct urb *tp_urb;		/* trackpad usb request block */
	struct tp_data *tp_data;	/* trackpad transferred data */
	int fingers;			/* number of fingers on trackpad */
};

/* logical dimensions */
#define DIM_PRESSURE	256		/* maximum finger pressure */
#define DIM_WIDTH	16		/* maximum finger width */
#define DIM_X		1280		/* maximum trackpad x value */
#define DIM_Y		800		/* maximum trackpad y value */

/* logical signal quality */
#define SN_PRESSURE	45		/* pressure signal-to-noise ratio */
#define SN_WIDTH	100		/* width signal-to-noise ratio */
#define SN_COORD	250		/* coordinate signal-to-noise ratio */

/* pressure thresholds */
#define PRESSURE_LOW	(2 * DIM_PRESSURE / SN_PRESSURE)
#define PRESSURE_HIGH	(3 * PRESSURE_LOW)

/* device constants */
static const struct bcm5974_config bcm5974_config_table[] = {
	{
		USB_DEVICE_ID_APPLE_WELLSPRING_ANSI,
		USB_DEVICE_ID_APPLE_WELLSPRING_ISO,
		USB_DEVICE_ID_APPLE_WELLSPRING_JIS,
		0x84, sizeof(struct bt_data),
		0x81, sizeof(struct tp_data),
		{ DIM_PRESSURE, DIM_PRESSURE / SN_PRESSURE, 0, 256 },
		{ DIM_WIDTH, DIM_WIDTH / SN_WIDTH, 0, 2048 },
		{ DIM_X, DIM_X / SN_COORD, -4824, 5342 },
		{ DIM_Y, DIM_Y / SN_COORD, -172, 5820 }
	},
	{
		USB_DEVICE_ID_APPLE_WELLSPRING2_ANSI,
		USB_DEVICE_ID_APPLE_WELLSPRING2_ISO,
		USB_DEVICE_ID_APPLE_WELLSPRING2_JIS,
		0x84, sizeof(struct bt_data),
		0x81, sizeof(struct tp_data),
		{ DIM_PRESSURE, DIM_PRESSURE / SN_PRESSURE, 0, 256 },
		{ DIM_WIDTH, DIM_WIDTH / SN_WIDTH, 0, 2048 },
		{ DIM_X, DIM_X / SN_COORD, -4824, 4824 },
		{ DIM_Y, DIM_Y / SN_COORD, -172, 4290 }
	},
	{}
};

/* return the device-specific configuration by device */
static const struct bcm5974_config *bcm5974_get_config(struct usb_device *udev)
{
	u16 id = le16_to_cpu(udev->descriptor.idProduct);
	const struct bcm5974_config *cfg;

	for (cfg = bcm5974_config_table; cfg->ansi; ++cfg)
		if (cfg->ansi == id || cfg->iso == id || cfg->jis == id)
			return cfg;

	return bcm5974_config_table;
}

/* convert 16-bit little endian to signed integer */
static inline int raw2int(__le16 x)
{
	return (signed short)le16_to_cpu(x);
}

/* scale device data to logical dimensions (asserts devmin < devmax) */
static inline int int2scale(const struct bcm5974_param *p, int x)
{
	return x * p->dim / (p->devmax - p->devmin);
}

/* all logical value ranges are [0,dim). */
static inline int int2bound(const struct bcm5974_param *p, int x)
{
	int s = int2scale(p, x);

	return clamp_val(s, 0, p->dim - 1);
}

/* setup which logical events to report */
static void setup_events_to_report(struct input_dev *input_dev,
				   const struct bcm5974_config *cfg)
{
	__set_bit(EV_ABS, input_dev->evbit);

	input_set_abs_params(input_dev, ABS_PRESSURE,
				0, cfg->p.dim, cfg->p.fuzz, 0);
	input_set_abs_params(input_dev, ABS_TOOL_WIDTH,
				0, cfg->w.dim, cfg->w.fuzz, 0);
	input_set_abs_params(input_dev, ABS_X,
				0, cfg->x.dim, cfg->x.fuzz, 0);
	input_set_abs_params(input_dev, ABS_Y,
				0, cfg->y.dim, cfg->y.fuzz, 0);

	__set_bit(EV_KEY, input_dev->evbit);
	__set_bit(BTN_TOUCH, input_dev->keybit);
	__set_bit(BTN_TOOL_FINGER, input_dev->keybit);
	__set_bit(BTN_TOOL_DOUBLETAP, input_dev->keybit);
	__set_bit(BTN_TOOL_TRIPLETAP, input_dev->keybit);
	__set_bit(BTN_LEFT, input_dev->keybit);
}

/* report button data as logical button state */
static int report_bt_state(struct bcm5974 *dev, int size)
{
	if (size != sizeof(struct bt_data))
		return -EIO;

	input_report_key(dev->input, BTN_LEFT, dev->bt_data->button);
	input_sync(dev->input);

	return 0;
}

/* report trackpad data as logical trackpad state */
static int report_tp_state(struct bcm5974 *dev, int size)
{
	const struct bcm5974_config *c = &dev->cfg;
	const struct tp_finger *f = dev->tp_data->finger;
	struct input_dev *input = dev->input;
	const int fingers = (size - 26) / 28;
	int raw_p, raw_w, raw_x, raw_y;
	int ptest = 0, origin = 0, nmin = 0, nmax = 0;
	int abs_p = 0, abs_w = 0, abs_x = 0, abs_y = 0;

	if (size < 26 || (size - 26) % 28 != 0)
		return -EIO;

	/* always track the first finger; when detached, start over */
	if (fingers) {
		raw_p = raw2int(f->force_major);
		raw_w = raw2int(f->size_major);
		raw_x = raw2int(f->abs_x);
		raw_y = raw2int(f->abs_y);

		dprintk(9,
			"bcm5974: raw: p: %+05d w: %+05d x: %+05d y: %+05d\n",
			raw_p, raw_w, raw_x, raw_y);

		ptest = int2bound(&c->p, raw_p);
		origin = raw2int(f->origin);
	}

	/* while tracking finger still valid, count all fingers */
	if (ptest > PRESSURE_LOW && origin) {
		abs_p = ptest;
		abs_w = int2bound(&c->w, raw_w);
		abs_x = int2bound(&c->x, raw_x - c->x.devmin);
		abs_y = int2bound(&c->y, c->y.devmax - raw_y);
		for (; f != dev->tp_data->finger + fingers; f++) {
			ptest = int2bound(&c->p, raw2int(f->force_major));
			if (ptest > PRESSURE_LOW)
				nmax++;
			if (ptest > PRESSURE_HIGH)
				nmin++;
		}
	}

	if (dev->fingers < nmin)
		dev->fingers = nmin;
	if (dev->fingers > nmax)
		dev->fingers = nmax;

	input_report_key(input, BTN_TOUCH, dev->fingers > 0);
	input_report_key(input, BTN_TOOL_FINGER, dev->fingers == 1);
	input_report_key(input, BTN_TOOL_DOUBLETAP, dev->fingers == 2);
	input_report_key(input, BTN_TOOL_TRIPLETAP, dev->fingers > 2);

	input_report_abs(input, ABS_PRESSURE, abs_p);
	input_report_abs(input, ABS_TOOL_WIDTH, abs_w);

	if (abs_p) {
		input_report_abs(input, ABS_X, abs_x);
		input_report_abs(input, ABS_Y, abs_y);

		dprintk(8,
			"bcm5974: abs: p: %+05d w: %+05d x: %+05d y: %+05d "
			"nmin: %d nmax: %d n: %d\n",
			abs_p, abs_w, abs_x, abs_y, nmin, nmax, dev->fingers);

	}

	input_sync(input);

	return 0;
}

/* Wellspring initialization constants */
#define BCM5974_WELLSPRING_MODE_READ_REQUEST_ID		1
#define BCM5974_WELLSPRING_MODE_WRITE_REQUEST_ID	9
#define BCM5974_WELLSPRING_MODE_REQUEST_VALUE		0x300
#define BCM5974_WELLSPRING_MODE_REQUEST_INDEX		0
#define BCM5974_WELLSPRING_MODE_VENDOR_VALUE		0x01
#define BCM5974_WELLSPRING_MODE_NORMAL_VALUE		0x08

static int bcm5974_wellspring_mode(struct bcm5974 *dev, bool on)
{
	char *data = kmalloc(8, GFP_KERNEL);
	int retval = 0, size;

	if (!data) {
		err("bcm5974: out of memory");
		retval = -ENOMEM;
		goto out;
	}

	/* read configuration */
	size = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0),
			BCM5974_WELLSPRING_MODE_READ_REQUEST_ID,
			USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
			BCM5974_WELLSPRING_MODE_REQUEST_VALUE,
			BCM5974_WELLSPRING_MODE_REQUEST_INDEX, data, 8, 5000);

	if (size != 8) {
		err("bcm5974: could not read from device");
		retval = -EIO;
		goto out;
	}

	/* apply the mode switch */
	data[0] = on ?
		BCM5974_WELLSPRING_MODE_VENDOR_VALUE :
		BCM5974_WELLSPRING_MODE_NORMAL_VALUE;

	/* write configuration */
	size = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
			BCM5974_WELLSPRING_MODE_WRITE_REQUEST_ID,
			USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
			BCM5974_WELLSPRING_MODE_REQUEST_VALUE,
			BCM5974_WELLSPRING_MODE_REQUEST_INDEX, data, 8, 5000);

	if (size != 8) {
		err("bcm5974: could not write to device");
		retval = -EIO;
		goto out;
	}

	dprintk(2, "bcm5974: switched to %s mode.\n",
		on ? "wellspring" : "normal");

 out:
	kfree(data);
	return retval;
}

static void bcm5974_irq_button(struct urb *urb)
{
	struct bcm5974 *dev = urb->context;
	int error;

	switch (urb->status) {
	case 0:
		break;
	case -EOVERFLOW:
	case -ECONNRESET:
	case -ENOENT:
	case -ESHUTDOWN:
		dbg("bcm5974: button urb shutting down: %d", urb->status);
		return;
	default:
		dbg("bcm5974: button urb status: %d", urb->status);
		goto exit;
	}

	if (report_bt_state(dev, dev->bt_urb->actual_length))
		dprintk(1, "bcm5974: bad button package, length: %d\n",
			dev->bt_urb->actual_length);

exit:
	error = usb_submit_urb(dev->bt_urb, GFP_ATOMIC);
	if (error)
		err("bcm5974: button urb failed: %d", error);
}

static void bcm5974_irq_trackpad(struct urb *urb)
{
	struct bcm5974 *dev = urb->context;
	int error;

	switch (urb->status) {
	case 0:
		break;
	case -EOVERFLOW:
	case -ECONNRESET:
	case -ENOENT:
	case -ESHUTDOWN:
		dbg("bcm5974: trackpad urb shutting down: %d", urb->status);
		return;
	default:
		dbg("bcm5974: trackpad urb status: %d", urb->status);
		goto exit;
	}

	/* control response ignored */
	if (dev->tp_urb->actual_length == 2)
		goto exit;

	if (report_tp_state(dev, dev->tp_urb->actual_length))
		dprintk(1, "bcm5974: bad trackpad package, length: %d\n",
			dev->tp_urb->actual_length);

exit:
	error = usb_submit_urb(dev->tp_urb, GFP_ATOMIC);
	if (error)
		err("bcm5974: trackpad urb failed: %d", error);
}

static int bcm5974_start_traffic(struct bcm5974 *dev)
{
	if (bcm5974_wellspring_mode(dev, true)) {
		dprintk(1, "bcm5974: mode switch failed\n");
		goto error;
	}

	if (usb_submit_urb(dev->bt_urb, GFP_KERNEL))
		goto error;

	if (usb_submit_urb(dev->tp_urb, GFP_KERNEL))
		goto err_kill_bt;

	return 0;

err_kill_bt:
	usb_kill_urb(dev->bt_urb);
error:
	return -EIO;
}

static void bcm5974_pause_traffic(struct bcm5974 *dev)
{
	usb_kill_urb(dev->tp_urb);
	usb_kill_urb(dev->bt_urb);
	bcm5974_wellspring_mode(dev, false);
}

static int bcm5974_open(struct input_dev *input)
{
	struct bcm5974 *dev = input_get_drvdata(input);
	int error;

	error = usb_autopm_get_interface(dev->intf);
	if (error)
		return error;

	mutex_lock(&dev->pm_mutex);

	error = bcm5974_start_traffic(dev);
	if (!error)
		dev->opened = 1;

	mutex_unlock(&dev->pm_mutex);

	if (error)
		usb_autopm_put_interface(dev->intf);

	return error;
}

static void bcm5974_close(struct input_dev *input)
{
	struct bcm5974 *dev = input_get_drvdata(input);

	mutex_lock(&dev->pm_mutex);

	bcm5974_pause_traffic(dev);
	dev->opened = 0;

	mutex_unlock(&dev->pm_mutex);

	usb_autopm_put_interface(dev->intf);
}

static int bcm5974_suspend(struct usb_interface *iface, pm_message_t message)
{
	struct bcm5974 *dev = usb_get_intfdata(iface);

	mutex_lock(&dev->pm_mutex);

	if (dev->opened)
		bcm5974_pause_traffic(dev);

	mutex_unlock(&dev->pm_mutex);

	return 0;
}

static int bcm5974_resume(struct usb_interface *iface)
{
	struct bcm5974 *dev = usb_get_intfdata(iface);
	int error = 0;

	mutex_lock(&dev->pm_mutex);

	if (dev->opened)
		error = bcm5974_start_traffic(dev);

	mutex_unlock(&dev->pm_mutex);

	return error;
}

static int bcm5974_probe(struct usb_interface *iface,
			 const struct usb_device_id *id)
{
	struct usb_device *udev = interface_to_usbdev(iface);
	const struct bcm5974_config *cfg;
	struct bcm5974 *dev;
	struct input_dev *input_dev;
	int error = -ENOMEM;

	/* find the product index */
	cfg = bcm5974_get_config(udev);

	/* allocate memory for our device state and initialize it */
	dev = kzalloc(sizeof(struct bcm5974), GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!dev || !input_dev) {
		err("bcm5974: out of memory");
		goto err_free_devs;
	}

	dev->udev = udev;
	dev->intf = iface;
	dev->input = input_dev;
	dev->cfg = *cfg;
	mutex_init(&dev->pm_mutex);

	/* setup urbs */
	dev->bt_urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!dev->bt_urb)
		goto err_free_devs;

	dev->tp_urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!dev->tp_urb)
		goto err_free_bt_urb;

	dev->bt_data = usb_buffer_alloc(dev->udev,
					dev->cfg.bt_datalen, GFP_KERNEL,
					&dev->bt_urb->transfer_dma);
	if (!dev->bt_data)
		goto err_free_urb;

	dev->tp_data = usb_buffer_alloc(dev->udev,
					dev->cfg.tp_datalen, GFP_KERNEL,
					&dev->tp_urb->transfer_dma);
	if (!dev->tp_data)
		goto err_free_bt_buffer;

	usb_fill_int_urb(dev->bt_urb, udev,
			 usb_rcvintpipe(udev, cfg->bt_ep),
			 dev->bt_data, dev->cfg.bt_datalen,
			 bcm5974_irq_button, dev, 1);

	usb_fill_int_urb(dev->tp_urb, udev,
			 usb_rcvintpipe(udev, cfg->tp_ep),
			 dev->tp_data, dev->cfg.tp_datalen,
			 bcm5974_irq_trackpad, dev, 1);

	/* create bcm5974 device */
	usb_make_path(udev, dev->phys, sizeof(dev->phys));
	strlcat(dev->phys, "/input0", sizeof(dev->phys));

	input_dev->name = "bcm5974";
	input_dev->phys = dev->phys;
	usb_to_input_id(dev->udev, &input_dev->id);
	input_dev->dev.parent = &iface->dev;

	input_set_drvdata(input_dev, dev);

	input_dev->open = bcm5974_open;
	input_dev->close = bcm5974_close;

	setup_events_to_report(input_dev, cfg);

	error = input_register_device(dev->input);
	if (error)
		goto err_free_buffer;

	/* save our data pointer in this interface device */
	usb_set_intfdata(iface, dev);

	return 0;

err_free_buffer:
	usb_buffer_free(dev->udev, dev->cfg.tp_datalen,
		dev->tp_data, dev->tp_urb->transfer_dma);
err_free_bt_buffer:
	usb_buffer_free(dev->udev, dev->cfg.bt_datalen,
		dev->bt_data, dev->bt_urb->transfer_dma);
err_free_urb:
	usb_free_urb(dev->tp_urb);
err_free_bt_urb:
	usb_free_urb(dev->bt_urb);
err_free_devs:
	usb_set_intfdata(iface, NULL);
	input_free_device(input_dev);
	kfree(dev);
	return error;
}

static void bcm5974_disconnect(struct usb_interface *iface)
{
	struct bcm5974 *dev = usb_get_intfdata(iface);

	usb_set_intfdata(iface, NULL);

	input_unregister_device(dev->input);
	usb_buffer_free(dev->udev, dev->cfg.tp_datalen,
			dev->tp_data, dev->tp_urb->transfer_dma);
	usb_buffer_free(dev->udev, dev->cfg.bt_datalen,
			dev->bt_data, dev->bt_urb->transfer_dma);
	usb_free_urb(dev->tp_urb);
	usb_free_urb(dev->bt_urb);
	kfree(dev);
}

static struct usb_driver bcm5974_driver = {
	.name			= "bcm5974",
	.probe			= bcm5974_probe,
	.disconnect		= bcm5974_disconnect,
	.suspend		= bcm5974_suspend,
	.resume			= bcm5974_resume,
	.reset_resume		= bcm5974_resume,
	.id_table		= bcm5974_table,
	.supports_autosuspend	= 1,
};

static int __init bcm5974_init(void)
{
	return usb_register(&bcm5974_driver);
}

static void __exit bcm5974_exit(void)
{
	usb_deregister(&bcm5974_driver);
}

module_init(bcm5974_init);
module_exit(bcm5974_exit);

