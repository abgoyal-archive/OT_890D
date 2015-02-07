

#ifndef __HASH__
#define __HASH__

#include "crc32c.h"
static inline u64 btrfs_name_hash(const char *name, int len)
{
	return btrfs_crc32c((u32)~1, name, len);
}
#endif
