

#ifndef _V4L2_DEVICE_H
#define _V4L2_DEVICE_H

#include <media/v4l2-subdev.h>


#define V4L2_DEVICE_NAME_SIZE (BUS_ID_SIZE + 16)

struct v4l2_device {
	/* dev->driver_data points to this struct */
	struct device *dev;
	/* used to keep track of the registered subdevs */
	struct list_head subdevs;
	/* lock this struct; can be used by the driver as well if this
	   struct is embedded into a larger struct. */
	spinlock_t lock;
	/* unique device name, by default the driver name + bus ID */
	char name[V4L2_DEVICE_NAME_SIZE];
};

/* Initialize v4l2_dev and make dev->driver_data point to v4l2_dev */
int __must_check v4l2_device_register(struct device *dev, struct v4l2_device *v4l2_dev);
/* Set v4l2_dev->dev->driver_data to NULL and unregister all sub-devices */
void v4l2_device_unregister(struct v4l2_device *v4l2_dev);

int __must_check v4l2_device_register_subdev(struct v4l2_device *dev, struct v4l2_subdev *sd);
void v4l2_device_unregister_subdev(struct v4l2_subdev *sd);

/* Iterate over all subdevs. */
#define v4l2_device_for_each_subdev(sd, dev)				\
	list_for_each_entry(sd, &(dev)->subdevs, list)

#define __v4l2_device_call_subdevs(dev, cond, o, f, args...) 		\
	do { 								\
		struct v4l2_subdev *sd; 				\
									\
		list_for_each_entry(sd, &(dev)->subdevs, list)   	\
			if ((cond) && sd->ops->o && sd->ops->o->f) 	\
				sd->ops->o->f(sd , ##args); 		\
	} while (0)

#define __v4l2_device_call_subdevs_until_err(dev, cond, o, f, args...)  \
({ 									\
	struct v4l2_subdev *sd; 					\
	long err = 0; 							\
									\
	list_for_each_entry(sd, &(dev)->subdevs, list) { 		\
		if ((cond) && sd->ops->o && sd->ops->o->f) 		\
			err = sd->ops->o->f(sd , ##args); 		\
		if (err && err != -ENOIOCTLCMD)				\
			break; 						\
	} 								\
	(err == -ENOIOCTLCMD) ? 0 : err; 				\
})

#define v4l2_device_call_all(dev, grpid, o, f, args...) 		\
	__v4l2_device_call_subdevs(dev, 				\
			!(grpid) || sd->grp_id == (grpid), o, f , ##args)

#define v4l2_device_call_until_err(dev, grpid, o, f, args...) 		\
	__v4l2_device_call_subdevs_until_err(dev,			\
		       !(grpid) || sd->grp_id == (grpid), o, f , ##args)

#endif
