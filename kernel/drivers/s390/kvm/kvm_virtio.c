

#include <linux/init.h>
#include <linux/bootmem.h>
#include <linux/err.h>
#include <linux/virtio.h>
#include <linux/virtio_config.h>
#include <linux/virtio_console.h>
#include <linux/interrupt.h>
#include <linux/virtio_ring.h>
#include <linux/pfn.h>
#include <asm/io.h>
#include <asm/kvm_para.h>
#include <asm/kvm_virtio.h>
#include <asm/setup.h>
#include <asm/s390_ext.h>

#define VIRTIO_SUBCODE_64 0x0D00

static void *kvm_devices;

struct kvm_device {
	struct virtio_device vdev;
	struct kvm_device_desc *desc;
};

#define to_kvmdev(vd) container_of(vd, struct kvm_device, vdev)

static struct kvm_vqconfig *kvm_vq_config(const struct kvm_device_desc *desc)
{
	return (struct kvm_vqconfig *)(desc + 1);
}

static u8 *kvm_vq_features(const struct kvm_device_desc *desc)
{
	return (u8 *)(kvm_vq_config(desc) + desc->num_vq);
}

static u8 *kvm_vq_configspace(const struct kvm_device_desc *desc)
{
	return kvm_vq_features(desc) + desc->feature_len * 2;
}

static unsigned desc_size(const struct kvm_device_desc *desc)
{
	return sizeof(*desc)
		+ desc->num_vq * sizeof(struct kvm_vqconfig)
		+ desc->feature_len * 2
		+ desc->config_len;
}

/* This gets the device's feature bits. */
static u32 kvm_get_features(struct virtio_device *vdev)
{
	unsigned int i;
	u32 features = 0;
	struct kvm_device_desc *desc = to_kvmdev(vdev)->desc;
	u8 *in_features = kvm_vq_features(desc);

	for (i = 0; i < min(desc->feature_len * 8, 32); i++)
		if (in_features[i / 8] & (1 << (i % 8)))
			features |= (1 << i);
	return features;
}

static void kvm_finalize_features(struct virtio_device *vdev)
{
	unsigned int i, bits;
	struct kvm_device_desc *desc = to_kvmdev(vdev)->desc;
	/* Second half of bitmap is features we accept. */
	u8 *out_features = kvm_vq_features(desc) + desc->feature_len;

	/* Give virtio_ring a chance to accept features. */
	vring_transport_features(vdev);

	memset(out_features, 0, desc->feature_len);
	bits = min_t(unsigned, desc->feature_len, sizeof(vdev->features)) * 8;
	for (i = 0; i < bits; i++) {
		if (test_bit(i, vdev->features))
			out_features[i / 8] |= (1 << (i % 8));
	}
}

static void kvm_get(struct virtio_device *vdev, unsigned int offset,
		   void *buf, unsigned len)
{
	struct kvm_device_desc *desc = to_kvmdev(vdev)->desc;

	BUG_ON(offset + len > desc->config_len);
	memcpy(buf, kvm_vq_configspace(desc) + offset, len);
}

static void kvm_set(struct virtio_device *vdev, unsigned int offset,
		   const void *buf, unsigned len)
{
	struct kvm_device_desc *desc = to_kvmdev(vdev)->desc;

	BUG_ON(offset + len > desc->config_len);
	memcpy(kvm_vq_configspace(desc) + offset, buf, len);
}

static u8 kvm_get_status(struct virtio_device *vdev)
{
	return to_kvmdev(vdev)->desc->status;
}

static void kvm_set_status(struct virtio_device *vdev, u8 status)
{
	BUG_ON(!status);
	to_kvmdev(vdev)->desc->status = status;
	kvm_hypercall1(KVM_S390_VIRTIO_SET_STATUS,
		       (unsigned long) to_kvmdev(vdev)->desc);
}

static void kvm_reset(struct virtio_device *vdev)
{
	kvm_hypercall1(KVM_S390_VIRTIO_RESET,
		       (unsigned long) to_kvmdev(vdev)->desc);
}

static void kvm_notify(struct virtqueue *vq)
{
	struct kvm_vqconfig *config = vq->priv;

	kvm_hypercall1(KVM_S390_VIRTIO_NOTIFY, config->address);
}

static struct virtqueue *kvm_find_vq(struct virtio_device *vdev,
				    unsigned index,
				    void (*callback)(struct virtqueue *vq))
{
	struct kvm_device *kdev = to_kvmdev(vdev);
	struct kvm_vqconfig *config;
	struct virtqueue *vq;
	int err;

	if (index >= kdev->desc->num_vq)
		return ERR_PTR(-ENOENT);

	config = kvm_vq_config(kdev->desc)+index;

	err = vmem_add_mapping(config->address,
			       vring_size(config->num,
					  KVM_S390_VIRTIO_RING_ALIGN));
	if (err)
		goto out;

	vq = vring_new_virtqueue(config->num, KVM_S390_VIRTIO_RING_ALIGN,
				 vdev, (void *) config->address,
				 kvm_notify, callback);
	if (!vq) {
		err = -ENOMEM;
		goto unmap;
	}

	/*
	 * register a callback token
	 * The host will sent this via the external interrupt parameter
	 */
	config->token = (u64) vq;

	vq->priv = config;
	return vq;
unmap:
	vmem_remove_mapping(config->address,
			    vring_size(config->num,
				       KVM_S390_VIRTIO_RING_ALIGN));
out:
	return ERR_PTR(err);
}

static void kvm_del_vq(struct virtqueue *vq)
{
	struct kvm_vqconfig *config = vq->priv;

	vring_del_virtqueue(vq);
	vmem_remove_mapping(config->address,
			    vring_size(config->num,
				       KVM_S390_VIRTIO_RING_ALIGN));
}

static struct virtio_config_ops kvm_vq_configspace_ops = {
	.get_features = kvm_get_features,
	.finalize_features = kvm_finalize_features,
	.get = kvm_get,
	.set = kvm_set,
	.get_status = kvm_get_status,
	.set_status = kvm_set_status,
	.reset = kvm_reset,
	.find_vq = kvm_find_vq,
	.del_vq = kvm_del_vq,
};

static struct device *kvm_root;

static void add_kvm_device(struct kvm_device_desc *d, unsigned int offset)
{
	struct kvm_device *kdev;

	kdev = kzalloc(sizeof(*kdev), GFP_KERNEL);
	if (!kdev) {
		printk(KERN_EMERG "Cannot allocate kvm dev %u type %u\n",
		       offset, d->type);
		return;
	}

	kdev->vdev.dev.parent = kvm_root;
	kdev->vdev.id.device = d->type;
	kdev->vdev.config = &kvm_vq_configspace_ops;
	kdev->desc = d;

	if (register_virtio_device(&kdev->vdev) != 0) {
		printk(KERN_ERR "Failed to register kvm device %u type %u\n",
		       offset, d->type);
		kfree(kdev);
	}
}

static void scan_devices(void)
{
	unsigned int i;
	struct kvm_device_desc *d;

	for (i = 0; i < PAGE_SIZE; i += desc_size(d)) {
		d = kvm_devices + i;

		if (d->type == 0)
			break;

		add_kvm_device(d, i);
	}
}

static void kvm_extint_handler(u16 code)
{
	struct virtqueue *vq;
	u16 subcode;
	int config_changed;

	subcode = S390_lowcore.cpu_addr;
	if ((subcode & 0xff00) != VIRTIO_SUBCODE_64)
		return;

	/* The LSB might be overloaded, we have to mask it */
	vq = (struct virtqueue *) ((*(long *) __LC_PFAULT_INTPARM) & ~1UL);

	/* We use the LSB of extparam, to decide, if this interrupt is a config
	 * change or a "standard" interrupt */
	config_changed =  (*(int *)  __LC_EXT_PARAMS & 1);

	if (config_changed) {
		struct virtio_driver *drv;
		drv = container_of(vq->vdev->dev.driver,
				   struct virtio_driver, driver);
		if (drv->config_changed)
			drv->config_changed(vq->vdev);
	} else
		vring_interrupt(0, vq);
}

static int __init kvm_devices_init(void)
{
	int rc;

	if (!MACHINE_IS_KVM)
		return -ENODEV;

	kvm_root = root_device_register("kvm_s390");
	if (IS_ERR(kvm_root)) {
		rc = PTR_ERR(kvm_root);
		printk(KERN_ERR "Could not register kvm_s390 root device");
		return rc;
	}

	rc = vmem_add_mapping(real_memory_size, PAGE_SIZE);
	if (rc) {
		root_device_unregister(kvm_root);
		return rc;
	}

	kvm_devices = (void *) real_memory_size;

	ctl_set_bit(0, 9);
	register_external_interrupt(0x2603, kvm_extint_handler);

	scan_devices();
	return 0;
}

/* code for early console output with virtio_console */
static __init int early_put_chars(u32 vtermno, const char *buf, int count)
{
	char scratch[17];
	unsigned int len = count;

	if (len > sizeof(scratch) - 1)
		len = sizeof(scratch) - 1;
	scratch[len] = '\0';
	memcpy(scratch, buf, len);
	kvm_hypercall1(KVM_S390_VIRTIO_NOTIFY, __pa(scratch));
	return len;
}

void __init s390_virtio_console_init(void)
{
	virtio_cons_early_init(early_put_chars);
}

postcore_initcall(kvm_devices_init);
