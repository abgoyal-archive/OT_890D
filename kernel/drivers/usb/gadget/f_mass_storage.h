

#ifndef __F_MASS_STORAGE_H
#define __F_MASS_STORAGE_H

int mass_storage_function_add(struct usb_composite_dev *cdev,
	struct usb_configuration *c, int nluns);

#endif /* __F_MASS_STORAGE_H */
