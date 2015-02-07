
#include <linux/linkage.h>
#include <linux/init.h>
#include <asm/page.h>
#include "proc-macros.S"

ENTRY(v4_flush_user_cache_all)
	/* FALLTHROUGH */
ENTRY(v4_flush_kern_cache_all)
#ifdef CONFIG_CPU_CP15
	mov	r0, #0
	mcr	p15, 0, r0, c7, c7, 0		@ flush ID cache
	mov	pc, lr
#else
	/* FALLTHROUGH */
#endif

ENTRY(v4_flush_user_cache_range)
#ifdef CONFIG_CPU_CP15
	mov	ip, #0
	mcreq	p15, 0, ip, c7, c7, 0		@ flush ID cache
	mov	pc, lr
#else
	/* FALLTHROUGH */
#endif

ENTRY(v4_coherent_kern_range)
	/* FALLTHROUGH */

ENTRY(v4_coherent_user_range)
	mov	pc, lr

ENTRY(v4_flush_kern_dcache_page)
	/* FALLTHROUGH */

ENTRY(v4_dma_inv_range)
	/* FALLTHROUGH */

ENTRY(v4_dma_flush_range)
#ifdef CONFIG_CPU_CP15
	mov	r0, #0
	mcr	p15, 0, r0, c7, c7, 0		@ flush ID cache
#endif
	/* FALLTHROUGH */

ENTRY(v4_dma_clean_range)
	mov	pc, lr

	__INITDATA

	.type	v4_cache_fns, #object
ENTRY(v4_cache_fns)
	.long	v4_flush_kern_cache_all
	.long	v4_flush_user_cache_all
	.long	v4_flush_user_cache_range
	.long	v4_coherent_kern_range
	.long	v4_coherent_user_range
	.long	v4_flush_kern_dcache_page
	.long	v4_dma_inv_range
	.long	v4_dma_clean_range
	.long	v4_dma_flush_range
	.size	v4_cache_fns, . - v4_cache_fns
