

#ifndef _ASM_X86_AMD_IOMMU_H
#define _ASM_X86_AMD_IOMMU_H

#include <linux/irqreturn.h>

#ifdef CONFIG_AMD_IOMMU
extern int amd_iommu_init(void);
extern int amd_iommu_init_dma_ops(void);
extern void amd_iommu_detect(void);
extern irqreturn_t amd_iommu_int_handler(int irq, void *data);
#else
static inline int amd_iommu_init(void) { return -ENODEV; }
static inline void amd_iommu_detect(void) { }
#endif

#endif /* _ASM_X86_AMD_IOMMU_H */
