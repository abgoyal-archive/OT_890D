
#ifndef _ALPHA_TYPES_H
#define _ALPHA_TYPES_H

#include <asm-generic/int-l64.h>

#ifndef __ASSEMBLY__

typedef unsigned int umode_t;

#endif /* __ASSEMBLY__ */

#ifdef __KERNEL__

#define BITS_PER_LONG 64

#ifndef __ASSEMBLY__

typedef u64 dma_addr_t;
typedef u64 dma64_addr_t;

#endif /* __ASSEMBLY__ */
#endif /* __KERNEL__ */
#endif /* _ALPHA_TYPES_H */
