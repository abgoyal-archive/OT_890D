
/* USB defines for older kernels */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 19)


static inline int usb_endpoint_dir_out(const struct usb_endpoint_descriptor *epd)
{
       return ((epd->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_OUT);
}

static inline int usb_endpoint_dir_in(const struct usb_endpoint_descriptor *epd)
{
       return ((epd->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN);
}


static inline int usb_endpoint_xfer_int(const struct usb_endpoint_descriptor *epd)
{
       return ((epd->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) ==
               USB_ENDPOINT_XFER_INT);
}



static inline int usb_endpoint_is_int_in(const struct usb_endpoint_descriptor *epd)
{
       return (usb_endpoint_xfer_int(epd) && usb_endpoint_dir_in(epd));
}


static inline int usb_endpoint_is_int_out(const struct usb_endpoint_descriptor *epd)
{
       return (usb_endpoint_xfer_int(epd) && usb_endpoint_dir_out(epd));
}

#endif /* older kernel versions */
