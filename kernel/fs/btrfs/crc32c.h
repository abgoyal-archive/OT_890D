

#ifndef __BTRFS_CRC32C__
#define __BTRFS_CRC32C__
#include <linux/crc32c.h>

#define btrfs_crc32c(seed, data, length) crc32c(seed, data, length)
#endif

