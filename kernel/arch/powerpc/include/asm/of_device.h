
#ifndef _ASM_POWERPC_OF_DEVICE_H
#define _ASM_POWERPC_OF_DEVICE_H
#ifdef __KERNEL__

#include <linux/device.h>
#include <linux/of.h>

struct of_device
{
	struct device_node	*node;		/* to be obsoleted */
	u64			dma_mask;	/* DMA mask */
	struct device		dev;		/* Generic device interface */
};

extern struct of_device *of_device_alloc(struct device_node *np,
					 const char *bus_id,
					 struct device *parent);

extern int of_device_uevent(struct device *dev,
			    struct kobj_uevent_env *env);

#endif /* __KERNEL__ */
#endif /* _ASM_POWERPC_OF_DEVICE_H */
