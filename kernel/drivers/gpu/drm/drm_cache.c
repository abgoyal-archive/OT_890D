

#include "drmP.h"

#if defined(CONFIG_X86)
static void
drm_clflush_page(struct page *page)
{
	uint8_t *page_virtual;
	unsigned int i;

	if (unlikely(page == NULL))
		return;

	page_virtual = kmap_atomic(page, KM_USER0);
	for (i = 0; i < PAGE_SIZE; i += boot_cpu_data.x86_clflush_size)
		clflush(page_virtual + i);
	kunmap_atomic(page_virtual, KM_USER0);
}
#endif

void
drm_clflush_pages(struct page *pages[], unsigned long num_pages)
{

#if defined(CONFIG_X86)
	if (cpu_has_clflush) {
		unsigned long i;

		mb();
		for (i = 0; i < num_pages; ++i)
			drm_clflush_page(*pages++);
		mb();

		return;
	}

	wbinvd();
#endif
}
EXPORT_SYMBOL(drm_clflush_pages);
