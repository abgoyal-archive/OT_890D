

#include <linux/types.h>
#include <linux/ioctl.h>
#include <linux/i2c.h>
#include <linux/videodev2.h>
#include <media/v4l2-device.h>

int v4l2_device_register(struct device *dev, struct v4l2_device *v4l2_dev)
{
	if (dev == NULL || v4l2_dev == NULL)
		return -EINVAL;
	/* Warn if we apparently re-register a device */
	WARN_ON(dev_get_drvdata(dev) != NULL);
	INIT_LIST_HEAD(&v4l2_dev->subdevs);
	spin_lock_init(&v4l2_dev->lock);
	v4l2_dev->dev = dev;
	snprintf(v4l2_dev->name, sizeof(v4l2_dev->name), "%s %s",
			dev->driver->name, dev->bus_id);
	dev_set_drvdata(dev, v4l2_dev);
	return 0;
}
EXPORT_SYMBOL_GPL(v4l2_device_register);

void v4l2_device_unregister(struct v4l2_device *v4l2_dev)
{
	struct v4l2_subdev *sd, *next;

	if (v4l2_dev == NULL || v4l2_dev->dev == NULL)
		return;
	dev_set_drvdata(v4l2_dev->dev, NULL);
	/* unregister subdevs */
	list_for_each_entry_safe(sd, next, &v4l2_dev->subdevs, list)
		v4l2_device_unregister_subdev(sd);

	v4l2_dev->dev = NULL;
}
EXPORT_SYMBOL_GPL(v4l2_device_unregister);

int v4l2_device_register_subdev(struct v4l2_device *dev, struct v4l2_subdev *sd)
{
	/* Check for valid input */
	if (dev == NULL || sd == NULL || !sd->name[0])
		return -EINVAL;
	/* Warn if we apparently re-register a subdev */
	WARN_ON(sd->dev != NULL);
	if (!try_module_get(sd->owner))
		return -ENODEV;
	sd->dev = dev;
	spin_lock(&dev->lock);
	list_add_tail(&sd->list, &dev->subdevs);
	spin_unlock(&dev->lock);
	return 0;
}
EXPORT_SYMBOL_GPL(v4l2_device_register_subdev);

void v4l2_device_unregister_subdev(struct v4l2_subdev *sd)
{
	/* return if it isn't registered */
	if (sd == NULL || sd->dev == NULL)
		return;
	spin_lock(&sd->dev->lock);
	list_del(&sd->list);
	spin_unlock(&sd->dev->lock);
	sd->dev = NULL;
	module_put(sd->owner);
}
EXPORT_SYMBOL_GPL(v4l2_device_unregister_subdev);
