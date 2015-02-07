
#include <linux/linkage.h>
#include <linux/init.h>
#include <asm/page.h>
#include "proc-macros.S"

#define CACHE_DLINESIZE	32

#define CACHE_DSEGMENTS	8

#define CACHE_DENTRIES	64

#define CACHE_DLIMIT	16384

ENTRY(v4wt_flush_user_cache_all)
	/* FALLTHROUGH */
ENTRY(v4wt_flush_kern_cache_all)
	mov	r2, #VM_EXEC
	mov	ip, #0
__flush_whole_cache:
	tst	r2, #VM_EXEC
	mcrne	p15, 0, ip, c7, c5, 0		@ invalidate I cache
	mcr	p15, 0, ip, c7, c6, 0		@ invalidate D cache
	mov	pc, lr

ENTRY(v4wt_flush_user_cache_range)
	sub	r3, r1, r0			@ calculate total size
	cmp	r3, #CACHE_DLIMIT
	bhs	__flush_whole_cache

1:	mcr	p15, 0, r0, c7, c6, 1		@ invalidate D entry
	tst	r2, #VM_EXEC
	mcrne	p15, 0, r0, c7, c5, 1		@ invalidate I entry
	add	r0, r0, #CACHE_DLINESIZE
	cmp	r0, r1
	blo	1b
	mov	pc, lr

ENTRY(v4wt_coherent_kern_range)
	/* FALLTRHOUGH */

ENTRY(v4wt_coherent_user_range)
	bic	r0, r0, #CACHE_DLINESIZE - 1
1:	mcr	p15, 0, r0, c7, c5, 1		@ invalidate I entry
	add	r0, r0, #CACHE_DLINESIZE
	cmp	r0, r1
	blo	1b
	mov	pc, lr

ENTRY(v4wt_flush_kern_dcache_page)
	mov	r2, #0
	mcr	p15, 0, r2, c7, c5, 0		@ invalidate I cache
	add	r1, r0, #PAGE_SZ
	/* fallthrough */

ENTRY(v4wt_dma_inv_range)
	bic	r0, r0, #CACHE_DLINESIZE - 1
1:	mcr	p15, 0, r0, c7, c6, 1		@ invalidate D entry
	add	r0, r0, #CACHE_DLINESIZE
	cmp	r0, r1
	blo	1b
	/* FALLTHROUGH */

ENTRY(v4wt_dma_clean_range)
	mov	pc, lr

	.globl	v4wt_dma_flush_range
	.equ	v4wt_dma_flush_range, v4wt_dma_inv_range

	__INITDATA

	.type	v4wt_cache_fns, #object
ENTRY(v4wt_cache_fns)
	.long	v4wt_flush_kern_cache_all
	.long	v4wt_flush_user_cache_all
	.long	v4wt_flush_user_cache_range
	.long	v4wt_coherent_kern_range
	.long	v4wt_coherent_user_range
	.long	v4wt_flush_kern_dcache_page
	.long	v4wt_dma_inv_range
	.long	v4wt_dma_clean_range
	.long	v4wt_dma_flush_range
	.size	v4wt_cache_fns, . - v4wt_cache_fns
