
#include "i2400m-usb.h"


#define D_SUBMODULE tx
#include "usb-debug-levels.h"


static
int i2400mu_tx(struct i2400mu *i2400mu, struct i2400m_msg_hdr *tx_msg,
	       size_t tx_msg_size)
{
	int result = 0;
	struct i2400m *i2400m = &i2400mu->i2400m;
	struct device *dev = &i2400mu->usb_iface->dev;
	int usb_pipe, sent_size, do_autopm;
	struct usb_endpoint_descriptor *epd;

	d_fnstart(4, dev, "(i2400mu %p)\n", i2400mu);
	do_autopm = atomic_read(&i2400mu->do_autopm);
	result = do_autopm ?
		usb_autopm_get_interface(i2400mu->usb_iface) : 0;
	if (result < 0) {
		dev_err(dev, "TX: can't get autopm: %d\n", result);
		do_autopm = 0;
	}
	epd = usb_get_epd(i2400mu->usb_iface, I2400MU_EP_BULK_OUT);
	usb_pipe = usb_sndbulkpipe(i2400mu->usb_dev, epd->bEndpointAddress);
retry:
	result = usb_bulk_msg(i2400mu->usb_dev, usb_pipe,
			      tx_msg, tx_msg_size, &sent_size, HZ);
	usb_mark_last_busy(i2400mu->usb_dev);
	switch (result) {
	case 0:
		if (sent_size != tx_msg_size) {	/* Too short? drop it */
			dev_err(dev, "TX: short write (%d B vs %zu "
				"expected)\n", sent_size, tx_msg_size);
			result = -EIO;
		}
		break;
	case -EINVAL:			/* while removing driver */
	case -ENODEV:			/* dev disconnect ... */
	case -ENOENT:			/* just ignore it */
	case -ESHUTDOWN:		/* and exit */
	case -ECONNRESET:
		result = -ESHUTDOWN;
		break;
	default:			/* Some error? */
		if (edc_inc(&i2400mu->urb_edc,
			    EDC_MAX_ERRORS, EDC_ERROR_TIMEFRAME)) {
			dev_err(dev, "TX: maximum errors in URB "
				"exceeded; resetting device\n");
			usb_queue_reset_device(i2400mu->usb_iface);
		} else {
			dev_err(dev, "TX: cannot send URB; retrying. "
				"tx_msg @%zu %zu B [%d sent]: %d\n",
				(void *) tx_msg - i2400m->tx_buf,
				tx_msg_size, sent_size, result);
			goto retry;
		}
	}
	if (do_autopm)
		usb_autopm_put_interface(i2400mu->usb_iface);
	d_fnend(4, dev, "(i2400mu %p) = result\n", i2400mu);
	return result;
}


static
int i2400mu_txd(void *_i2400mu)
{
	int result = 0;
	struct i2400mu *i2400mu = _i2400mu;
	struct i2400m *i2400m = &i2400mu->i2400m;
	struct device *dev = &i2400mu->usb_iface->dev;
	struct i2400m_msg_hdr *tx_msg;
	size_t tx_msg_size;

	d_fnstart(4, dev, "(i2400mu %p)\n", i2400mu);

	while (1) {
		d_printf(2, dev, "TX: waiting for messages\n");
		tx_msg = NULL;
		wait_event_interruptible(
			i2400mu->tx_wq,
			(kthread_should_stop()	/* check this first! */
			 || (tx_msg = i2400m_tx_msg_get(i2400m, &tx_msg_size)))
			);
		if (kthread_should_stop())
			break;
		WARN_ON(tx_msg == NULL);	/* should not happen...*/
		d_printf(2, dev, "TX: submitting %zu bytes\n", tx_msg_size);
		d_dump(5, dev, tx_msg, tx_msg_size);
		/* Yeah, we ignore errors ... not much we can do */
		i2400mu_tx(i2400mu, tx_msg, tx_msg_size);
		i2400m_tx_msg_sent(i2400m);	/* ack it, advance the FIFO */
		if (result < 0)
			break;
	}
	d_fnend(4, dev, "(i2400mu %p) = %d\n", i2400mu, result);
	return result;
}


void i2400mu_bus_tx_kick(struct i2400m *i2400m)
{
	struct i2400mu *i2400mu = container_of(i2400m, struct i2400mu, i2400m);
	struct device *dev = &i2400mu->usb_iface->dev;

	d_fnstart(3, dev, "(i2400m %p) = void\n", i2400m);
	wake_up_all(&i2400mu->tx_wq);
	d_fnend(3, dev, "(i2400m %p) = void\n", i2400m);
}


int i2400mu_tx_setup(struct i2400mu *i2400mu)
{
	int result = 0;
	struct i2400m *i2400m = &i2400mu->i2400m;
	struct device *dev = &i2400mu->usb_iface->dev;
	struct wimax_dev *wimax_dev = &i2400m->wimax_dev;

	i2400mu->tx_kthread = kthread_run(i2400mu_txd, i2400mu, "%s-tx",
					  wimax_dev->name);
	if (IS_ERR(i2400mu->tx_kthread)) {
		result = PTR_ERR(i2400mu->tx_kthread);
		dev_err(dev, "TX: cannot start thread: %d\n", result);
	}
	return result;
}

void i2400mu_tx_release(struct i2400mu *i2400mu)
{
	kthread_stop(i2400mu->tx_kthread);
}
