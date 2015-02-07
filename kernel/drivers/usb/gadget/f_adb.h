

#ifndef __F_ADB_H
#define __F_ADB_H

int adb_function_add(struct usb_composite_dev *cdev,
	struct usb_configuration *c);
void adb_function_enable(int enable);

#endif /* __F_ADB_H */
