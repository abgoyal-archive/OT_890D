
#ifndef _ASM_IA64_TYPES_H
#define _ASM_IA64_TYPES_H


#include <asm-generic/int-l64.h>

#ifdef __ASSEMBLY__
# define __IA64_UL(x)		(x)
# define __IA64_UL_CONST(x)	x

# ifdef __KERNEL__
#  define BITS_PER_LONG 64
# endif

#else
# define __IA64_UL(x)		((unsigned long)(x))
# define __IA64_UL_CONST(x)	x##UL

typedef unsigned int umode_t;

# ifdef __KERNEL__

#define BITS_PER_LONG 64

/* DMA addresses are 64-bits wide, in general.  */

typedef u64 dma_addr_t;

# endif /* __KERNEL__ */
#endif /* !__ASSEMBLY__ */

#endif /* _ASM_IA64_TYPES_H */
