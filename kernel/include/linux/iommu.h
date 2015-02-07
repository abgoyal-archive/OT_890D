

#ifndef __LINUX_IOMMU_H
#define __LINUX_IOMMU_H

#define IOMMU_READ	(1)
#define IOMMU_WRITE	(2)

struct device;

struct iommu_domain {
	void *priv;
};

struct iommu_ops {
	int (*domain_init)(struct iommu_domain *domain);
	void (*domain_destroy)(struct iommu_domain *domain);
	int (*attach_dev)(struct iommu_domain *domain, struct device *dev);
	void (*detach_dev)(struct iommu_domain *domain, struct device *dev);
	int (*map)(struct iommu_domain *domain, unsigned long iova,
		   phys_addr_t paddr, size_t size, int prot);
	void (*unmap)(struct iommu_domain *domain, unsigned long iova,
		      size_t size);
	phys_addr_t (*iova_to_phys)(struct iommu_domain *domain,
				    unsigned long iova);
};

#ifdef CONFIG_IOMMU_API

extern void register_iommu(struct iommu_ops *ops);
extern bool iommu_found(void);
extern struct iommu_domain *iommu_domain_alloc(void);
extern void iommu_domain_free(struct iommu_domain *domain);
extern int iommu_attach_device(struct iommu_domain *domain,
			       struct device *dev);
extern void iommu_detach_device(struct iommu_domain *domain,
				struct device *dev);
extern int iommu_map_range(struct iommu_domain *domain, unsigned long iova,
			   phys_addr_t paddr, size_t size, int prot);
extern void iommu_unmap_range(struct iommu_domain *domain, unsigned long iova,
			      size_t size);
extern phys_addr_t iommu_iova_to_phys(struct iommu_domain *domain,
				      unsigned long iova);

#else /* CONFIG_IOMMU_API */

static inline void register_iommu(struct iommu_ops *ops)
{
}

static inline bool iommu_found(void)
{
	return false;
}

static inline struct iommu_domain *iommu_domain_alloc(void)
{
	return NULL;
}

static inline void iommu_domain_free(struct iommu_domain *domain)
{
}

static inline int iommu_attach_device(struct iommu_domain *domain,
				      struct device *dev)
{
	return -ENODEV;
}

static inline void iommu_detach_device(struct iommu_domain *domain,
				       struct device *dev)
{
}

static inline int iommu_map_range(struct iommu_domain *domain,
				  unsigned long iova, phys_addr_t paddr,
				  size_t size, int prot)
{
	return -ENODEV;
}

static inline void iommu_unmap_range(struct iommu_domain *domain,
				     unsigned long iova, size_t size)
{
}

static inline phys_addr_t iommu_iova_to_phys(struct iommu_domain *domain,
					     unsigned long iova)
{
	return 0;
}

#endif /* CONFIG_IOMMU_API */

#endif /* __LINUX_IOMMU_H */
