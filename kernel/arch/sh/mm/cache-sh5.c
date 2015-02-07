
#include <linux/init.h>
#include <linux/mman.h>
#include <linux/mm.h>
#include <asm/tlb.h>
#include <asm/processor.h>
#include <asm/cache.h>
#include <asm/pgalloc.h>
#include <asm/uaccess.h>
#include <asm/mmu_context.h>

/* Wired TLB entry for the D-cache */
static unsigned long long dtlb_cache_slot;

void __init p3_cache_init(void)
{
	/* Reserve a slot for dcache colouring in the DTLB */
	dtlb_cache_slot	= sh64_get_wired_dtlb_entry();
}

#ifdef CONFIG_DCACHE_DISABLED
#define sh64_dcache_purge_all()					do { } while (0)
#define sh64_dcache_purge_coloured_phy_page(paddr, eaddr)	do { } while (0)
#define sh64_dcache_purge_user_range(mm, start, end)		do { } while (0)
#define sh64_dcache_purge_phy_page(paddr)			do { } while (0)
#define sh64_dcache_purge_virt_page(mm, eaddr)			do { } while (0)
#endif

static inline void
sh64_setup_dtlb_cache_slot(unsigned long eaddr, unsigned long asid,
			   unsigned long paddr)
{
	local_irq_disable();
	sh64_setup_tlb_slot(dtlb_cache_slot, eaddr, asid, paddr);
}

static inline void sh64_teardown_dtlb_cache_slot(void)
{
	sh64_teardown_tlb_slot(dtlb_cache_slot);
	local_irq_enable();
}

#ifndef CONFIG_ICACHE_DISABLED
static inline void sh64_icache_inv_all(void)
{
	unsigned long long addr, flag, data;
	unsigned int flags;

	addr = ICCR0;
	flag = ICCR0_ICI;
	data = 0;

	/* Make this a critical section for safety (probably not strictly necessary.) */
	local_irq_save(flags);

	/* Without %1 it gets unexplicably wrong */
	__asm__ __volatile__ (
		"getcfg	%3, 0, %0\n\t"
		"or	%0, %2, %0\n\t"
		"putcfg	%3, 0, %0\n\t"
		"synci"
		: "=&r" (data)
		: "0" (data), "r" (flag), "r" (addr));

	local_irq_restore(flags);
}

static void sh64_icache_inv_kernel_range(unsigned long start, unsigned long end)
{
	/* Invalidate range of addresses [start,end] from the I-cache, where
	 * the addresses lie in the kernel superpage. */

	unsigned long long ullend, addr, aligned_start;
	aligned_start = (unsigned long long)(signed long long)(signed long) start;
	addr = L1_CACHE_ALIGN(aligned_start);
	ullend = (unsigned long long) (signed long long) (signed long) end;

	while (addr <= ullend) {
		__asm__ __volatile__ ("icbi %0, 0" : : "r" (addr));
		addr += L1_CACHE_BYTES;
	}
}

static void sh64_icache_inv_user_page(struct vm_area_struct *vma, unsigned long eaddr)
{
	/* If we get called, we know that vma->vm_flags contains VM_EXEC.
	   Also, eaddr is page-aligned. */
	unsigned int cpu = smp_processor_id();
	unsigned long long addr, end_addr;
	unsigned long flags = 0;
	unsigned long running_asid, vma_asid;
	addr = eaddr;
	end_addr = addr + PAGE_SIZE;

	/* Check whether we can use the current ASID for the I-cache
	   invalidation.  For example, if we're called via
	   access_process_vm->flush_cache_page->here, (e.g. when reading from
	   /proc), 'running_asid' will be that of the reader, not of the
	   victim.

	   Also, note the risk that we might get pre-empted between the ASID
	   compare and blocking IRQs, and before we regain control, the
	   pid->ASID mapping changes.  However, the whole cache will get
	   invalidated when the mapping is renewed, so the worst that can
	   happen is that the loop below ends up invalidating somebody else's
	   cache entries.
	*/

	running_asid = get_asid();
	vma_asid = cpu_asid(cpu, vma->vm_mm);
	if (running_asid != vma_asid) {
		local_irq_save(flags);
		switch_and_save_asid(vma_asid);
	}
	while (addr < end_addr) {
		/* Worth unrolling a little */
		__asm__ __volatile__("icbi %0,  0" : : "r" (addr));
		__asm__ __volatile__("icbi %0, 32" : : "r" (addr));
		__asm__ __volatile__("icbi %0, 64" : : "r" (addr));
		__asm__ __volatile__("icbi %0, 96" : : "r" (addr));
		addr += 128;
	}
	if (running_asid != vma_asid) {
		switch_and_save_asid(running_asid);
		local_irq_restore(flags);
	}
}

static void sh64_icache_inv_user_page_range(struct mm_struct *mm,
			  unsigned long start, unsigned long end)
{
	/* Used for invalidating big chunks of I-cache, i.e. assume the range
	   is whole pages.  If 'start' or 'end' is not page aligned, the code
	   is conservative and invalidates to the ends of the enclosing pages.
	   This is functionally OK, just a performance loss. */

	/* See the comments below in sh64_dcache_purge_user_range() regarding
	   the choice of algorithm.  However, for the I-cache option (2) isn't
	   available because there are no physical tags so aliases can't be
	   resolved.  The icbi instruction has to be used through the user
	   mapping.   Because icbi is cheaper than ocbp on a cache hit, it
	   would be cheaper to use the selective code for a large range than is
	   possible with the D-cache.  Just assume 64 for now as a working
	   figure.
	   */
	int n_pages;

	if (!mm)
		return;

	n_pages = ((end - start) >> PAGE_SHIFT);
	if (n_pages >= 64) {
		sh64_icache_inv_all();
	} else {
		unsigned long aligned_start;
		unsigned long eaddr;
		unsigned long after_last_page_start;
		unsigned long mm_asid, current_asid;
		unsigned long long flags = 0ULL;

		mm_asid = cpu_asid(smp_processor_id(), mm);
		current_asid = get_asid();

		if (mm_asid != current_asid) {
			/* Switch ASID and run the invalidate loop under cli */
			local_irq_save(flags);
			switch_and_save_asid(mm_asid);
		}

		aligned_start = start & PAGE_MASK;
		after_last_page_start = PAGE_SIZE + ((end - 1) & PAGE_MASK);

		while (aligned_start < after_last_page_start) {
			struct vm_area_struct *vma;
			unsigned long vma_end;
			vma = find_vma(mm, aligned_start);
			if (!vma || (aligned_start <= vma->vm_end)) {
				/* Avoid getting stuck in an error condition */
				aligned_start += PAGE_SIZE;
				continue;
			}
			vma_end = vma->vm_end;
			if (vma->vm_flags & VM_EXEC) {
				/* Executable */
				eaddr = aligned_start;
				while (eaddr < vma_end) {
					sh64_icache_inv_user_page(vma, eaddr);
					eaddr += PAGE_SIZE;
				}
			}
			aligned_start = vma->vm_end; /* Skip to start of next region */
		}

		if (mm_asid != current_asid) {
			switch_and_save_asid(current_asid);
			local_irq_restore(flags);
		}
	}
}

static void sh64_icache_inv_user_small_range(struct mm_struct *mm,
						unsigned long start, int len)
{
	unsigned long long eaddr = start;
	unsigned long long eaddr_end = start + len;
	unsigned long current_asid, mm_asid;
	unsigned long long flags;
	unsigned long long epage_start;

	/*
	 * Align to start of cache line.  Otherwise, suppose len==8 and
	 * start was at 32N+28 : the last 4 bytes wouldn't get invalidated.
	 */
	eaddr = L1_CACHE_ALIGN(start);
	eaddr_end = start + len;

	mm_asid = cpu_asid(smp_processor_id(), mm);
	local_irq_save(flags);
	current_asid = switch_and_save_asid(mm_asid);

	epage_start = eaddr & PAGE_MASK;

	while (eaddr < eaddr_end) {
		__asm__ __volatile__("icbi %0, 0" : : "r" (eaddr));
		eaddr += L1_CACHE_BYTES;
	}
	switch_and_save_asid(current_asid);
	local_irq_restore(flags);
}

static void sh64_icache_inv_current_user_range(unsigned long start, unsigned long end)
{
	/* The icbi instruction never raises ITLBMISS.  i.e. if there's not a
	   cache hit on the virtual tag the instruction ends there, without a
	   TLB lookup. */

	unsigned long long aligned_start;
	unsigned long long ull_end;
	unsigned long long addr;

	ull_end = end;

	/* Just invalidate over the range using the natural addresses.  TLB
	   miss handling will be OK (TBC).  Since it's for the current process,
	   either we're already in the right ASID context, or the ASIDs have
	   been recycled since we were last active in which case we might just
	   invalidate another processes I-cache entries : no worries, just a
	   performance drop for him. */
	aligned_start = L1_CACHE_ALIGN(start);
	addr = aligned_start;
	while (addr < ull_end) {
		__asm__ __volatile__ ("icbi %0, 0" : : "r" (addr));
		__asm__ __volatile__ ("nop");
		__asm__ __volatile__ ("nop");
		addr += L1_CACHE_BYTES;
	}
}
#endif /* !CONFIG_ICACHE_DISABLED */

#ifndef CONFIG_DCACHE_DISABLED
#define DUMMY_ALLOCO_AREA_SIZE ((L1_CACHE_BYTES << 10) + (1024 * 4))
static unsigned char dummy_alloco_area[DUMMY_ALLOCO_AREA_SIZE] __cacheline_aligned = { 0, };

static void inline sh64_dcache_purge_sets(int sets_to_purge_base, int n_sets)
{
	/* Purge all ways in a particular block of sets, specified by the base
	   set number and number of sets.  Can handle wrap-around, if that's
	   needed.  */

	int dummy_buffer_base_set;
	unsigned long long eaddr, eaddr0, eaddr1;
	int j;
	int set_offset;

	dummy_buffer_base_set = ((int)&dummy_alloco_area &
				 cpu_data->dcache.entry_mask) >>
				 cpu_data->dcache.entry_shift;
	set_offset = sets_to_purge_base - dummy_buffer_base_set;

	for (j = 0; j < n_sets; j++, set_offset++) {
		set_offset &= (cpu_data->dcache.sets - 1);
		eaddr0 = (unsigned long long)dummy_alloco_area +
			(set_offset << cpu_data->dcache.entry_shift);

		/*
		 * Do one alloco which hits the required set per cache
		 * way.  For write-back mode, this will purge the #ways
		 * resident lines.  There's little point unrolling this
		 * loop because the allocos stall more if they're too
		 * close together.
		 */
		eaddr1 = eaddr0 + cpu_data->dcache.way_size *
				  cpu_data->dcache.ways;

		for (eaddr = eaddr0; eaddr < eaddr1;
		     eaddr += cpu_data->dcache.way_size) {
			__asm__ __volatile__ ("alloco %0, 0" : : "r" (eaddr));
			__asm__ __volatile__ ("synco"); /* TAKum03020 */
		}

		eaddr1 = eaddr0 + cpu_data->dcache.way_size *
				  cpu_data->dcache.ways;

		for (eaddr = eaddr0; eaddr < eaddr1;
		     eaddr += cpu_data->dcache.way_size) {
			/*
			 * Load from each address.  Required because
			 * alloco is a NOP if the cache is write-through.
			 */
			if (test_bit(SH_CACHE_MODE_WT, &(cpu_data->dcache.flags)))
				ctrl_inb(eaddr);
		}
	}

	/*
	 * Don't use OCBI to invalidate the lines.  That costs cycles
	 * directly.  If the dummy block is just left resident, it will
	 * naturally get evicted as required.
	 */
}

static void sh64_dcache_purge_all(void)
{

	sh64_dcache_purge_sets(0, cpu_data->dcache.sets);
}


#define MAGIC_PAGE0_START 0xffffffffec000000ULL

static void sh64_dcache_purge_coloured_phy_page(unsigned long paddr,
					        unsigned long eaddr)
{
	unsigned long long magic_page_start;
	unsigned long long magic_eaddr, magic_eaddr_end;

	magic_page_start = MAGIC_PAGE0_START + (eaddr & CACHE_OC_SYN_MASK);

	/* As long as the kernel is not pre-emptible, this doesn't need to be
	   under cli/sti. */
	sh64_setup_dtlb_cache_slot(magic_page_start, get_asid(), paddr);

	magic_eaddr = magic_page_start;
	magic_eaddr_end = magic_eaddr + PAGE_SIZE;

	while (magic_eaddr < magic_eaddr_end) {
		/* Little point in unrolling this loop - the OCBPs are blocking
		   and won't go any quicker (i.e. the loop overhead is parallel
		   to part of the OCBP execution.) */
		__asm__ __volatile__ ("ocbp %0, 0" : : "r" (magic_eaddr));
		magic_eaddr += L1_CACHE_BYTES;
	}

	sh64_teardown_dtlb_cache_slot();
}

static void sh64_dcache_purge_phy_page(unsigned long paddr)
{
	unsigned long long eaddr_start, eaddr, eaddr_end;
	int i;

	/* As long as the kernel is not pre-emptible, this doesn't need to be
	   under cli/sti. */
	eaddr_start = MAGIC_PAGE0_START;
	for (i = 0; i < (1 << CACHE_OC_N_SYNBITS); i++) {
		sh64_setup_dtlb_cache_slot(eaddr_start, get_asid(), paddr);

		eaddr = eaddr_start;
		eaddr_end = eaddr + PAGE_SIZE;
		while (eaddr < eaddr_end) {
			__asm__ __volatile__ ("ocbp %0, 0" : : "r" (eaddr));
			eaddr += L1_CACHE_BYTES;
		}

		sh64_teardown_dtlb_cache_slot();
		eaddr_start += PAGE_SIZE;
	}
}

static void sh64_dcache_purge_user_pages(struct mm_struct *mm,
				unsigned long addr, unsigned long end)
{
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;
	pte_t entry;
	spinlock_t *ptl;
	unsigned long paddr;

	if (!mm)
		return; /* No way to find physical address of page */

	pgd = pgd_offset(mm, addr);
	if (pgd_bad(*pgd))
		return;

	pud = pud_offset(pgd, addr);
	if (pud_none(*pud) || pud_bad(*pud))
		return;

	pmd = pmd_offset(pud, addr);
	if (pmd_none(*pmd) || pmd_bad(*pmd))
		return;

	pte = pte_offset_map_lock(mm, pmd, addr, &ptl);
	do {
		entry = *pte;
		if (pte_none(entry) || !pte_present(entry))
			continue;
		paddr = pte_val(entry) & PAGE_MASK;
		sh64_dcache_purge_coloured_phy_page(paddr, addr);
	} while (pte++, addr += PAGE_SIZE, addr != end);
	pte_unmap_unlock(pte - 1, ptl);
}

static void sh64_dcache_purge_user_range(struct mm_struct *mm,
			  unsigned long start, unsigned long end)
{
	int n_pages = ((end - start) >> PAGE_SHIFT);

	if (n_pages >= 64 || ((start ^ (end - 1)) & PMD_MASK)) {
		sh64_dcache_purge_all();
	} else {
		/* Small range, covered by a single page table page */
		start &= PAGE_MASK;	/* should already be so */
		end = PAGE_ALIGN(end);	/* should already be so */
		sh64_dcache_purge_user_pages(mm, start, end);
	}
}

void __flush_purge_region(void *start, int size)
{
	unsigned long long ullend, addr, aligned_start;

	aligned_start = (unsigned long long)(signed long long)(signed long) start;
	addr = L1_CACHE_ALIGN(aligned_start);
	ullend = (unsigned long long) (signed long long) (signed long) start + size;

	while (addr <= ullend) {
		__asm__ __volatile__ ("ocbp %0, 0" : : "r" (addr));
		addr += L1_CACHE_BYTES;
	}
}

void __flush_wback_region(void *start, int size)
{
	unsigned long long ullend, addr, aligned_start;

	aligned_start = (unsigned long long)(signed long long)(signed long) start;
	addr = L1_CACHE_ALIGN(aligned_start);
	ullend = (unsigned long long) (signed long long) (signed long) start + size;

	while (addr < ullend) {
		__asm__ __volatile__ ("ocbwb %0, 0" : : "r" (addr));
		addr += L1_CACHE_BYTES;
	}
}

void __flush_invalidate_region(void *start, int size)
{
	unsigned long long ullend, addr, aligned_start;

	aligned_start = (unsigned long long)(signed long long)(signed long) start;
	addr = L1_CACHE_ALIGN(aligned_start);
	ullend = (unsigned long long) (signed long long) (signed long) start + size;

	while (addr < ullend) {
		__asm__ __volatile__ ("ocbi %0, 0" : : "r" (addr));
		addr += L1_CACHE_BYTES;
	}
}
#endif /* !CONFIG_DCACHE_DISABLED */

void flush_cache_all(void)
{
	sh64_dcache_purge_all();
	sh64_icache_inv_all();
}

void flush_cache_mm(struct mm_struct *mm)
{
	sh64_dcache_purge_all();
}

void flush_cache_range(struct vm_area_struct *vma, unsigned long start,
		       unsigned long end)
{
	struct mm_struct *mm = vma->vm_mm;

	sh64_dcache_purge_user_range(mm, start, end);
	sh64_icache_inv_user_page_range(mm, start, end);
}

void flush_cache_page(struct vm_area_struct *vma, unsigned long eaddr,
		      unsigned long pfn)
{
	sh64_dcache_purge_phy_page(pfn << PAGE_SHIFT);

	if (vma->vm_flags & VM_EXEC)
		sh64_icache_inv_user_page(vma, eaddr);
}

void flush_dcache_page(struct page *page)
{
	sh64_dcache_purge_phy_page(page_to_phys(page));
	wmb();
}

void flush_icache_range(unsigned long start, unsigned long end)
{
	__flush_purge_region((void *)start, end);
	wmb();
	sh64_icache_inv_kernel_range(start, end);
}

void flush_icache_user_range(struct vm_area_struct *vma,
			struct page *page, unsigned long addr, int len)
{

	sh64_dcache_purge_coloured_phy_page(page_to_phys(page), addr);
	mb();

	if (vma->vm_flags & VM_EXEC)
		sh64_icache_inv_user_small_range(vma->vm_mm, addr, len);
}

void flush_cache_sigtramp(unsigned long vaddr)
{
	unsigned long end = vaddr + L1_CACHE_BYTES;

	__flush_wback_region((void *)vaddr, L1_CACHE_BYTES);
	wmb();
	sh64_icache_inv_current_user_range(vaddr, end);
}

#ifdef CONFIG_MMU
#define UNIQUE_EADDR_START 0xe0000000UL
#define UNIQUE_EADDR_END   0xe8000000UL

static unsigned long sh64_make_unique_eaddr(unsigned long user_eaddr,
					    unsigned long paddr)
{
	static unsigned long current_pointer = UNIQUE_EADDR_START;
	unsigned long coloured_pointer;

	if (current_pointer == UNIQUE_EADDR_END) {
		sh64_dcache_purge_all();
		current_pointer = UNIQUE_EADDR_START;
	}

	coloured_pointer = (current_pointer & ~CACHE_OC_SYN_MASK) |
				(user_eaddr & CACHE_OC_SYN_MASK);
	sh64_setup_dtlb_cache_slot(coloured_pointer, get_asid(), paddr);

	current_pointer += (PAGE_SIZE << CACHE_OC_N_SYNBITS);

	return coloured_pointer;
}

static void sh64_copy_user_page_coloured(void *to, void *from,
					 unsigned long address)
{
	void *coloured_to;

	/*
	 * Discard any existing cache entries of the wrong colour.  These are
	 * present quite often, if the kernel has recently used the page
	 * internally, then given it up, then it's been allocated to the user.
	 */
	sh64_dcache_purge_coloured_phy_page(__pa(to), (unsigned long)to);

	coloured_to = (void *)sh64_make_unique_eaddr(address, __pa(to));
	copy_page(from, coloured_to);

	sh64_teardown_dtlb_cache_slot();
}

static void sh64_clear_user_page_coloured(void *to, unsigned long address)
{
	void *coloured_to;

	/*
	 * Discard any existing kernel-originated lines of the wrong
	 * colour (as above)
	 */
	sh64_dcache_purge_coloured_phy_page(__pa(to), (unsigned long)to);

	coloured_to = (void *)sh64_make_unique_eaddr(address, __pa(to));
	clear_page(coloured_to);

	sh64_teardown_dtlb_cache_slot();
}

void copy_user_page(void *to, void *from, unsigned long address,
		    struct page *page)
{
	if (((address ^ (unsigned long) from) & CACHE_OC_SYN_MASK) != 0)
		sh64_dcache_purge_coloured_phy_page(__pa(from), address);

	if (((address ^ (unsigned long) to) & CACHE_OC_SYN_MASK) == 0)
		copy_page(to, from);
	else
		sh64_copy_user_page_coloured(to, from, address);
}

void clear_user_page(void *to, unsigned long address, struct page *page)
{
	if (((address ^ (unsigned long) to) & CACHE_OC_SYN_MASK) == 0)
		clear_page(to);
	else
		sh64_clear_user_page_coloured(to, address);
}
#endif
