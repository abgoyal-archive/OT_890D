
#ifndef _ASM_POWERPC_PGTABLE_PPC32_H
#define _ASM_POWERPC_PGTABLE_PPC32_H

#include <asm-generic/pgtable-nopmd.h>

#ifndef __ASSEMBLY__
#include <linux/sched.h>
#include <linux/threads.h>
#include <asm/io.h>			/* For sub-arch specific PPC_PIN_SIZE */

extern unsigned long va_to_phys(unsigned long address);
extern pte_t *va_to_pte(unsigned long address);
extern unsigned long ioremap_bot, ioremap_base;

#ifdef CONFIG_44x
extern int icache_44x_need_flush;
#endif

#endif /* __ASSEMBLY__ */




/* PGDIR_SHIFT determines what a top-level page table entry can map */
#define PGDIR_SHIFT	(PAGE_SHIFT + PTE_SHIFT)
#define PGDIR_SIZE	(1UL << PGDIR_SHIFT)
#define PGDIR_MASK	(~(PGDIR_SIZE-1))

#ifndef __ASSEMBLY__
#define PTE_TABLE_SIZE	(sizeof(pte_t) << PTE_SHIFT)
#define PGD_TABLE_SIZE	(sizeof(pgd_t) << (32 - PGDIR_SHIFT))
#endif	/* __ASSEMBLY__ */

#define PTRS_PER_PTE	(1 << PTE_SHIFT)
#define PTRS_PER_PMD	1
#define PTRS_PER_PGD	(1 << (32 - PGDIR_SHIFT))

#define USER_PTRS_PER_PGD	(TASK_SIZE / PGDIR_SIZE)
#define FIRST_USER_ADDRESS	0

#define pte_ERROR(e) \
	printk("%s:%d: bad pte %llx.\n", __FILE__, __LINE__, \
		(unsigned long long)pte_val(e))
#define pgd_ERROR(e) \
	printk("%s:%d: bad pgd %08lx.\n", __FILE__, __LINE__, pgd_val(e))

#define VMALLOC_OFFSET (0x1000000) /* 16M */
#ifdef PPC_PIN_SIZE
#define VMALLOC_START (((_ALIGN((long)high_memory, PPC_PIN_SIZE) + VMALLOC_OFFSET) & ~(VMALLOC_OFFSET-1)))
#else
#define VMALLOC_START ((((long)high_memory + VMALLOC_OFFSET) & ~(VMALLOC_OFFSET-1)))
#endif
#define VMALLOC_END	ioremap_bot


#if defined(CONFIG_40x)


/* Definitions for 40x embedded chips. */
#define	_PAGE_GUARDED	0x001	/* G: page is guarded from prefetch */
#define _PAGE_FILE	0x001	/* when !present: nonlinear file mapping */
#define _PAGE_PRESENT	0x002	/* software: PTE contains a translation */
#define	_PAGE_NO_CACHE	0x004	/* I: caching is inhibited */
#define	_PAGE_WRITETHRU	0x008	/* W: caching is write-through */
#define	_PAGE_USER	0x010	/* matches one of the zone permission bits */
#define	_PAGE_RW	0x040	/* software: Writes permitted */
#define	_PAGE_DIRTY	0x080	/* software: dirty page */
#define _PAGE_HWWRITE	0x100	/* hardware: Dirty & RW, set in exception */
#define _PAGE_HWEXEC	0x200	/* hardware: EX permission */
#define _PAGE_ACCESSED	0x400	/* software: R: page referenced */

#define _PMD_PRESENT	0x400	/* PMD points to page of PTEs */
#define _PMD_BAD	0x802
#define _PMD_SIZE	0x0e0	/* size field, != 0 for large-page PMD entry */
#define _PMD_SIZE_4M	0x0c0
#define _PMD_SIZE_16M	0x0e0
#define PMD_PAGE_SIZE(pmdval)	(1024 << (((pmdval) & _PMD_SIZE) >> 4))

/* Until my rework is finished, 40x still needs atomic PTE updates */
#define PTE_ATOMIC_UPDATES	1

#elif defined(CONFIG_44x)

#define _PAGE_PRESENT	0x00000001		/* S: PTE valid */
#define _PAGE_RW	0x00000002		/* S: Write permission */
#define _PAGE_FILE	0x00000004		/* S: nonlinear file mapping */
#define _PAGE_HWEXEC	0x00000004		/* H: Execute permission */
#define _PAGE_ACCESSED	0x00000008		/* S: Page referenced */
#define _PAGE_DIRTY	0x00000010		/* S: Page dirty */
#define _PAGE_SPECIAL	0x00000020		/* S: Special page */
#define _PAGE_USER	0x00000040		/* S: User page */
#define _PAGE_ENDIAN	0x00000080		/* H: E bit */
#define _PAGE_GUARDED	0x00000100		/* H: G bit */
#define _PAGE_COHERENT	0x00000200		/* H: M bit */
#define _PAGE_NO_CACHE	0x00000400		/* H: I bit */
#define _PAGE_WRITETHRU	0x00000800		/* H: W bit */

/* TODO: Add large page lowmem mapping support */
#define _PMD_PRESENT	0
#define _PMD_PRESENT_MASK (PAGE_MASK)
#define _PMD_BAD	(~PAGE_MASK)

/* ERPN in a PTE never gets cleared, ignore it */
#define _PTE_NONE_MASK	0xffffffff00000000ULL

#define __HAVE_ARCH_PTE_SPECIAL

#elif defined(CONFIG_FSL_BOOKE)

/* Definitions for FSL Book-E Cores */
#define _PAGE_PRESENT	0x00001	/* S: PTE contains a translation */
#define _PAGE_USER	0x00002	/* S: User page (maps to UR) */
#define _PAGE_FILE	0x00002	/* S: when !present: nonlinear file mapping */
#define _PAGE_RW	0x00004	/* S: Write permission (SW) */
#define _PAGE_DIRTY	0x00008	/* S: Page dirty */
#define _PAGE_HWEXEC	0x00010	/* H: SX permission */
#define _PAGE_ACCESSED	0x00020	/* S: Page referenced */

#define _PAGE_ENDIAN	0x00040	/* H: E bit */
#define _PAGE_GUARDED	0x00080	/* H: G bit */
#define _PAGE_COHERENT	0x00100	/* H: M bit */
#define _PAGE_NO_CACHE	0x00200	/* H: I bit */
#define _PAGE_WRITETHRU	0x00400	/* H: W bit */
#define _PAGE_SPECIAL	0x00800 /* S: Special page */

#ifdef CONFIG_PTE_64BIT
/* ERPN in a PTE never gets cleared, ignore it */
#define _PTE_NONE_MASK	0xffffffffffff0000ULL
#endif

#define _PMD_PRESENT	0
#define _PMD_PRESENT_MASK (PAGE_MASK)
#define _PMD_BAD	(~PAGE_MASK)

#define __HAVE_ARCH_PTE_SPECIAL

#elif defined(CONFIG_8xx)
/* Definitions for 8xx embedded chips. */
#define _PAGE_PRESENT	0x0001	/* Page is valid */
#define _PAGE_FILE	0x0002	/* when !present: nonlinear file mapping */
#define _PAGE_NO_CACHE	0x0002	/* I: cache inhibit */
#define _PAGE_SHARED	0x0004	/* No ASID (context) compare */

#define _PAGE_EXEC	0x0008	/* software: i-cache coherency required */
#define _PAGE_GUARDED	0x0010	/* software: guarded access */
#define _PAGE_DIRTY	0x0020	/* software: page changed */
#define _PAGE_RW	0x0040	/* software: user write access allowed */
#define _PAGE_ACCESSED	0x0080	/* software: page referenced */

#define _PAGE_HWWRITE	0x0100	/* h/w write enable: never set in Linux PTE */
#define _PAGE_USER	0x0800	/* One of the PP bits, the other is USER&~RW */

#define _PMD_PRESENT	0x0001
#define _PMD_BAD	0x0ff0
#define _PMD_PAGE_MASK	0x000c
#define _PMD_PAGE_8M	0x000c

#define _PTE_NONE_MASK _PAGE_ACCESSED

/* Until my rework is finished, 8xx still needs atomic PTE updates */
#define PTE_ATOMIC_UPDATES	1

#else /* CONFIG_6xx */
/* Definitions for 60x, 740/750, etc. */
#define _PAGE_PRESENT	0x001	/* software: pte contains a translation */
#define _PAGE_HASHPTE	0x002	/* hash_page has made an HPTE for this pte */
#define _PAGE_FILE	0x004	/* when !present: nonlinear file mapping */
#define _PAGE_USER	0x004	/* usermode access allowed */
#define _PAGE_GUARDED	0x008	/* G: prohibit speculative access */
#define _PAGE_COHERENT	0x010	/* M: enforce memory coherence (SMP systems) */
#define _PAGE_NO_CACHE	0x020	/* I: cache inhibit */
#define _PAGE_WRITETHRU	0x040	/* W: cache write-through */
#define _PAGE_DIRTY	0x080	/* C: page changed */
#define _PAGE_ACCESSED	0x100	/* R: page referenced */
#define _PAGE_EXEC	0x200	/* software: i-cache coherency required */
#define _PAGE_RW	0x400	/* software: user write access allowed */
#define _PAGE_SPECIAL	0x800	/* software: Special page */

#ifdef CONFIG_PTE_64BIT
/* We never clear the high word of the pte */
#define _PTE_NONE_MASK	(0xffffffff00000000ULL | _PAGE_HASHPTE)
#else
#define _PTE_NONE_MASK	_PAGE_HASHPTE
#endif

#define _PMD_PRESENT	0
#define _PMD_PRESENT_MASK (PAGE_MASK)
#define _PMD_BAD	(~PAGE_MASK)

/* Hash table based platforms need atomic updates of the linux PTE */
#define PTE_ATOMIC_UPDATES	1

#define __HAVE_ARCH_PTE_SPECIAL

#endif

#ifndef _PAGE_HASHPTE
#define _PAGE_HASHPTE	0
#endif
#ifndef _PTE_NONE_MASK
#define _PTE_NONE_MASK 0
#endif
#ifndef _PAGE_SHARED
#define _PAGE_SHARED	0
#endif
#ifndef _PAGE_HWWRITE
#define _PAGE_HWWRITE	0
#endif
#ifndef _PAGE_HWEXEC
#define _PAGE_HWEXEC	0
#endif
#ifndef _PAGE_EXEC
#define _PAGE_EXEC	0
#endif
#ifndef _PAGE_ENDIAN
#define _PAGE_ENDIAN	0
#endif
#ifndef _PAGE_COHERENT
#define _PAGE_COHERENT	0
#endif
#ifndef _PAGE_WRITETHRU
#define _PAGE_WRITETHRU	0
#endif
#ifndef _PAGE_SPECIAL
#define _PAGE_SPECIAL	0
#endif
#ifndef _PMD_PRESENT_MASK
#define _PMD_PRESENT_MASK	_PMD_PRESENT
#endif
#ifndef _PMD_SIZE
#define _PMD_SIZE	0
#define PMD_PAGE_SIZE(pmd)	bad_call_to_PMD_PAGE_SIZE()
#endif

#define _PAGE_CHG_MASK	(PAGE_MASK | _PAGE_ACCESSED | _PAGE_DIRTY | \
			 _PAGE_SPECIAL)


#define PAGE_PROT_BITS	(_PAGE_GUARDED | _PAGE_COHERENT | _PAGE_NO_CACHE | \
			 _PAGE_WRITETHRU | _PAGE_ENDIAN | \
			 _PAGE_USER | _PAGE_ACCESSED | \
			 _PAGE_RW | _PAGE_HWWRITE | _PAGE_DIRTY | \
			 _PAGE_EXEC | _PAGE_HWEXEC)

#if defined(CONFIG_SMP) || defined(CONFIG_PPC_STD_MMU)
#define _PAGE_BASE	(_PAGE_PRESENT | _PAGE_ACCESSED | _PAGE_COHERENT)
#else
#define _PAGE_BASE	(_PAGE_PRESENT | _PAGE_ACCESSED)
#endif
#define _PAGE_BASE_NC	(_PAGE_PRESENT | _PAGE_ACCESSED | _PAGE_NO_CACHE)

#define _PAGE_WRENABLE	(_PAGE_RW | _PAGE_DIRTY | _PAGE_HWWRITE)
#define _PAGE_KERNEL	(_PAGE_BASE | _PAGE_SHARED | _PAGE_WRENABLE)
#define _PAGE_KERNEL_NC	(_PAGE_BASE_NC | _PAGE_SHARED | _PAGE_WRENABLE)

#ifdef CONFIG_PPC_STD_MMU
#define _PAGE_KERNEL_RO	(_PAGE_BASE | _PAGE_SHARED | _PAGE_USER)
#else
#define _PAGE_KERNEL_RO	(_PAGE_BASE | _PAGE_SHARED)
#endif

#define _PAGE_IO	(_PAGE_KERNEL_NC | _PAGE_GUARDED)
#define _PAGE_RAM	(_PAGE_KERNEL | _PAGE_HWEXEC)

#if defined(CONFIG_KGDB) || defined(CONFIG_XMON) || defined(CONFIG_BDI_SWITCH) ||\
	defined(CONFIG_KPROBES)
#define _PAGE_RAM_TEXT	_PAGE_RAM
#else
#define _PAGE_RAM_TEXT	(_PAGE_KERNEL_RO | _PAGE_HWEXEC)
#endif

#define PAGE_NONE	__pgprot(_PAGE_BASE)
#define PAGE_READONLY	__pgprot(_PAGE_BASE | _PAGE_USER)
#define PAGE_READONLY_X	__pgprot(_PAGE_BASE | _PAGE_USER | _PAGE_EXEC)
#define PAGE_SHARED	__pgprot(_PAGE_BASE | _PAGE_USER | _PAGE_RW)
#define PAGE_SHARED_X	__pgprot(_PAGE_BASE | _PAGE_USER | _PAGE_RW | _PAGE_EXEC)
#define PAGE_COPY	__pgprot(_PAGE_BASE | _PAGE_USER)
#define PAGE_COPY_X	__pgprot(_PAGE_BASE | _PAGE_USER | _PAGE_EXEC)

#define PAGE_KERNEL		__pgprot(_PAGE_RAM)
#define PAGE_KERNEL_NOCACHE	__pgprot(_PAGE_IO)

#define __P000	PAGE_NONE
#define __P001	PAGE_READONLY_X
#define __P010	PAGE_COPY
#define __P011	PAGE_COPY_X
#define __P100	PAGE_READONLY
#define __P101	PAGE_READONLY_X
#define __P110	PAGE_COPY
#define __P111	PAGE_COPY_X

#define __S000	PAGE_NONE
#define __S001	PAGE_READONLY_X
#define __S010	PAGE_SHARED
#define __S011	PAGE_SHARED_X
#define __S100	PAGE_READONLY
#define __S101	PAGE_READONLY_X
#define __S110	PAGE_SHARED
#define __S111	PAGE_SHARED_X

#ifndef __ASSEMBLY__
extern unsigned long bad_call_to_PMD_PAGE_SIZE(void);


#if defined(CONFIG_FSL_BOOKE) && defined(CONFIG_PTE_64BIT)
#define PFN_SHIFT_OFFSET	(PAGE_SHIFT + 8)
#else
#define PFN_SHIFT_OFFSET	(PAGE_SHIFT)
#endif

#define pte_pfn(x)		(pte_val(x) >> PFN_SHIFT_OFFSET)
#define pte_page(x)		pfn_to_page(pte_pfn(x))

#define pfn_pte(pfn, prot)	__pte(((pte_basic_t)(pfn) << PFN_SHIFT_OFFSET) |\
					pgprot_val(prot))
#define mk_pte(page, prot)	pfn_pte(page_to_pfn(page), prot)
#endif /* __ASSEMBLY__ */

#define pte_none(pte)		((pte_val(pte) & ~_PTE_NONE_MASK) == 0)
#define pte_present(pte)	(pte_val(pte) & _PAGE_PRESENT)
#define pte_clear(mm, addr, ptep) \
	do { pte_update(ptep, ~_PAGE_HASHPTE, 0); } while (0)

#define pmd_none(pmd)		(!pmd_val(pmd))
#define	pmd_bad(pmd)		(pmd_val(pmd) & _PMD_BAD)
#define	pmd_present(pmd)	(pmd_val(pmd) & _PMD_PRESENT_MASK)
#define	pmd_clear(pmdp)		do { pmd_val(*(pmdp)) = 0; } while (0)

#ifndef __ASSEMBLY__
static inline int pte_write(pte_t pte)		{ return pte_val(pte) & _PAGE_RW; }
static inline int pte_dirty(pte_t pte)		{ return pte_val(pte) & _PAGE_DIRTY; }
static inline int pte_young(pte_t pte)		{ return pte_val(pte) & _PAGE_ACCESSED; }
static inline int pte_file(pte_t pte)		{ return pte_val(pte) & _PAGE_FILE; }
static inline int pte_special(pte_t pte)	{ return pte_val(pte) & _PAGE_SPECIAL; }

static inline pte_t pte_wrprotect(pte_t pte) {
	pte_val(pte) &= ~(_PAGE_RW | _PAGE_HWWRITE); return pte; }
static inline pte_t pte_mkclean(pte_t pte) {
	pte_val(pte) &= ~(_PAGE_DIRTY | _PAGE_HWWRITE); return pte; }
static inline pte_t pte_mkold(pte_t pte) {
	pte_val(pte) &= ~_PAGE_ACCESSED; return pte; }

static inline pte_t pte_mkwrite(pte_t pte) {
	pte_val(pte) |= _PAGE_RW; return pte; }
static inline pte_t pte_mkdirty(pte_t pte) {
	pte_val(pte) |= _PAGE_DIRTY; return pte; }
static inline pte_t pte_mkyoung(pte_t pte) {
	pte_val(pte) |= _PAGE_ACCESSED; return pte; }
static inline pte_t pte_mkspecial(pte_t pte) {
	pte_val(pte) |= _PAGE_SPECIAL; return pte; }
static inline pgprot_t pte_pgprot(pte_t pte)
{
	return __pgprot(pte_val(pte) & PAGE_PROT_BITS);
}

static inline pte_t pte_modify(pte_t pte, pgprot_t newprot)
{
	pte_val(pte) = (pte_val(pte) & _PAGE_CHG_MASK) | pgprot_val(newprot);
	return pte;
}

extern int flush_hash_pages(unsigned context, unsigned long va,
			    unsigned long pmdval, int count);

/* Add an HPTE to the hash table */
extern void add_hash_page(unsigned context, unsigned long va,
			  unsigned long pmdval);

/* Flush an entry from the TLB/hash table */
extern void flush_hash_entry(struct mm_struct *mm, pte_t *ptep,
			     unsigned long address);

#ifndef CONFIG_PTE_64BIT
static inline unsigned long pte_update(pte_t *p,
				       unsigned long clr,
				       unsigned long set)
{
#ifdef PTE_ATOMIC_UPDATES
	unsigned long old, tmp;

	__asm__ __volatile__("\
1:	lwarx	%0,0,%3\n\
	andc	%1,%0,%4\n\
	or	%1,%1,%5\n"
	PPC405_ERR77(0,%3)
"	stwcx.	%1,0,%3\n\
	bne-	1b"
	: "=&r" (old), "=&r" (tmp), "=m" (*p)
	: "r" (p), "r" (clr), "r" (set), "m" (*p)
	: "cc" );
#else /* PTE_ATOMIC_UPDATES */
	unsigned long old = pte_val(*p);
	*p = __pte((old & ~clr) | set);
#endif /* !PTE_ATOMIC_UPDATES */

#ifdef CONFIG_44x
	if ((old & _PAGE_USER) && (old & _PAGE_HWEXEC))
		icache_44x_need_flush = 1;
#endif
	return old;
}
#else /* CONFIG_PTE_64BIT */
static inline unsigned long long pte_update(pte_t *p,
					    unsigned long clr,
					    unsigned long set)
{
#ifdef PTE_ATOMIC_UPDATES
	unsigned long long old;
	unsigned long tmp;

	__asm__ __volatile__("\
1:	lwarx	%L0,0,%4\n\
	lwzx	%0,0,%3\n\
	andc	%1,%L0,%5\n\
	or	%1,%1,%6\n"
	PPC405_ERR77(0,%3)
"	stwcx.	%1,0,%4\n\
	bne-	1b"
	: "=&r" (old), "=&r" (tmp), "=m" (*p)
	: "r" (p), "r" ((unsigned long)(p) + 4), "r" (clr), "r" (set), "m" (*p)
	: "cc" );
#else /* PTE_ATOMIC_UPDATES */
	unsigned long long old = pte_val(*p);
	*p = __pte((old & ~(unsigned long long)clr) | set);
#endif /* !PTE_ATOMIC_UPDATES */

#ifdef CONFIG_44x
	if ((old & _PAGE_USER) && (old & _PAGE_HWEXEC))
		icache_44x_need_flush = 1;
#endif
	return old;
}
#endif /* CONFIG_PTE_64BIT */


static inline void __set_pte_at(struct mm_struct *mm, unsigned long addr,
			      pte_t *ptep, pte_t pte)
{
#if (_PAGE_HASHPTE != 0) && defined(CONFIG_SMP) && !defined(CONFIG_PTE_64BIT)
	pte_update(ptep, ~_PAGE_HASHPTE, pte_val(pte) & ~_PAGE_HASHPTE);
#elif defined(CONFIG_PTE_64BIT) && defined(CONFIG_SMP)
#if _PAGE_HASHPTE != 0
	if (pte_val(*ptep) & _PAGE_HASHPTE)
		flush_hash_entry(mm, ptep, addr);
#endif
	__asm__ __volatile__("\
		stw%U0%X0 %2,%0\n\
		eieio\n\
		stw%U0%X0 %L2,%1"
	: "=m" (*ptep), "=m" (*((unsigned char *)ptep+4))
	: "r" (pte) : "memory");
#else
	*ptep = __pte((pte_val(*ptep) & _PAGE_HASHPTE)
		      | (pte_val(pte) & ~_PAGE_HASHPTE));
#endif
}


static inline void set_pte_at(struct mm_struct *mm, unsigned long addr,
			      pte_t *ptep, pte_t pte)
{
#if defined(CONFIG_PTE_64BIT) && defined(CONFIG_SMP) && defined(CONFIG_DEBUG_VM)
	WARN_ON(pte_present(*ptep));
#endif
	__set_pte_at(mm, addr, ptep, pte);
}

#define __HAVE_ARCH_PTEP_TEST_AND_CLEAR_YOUNG
static inline int __ptep_test_and_clear_young(unsigned int context, unsigned long addr, pte_t *ptep)
{
	unsigned long old;
	old = pte_update(ptep, _PAGE_ACCESSED, 0);
#if _PAGE_HASHPTE != 0
	if (old & _PAGE_HASHPTE) {
		unsigned long ptephys = __pa(ptep) & PAGE_MASK;
		flush_hash_pages(context, addr, ptephys, 1);
	}
#endif
	return (old & _PAGE_ACCESSED) != 0;
}
#define ptep_test_and_clear_young(__vma, __addr, __ptep) \
	__ptep_test_and_clear_young((__vma)->vm_mm->context.id, __addr, __ptep)

#define __HAVE_ARCH_PTEP_GET_AND_CLEAR
static inline pte_t ptep_get_and_clear(struct mm_struct *mm, unsigned long addr,
				       pte_t *ptep)
{
	return __pte(pte_update(ptep, ~_PAGE_HASHPTE, 0));
}

#define __HAVE_ARCH_PTEP_SET_WRPROTECT
static inline void ptep_set_wrprotect(struct mm_struct *mm, unsigned long addr,
				      pte_t *ptep)
{
	pte_update(ptep, (_PAGE_RW | _PAGE_HWWRITE), 0);
}
static inline void huge_ptep_set_wrprotect(struct mm_struct *mm,
					   unsigned long addr, pte_t *ptep)
{
	ptep_set_wrprotect(mm, addr, ptep);
}


#define __HAVE_ARCH_PTEP_SET_ACCESS_FLAGS
static inline void __ptep_set_access_flags(pte_t *ptep, pte_t entry, int dirty)
{
	unsigned long bits = pte_val(entry) &
		(_PAGE_DIRTY | _PAGE_ACCESSED | _PAGE_RW);
	pte_update(ptep, 0, bits);
}

#define  ptep_set_access_flags(__vma, __address, __ptep, __entry, __dirty) \
({									   \
	int __changed = !pte_same(*(__ptep), __entry);			   \
	if (__changed) {						   \
		__ptep_set_access_flags(__ptep, __entry, __dirty);         \
		flush_tlb_page_nohash(__vma, __address);		   \
	}								   \
	__changed;							   \
})

#define __HAVE_ARCH_PTE_SAME
#define pte_same(A,B)	(((pte_val(A) ^ pte_val(B)) & ~_PAGE_HASHPTE) == 0)

#ifndef CONFIG_BOOKE
#define pmd_page_vaddr(pmd)	\
	((unsigned long) __va(pmd_val(pmd) & PAGE_MASK))
#define pmd_page(pmd)		\
	(mem_map + (pmd_val(pmd) >> PAGE_SHIFT))
#else
#define pmd_page_vaddr(pmd)	\
	((unsigned long) (pmd_val(pmd) & PAGE_MASK))
#define pmd_page(pmd)		\
	pfn_to_page((__pa(pmd_val(pmd)) >> PAGE_SHIFT))
#endif

/* to find an entry in a kernel page-table-directory */
#define pgd_offset_k(address) pgd_offset(&init_mm, address)

/* to find an entry in a page-table-directory */
#define pgd_index(address)	 ((address) >> PGDIR_SHIFT)
#define pgd_offset(mm, address)	 ((mm)->pgd + pgd_index(address))

/* Find an entry in the third-level page table.. */
#define pte_index(address)		\
	(((address) >> PAGE_SHIFT) & (PTRS_PER_PTE - 1))
#define pte_offset_kernel(dir, addr)	\
	((pte_t *) pmd_page_vaddr(*(dir)) + pte_index(addr))
#define pte_offset_map(dir, addr)		\
	((pte_t *) kmap_atomic(pmd_page(*(dir)), KM_PTE0) + pte_index(addr))
#define pte_offset_map_nested(dir, addr)	\
	((pte_t *) kmap_atomic(pmd_page(*(dir)), KM_PTE1) + pte_index(addr))

#define pte_unmap(pte)		kunmap_atomic(pte, KM_PTE0)
#define pte_unmap_nested(pte)	kunmap_atomic(pte, KM_PTE1)

#define __swp_type(entry)		((entry).val & 0x1f)
#define __swp_offset(entry)		((entry).val >> 5)
#define __swp_entry(type, offset)	((swp_entry_t) { (type) | ((offset) << 5) })
#define __pte_to_swp_entry(pte)		((swp_entry_t) { pte_val(pte) >> 3 })
#define __swp_entry_to_pte(x)		((pte_t) { (x).val << 3 })

/* Encode and decode a nonlinear file mapping entry */
#define PTE_FILE_MAX_BITS	29
#define pte_to_pgoff(pte)	(pte_val(pte) >> 3)
#define pgoff_to_pte(off)	((pte_t) { ((off) << 3) | _PAGE_FILE })

#define pgtable_cache_init()	do { } while (0)

extern int get_pteptr(struct mm_struct *mm, unsigned long addr, pte_t **ptep,
		      pmd_t **pmdp);

#endif /* !__ASSEMBLY__ */

#endif /* _ASM_POWERPC_PGTABLE_PPC32_H */
