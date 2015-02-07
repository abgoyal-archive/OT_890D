
#include <linux/linkage.h>
#include <linux/init.h>
#include <asm/assembler.h>

#include "proc-macros.S"

#define HARVARD_CACHE
#define CACHE_LINE_SIZE		32
#define D_CACHE_LINE_SIZE	32
#define BTB_FLUSH_SIZE		8

ENTRY(v6_flush_kern_cache_all)
	mov	r0, #0
#ifdef HARVARD_CACHE
	mcr	p15, 0, r0, c7, c14, 0		@ D cache clean+invalidate
	mcr	p15, 0, r0, c7, c5, 0		@ I+BTB cache invalidate
#else
	mcr	p15, 0, r0, c7, c15, 0		@ Cache clean+invalidate
#endif
	mov	pc, lr

ENTRY(v6_flush_user_cache_all)
	/*FALLTHROUGH*/

ENTRY(v6_flush_user_cache_range)
	mov	pc, lr

ENTRY(v6_coherent_kern_range)
	/* FALLTHROUGH */

ENTRY(v6_coherent_user_range)

#ifdef HARVARD_CACHE
	bic	r0, r0, #CACHE_LINE_SIZE - 1
1:	mcr	p15, 0, r0, c7, c10, 1		@ clean D line
	add	r0, r0, #CACHE_LINE_SIZE
	cmp	r0, r1
	blo	1b
#endif
	mov	r0, #0
#ifdef HARVARD_CACHE
	mcr	p15, 0, r0, c7, c10, 4		@ drain write buffer
	mcr	p15, 0, r0, c7, c5, 0		@ I+BTB cache invalidate
#else
	mcr	p15, 0, r0, c7, c5, 6		@ invalidate BTB
#endif
	mov	pc, lr

ENTRY(v6_flush_kern_dcache_page)
	add	r1, r0, #PAGE_SZ
1:
#ifdef HARVARD_CACHE
	mcr	p15, 0, r0, c7, c14, 1		@ clean & invalidate D line
#else
	mcr	p15, 0, r0, c7, c15, 1		@ clean & invalidate unified line
#endif	
	add	r0, r0, #D_CACHE_LINE_SIZE
	cmp	r0, r1
	blo	1b
#ifdef HARVARD_CACHE
	mov	r0, #0
	mcr	p15, 0, r0, c7, c10, 4
#endif
	mov	pc, lr


ENTRY(v6_dma_inv_range)
	tst	r0, #D_CACHE_LINE_SIZE - 1
	bic	r0, r0, #D_CACHE_LINE_SIZE - 1
#ifdef HARVARD_CACHE
	mcrne	p15, 0, r0, c7, c10, 1		@ clean D line
#else
	mcrne	p15, 0, r0, c7, c11, 1		@ clean unified line
#endif
	tst	r1, #D_CACHE_LINE_SIZE - 1
	bic	r1, r1, #D_CACHE_LINE_SIZE - 1
#ifdef HARVARD_CACHE
	mcrne	p15, 0, r1, c7, c14, 1		@ clean & invalidate D line
#else
	mcrne	p15, 0, r1, c7, c15, 1		@ clean & invalidate unified line
#endif
1:
#ifdef HARVARD_CACHE
	mcr	p15, 0, r0, c7, c6, 1		@ invalidate D line
#else
	mcr	p15, 0, r0, c7, c7, 1		@ invalidate unified line
#endif
	add	r0, r0, #D_CACHE_LINE_SIZE
	cmp	r0, r1
	blo	1b
	mov	r0, #0
	mcr	p15, 0, r0, c7, c10, 4		@ drain write buffer
	mov	pc, lr

ENTRY(v6_dma_clean_range)
	bic	r0, r0, #D_CACHE_LINE_SIZE - 1
1:
#ifdef HARVARD_CACHE
	mcr	p15, 0, r0, c7, c10, 1		@ clean D line
#else
	mcr	p15, 0, r0, c7, c11, 1		@ clean unified line
#endif
	add	r0, r0, #D_CACHE_LINE_SIZE
	cmp	r0, r1
	blo	1b
	mov	r0, #0
	mcr	p15, 0, r0, c7, c10, 4		@ drain write buffer
	mov	pc, lr

ENTRY(v6_dma_flush_range)
#ifdef CONFIG_CACHE_FLUSH_RANGE_LIMIT
	sub	r2, r1, r0
	cmp	r2, #CONFIG_CACHE_FLUSH_RANGE_LIMIT
	bhi	v6_dma_flush_dcache_all
#endif
	bic	r0, r0, #D_CACHE_LINE_SIZE - 1
1:
#ifdef HARVARD_CACHE
	mcr	p15, 0, r0, c7, c14, 1		@ clean & invalidate D line
#else
	mcr	p15, 0, r0, c7, c15, 1		@ clean & invalidate line
#endif
	add	r0, r0, #D_CACHE_LINE_SIZE
	cmp	r0, r1
	blo	1b
	mov	r0, #0
	mcr	p15, 0, r0, c7, c10, 4		@ drain write buffer
	mov	pc, lr

#ifdef CONFIG_CACHE_FLUSH_RANGE_LIMIT
v6_dma_flush_dcache_all:
	mov	r0, #0
#ifdef HARVARD_CACHE
	mcr	p15, 0, r0, c7, c14, 0		@ D cache clean+invalidate
#else
	mcr	p15, 0, r0, c7, c15, 0		@ Cache clean+invalidate
#endif
	mcr	p15, 0, r0, c7, c10, 4		@ drain write buffer
	mov	pc, lr
#endif

	__INITDATA

	.type	v6_cache_fns, #object
ENTRY(v6_cache_fns)
	.long	v6_flush_kern_cache_all
	.long	v6_flush_user_cache_all
	.long	v6_flush_user_cache_range
	.long	v6_coherent_kern_range
	.long	v6_coherent_user_range
	.long	v6_flush_kern_dcache_page
	.long	v6_dma_inv_range
	.long	v6_dma_clean_range
	.long	v6_dma_flush_range
	.size	v6_cache_fns, . - v6_cache_fns
