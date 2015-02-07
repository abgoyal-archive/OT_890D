
#ifndef _ASM_X86_PGTABLE_3LEVEL_DEFS_H
#define _ASM_X86_PGTABLE_3LEVEL_DEFS_H

#ifdef CONFIG_PARAVIRT
#define SHARED_KERNEL_PMD	(pv_info.shared_kernel_pmd)
#else
#define SHARED_KERNEL_PMD	1
#endif

#define PGDIR_SHIFT	30
#define PTRS_PER_PGD	4

#define PMD_SHIFT	21
#define PTRS_PER_PMD	512

#define PTRS_PER_PTE	512

#endif /* _ASM_X86_PGTABLE_3LEVEL_DEFS_H */
