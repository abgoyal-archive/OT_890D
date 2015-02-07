

#ifndef AFS_CACHE_H
#define AFS_CACHE_H

#undef AFS_CACHING_SUPPORT

#include <linux/mm.h>
#ifdef AFS_CACHING_SUPPORT
#include <linux/cachefs.h>
#endif
#include "types.h"

#endif /* AFS_CACHE_H */
