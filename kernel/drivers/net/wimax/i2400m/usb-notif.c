
#include <linux/usb.h>
#include "i2400m-usb.h"


#define D_SUBMODULE notif
#include "usb-debug-levels.h"


static const
__le32 i2400m_ZERO_BARKER[4] = { 0, 0, 0, 0 };


static
int i2400mu_notification_grok(struct i2400mu *i2400mu, const void *buf,
				 size_t buf_len)
{
	int ret;
	struct device *dev = &i2400mu->usb_iface->dev;
	struct i2400m *i2400m = &i2400mu->i2400m;

	d_fnstart(4, dev, "(i2400m %p buf %p buf_len %zu)\n",
		  i2400mu, buf, buf_len);
	ret = -EIO;
	if (buf_len < sizeof(i2400m_NBOOT_BARKER))
		/* Not a bug, just ignore */
		goto error_bad_size;
	if (!memcmp(i2400m_NBOOT_BARKER, buf, sizeof(i2400m_NBOOT_BARKER))
	    || !memcmp(i2400m_SBOOT_BARKER, buf, sizeof(i2400m_SBOOT_BARKER)))
		ret = i2400m_dev_reset_handle(i2400m);
	else if (!memcmp(i2400m_ZERO_BARKER, buf, sizeof(i2400m_ZERO_BARKER))) {
		i2400mu_rx_kick(i2400mu);
		ret = 0;
	} else {	/* Unknown or unexpected data in the notif message */
		char prefix[64];
		ret = -EIO;
		dev_err(dev, "HW BUG? Unknown/unexpected data in notification "
			"message (%zu bytes)\n", buf_len);
		snprintf(prefix, sizeof(prefix), "%s %s: ",
			 dev_driver_string(dev) , dev->bus_id);
		if (buf_len > 64) {
			print_hex_dump(KERN_ERR, prefix, DUMP_PREFIX_OFFSET,
				       8, 4, buf, 64, 0);
			printk(KERN_ERR "%s... (only first 64 bytes "
			       "dumped)\n", prefix);
		} else
			print_hex_dump(KERN_ERR, prefix, DUMP_PREFIX_OFFSET,
				       8, 4, buf, buf_len, 0);
	}
error_bad_size:
	d_fnend(4, dev, "(i2400m %p buf %p buf_len %zu) = %d\n",
		i2400mu, buf, buf_len, ret);
	return ret;
}


static
void i2400mu_notification_cb(struct urb *urb)
{
	int ret;
	struct i2400mu *i2400mu = urb->context;
	struct device *dev = &i2400mu->usb_iface->dev;

	d_fnstart(4, dev, "(urb %p status %d actual_length %d)\n",
		  urb, urb->status, urb->actual_length);
	ret = urb->status;
	switch (ret) {
	case 0:
		ret = i2400mu_notification_grok(i2400mu, urb->transfer_buffer,
						urb->actual_length);
		if (ret == -EIO && edc_inc(&i2400mu->urb_edc, EDC_MAX_ERRORS,
					   EDC_ERROR_TIMEFRAME))
			goto error_exceeded;
		if (ret == -ENOMEM)	/* uff...power cycle? shutdown? */
			goto error_exceeded;
		break;
	case -EINVAL:			/* while removing driver */
	case -ENODEV:			/* dev disconnect ... */
	case -ENOENT:			/* ditto */
	case -ESHUTDOWN:		/* URB killed */
	case -ECONNRESET:		/* disconnection */
		goto out;		/* Notify around */
	default:			/* Some error? */
		if (edc_inc(&i2400mu->urb_edc,
			    EDC_MAX_ERRORS, EDC_ERROR_TIMEFRAME))
			goto error_exceeded;
		dev_err(dev, "notification: URB error %d, retrying\n",
			urb->status);
	}
	usb_mark_last_busy(i2400mu->usb_dev);
	ret = usb_submit_urb(i2400mu->notif_urb, GFP_ATOMIC);
	switch (ret) {
	case 0:
	case -EINVAL:			/* while removing driver */
	case -ENODEV:			/* dev disconnect ... */
	case -ENOENT:			/* ditto */
	case -ESHUTDOWN:		/* URB killed */
	case -ECONNRESET:		/* disconnection */
		break;			/* just ignore */
	default:			/* Some error? */
		dev_err(dev, "notification: cannot submit URB: %d\n", ret);
		goto error_submit;
	}
	d_fnend(4, dev, "(urb %p status %d actual_length %d) = void\n",
		urb, urb->status, urb->actual_length);
	return;

error_exceeded:
	dev_err(dev, "maximum errors in notification URB exceeded; "
		"resetting device\n");
error_submit:
	usb_queue_reset_device(i2400mu->usb_iface);
out:
	d_fnend(4, dev, "(urb %p status %d actual_length %d) = void\n",
		urb, urb->status, urb->actual_length);
	return;
}


int i2400mu_notification_setup(struct i2400mu *i2400mu)
{
	struct device *dev = &i2400mu->usb_iface->dev;
	int usb_pipe, ret = 0;
	struct usb_endpoint_descriptor *epd;
	char *buf;

	d_fnstart(4, dev, "(i2400m %p)\n", i2400mu);
	buf = kmalloc(I2400MU_MAX_NOTIFICATION_LEN, GFP_KERNEL | GFP_DMA);
	if (buf == NULL) {
		dev_err(dev, "notification: buffer allocation failed\n");
		ret = -ENOMEM;
		goto error_buf_alloc;
	}

	i2400mu->notif_urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!i2400mu->notif_urb) {
		ret = -ENOMEM;
		dev_err(dev, "notification: cannot allocate URB\n");
		goto error_alloc_urb;
	}
	epd = usb_get_epd(i2400mu->usb_iface, I2400MU_EP_NOTIFICATION);
	usb_pipe = usb_rcvintpipe(i2400mu->usb_dev, epd->bEndpointAddress);
	usb_fill_int_urb(i2400mu->notif_urb, i2400mu->usb_dev, usb_pipe,
			 buf, I2400MU_MAX_NOTIFICATION_LEN,
			 i2400mu_notification_cb, i2400mu, epd->bInterval);
	ret = usb_submit_urb(i2400mu->notif_urb, GFP_KERNEL);
	if (ret != 0) {
		dev_err(dev, "notification: cannot submit URB: %d\n", ret);
		goto error_submit;
	}
	d_fnend(4, dev, "(i2400m %p) = %d\n", i2400mu, ret);
	return ret;

error_submit:
	usb_free_urb(i2400mu->notif_urb);
error_alloc_urb:
	kfree(buf);
error_buf_alloc:
	d_fnend(4, dev, "(i2400m %p) = %d\n", i2400mu, ret);
	return ret;
}


void i2400mu_notification_release(struct i2400mu *i2400mu)
{
	struct device *dev = &i2400mu->usb_iface->dev;

	d_fnstart(4, dev, "(i2400mu %p)\n", i2400mu);
	if (i2400mu->notif_urb != NULL) {
		usb_kill_urb(i2400mu->notif_urb);
		kfree(i2400mu->notif_urb->transfer_buffer);
		usb_free_urb(i2400mu->notif_urb);
		i2400mu->notif_urb = NULL;
	}
	d_fnend(4, dev, "(i2400mu %p)\n", i2400mu);
}
