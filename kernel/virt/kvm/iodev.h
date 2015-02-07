

#ifndef __KVM_IODEV_H__
#define __KVM_IODEV_H__

#include <linux/kvm_types.h>

struct kvm_io_device {
	void (*read)(struct kvm_io_device *this,
		     gpa_t addr,
		     int len,
		     void *val);
	void (*write)(struct kvm_io_device *this,
		      gpa_t addr,
		      int len,
		      const void *val);
	int (*in_range)(struct kvm_io_device *this, gpa_t addr, int len,
			int is_write);
	void (*destructor)(struct kvm_io_device *this);

	void             *private;
};

static inline void kvm_iodevice_read(struct kvm_io_device *dev,
				     gpa_t addr,
				     int len,
				     void *val)
{
	dev->read(dev, addr, len, val);
}

static inline void kvm_iodevice_write(struct kvm_io_device *dev,
				      gpa_t addr,
				      int len,
				      const void *val)
{
	dev->write(dev, addr, len, val);
}

static inline int kvm_iodevice_inrange(struct kvm_io_device *dev,
				       gpa_t addr, int len, int is_write)
{
	return dev->in_range(dev, addr, len, is_write);
}

static inline void kvm_iodevice_destructor(struct kvm_io_device *dev)
{
	if (dev->destructor)
		dev->destructor(dev);
}

#endif /* __KVM_IODEV_H__ */
