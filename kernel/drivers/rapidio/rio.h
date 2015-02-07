

#include <linux/device.h>
#include <linux/list.h>
#include <linux/rio.h>

/* Functions internal to the RIO core code */

extern u32 rio_mport_get_feature(struct rio_mport *mport, int local, u16 destid,
				 u8 hopcount, int ftr);
extern int rio_create_sysfs_dev_files(struct rio_dev *rdev);
extern int rio_enum_mport(struct rio_mport *mport);
extern int rio_disc_mport(struct rio_mport *mport);

/* Structures internal to the RIO core code */
extern struct device_attribute rio_dev_attrs[];
extern spinlock_t rio_global_list_lock;

extern struct rio_route_ops __start_rio_route_ops[];
extern struct rio_route_ops __end_rio_route_ops[];

/* Helpers internal to the RIO core code */
#define DECLARE_RIO_ROUTE_SECTION(section, vid, did, add_hook, get_hook)  \
	static struct rio_route_ops __rio_route_ops __used   \
	__section(section)= { vid, did, add_hook, get_hook };

#define DECLARE_RIO_ROUTE_OPS(vid, did, add_hook, get_hook)		\
	DECLARE_RIO_ROUTE_SECTION(.rio_route_ops,			\
			vid, did, add_hook, get_hook)

#define RIO_GET_DID(size, x)	(size ? (x & 0xffff) : ((x & 0x00ff0000) >> 16))
#define RIO_SET_DID(size, x)	(size ? (x & 0xffff) : ((x & 0x000000ff) << 16))
