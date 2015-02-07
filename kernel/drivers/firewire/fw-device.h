

#ifndef __fw_device_h
#define __fw_device_h

#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/idr.h>
#include <linux/rwsem.h>
#include <asm/atomic.h>

enum fw_device_state {
	FW_DEVICE_INITIALIZING,
	FW_DEVICE_RUNNING,
	FW_DEVICE_GONE,
	FW_DEVICE_SHUTDOWN,
};

struct fw_attribute_group {
	struct attribute_group *groups[2];
	struct attribute_group group;
	struct attribute *attrs[11];
};

struct fw_device {
	atomic_t state;
	struct fw_node *node;
	int node_id;
	int generation;
	unsigned max_speed;
	bool cmc;
	struct fw_card *card;
	struct device device;
	struct list_head client_list;
	u32 *config_rom;
	size_t config_rom_length;
	int config_rom_retries;
	struct delayed_work work;
	struct fw_attribute_group attribute_group;
};

static inline struct fw_device *fw_device(struct device *dev)
{
	return container_of(dev, struct fw_device, device);
}

static inline int fw_device_is_shutdown(struct fw_device *device)
{
	return atomic_read(&device->state) == FW_DEVICE_SHUTDOWN;
}

static inline struct fw_device *fw_device_get(struct fw_device *device)
{
	get_device(&device->device);

	return device;
}

static inline void fw_device_put(struct fw_device *device)
{
	put_device(&device->device);
}

struct fw_device *fw_device_get_by_devt(dev_t devt);
int fw_device_enable_phys_dma(struct fw_device *device);

void fw_device_cdev_update(struct fw_device *device);
void fw_device_cdev_remove(struct fw_device *device);

extern struct rw_semaphore fw_device_rwsem;
extern struct idr fw_device_idr;
extern int fw_cdev_major;

struct fw_unit {
	struct device device;
	u32 *directory;
	struct fw_attribute_group attribute_group;
};

static inline struct fw_unit *fw_unit(struct device *dev)
{
	return container_of(dev, struct fw_unit, device);
}

static inline struct fw_unit *fw_unit_get(struct fw_unit *unit)
{
	get_device(&unit->device);

	return unit;
}

static inline void fw_unit_put(struct fw_unit *unit)
{
	put_device(&unit->device);
}

#define CSR_OFFSET	0x40
#define CSR_LEAF	0x80
#define CSR_DIRECTORY	0xc0

#define CSR_DESCRIPTOR		0x01
#define CSR_VENDOR		0x03
#define CSR_HARDWARE_VERSION	0x04
#define CSR_NODE_CAPABILITIES	0x0c
#define CSR_UNIT		0x11
#define CSR_SPECIFIER_ID	0x12
#define CSR_VERSION		0x13
#define CSR_DEPENDENT_INFO	0x14
#define CSR_MODEL		0x17
#define CSR_INSTANCE		0x18
#define CSR_DIRECTORY_ID	0x20

struct fw_csr_iterator {
	u32 *p;
	u32 *end;
};

void fw_csr_iterator_init(struct fw_csr_iterator *ci, u32 *p);
int fw_csr_iterator_next(struct fw_csr_iterator *ci,
			 int *key, int *value);

#define FW_MATCH_VENDOR		0x0001
#define FW_MATCH_MODEL		0x0002
#define FW_MATCH_SPECIFIER_ID	0x0004
#define FW_MATCH_VERSION	0x0008

struct fw_device_id {
	u32 match_flags;
	u32 vendor;
	u32 model;
	u32 specifier_id;
	u32 version;
	void *driver_data;
};

struct fw_driver {
	struct device_driver driver;
	/* Called when the parent device sits through a bus reset. */
	void (*update) (struct fw_unit *unit);
	const struct fw_device_id *id_table;
};

static inline struct fw_driver *
fw_driver(struct device_driver *drv)
{
	return container_of(drv, struct fw_driver, driver);
}

extern const struct file_operations fw_device_ops;

#endif /* __fw_device_h */
