



#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/version.h>

#include <asm/uaccess.h>
#include <linux/input.h>
#include <linux/usb.h>
#include <linux/poll.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 19)
#include frontier_compat.h
#endif

/* Define these values to match your devices */
#define VENDOR_ID   0x165b
#define PRODUCT_ID	0x8101

#ifdef CONFIG_USB_DYNAMIC_MINORS
#define USB_TRANZPORT_MINOR_BASE	0
#else
// FIXME 176 - is the ldusb driver's minor - apply for a minor soon
#define USB_TRANZPORT_MINOR_BASE	177
#endif

/* table of devices that work with this driver */
static struct usb_device_id usb_tranzport_table [] = {
	{ USB_DEVICE(VENDOR_ID, PRODUCT_ID) },
	{ }					/* Terminating entry */
};

MODULE_DEVICE_TABLE(usb, usb_tranzport_table);
MODULE_VERSION("0.33");
MODULE_AUTHOR("Mike Taht <m@taht.net>");
MODULE_DESCRIPTION("Tranzport USB Driver");
MODULE_LICENSE("GPL");
MODULE_SUPPORTED_DEVICE("Frontier Designs Tranzport Control Surface");

/* These two aren't done yet */

#define SUPPRESS_EXTRA_ONLINE_EVENTS 0
#define BUFFERED_WRITES 0

#define SUPPRESS_EXTRA_OFFLINE_EVENTS 1
#define COMPRESS_WHEEL_EVENTS 1
#define BUFFERED_READS 1
#define RING_BUFFER_SIZE 1000
#define WRITE_BUFFER_SIZE 34
#define TRANZPORT_USB_TIMEOUT 10


static int debug = 0;

/* Use our own dbg macro */
#define dbg_info(dev, format, arg...) do { if (debug) dev_info(dev , format , ## arg); } while (0)

/* Module parameters */

module_param(debug, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(debug, "Debug enabled or not");


static int ring_buffer_size = RING_BUFFER_SIZE;

module_param(ring_buffer_size, int,  S_IRUGO);
MODULE_PARM_DESC(ring_buffer_size, "Read ring buffer size in reports");

static int write_buffer_size = WRITE_BUFFER_SIZE;
module_param(write_buffer_size, int,  S_IRUGO);
MODULE_PARM_DESC(write_buffer_size, "Write buffer size");


static int min_interrupt_in_interval = TRANZPORT_USB_TIMEOUT;
module_param(min_interrupt_in_interval, int, 0);
MODULE_PARM_DESC(min_interrupt_in_interval, "Minimum interrupt in interval in ms");

static int min_interrupt_out_interval = TRANZPORT_USB_TIMEOUT;
module_param(min_interrupt_out_interval, int, 0);
MODULE_PARM_DESC(min_interrupt_out_interval, "Minimum interrupt out interval in ms");

struct tranzport_cmd {
    unsigned char cmd[8];
};

enum LightID {
	LightRecord = 0,
	LightTrackrec,
	LightTrackmute,
	LightTracksolo,
	LightAnysolo,
	LightLoop,
	LightPunch
	};

/* Structure to hold all of our device specific stuff */

struct usb_tranzport {
	struct semaphore	sem;		/* locks this structure */
	struct usb_interface*	intf;		/* save off the usb interface pointer */

	int			open_count;	/* number of times this port has been opened */

	struct tranzport_cmd	(*ring_buffer)[RING_BUFFER_SIZE]; /* just make c happy */
	unsigned int		ring_head;
	unsigned int		ring_tail;

	wait_queue_head_t	read_wait;
	wait_queue_head_t	write_wait;

	unsigned char*		interrupt_in_buffer;
	struct usb_endpoint_descriptor* interrupt_in_endpoint;
	struct urb*		interrupt_in_urb;
	int			interrupt_in_interval;
	size_t	interrupt_in_endpoint_size;
	int			interrupt_in_running;
	int			interrupt_in_done;

	char*			interrupt_out_buffer;
	struct usb_endpoint_descriptor* interrupt_out_endpoint;
	struct urb*		interrupt_out_urb;
	int			interrupt_out_interval;
	size_t	interrupt_out_endpoint_size;
	int			interrupt_out_busy;

	/* Sysfs and translation support */

	int event; /* alternate interface to events */
	int wheel; /* - for negative, 0 for none, + for positive */
	unsigned char dump_state; /* 0 if disabled 1 if enabled */
	unsigned char enable; /* 0 if disabled 1 if enabled */
	unsigned char offline; /* if the device is out of range or asleep */
	unsigned char compress_wheel; /* flag to compress wheel events */
	unsigned char light; /* 7 bits used */
	unsigned char last_cmd[8];
	unsigned char last_input[8];
	unsigned char screen[40]; // We'll also have cells

};

/* prevent races between open() and disconnect() */
static DEFINE_MUTEX(disconnect_mutex);

static struct usb_driver usb_tranzport_driver;

static void usb_tranzport_abort_transfers(struct usb_tranzport *dev)
{
	/* shutdown transfer */
	if (dev->interrupt_in_running) {
		dev->interrupt_in_running = 0;
		if (dev->intf)
			usb_kill_urb(dev->interrupt_in_urb);
	}
	if (dev->interrupt_out_busy)
		if (dev->intf)
			usb_kill_urb(dev->interrupt_out_urb);
}

// FIXME ~light not good enough or correct - need atomic set_bit

#define show_set_light(value)	\
static ssize_t show_##value(struct device *dev, struct device_attribute *attr, char *buf)		\
{									\
	struct usb_interface *intf = to_usb_interface(dev);		\
	struct usb_tranzport *t = usb_get_intfdata(intf);		\
	enum LightID light = value;					\
	int temp = (1 && (t->light & (1 << light)));			\
	return sprintf(buf, "%d\n", temp );				\
}									\
static ssize_t set_##value(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)	\
{									\
	struct usb_interface *intf = to_usb_interface(dev);		\
	struct usb_tranzport *t = usb_get_intfdata(intf);		\
	int temp = simple_strtoul(buf, NULL, 10);			\
	enum LightID light = (temp << value) & (t->light << value);	\
	t->light = (t->light & ~light) ;				\
	return count;							\
}									\
static DEVICE_ATTR(value, S_IWUGO | S_IRUGO, show_##value, set_##value);

show_set_light(LightRecord);
show_set_light(LightTrackrec);
show_set_light(LightTrackmute);
show_set_light(LightTracksolo);
show_set_light(LightAnysolo);
show_set_light(LightLoop);
show_set_light(LightPunch);


#define show_set_int(value)	\
static ssize_t show_##value(struct device *dev, struct device_attribute *attr, char *buf)		\
{									\
	struct usb_interface *intf = to_usb_interface(dev);		\
	struct usb_tranzport *t = usb_get_intfdata(intf);			\
									\
	return sprintf(buf, "%d\n", t->value);			\
}									\
static ssize_t set_##value(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)	\
{									\
	struct usb_interface *intf = to_usb_interface(dev);		\
	struct usb_tranzport *t = usb_get_intfdata(intf);			\
	int temp = simple_strtoul(buf, NULL, 10);			\
									\
	t->value = temp;						\
	return count;							\
}									\
static DEVICE_ATTR(value, S_IWUGO | S_IRUGO, show_##value, set_##value);

show_set_int(enable);
show_set_int(offline);
show_set_int(compress_wheel);
show_set_int(dump_state);
show_set_int(wheel);
show_set_int(event);

#define show_set_cmd(value)	\
static ssize_t show_##value(struct device *dev, struct device_attribute *attr, char *buf)		\
{									\
	struct usb_interface *intf = to_usb_interface(dev);		\
	struct usb_tranzport *t = usb_get_intfdata(intf);			\
									\
	return sprintf(buf, "%d\n", t->value);			\
}									\
static ssize_t set_##value(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)	\
{									\
	struct usb_interface *intf = to_usb_interface(dev);		\
	struct usb_tranzport *t = usb_get_intfdata(intf);			\
	int temp = simple_strtoul(buf, NULL, 10);			\
									\
	t->value = temp;						\
	return count;							\
}									\
static DEVICE_ATTR(value, S_IWUGO | S_IRUGO, show_##value, set_##value);




static void usb_tranzport_delete(struct usb_tranzport *dev)
{
	usb_tranzport_abort_transfers(dev);
	/*  This is just too twisted to be correct */
	if(dev->intf != NULL) {
	device_remove_file(&dev->intf->dev, &dev_attr_LightRecord);
	device_remove_file(&dev->intf->dev, &dev_attr_LightTrackrec);
	device_remove_file(&dev->intf->dev, &dev_attr_LightTrackmute);
	device_remove_file(&dev->intf->dev, &dev_attr_LightTracksolo);
	device_remove_file(&dev->intf->dev, &dev_attr_LightTrackmute);
	device_remove_file(&dev->intf->dev, &dev_attr_LightAnysolo);
	device_remove_file(&dev->intf->dev, &dev_attr_LightLoop);
	device_remove_file(&dev->intf->dev, &dev_attr_LightPunch);
	device_remove_file(&dev->intf->dev, &dev_attr_wheel);
	device_remove_file(&dev->intf->dev, &dev_attr_enable);
	device_remove_file(&dev->intf->dev, &dev_attr_event);
	device_remove_file(&dev->intf->dev, &dev_attr_offline);
	device_remove_file(&dev->intf->dev, &dev_attr_compress_wheel);

	device_remove_file(&dev->intf->dev, &dev_attr_dump_state);
	}

	/* free data structures */
	usb_free_urb(dev->interrupt_in_urb);
	usb_free_urb(dev->interrupt_out_urb);
	kfree(dev->ring_buffer);
	kfree(dev->interrupt_in_buffer);
	kfree(dev->interrupt_out_buffer);
	kfree(dev);
}


static void usb_tranzport_interrupt_in_callback(struct urb *urb)
{
	struct usb_tranzport *dev = urb->context;
	unsigned int next_ring_head;
	int retval = -1;

	if (urb->status) {
		if (urb->status == -ENOENT ||
		    urb->status == -ECONNRESET ||
		    urb->status == -ESHUTDOWN) {
			goto exit;
		} else {
			dbg_info(&dev->intf->dev, "%s: nonzero status received: %d\n",
				 __func__, urb->status);
			goto resubmit; /* maybe we can recover */
		}
	}

	if (urb->actual_length != 8) {
		dev_warn(&dev->intf->dev,
			 "Urb length was %d bytes!! Do something intelligent \n", urb->actual_length);
	} else {
		dbg_info(&dev->intf->dev, "%s: received: %02x%02x%02x%02x%02x%02x%02x%02x\n",
			 __func__, dev->interrupt_in_buffer[0],dev->interrupt_in_buffer[1],dev->interrupt_in_buffer[2],dev->interrupt_in_buffer[3],dev->interrupt_in_buffer[4],dev->interrupt_in_buffer[5],dev->interrupt_in_buffer[6],dev->interrupt_in_buffer[7]);
#if SUPPRESS_EXTRA_OFFLINE_EVENTS
		if(dev->offline == 2 && dev->interrupt_in_buffer[1] == 0xff) { goto resubmit; }
		if(dev->offline == 1 && dev->interrupt_in_buffer[1] == 0xff) { dev->offline = 2; goto resubmit; }

/* Always pass one offline event up the stack */
		if(dev->offline > 0 && dev->interrupt_in_buffer[1] != 0xff) { dev->offline = 0; }
		if(dev->offline == 0 && dev->interrupt_in_buffer[1] == 0xff) { dev->offline = 1; }

#endif
		dbg_info(&dev->intf->dev, "%s: head, tail are %x, %x\n", __func__,dev->ring_head,dev->ring_tail);

		next_ring_head = (dev->ring_head+1) % ring_buffer_size;

		if (next_ring_head != dev->ring_tail) {
			memcpy(&((*dev->ring_buffer)[dev->ring_head]), dev->interrupt_in_buffer, urb->actual_length);
			dev->ring_head = next_ring_head;
			retval = 0;
			memset(dev->interrupt_in_buffer, 0, urb->actual_length);
		} else {
			dev_warn(&dev->intf->dev,
				 "Ring buffer overflow, %d bytes dropped\n",
				 urb->actual_length);
			memset(dev->interrupt_in_buffer, 0, urb->actual_length);
		}
	}

resubmit:
	/* resubmit if we're still running */
	if (dev->interrupt_in_running && dev->intf) {
		retval = usb_submit_urb(dev->interrupt_in_urb, GFP_ATOMIC);
		if (retval)
			dev_err(&dev->intf->dev,
				"usb_submit_urb failed (%d)\n", retval);
	}

exit:
	dev->interrupt_in_done = 1;
	wake_up_interruptible(&dev->read_wait);
}

static void usb_tranzport_interrupt_out_callback(struct urb *urb)
{
	struct usb_tranzport *dev = urb->context;

	/* sync/async unlink faults aren't errors */
	if (urb->status && !(urb->status == -ENOENT ||
			     urb->status == -ECONNRESET ||
			     urb->status == -ESHUTDOWN))
		dbg_info(&dev->intf->dev,
			 "%s - nonzero write interrupt status received: %d\n",
			 __func__, urb->status);

	dev->interrupt_out_busy = 0;
	wake_up_interruptible(&dev->write_wait);
}

static int usb_tranzport_open(struct inode *inode, struct file *file)
{
	struct usb_tranzport *dev;
	int subminor;
	int retval = 0;
	struct usb_interface *interface;

	nonseekable_open(inode, file);
	subminor = iminor(inode);

	mutex_lock(&disconnect_mutex);

	interface = usb_find_interface(&usb_tranzport_driver, subminor);

	if (!interface) {
		err("%s - error, can't find device for minor %d\n",
		     __func__, subminor);
		retval = -ENODEV;
		goto unlock_disconnect_exit;
	}

	dev = usb_get_intfdata(interface);

	if (!dev) {
		retval = -ENODEV;
		goto unlock_disconnect_exit;
	}

	/* lock this device */
	if (down_interruptible(&dev->sem)) {
		retval = -ERESTARTSYS;
		goto unlock_disconnect_exit;
	}

	/* allow opening only once */
	if (dev->open_count) {
		retval = -EBUSY;
		goto unlock_exit;
	}
	dev->open_count = 1;

	/* initialize in direction */
	dev->ring_head = 0;
	dev->ring_tail = 0;
	usb_fill_int_urb(dev->interrupt_in_urb,
			 interface_to_usbdev(interface),
			 usb_rcvintpipe(interface_to_usbdev(interface),
					dev->interrupt_in_endpoint->bEndpointAddress),
			 dev->interrupt_in_buffer,
			 dev->interrupt_in_endpoint_size,
			 usb_tranzport_interrupt_in_callback,
			 dev,
			 dev->interrupt_in_interval);

	dev->interrupt_in_running = 1;
	dev->interrupt_in_done = 0;
	dev->enable = 1;
	dev->offline = 0;
	dev->compress_wheel = 1;

	retval = usb_submit_urb(dev->interrupt_in_urb, GFP_KERNEL);
	if (retval) {
		dev_err(&interface->dev, "Couldn't submit interrupt_in_urb %d\n", retval);
		dev->interrupt_in_running = 0;
		dev->open_count = 0;
		goto unlock_exit;
	}

	/* save device in the file's private structure */
	file->private_data = dev;


unlock_exit:
	up(&dev->sem);

unlock_disconnect_exit:
	mutex_unlock(&disconnect_mutex);

	return retval;
}

static int usb_tranzport_release(struct inode *inode, struct file *file)
{
	struct usb_tranzport *dev;
	int retval = 0;

	dev = file->private_data;

	if (dev == NULL) {
		retval = -ENODEV;
		goto exit;
	}

	if (down_interruptible(&dev->sem)) {
		retval = -ERESTARTSYS;
		goto exit;
	}

	if (dev->open_count != 1) {
		retval = -ENODEV;
		goto unlock_exit;
	}

	if (dev->intf == NULL) {
		/* the device was unplugged before the file was released */
		up(&dev->sem);
		/* unlock here as usb_tranzport_delete frees dev */
		usb_tranzport_delete(dev);
		retval = -ENODEV;
		goto exit;
	}

	/* wait until write transfer is finished */
	if (dev->interrupt_out_busy)
		wait_event_interruptible_timeout(dev->write_wait, !dev->interrupt_out_busy, 2 * HZ);
	usb_tranzport_abort_transfers(dev);
	dev->open_count = 0;

unlock_exit:
	up(&dev->sem);

exit:
	return retval;
}

static unsigned int usb_tranzport_poll(struct file *file, poll_table *wait)
{
	struct usb_tranzport *dev;
	unsigned int mask = 0;

	dev = file->private_data;

	poll_wait(file, &dev->read_wait, wait);
	poll_wait(file, &dev->write_wait, wait);

	if (dev->ring_head != dev->ring_tail)
		mask |= POLLIN | POLLRDNORM;
	if (!dev->interrupt_out_busy)
		mask |= POLLOUT | POLLWRNORM;

	return mask;
}

static ssize_t usb_tranzport_read(struct file *file, char __user *buffer, size_t count,
			   loff_t *ppos)
{
	struct usb_tranzport *dev;
	int retval = 0;

#if BUFFERED_READS
	int c = 0;
#endif

#if COMPRESS_WHEEL_EVENTS
	signed char oldwheel;
	signed char newwheel;
	int cancompress = 1;
	int next_tail;
#endif

/* do I have such a thing as a null event? */

	dev = file->private_data;

	/* verify that we actually have some data to read */
	if (count == 0)
		goto exit;

	/* lock this object */
	if (down_interruptible(&dev->sem)) {
		retval = -ERESTARTSYS;
		goto exit;
	}

	/* verify that the device wasn't unplugged */
	if (dev->intf == NULL) {
		retval = -ENODEV;
		err("No device or device unplugged %d\n", retval);
		goto unlock_exit;
	}

	while (dev->ring_head == dev->ring_tail) {

		if (file->f_flags & O_NONBLOCK) {
			retval = -EAGAIN;
			goto unlock_exit;
		}
		// atomic_cmp_exchange(&dev->interrupt_in_done,0,0);
		dev->interrupt_in_done = 0 ; /* tiny race - FIXME: make atomic? */
		retval = wait_event_interruptible(dev->read_wait, dev->interrupt_in_done);
		if (retval < 0) {
			goto unlock_exit;
		}
	}

	dbg_info(&dev->intf->dev, "%s: copying to userspace: %02x%02x%02x%02x%02x%02x%02x%02x\n",
			 __func__, (*dev->ring_buffer)[dev->ring_tail].cmd[0],(*dev->ring_buffer)[dev->ring_tail].cmd[1],(*dev->ring_buffer)[dev->ring_tail].cmd[2],(*dev->ring_buffer)[dev->ring_tail].cmd[3],(*dev->ring_buffer)[dev->ring_tail].cmd[4],(*dev->ring_buffer)[dev->ring_tail].cmd[5],(*dev->ring_buffer)[dev->ring_tail].cmd[6],(*dev->ring_buffer)[dev->ring_tail].cmd[7]);

#if BUFFERED_READS
	   c = 0;
	   while((c < count) && (dev->ring_tail != dev->ring_head)) {

/* This started off in the lower level service routine, and I moved it here. Then my brain died. Not done yet. */
#if COMPRESS_WHEEL_EVENTS
		next_tail = (dev->ring_tail+1) % ring_buffer_size;
		if(dev->compress_wheel) cancompress = 1;
		while(dev->ring_head != next_tail && cancompress == 1 ) {
			newwheel = (*dev->ring_buffer)[next_tail].cmd[6];
			oldwheel = (*dev->ring_buffer)[dev->ring_tail].cmd[6];
			// if both are wheel events, and no buttons have changes (FIXME, do I have to check?),
			// and we are the same sign, we can compress +- 7F
			// FIXME: saner check for overflow! - max of +- 7F
			// FIXME the math is wrong for going in reverse, actually, as the midi spec doesn't allow signed chars

	dbg_info(&dev->intf->dev, "%s: trying to compress: %02x%02x%02x%02x%02x %02x %02x %02x\n",
			 __func__, (*dev->ring_buffer)[dev->ring_tail].cmd[0],(*dev->ring_buffer)[dev->ring_tail].cmd[1],(*dev->ring_buffer)[dev->ring_tail].cmd[2],(*dev->ring_buffer)[dev->ring_tail].cmd[3],(*dev->ring_buffer)[dev->ring_tail].cmd[4],(*dev->ring_buffer)[dev->ring_tail].cmd[5],(*dev->ring_buffer)[dev->ring_tail].cmd[6],(*dev->ring_buffer)[dev->ring_tail].cmd[7]);


			if(((*dev->ring_buffer)[dev->ring_tail].cmd[6] != 0 &&
			    (*dev->ring_buffer)[next_tail].cmd[6] != 0 ) &&
				((newwheel > 0 && oldwheel > 0) ||
				(newwheel < 0 && oldwheel < 0)) &&
				((*dev->ring_buffer)[dev->ring_tail].cmd[2] == (*dev->ring_buffer)[next_tail].cmd[2]) &&
				((*dev->ring_buffer)[dev->ring_tail].cmd[3] == (*dev->ring_buffer)[next_tail].cmd[3]) &&
				((*dev->ring_buffer)[dev->ring_tail].cmd[4] == (*dev->ring_buffer)[next_tail].cmd[4]) &&
				((*dev->ring_buffer)[dev->ring_tail].cmd[5] == (*dev->ring_buffer)[next_tail].cmd[5]))
 {
	dbg_info(&dev->intf->dev, "%s: should compress: %02x%02x%02x%02x%02x%02x%02x%02x\n",
			 __func__, (*dev->ring_buffer)[dev->ring_tail].cmd[0],(*dev->ring_buffer)[dev->ring_tail].cmd[1],(*dev->ring_buffer)[dev->ring_tail].cmd[2],(*dev->ring_buffer)[dev->ring_tail].cmd[3],(*dev->ring_buffer)[dev->ring_tail].cmd[4],(*dev->ring_buffer)[dev->ring_tail].cmd[5],(*dev->ring_buffer)[dev->ring_tail].cmd[6],(*dev->ring_buffer)[dev->ring_tail].cmd[7]);

				newwheel += oldwheel;
				if(oldwheel > 0 && !(newwheel > 0)) {
					newwheel = 0x7f;
					cancompress = 0;
				}
				if(oldwheel < 0 && !(newwheel < 0)) {
					newwheel = 0x80;
					cancompress = 0;
				}

				(*dev->ring_buffer)[next_tail].cmd[6] = newwheel;
				dev->ring_tail = next_tail;
				next_tail = (dev->ring_tail+1) % ring_buffer_size;
			} else {
				cancompress = 0;
			}
		}
#endif /* COMPRESS_WHEEL_EVENTS */

		if (copy_to_user(&buffer[c], &(*dev->ring_buffer)[dev->ring_tail], 8)) {
			retval = -EFAULT;
			goto unlock_exit;
		}

		dev->ring_tail = (dev->ring_tail+1) % ring_buffer_size;
		c+=8;
		dbg_info(&dev->intf->dev, "%s: head, tail are %x, %x\n", __func__,dev->ring_head,dev->ring_tail);
	   }
	   retval = c;

#else
	if (copy_to_user(buffer, &(*dev->ring_buffer)[dev->ring_tail], 8)) {
		retval = -EFAULT;
		goto unlock_exit;
	}

	dev->ring_tail = (dev->ring_tail+1) % ring_buffer_size;
	dbg_info(&dev->intf->dev, "%s: head, tail are %x, %x\n", __func__,dev->ring_head,dev->ring_tail);

	retval = 8;
#endif /* BUFFERED_READS */

unlock_exit:
	/* unlock the device */
	up(&dev->sem);

exit:
	return retval;
}

static ssize_t usb_tranzport_write(struct file *file, const char __user *buffer,
			    size_t count, loff_t *ppos)
{
	struct usb_tranzport *dev;
	size_t bytes_to_write;
	int retval = 0;

	dev = file->private_data;

	/* verify that we actually have some data to write */
	if (count == 0)
		goto exit;

	/* lock this object */
	if (down_interruptible(&dev->sem)) {
		retval = -ERESTARTSYS;
		goto exit;
	}

	/* verify that the device wasn't unplugged */
	if (dev->intf == NULL) {
		retval = -ENODEV;
		err("No device or device unplugged %d\n", retval);
		goto unlock_exit;
	}

	/* wait until previous transfer is finished */
	if (dev->interrupt_out_busy) {
		if (file->f_flags & O_NONBLOCK) {
			retval = -EAGAIN;
			goto unlock_exit;
		}
		retval = wait_event_interruptible(dev->write_wait, !dev->interrupt_out_busy);
		if (retval < 0) {
			goto unlock_exit;
		}
	}

	/* write the data into interrupt_out_buffer from userspace */
	bytes_to_write = min(count, write_buffer_size*dev->interrupt_out_endpoint_size);
	if (bytes_to_write < count)
		dev_warn(&dev->intf->dev, "Write buffer overflow, %zd bytes dropped\n",count-bytes_to_write);

	dbg_info(&dev->intf->dev, "%s: count = %zd, bytes_to_write = %zd\n", __func__, count, bytes_to_write);

	if (copy_from_user(dev->interrupt_out_buffer, buffer, bytes_to_write)) {
		retval = -EFAULT;
		goto unlock_exit;
	}

	if (dev->interrupt_out_endpoint == NULL) {
		err("Endpoint should not be be null! \n");
		goto unlock_exit;
	}

	/* send off the urb */
	usb_fill_int_urb(dev->interrupt_out_urb,
			 interface_to_usbdev(dev->intf),
			 usb_sndintpipe(interface_to_usbdev(dev->intf),
					dev->interrupt_out_endpoint->bEndpointAddress),
			 dev->interrupt_out_buffer,
			 bytes_to_write,
			 usb_tranzport_interrupt_out_callback,
			 dev,
			 dev->interrupt_out_interval);

	dev->interrupt_out_busy = 1;
	wmb();

	retval = usb_submit_urb(dev->interrupt_out_urb, GFP_KERNEL);
	if (retval) {
		dev->interrupt_out_busy = 0;
		err("Couldn't submit interrupt_out_urb %d\n", retval);
		goto unlock_exit;
	}
	retval = bytes_to_write;

unlock_exit:
	/* unlock the device */
	up(&dev->sem);

exit:
	return retval;
}

/* file operations needed when we register this driver */
static const struct file_operations usb_tranzport_fops = {
	.owner =	THIS_MODULE,
	.read  =	usb_tranzport_read,
	.write =	usb_tranzport_write,
	.open =		usb_tranzport_open,
	.release =	usb_tranzport_release,
	.poll =		usb_tranzport_poll,
};

static struct usb_class_driver usb_tranzport_class = {
	.name =		"tranzport%d",
	.fops =		&usb_tranzport_fops,
	.minor_base =	USB_TRANZPORT_MINOR_BASE,
};


static int usb_tranzport_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct usb_device *udev = interface_to_usbdev(intf);
	struct usb_tranzport *dev = NULL;
	struct usb_host_interface *iface_desc;
	struct usb_endpoint_descriptor *endpoint;
	int i;
	int true_size;
	int retval = -ENOMEM;

	/* allocate memory for our device state and intialize it */

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (dev == NULL) {
		dev_err(&intf->dev, "Out of memory\n");
		goto exit;
	}
	init_MUTEX(&dev->sem);
	dev->intf = intf;
	init_waitqueue_head(&dev->read_wait);
	init_waitqueue_head(&dev->write_wait);

	iface_desc = intf->cur_altsetting;

	/* set up the endpoint information */
	for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i) {
		endpoint = &iface_desc->endpoint[i].desc;

		if (usb_endpoint_is_int_in(endpoint))
			dev->interrupt_in_endpoint = endpoint;

		if (usb_endpoint_is_int_out(endpoint))
			dev->interrupt_out_endpoint = endpoint;
	}
	if (dev->interrupt_in_endpoint == NULL) {
		dev_err(&intf->dev, "Interrupt in endpoint not found\n");
		goto error;
	}
	if (dev->interrupt_out_endpoint == NULL)
		dev_warn(&intf->dev, "Interrupt out endpoint not found (using control endpoint instead)\n");


	dev->interrupt_in_endpoint_size = le16_to_cpu(dev->interrupt_in_endpoint->wMaxPacketSize);

	if (dev->interrupt_in_endpoint_size != 8)
	    dev_warn(&intf->dev, "Interrupt in endpoint size is not 8!\n");

	if(ring_buffer_size == 0) { ring_buffer_size = RING_BUFFER_SIZE; }
	true_size = min(ring_buffer_size,RING_BUFFER_SIZE);
	/* FIXME - there are more usb_alloc routines for dma correctness. Needed? */

	dev->ring_buffer = kmalloc((true_size*sizeof(struct tranzport_cmd))+8, GFP_KERNEL);

	if (!dev->ring_buffer) {
		dev_err(&intf->dev, "Couldn't allocate ring_buffer of size %d\n",true_size);
		goto error;
	}
	dev->interrupt_in_buffer = kmalloc(dev->interrupt_in_endpoint_size, GFP_KERNEL);
	if (!dev->interrupt_in_buffer) {
		dev_err(&intf->dev, "Couldn't allocate interrupt_in_buffer\n");
		goto error;
	}
	dev->interrupt_in_urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!dev->interrupt_in_urb) {
		dev_err(&intf->dev, "Couldn't allocate interrupt_in_urb\n");
		goto error;
	}
	dev->interrupt_out_endpoint_size = dev->interrupt_out_endpoint ? le16_to_cpu(dev->interrupt_out_endpoint->wMaxPacketSize) :
									 udev->descriptor.bMaxPacketSize0;

	if (dev->interrupt_out_endpoint_size !=8)
		dev_warn(&intf->dev, "Interrupt out endpoint size is not 8!)\n");

	dev->interrupt_out_buffer = kmalloc(write_buffer_size*dev->interrupt_out_endpoint_size, GFP_KERNEL);
	if (!dev->interrupt_out_buffer) {
		dev_err(&intf->dev, "Couldn't allocate interrupt_out_buffer\n");
		goto error;
	}
	dev->interrupt_out_urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!dev->interrupt_out_urb) {
		dev_err(&intf->dev, "Couldn't allocate interrupt_out_urb\n");
		goto error;
	}
	dev->interrupt_in_interval = min_interrupt_in_interval > dev->interrupt_in_endpoint->bInterval ? min_interrupt_in_interval : dev->interrupt_in_endpoint->bInterval;
	if (dev->interrupt_out_endpoint)
		dev->interrupt_out_interval = min_interrupt_out_interval > dev->interrupt_out_endpoint->bInterval ? min_interrupt_out_interval : dev->interrupt_out_endpoint->bInterval;

	/* we can register the device now, as it is ready */
	usb_set_intfdata(intf, dev);

	retval = usb_register_dev(intf, &usb_tranzport_class);
	if (retval) {
		/* something prevented us from registering this driver */
		dev_err(&intf->dev, "Not able to get a minor for this device.\n");
		usb_set_intfdata(intf, NULL);
		goto error;
	}

	if((retval = device_create_file(&intf->dev, &dev_attr_LightRecord))) goto error;
	if((retval = device_create_file(&intf->dev, &dev_attr_LightTrackrec))) goto error;
	if((retval = device_create_file(&intf->dev, &dev_attr_LightTrackmute))) goto error;
	if((retval = device_create_file(&intf->dev, &dev_attr_LightTracksolo))) goto error;
	if((retval = device_create_file(&intf->dev, &dev_attr_LightAnysolo))) goto error;
	if((retval = device_create_file(&intf->dev, &dev_attr_LightLoop))) goto error;
	if((retval = device_create_file(&intf->dev, &dev_attr_LightPunch))) goto error;
	if((retval = device_create_file(&intf->dev, &dev_attr_wheel))) goto error;
	if((retval = device_create_file(&intf->dev, &dev_attr_event))) goto error;
	if((retval = device_create_file(&intf->dev, &dev_attr_dump_state))) goto error;
	if((retval = device_create_file(&intf->dev, &dev_attr_compress_wheel))) goto error;
	if((retval = device_create_file(&intf->dev, &dev_attr_enable))) goto error;
	if((retval = device_create_file(&intf->dev, &dev_attr_offline))) goto error;

	/* let the user know what node this device is now attached to */
	dev_info(&intf->dev, "Tranzport Device #%d now attached to major %d minor %d\n",
		(intf->minor - USB_TRANZPORT_MINOR_BASE), USB_MAJOR, intf->minor);

exit:
	return retval;

error:
	usb_tranzport_delete(dev);

	return retval;
}

static void usb_tranzport_disconnect(struct usb_interface *intf)
{
	struct usb_tranzport *dev;
	int minor;
	mutex_lock(&disconnect_mutex);
	dev = usb_get_intfdata(intf);
	usb_set_intfdata(intf, NULL);
	down(&dev->sem);
	minor = intf->minor;
	/* give back our minor */
	usb_deregister_dev(intf, &usb_tranzport_class);

	/* if the device is not opened, then we clean up right now */
	if (!dev->open_count) {
		up(&dev->sem);
		usb_tranzport_delete(dev);
	} else {
		dev->intf = NULL;
		up(&dev->sem);
	}

	mutex_unlock(&disconnect_mutex);

	dev_info(&intf->dev, "Tranzport Surface #%d now disconnected\n",
		 (minor - USB_TRANZPORT_MINOR_BASE));
}

/* usb specific object needed to register this driver with the usb subsystem */
static struct usb_driver usb_tranzport_driver = {
	.name =		"tranzport",
	.probe =	usb_tranzport_probe,
	.disconnect =	usb_tranzport_disconnect,
	.id_table =	usb_tranzport_table,
};

static int __init usb_tranzport_init(void)
{
	int retval;

	/* register this driver with the USB subsystem */
	retval = usb_register(&usb_tranzport_driver);
	if (retval)
		err("usb_register failed for the "__FILE__" driver. Error number %d\n", retval);

	return retval;
}

static void __exit usb_tranzport_exit(void)
{
	/* deregister this driver with the USB subsystem */
	usb_deregister(&usb_tranzport_driver);
}

module_init(usb_tranzport_init);
module_exit(usb_tranzport_exit);
