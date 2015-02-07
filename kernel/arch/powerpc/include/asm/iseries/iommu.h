
#ifndef _ASM_POWERPC_ISERIES_IOMMU_H
#define _ASM_POWERPC_ISERIES_IOMMU_H


struct pci_dev;
struct vio_dev;
struct device_node;
struct iommu_table;

/* Creates table for an individual device node */
extern void iommu_devnode_init_iSeries(struct pci_dev *pdev,
				       struct device_node *dn);

/* Get table parameters from HV */
extern void iommu_table_getparms_iSeries(unsigned long busno,
		unsigned char slotno, unsigned char virtbus,
		struct iommu_table *tbl);

extern struct iommu_table *vio_build_iommu_table_iseries(struct vio_dev *dev);
extern void iommu_vio_init(void);

#endif /* _ASM_POWERPC_ISERIES_IOMMU_H */
