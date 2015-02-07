

#include <linux/mm.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/random.h>
#include <linux/percpu.h>
#include <asm/tlbflush.h>
#include <asm/uaccess.h>
#include <asm/bootparam.h>
#include "lg.h"




#define SWITCHER_PGD_INDEX (PTRS_PER_PGD - 1)

static DEFINE_PER_CPU(pte_t *, switcher_pte_pages);
#define switcher_pte_page(cpu) per_cpu(switcher_pte_pages, cpu)

static pgd_t *spgd_addr(struct lg_cpu *cpu, u32 i, unsigned long vaddr)
{
	unsigned int index = pgd_index(vaddr);

	/* We kill any Guest trying to touch the Switcher addresses. */
	if (index >= SWITCHER_PGD_INDEX) {
		kill_guest(cpu, "attempt to access switcher pages");
		index = 0;
	}
	/* Return a pointer index'th pgd entry for the i'th page table. */
	return &cpu->lg->pgdirs[i].pgdir[index];
}

static pte_t *spte_addr(pgd_t spgd, unsigned long vaddr)
{
	pte_t *page = __va(pgd_pfn(spgd) << PAGE_SHIFT);
	/* You should never call this if the PGD entry wasn't valid */
	BUG_ON(!(pgd_flags(spgd) & _PAGE_PRESENT));
	return &page[(vaddr >> PAGE_SHIFT) % PTRS_PER_PTE];
}

static unsigned long gpgd_addr(struct lg_cpu *cpu, unsigned long vaddr)
{
	unsigned int index = vaddr >> (PGDIR_SHIFT);
	return cpu->lg->pgdirs[cpu->cpu_pgd].gpgdir + index * sizeof(pgd_t);
}

static unsigned long gpte_addr(pgd_t gpgd, unsigned long vaddr)
{
	unsigned long gpage = pgd_pfn(gpgd) << PAGE_SHIFT;
	BUG_ON(!(pgd_flags(gpgd) & _PAGE_PRESENT));
	return gpage + ((vaddr>>PAGE_SHIFT) % PTRS_PER_PTE) * sizeof(pte_t);
}
/*:*/


static unsigned long get_pfn(unsigned long virtpfn, int write)
{
	struct page *page;

	/* gup me one page at this address please! */
	if (get_user_pages_fast(virtpfn << PAGE_SHIFT, 1, write, &page) == 1)
		return page_to_pfn(page);

	/* This value indicates failure. */
	return -1UL;
}

static pte_t gpte_to_spte(struct lg_cpu *cpu, pte_t gpte, int write)
{
	unsigned long pfn, base, flags;

	/* The Guest sets the global flag, because it thinks that it is using
	 * PGE.  We only told it to use PGE so it would tell us whether it was
	 * flushing a kernel mapping or a userspace mapping.  We don't actually
	 * use the global bit, so throw it away. */
	flags = (pte_flags(gpte) & ~_PAGE_GLOBAL);

	/* The Guest's pages are offset inside the Launcher. */
	base = (unsigned long)cpu->lg->mem_base / PAGE_SIZE;

	/* We need a temporary "unsigned long" variable to hold the answer from
	 * get_pfn(), because it returns 0xFFFFFFFF on failure, which wouldn't
	 * fit in spte.pfn.  get_pfn() finds the real physical number of the
	 * page, given the virtual number. */
	pfn = get_pfn(base + pte_pfn(gpte), write);
	if (pfn == -1UL) {
		kill_guest(cpu, "failed to get page %lu", pte_pfn(gpte));
		/* When we destroy the Guest, we'll go through the shadow page
		 * tables and release_pte() them.  Make sure we don't think
		 * this one is valid! */
		flags = 0;
	}
	/* Now we assemble our shadow PTE from the page number and flags. */
	return pfn_pte(pfn, __pgprot(flags));
}

/*H:460 And to complete the chain, release_pte() looks like this: */
static void release_pte(pte_t pte)
{
	/* Remember that get_user_pages_fast() took a reference to the page, in
	 * get_pfn()?  We have to put it back now. */
	if (pte_flags(pte) & _PAGE_PRESENT)
		put_page(pfn_to_page(pte_pfn(pte)));
}
/*:*/

static void check_gpte(struct lg_cpu *cpu, pte_t gpte)
{
	if ((pte_flags(gpte) & _PAGE_PSE) ||
	    pte_pfn(gpte) >= cpu->lg->pfn_limit)
		kill_guest(cpu, "bad page table entry");
}

static void check_gpgd(struct lg_cpu *cpu, pgd_t gpgd)
{
	if ((pgd_flags(gpgd) & ~_PAGE_TABLE) ||
	   (pgd_pfn(gpgd) >= cpu->lg->pfn_limit))
		kill_guest(cpu, "bad page directory entry");
}

int demand_page(struct lg_cpu *cpu, unsigned long vaddr, int errcode)
{
	pgd_t gpgd;
	pgd_t *spgd;
	unsigned long gpte_ptr;
	pte_t gpte;
	pte_t *spte;

	/* First step: get the top-level Guest page table entry. */
	gpgd = lgread(cpu, gpgd_addr(cpu, vaddr), pgd_t);
	/* Toplevel not present?  We can't map it in. */
	if (!(pgd_flags(gpgd) & _PAGE_PRESENT))
		return 0;

	/* Now look at the matching shadow entry. */
	spgd = spgd_addr(cpu, cpu->cpu_pgd, vaddr);
	if (!(pgd_flags(*spgd) & _PAGE_PRESENT)) {
		/* No shadow entry: allocate a new shadow PTE page. */
		unsigned long ptepage = get_zeroed_page(GFP_KERNEL);
		/* This is not really the Guest's fault, but killing it is
		 * simple for this corner case. */
		if (!ptepage) {
			kill_guest(cpu, "out of memory allocating pte page");
			return 0;
		}
		/* We check that the Guest pgd is OK. */
		check_gpgd(cpu, gpgd);
		/* And we copy the flags to the shadow PGD entry.  The page
		 * number in the shadow PGD is the page we just allocated. */
		*spgd = __pgd(__pa(ptepage) | pgd_flags(gpgd));
	}

	/* OK, now we look at the lower level in the Guest page table: keep its
	 * address, because we might update it later. */
	gpte_ptr = gpte_addr(gpgd, vaddr);
	gpte = lgread(cpu, gpte_ptr, pte_t);

	/* If this page isn't in the Guest page tables, we can't page it in. */
	if (!(pte_flags(gpte) & _PAGE_PRESENT))
		return 0;

	/* Check they're not trying to write to a page the Guest wants
	 * read-only (bit 2 of errcode == write). */
	if ((errcode & 2) && !(pte_flags(gpte) & _PAGE_RW))
		return 0;

	/* User access to a kernel-only page? (bit 3 == user access) */
	if ((errcode & 4) && !(pte_flags(gpte) & _PAGE_USER))
		return 0;

	/* Check that the Guest PTE flags are OK, and the page number is below
	 * the pfn_limit (ie. not mapping the Launcher binary). */
	check_gpte(cpu, gpte);

	/* Add the _PAGE_ACCESSED and (for a write) _PAGE_DIRTY flag */
	gpte = pte_mkyoung(gpte);
	if (errcode & 2)
		gpte = pte_mkdirty(gpte);

	/* Get the pointer to the shadow PTE entry we're going to set. */
	spte = spte_addr(*spgd, vaddr);
	/* If there was a valid shadow PTE entry here before, we release it.
	 * This can happen with a write to a previously read-only entry. */
	release_pte(*spte);

	/* If this is a write, we insist that the Guest page is writable (the
	 * final arg to gpte_to_spte()). */
	if (pte_dirty(gpte))
		*spte = gpte_to_spte(cpu, gpte, 1);
	else
		/* If this is a read, don't set the "writable" bit in the page
		 * table entry, even if the Guest says it's writable.  That way
		 * we will come back here when a write does actually occur, so
		 * we can update the Guest's _PAGE_DIRTY flag. */
		*spte = gpte_to_spte(cpu, pte_wrprotect(gpte), 0);

	/* Finally, we write the Guest PTE entry back: we've set the
	 * _PAGE_ACCESSED and maybe the _PAGE_DIRTY flags. */
	lgwrite(cpu, gpte_ptr, pte_t, gpte);

	/* The fault is fixed, the page table is populated, the mapping
	 * manipulated, the result returned and the code complete.  A small
	 * delay and a trace of alliteration are the only indications the Guest
	 * has that a page fault occurred at all. */
	return 1;
}

static int page_writable(struct lg_cpu *cpu, unsigned long vaddr)
{
	pgd_t *spgd;
	unsigned long flags;

	/* Look at the current top level entry: is it present? */
	spgd = spgd_addr(cpu, cpu->cpu_pgd, vaddr);
	if (!(pgd_flags(*spgd) & _PAGE_PRESENT))
		return 0;

	/* Check the flags on the pte entry itself: it must be present and
	 * writable. */
	flags = pte_flags(*(spte_addr(*spgd, vaddr)));

	return (flags & (_PAGE_PRESENT|_PAGE_RW)) == (_PAGE_PRESENT|_PAGE_RW);
}

void pin_page(struct lg_cpu *cpu, unsigned long vaddr)
{
	if (!page_writable(cpu, vaddr) && !demand_page(cpu, vaddr, 2))
		kill_guest(cpu, "bad stack page %#lx", vaddr);
}

/*H:450 If we chase down the release_pgd() code, it looks like this: */
static void release_pgd(struct lguest *lg, pgd_t *spgd)
{
	/* If the entry's not present, there's nothing to release. */
	if (pgd_flags(*spgd) & _PAGE_PRESENT) {
		unsigned int i;
		/* Converting the pfn to find the actual PTE page is easy: turn
		 * the page number into a physical address, then convert to a
		 * virtual address (easy for kernel pages like this one). */
		pte_t *ptepage = __va(pgd_pfn(*spgd) << PAGE_SHIFT);
		/* For each entry in the page, we might need to release it. */
		for (i = 0; i < PTRS_PER_PTE; i++)
			release_pte(ptepage[i]);
		/* Now we can free the page of PTEs */
		free_page((long)ptepage);
		/* And zero out the PGD entry so we never release it twice. */
		*spgd = __pgd(0);
	}
}

static void flush_user_mappings(struct lguest *lg, int idx)
{
	unsigned int i;
	/* Release every pgd entry up to the kernel's address. */
	for (i = 0; i < pgd_index(lg->kernel_address); i++)
		release_pgd(lg, lg->pgdirs[idx].pgdir + i);
}

void guest_pagetable_flush_user(struct lg_cpu *cpu)
{
	/* Drop the userspace part of the current page table. */
	flush_user_mappings(cpu->lg, cpu->cpu_pgd);
}
/*:*/

/* We walk down the guest page tables to get a guest-physical address */
unsigned long guest_pa(struct lg_cpu *cpu, unsigned long vaddr)
{
	pgd_t gpgd;
	pte_t gpte;

	/* First step: get the top-level Guest page table entry. */
	gpgd = lgread(cpu, gpgd_addr(cpu, vaddr), pgd_t);
	/* Toplevel not present?  We can't map it in. */
	if (!(pgd_flags(gpgd) & _PAGE_PRESENT))
		kill_guest(cpu, "Bad address %#lx", vaddr);

	gpte = lgread(cpu, gpte_addr(gpgd, vaddr), pte_t);
	if (!(pte_flags(gpte) & _PAGE_PRESENT))
		kill_guest(cpu, "Bad address %#lx", vaddr);

	return pte_pfn(gpte) * PAGE_SIZE | (vaddr & ~PAGE_MASK);
}

static unsigned int find_pgdir(struct lguest *lg, unsigned long pgtable)
{
	unsigned int i;
	for (i = 0; i < ARRAY_SIZE(lg->pgdirs); i++)
		if (lg->pgdirs[i].pgdir && lg->pgdirs[i].gpgdir == pgtable)
			break;
	return i;
}

static unsigned int new_pgdir(struct lg_cpu *cpu,
			      unsigned long gpgdir,
			      int *blank_pgdir)
{
	unsigned int next;

	/* We pick one entry at random to throw out.  Choosing the Least
	 * Recently Used might be better, but this is easy. */
	next = random32() % ARRAY_SIZE(cpu->lg->pgdirs);
	/* If it's never been allocated at all before, try now. */
	if (!cpu->lg->pgdirs[next].pgdir) {
		cpu->lg->pgdirs[next].pgdir =
					(pgd_t *)get_zeroed_page(GFP_KERNEL);
		/* If the allocation fails, just keep using the one we have */
		if (!cpu->lg->pgdirs[next].pgdir)
			next = cpu->cpu_pgd;
		else
			/* This is a blank page, so there are no kernel
			 * mappings: caller must map the stack! */
			*blank_pgdir = 1;
	}
	/* Record which Guest toplevel this shadows. */
	cpu->lg->pgdirs[next].gpgdir = gpgdir;
	/* Release all the non-kernel mappings. */
	flush_user_mappings(cpu->lg, next);

	return next;
}

void guest_new_pagetable(struct lg_cpu *cpu, unsigned long pgtable)
{
	int newpgdir, repin = 0;

	/* Look to see if we have this one already. */
	newpgdir = find_pgdir(cpu->lg, pgtable);
	/* If not, we allocate or mug an existing one: if it's a fresh one,
	 * repin gets set to 1. */
	if (newpgdir == ARRAY_SIZE(cpu->lg->pgdirs))
		newpgdir = new_pgdir(cpu, pgtable, &repin);
	/* Change the current pgd index to the new one. */
	cpu->cpu_pgd = newpgdir;
	/* If it was completely blank, we map in the Guest kernel stack */
	if (repin)
		pin_stack_pages(cpu);
}

static void release_all_pagetables(struct lguest *lg)
{
	unsigned int i, j;

	/* Every shadow pagetable this Guest has */
	for (i = 0; i < ARRAY_SIZE(lg->pgdirs); i++)
		if (lg->pgdirs[i].pgdir)
			/* Every PGD entry except the Switcher at the top */
			for (j = 0; j < SWITCHER_PGD_INDEX; j++)
				release_pgd(lg, lg->pgdirs[i].pgdir + j);
}

void guest_pagetable_clear_all(struct lg_cpu *cpu)
{
	release_all_pagetables(cpu->lg);
	/* We need the Guest kernel stack mapped again. */
	pin_stack_pages(cpu);
}
/*:*/

static void do_set_pte(struct lg_cpu *cpu, int idx,
		       unsigned long vaddr, pte_t gpte)
{
	/* Look up the matching shadow page directory entry. */
	pgd_t *spgd = spgd_addr(cpu, idx, vaddr);

	/* If the top level isn't present, there's no entry to update. */
	if (pgd_flags(*spgd) & _PAGE_PRESENT) {
		/* Otherwise, we start by releasing the existing entry. */
		pte_t *spte = spte_addr(*spgd, vaddr);
		release_pte(*spte);

		/* If they're setting this entry as dirty or accessed, we might
		 * as well put that entry they've given us in now.  This shaves
		 * 10% off a copy-on-write micro-benchmark. */
		if (pte_flags(gpte) & (_PAGE_DIRTY | _PAGE_ACCESSED)) {
			check_gpte(cpu, gpte);
			*spte = gpte_to_spte(cpu, gpte,
					     pte_flags(gpte) & _PAGE_DIRTY);
		} else
			/* Otherwise kill it and we can demand_page() it in
			 * later. */
			*spte = __pte(0);
	}
}

void guest_set_pte(struct lg_cpu *cpu,
		   unsigned long gpgdir, unsigned long vaddr, pte_t gpte)
{
	/* Kernel mappings must be changed on all top levels.  Slow, but doesn't
	 * happen often. */
	if (vaddr >= cpu->lg->kernel_address) {
		unsigned int i;
		for (i = 0; i < ARRAY_SIZE(cpu->lg->pgdirs); i++)
			if (cpu->lg->pgdirs[i].pgdir)
				do_set_pte(cpu, i, vaddr, gpte);
	} else {
		/* Is this page table one we have a shadow for? */
		int pgdir = find_pgdir(cpu->lg, gpgdir);
		if (pgdir != ARRAY_SIZE(cpu->lg->pgdirs))
			/* If so, do the update. */
			do_set_pte(cpu, pgdir, vaddr, gpte);
	}
}

void guest_set_pmd(struct lguest *lg, unsigned long gpgdir, u32 idx)
{
	int pgdir;

	/* The kernel seems to try to initialize this early on: we ignore its
	 * attempts to map over the Switcher. */
	if (idx >= SWITCHER_PGD_INDEX)
		return;

	/* If they're talking about a page table we have a shadow for... */
	pgdir = find_pgdir(lg, gpgdir);
	if (pgdir < ARRAY_SIZE(lg->pgdirs))
		/* ... throw it away. */
		release_pgd(lg, lg->pgdirs[pgdir].pgdir + idx);
}

static unsigned long setup_pagetables(struct lguest *lg,
				      unsigned long mem,
				      unsigned long initrd_size)
{
	pgd_t __user *pgdir;
	pte_t __user *linear;
	unsigned int mapped_pages, i, linear_pages, phys_linear;
	unsigned long mem_base = (unsigned long)lg->mem_base;

	/* We have mapped_pages frames to map, so we need
	 * linear_pages page tables to map them. */
	mapped_pages = mem / PAGE_SIZE;
	linear_pages = (mapped_pages + PTRS_PER_PTE - 1) / PTRS_PER_PTE;

	/* We put the toplevel page directory page at the top of memory. */
	pgdir = (pgd_t *)(mem + mem_base - initrd_size - PAGE_SIZE);

	/* Now we use the next linear_pages pages as pte pages */
	linear = (void *)pgdir - linear_pages * PAGE_SIZE;

	/* Linear mapping is easy: put every page's address into the
	 * mapping in order. */
	for (i = 0; i < mapped_pages; i++) {
		pte_t pte;
		pte = pfn_pte(i, __pgprot(_PAGE_PRESENT|_PAGE_RW|_PAGE_USER));
		if (copy_to_user(&linear[i], &pte, sizeof(pte)) != 0)
			return -EFAULT;
	}

	/* The top level points to the linear page table pages above.
	 * We setup the identity and linear mappings here. */
	phys_linear = (unsigned long)linear - mem_base;
	for (i = 0; i < mapped_pages; i += PTRS_PER_PTE) {
		pgd_t pgd;
		pgd = __pgd((phys_linear + i * sizeof(pte_t)) |
			    (_PAGE_PRESENT | _PAGE_RW | _PAGE_USER));

		if (copy_to_user(&pgdir[i / PTRS_PER_PTE], &pgd, sizeof(pgd))
		    || copy_to_user(&pgdir[pgd_index(PAGE_OFFSET)
					   + i / PTRS_PER_PTE],
				    &pgd, sizeof(pgd)))
			return -EFAULT;
	}

	/* We return the top level (guest-physical) address: remember where
	 * this is. */
	return (unsigned long)pgdir - mem_base;
}

int init_guest_pagetable(struct lguest *lg)
{
	u64 mem;
	u32 initrd_size;
	struct boot_params __user *boot = (struct boot_params *)lg->mem_base;

	/* Get the Guest memory size and the ramdisk size from the boot header
	 * located at lg->mem_base (Guest address 0). */
	if (copy_from_user(&mem, &boot->e820_map[0].size, sizeof(mem))
	    || get_user(initrd_size, &boot->hdr.ramdisk_size))
		return -EFAULT;

	/* We start on the first shadow page table, and give it a blank PGD
	 * page. */
	lg->pgdirs[0].gpgdir = setup_pagetables(lg, mem, initrd_size);
	if (IS_ERR_VALUE(lg->pgdirs[0].gpgdir))
		return lg->pgdirs[0].gpgdir;
	lg->pgdirs[0].pgdir = (pgd_t *)get_zeroed_page(GFP_KERNEL);
	if (!lg->pgdirs[0].pgdir)
		return -ENOMEM;
	lg->cpus[0].cpu_pgd = 0;
	return 0;
}

/* When the Guest calls LHCALL_LGUEST_INIT we do more setup. */
void page_table_guest_data_init(struct lg_cpu *cpu)
{
	/* We get the kernel address: above this is all kernel memory. */
	if (get_user(cpu->lg->kernel_address,
		     &cpu->lg->lguest_data->kernel_address)
	    /* We tell the Guest that it can't use the top 4MB of virtual
	     * addresses used by the Switcher. */
	    || put_user(4U*1024*1024, &cpu->lg->lguest_data->reserve_mem)
	    || put_user(cpu->lg->pgdirs[0].gpgdir, &cpu->lg->lguest_data->pgdir))
		kill_guest(cpu, "bad guest page %p", cpu->lg->lguest_data);

	/* In flush_user_mappings() we loop from 0 to
	 * "pgd_index(lg->kernel_address)".  This assumes it won't hit the
	 * Switcher mappings, so check that now. */
	if (pgd_index(cpu->lg->kernel_address) >= SWITCHER_PGD_INDEX)
		kill_guest(cpu, "bad kernel address %#lx",
				 cpu->lg->kernel_address);
}

/* When a Guest dies, our cleanup is fairly simple. */
void free_guest_pagetable(struct lguest *lg)
{
	unsigned int i;

	/* Throw away all page table pages. */
	release_all_pagetables(lg);
	/* Now free the top levels: free_page() can handle 0 just fine. */
	for (i = 0; i < ARRAY_SIZE(lg->pgdirs); i++)
		free_page((long)lg->pgdirs[i].pgdir);
}

void map_switcher_in_guest(struct lg_cpu *cpu, struct lguest_pages *pages)
{
	pte_t *switcher_pte_page = __get_cpu_var(switcher_pte_pages);
	pgd_t switcher_pgd;
	pte_t regs_pte;
	unsigned long pfn;

	/* Make the last PGD entry for this Guest point to the Switcher's PTE
	 * page for this CPU (with appropriate flags). */
	switcher_pgd = __pgd(__pa(switcher_pte_page) | __PAGE_KERNEL);

	cpu->lg->pgdirs[cpu->cpu_pgd].pgdir[SWITCHER_PGD_INDEX] = switcher_pgd;

	/* We also change the Switcher PTE page.  When we're running the Guest,
	 * we want the Guest's "regs" page to appear where the first Switcher
	 * page for this CPU is.  This is an optimization: when the Switcher
	 * saves the Guest registers, it saves them into the first page of this
	 * CPU's "struct lguest_pages": if we make sure the Guest's register
	 * page is already mapped there, we don't have to copy them out
	 * again. */
	pfn = __pa(cpu->regs_page) >> PAGE_SHIFT;
	regs_pte = pfn_pte(pfn, __pgprot(__PAGE_KERNEL));
	switcher_pte_page[(unsigned long)pages/PAGE_SIZE%PTRS_PER_PTE] = regs_pte;
}
/*:*/

static void free_switcher_pte_pages(void)
{
	unsigned int i;

	for_each_possible_cpu(i)
		free_page((long)switcher_pte_page(i));
}

static __init void populate_switcher_pte_page(unsigned int cpu,
					      struct page *switcher_page[],
					      unsigned int pages)
{
	unsigned int i;
	pte_t *pte = switcher_pte_page(cpu);

	/* The first entries are easy: they map the Switcher code. */
	for (i = 0; i < pages; i++) {
		pte[i] = mk_pte(switcher_page[i],
				__pgprot(_PAGE_PRESENT|_PAGE_ACCESSED));
	}

	/* The only other thing we map is this CPU's pair of pages. */
	i = pages + cpu*2;

	/* First page (Guest registers) is writable from the Guest */
	pte[i] = pfn_pte(page_to_pfn(switcher_page[i]),
			 __pgprot(_PAGE_PRESENT|_PAGE_ACCESSED|_PAGE_RW));

	/* The second page contains the "struct lguest_ro_state", and is
	 * read-only. */
	pte[i+1] = pfn_pte(page_to_pfn(switcher_page[i+1]),
			   __pgprot(_PAGE_PRESENT|_PAGE_ACCESSED));
}


__init int init_pagetables(struct page **switcher_page, unsigned int pages)
{
	unsigned int i;

	for_each_possible_cpu(i) {
		switcher_pte_page(i) = (pte_t *)get_zeroed_page(GFP_KERNEL);
		if (!switcher_pte_page(i)) {
			free_switcher_pte_pages();
			return -ENOMEM;
		}
		populate_switcher_pte_page(i, switcher_page, pages);
	}
	return 0;
}
/*:*/

/* Cleaning up simply involves freeing the PTE page for each CPU. */
void free_pagetables(void)
{
	free_switcher_pte_pages();
}