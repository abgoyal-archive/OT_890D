


#include <linux/kernel.h>
#include <linux/module.h>

#include "rt2x00.h"
#include "rt2x00lib.h"

static int rt2x00lib_request_firmware(struct rt2x00_dev *rt2x00dev)
{
	struct device *device = wiphy_dev(rt2x00dev->hw->wiphy);
	const struct firmware *fw;
	char *fw_name;
	int retval;
	u16 crc;

	/*
	 * Read correct firmware from harddisk.
	 */
	fw_name = rt2x00dev->ops->lib->get_firmware_name(rt2x00dev);
	if (!fw_name) {
		ERROR(rt2x00dev,
		      "Invalid firmware filename.\n"
		      "Please file bug report to %s.\n", DRV_PROJECT);
		return -EINVAL;
	}

	INFO(rt2x00dev, "Loading firmware file '%s'.\n", fw_name);

	retval = request_firmware(&fw, fw_name, device);
	if (retval) {
		ERROR(rt2x00dev, "Failed to request Firmware.\n");
		return retval;
	}

	if (!fw || !fw->size || !fw->data) {
		ERROR(rt2x00dev, "Failed to read Firmware.\n");
		return -ENOENT;
	}

	crc = rt2x00dev->ops->lib->get_firmware_crc(fw->data, fw->size);
	if (crc != (fw->data[fw->size - 2] << 8 | fw->data[fw->size - 1])) {
		ERROR(rt2x00dev, "Firmware checksum error.\n");
		retval = -ENOENT;
		goto exit;
	}

	INFO(rt2x00dev, "Firmware detected - version: %d.%d.\n",
	     fw->data[fw->size - 4], fw->data[fw->size - 3]);

	rt2x00dev->fw = fw;

	return 0;

exit:
	release_firmware(fw);

	return retval;
}

int rt2x00lib_load_firmware(struct rt2x00_dev *rt2x00dev)
{
	int retval;

	if (!test_bit(DRIVER_REQUIRE_FIRMWARE, &rt2x00dev->flags))
		return 0;

	if (!rt2x00dev->fw) {
		retval = rt2x00lib_request_firmware(rt2x00dev);
		if (retval)
			return retval;
	}

	/*
	 * Send firmware to the device.
	 */
	retval = rt2x00dev->ops->lib->load_firmware(rt2x00dev,
						    rt2x00dev->fw->data,
						    rt2x00dev->fw->size);

	/*
	 * When the firmware is uploaded to the hardware the LED
	 * association status might have been triggered, for correct
	 * LED handling it should now be reset.
	 */
	rt2x00leds_led_assoc(rt2x00dev, false);

	return retval;
}

void rt2x00lib_free_firmware(struct rt2x00_dev *rt2x00dev)
{
	release_firmware(rt2x00dev->fw);
	rt2x00dev->fw = NULL;
}
