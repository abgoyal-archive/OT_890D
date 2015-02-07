

#ifndef _UIO_DRIVER_H_
#define _UIO_DRIVER_H_

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/interrupt.h>

struct uio_map;

struct uio_mem {
	unsigned long		addr;
	unsigned long		size;
	int			memtype;
	void __iomem		*internal_addr;
	struct uio_map		*map;
};

#define MAX_UIO_MAPS	5

struct uio_portio;

struct uio_port {
	unsigned long		start;
	unsigned long		size;
	int			porttype;
	struct uio_portio	*portio;
};

#define MAX_UIO_PORT_REGIONS	5

struct uio_device;

struct uio_info {
	struct uio_device	*uio_dev;
	const char		*name;
	const char		*version;
	struct uio_mem		mem[MAX_UIO_MAPS];
	struct uio_port		port[MAX_UIO_PORT_REGIONS];
	long			irq;
	unsigned long		irq_flags;
	void			*priv;
	irqreturn_t (*handler)(int irq, struct uio_info *dev_info);
	int (*mmap)(struct uio_info *info, struct vm_area_struct *vma);
	int (*open)(struct uio_info *info, struct inode *inode);
	int (*release)(struct uio_info *info, struct inode *inode);
	int (*irqcontrol)(struct uio_info *info, s32 irq_on);
};

extern int __must_check
	__uio_register_device(struct module *owner,
			      struct device *parent,
			      struct uio_info *info);
static inline int __must_check
	uio_register_device(struct device *parent, struct uio_info *info)
{
	return __uio_register_device(THIS_MODULE, parent, info);
}
extern void uio_unregister_device(struct uio_info *info);
extern void uio_event_notify(struct uio_info *info);

/* defines for uio_info->irq */
#define UIO_IRQ_CUSTOM	-1
#define UIO_IRQ_NONE	-2

/* defines for uio_mem->memtype */
#define UIO_MEM_NONE	0
#define UIO_MEM_PHYS	1
#define UIO_MEM_LOGICAL	2
#define UIO_MEM_VIRTUAL 3

/* defines for uio_port->porttype */
#define UIO_PORT_NONE	0
#define UIO_PORT_X86	1
#define UIO_PORT_GPIO	2
#define UIO_PORT_OTHER	3

#endif /* _LINUX_UIO_DRIVER_H_ */
