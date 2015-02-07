
#ifndef _ASM_X86_AGP_H
#define _ASM_X86_AGP_H

#include <asm/pgtable.h>
#include <asm/cacheflush.h>


#define map_page_into_agp(page) set_pages_uc(page, 1)
#define unmap_page_from_agp(page) set_pages_wb(page, 1)

#define flush_agp_cache() wbinvd()

/* Convert a physical address to an address suitable for the GART. */
#define phys_to_gart(x) (x)
#define gart_to_phys(x) (x)

/* GATT allocation. Returns/accepts GATT kernel virtual address. */
#define alloc_gatt_pages(order)		\
	((char *)__get_free_pages(GFP_KERNEL, (order)))
#define free_gatt_pages(table, order)	\
	free_pages((unsigned long)(table), (order))

#endif /* _ASM_X86_AGP_H */
