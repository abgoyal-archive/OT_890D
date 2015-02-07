

#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <asm/cacheflush.h>
#include <asm/pgtable.h>
#include <asm/tlbflush.h>

int
is_io_mapping_possible(resource_size_t base, unsigned long size);

void *
iomap_atomic_prot_pfn(unsigned long pfn, enum km_type type, pgprot_t prot);

void
iounmap_atomic(void *kvaddr, enum km_type type);
