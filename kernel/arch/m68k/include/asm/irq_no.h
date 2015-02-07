
#ifndef _M68KNOMMU_IRQ_H_
#define _M68KNOMMU_IRQ_H_

#ifdef CONFIG_COLDFIRE
#define	SYS_IRQS	256
#define	NR_IRQS		SYS_IRQS

#else

#define SYS_IRQS	8
#define NR_IRQS		(24 + SYS_IRQS)

#endif /* CONFIG_COLDFIRE */


#define irq_canonicalize(irq)	(irq)

#endif /* _M68KNOMMU_IRQ_H_ */
