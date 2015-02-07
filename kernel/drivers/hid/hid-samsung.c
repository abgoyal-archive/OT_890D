


#include <linux/device.h>
#include <linux/hid.h>
#include <linux/module.h>

#include "hid-ids.h"

static void samsung_report_fixup(struct hid_device *hdev, __u8 *rdesc,
		unsigned int rsize)
{
	if (rsize >= 182 && rdesc[175] == 0x25 && rdesc[176] == 0x40 &&
			rdesc[177] == 0x75 && rdesc[178] == 0x30 &&
			rdesc[179] == 0x95 && rdesc[180] == 0x01 &&
			rdesc[182] == 0x40) {
		dev_info(&hdev->dev, "fixing up Samsung IrDA report "
				"descriptor\n");
		rdesc[176] = 0xff;
		rdesc[178] = 0x08;
		rdesc[180] = 0x06;
		rdesc[182] = 0x42;
	}
}

static int samsung_probe(struct hid_device *hdev,
		const struct hid_device_id *id)
{
	int ret;

	ret = hid_parse(hdev);
	if (ret) {
		dev_err(&hdev->dev, "parse failed\n");
		goto err_free;
	}

	ret = hid_hw_start(hdev, (HID_CONNECT_DEFAULT & ~HID_CONNECT_HIDINPUT) |
			HID_CONNECT_HIDDEV_FORCE);
	if (ret) {
		dev_err(&hdev->dev, "hw start failed\n");
		goto err_free;
	}

	return 0;
err_free:
	return ret;
}

static const struct hid_device_id samsung_devices[] = {
	{ HID_USB_DEVICE(USB_VENDOR_ID_SAMSUNG, USB_DEVICE_ID_SAMSUNG_IR_REMOTE) },
	{ }
};
MODULE_DEVICE_TABLE(hid, samsung_devices);

static struct hid_driver samsung_driver = {
	.name = "samsung",
	.id_table = samsung_devices,
	.report_fixup = samsung_report_fixup,
	.probe = samsung_probe,
};

static int samsung_init(void)
{
	return hid_register_driver(&samsung_driver);
}

static void samsung_exit(void)
{
	hid_unregister_driver(&samsung_driver);
}

module_init(samsung_init);
module_exit(samsung_exit);
MODULE_LICENSE("GPL");

HID_COMPAT_LOAD_DRIVER(samsung);
