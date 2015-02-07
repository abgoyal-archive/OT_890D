

#ifndef _LINUX_PM_WAKEUP_H
#define _LINUX_PM_WAKEUP_H

#ifndef _DEVICE_H_
# error "please don't include this file directly"
#endif

#ifdef CONFIG_PM

static inline void device_init_wakeup(struct device *dev, int val)
{
	dev->power.can_wakeup = dev->power.should_wakeup = !!val;
}

static inline void device_set_wakeup_capable(struct device *dev, int val)
{
	dev->power.can_wakeup = !!val;
}

static inline int device_can_wakeup(struct device *dev)
{
	return dev->power.can_wakeup;
}

static inline void device_set_wakeup_enable(struct device *dev, int val)
{
	dev->power.should_wakeup = !!val;
}

static inline int device_may_wakeup(struct device *dev)
{
	return dev->power.can_wakeup && dev->power.should_wakeup;
}

#else /* !CONFIG_PM */

/* For some reason the next two routines work even without CONFIG_PM */
static inline void device_init_wakeup(struct device *dev, int val)
{
	dev->power.can_wakeup = !!val;
}

static inline void device_set_wakeup_capable(struct device *dev, int val) { }

static inline int device_can_wakeup(struct device *dev)
{
	return dev->power.can_wakeup;
}

#define device_set_wakeup_enable(dev, val)	do {} while (0)
#define device_may_wakeup(dev)			0

#endif /* !CONFIG_PM */

#endif /* _LINUX_PM_WAKEUP_H */
