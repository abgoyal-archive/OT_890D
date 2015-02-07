
#ifndef	__LINUX_USB_ANDROID_H
#define	__LINUX_USB_ANDROID_H

struct android_usb_platform_data {
	/* USB device descriptor fields */
	__u16 vendor_id;

	/* Default product ID. */
	__u16 product_id;

	/* Product ID when adb is enabled. */
	__u16 adb_product_id;

	__u16 version;

	char *product_name;
	char *manufacturer_name;
	char *serial_number;

	/* number of LUNS for mass storage function */
	int nluns;
};

struct usb_mass_storage_platform_data {
	char *vendor;
	char *product;
	int release;
};

extern void android_usb_set_connected(int on);

#endif	/* __LINUX_USB_ANDROID_H */
