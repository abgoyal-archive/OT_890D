
#ifndef _ASM_GENERIC_TOPOLOGY_H
#define _ASM_GENERIC_TOPOLOGY_H

#ifndef	CONFIG_NUMA

#ifndef cpu_to_node
#define cpu_to_node(cpu)	((void)(cpu),0)
#endif
#ifndef parent_node
#define parent_node(node)	((void)(node),0)
#endif
#ifndef node_to_cpumask
#define node_to_cpumask(node)	((void)node, cpu_online_map)
#endif
#ifndef cpumask_of_node
#define cpumask_of_node(node)	((void)node, cpu_online_mask)
#endif
#ifndef node_to_first_cpu
#define node_to_first_cpu(node)	((void)(node),0)
#endif
#ifndef pcibus_to_node
#define pcibus_to_node(bus)	((void)(bus), -1)
#endif

#ifndef pcibus_to_cpumask
#define pcibus_to_cpumask(bus)	(pcibus_to_node(bus) == -1 ? \
					CPU_MASK_ALL : \
					node_to_cpumask(pcibus_to_node(bus)) \
				)
#endif

#ifndef cpumask_of_pcibus
#define cpumask_of_pcibus(bus)	(pcibus_to_node(bus) == -1 ?		\
				 cpu_all_mask :				\
				 cpumask_of_node(pcibus_to_node(bus)))
#endif

#endif	/* CONFIG_NUMA */

#ifndef node_to_cpumask_ptr

#define	node_to_cpumask_ptr(v, node) 					\
		cpumask_t _##v = node_to_cpumask(node);			\
		const cpumask_t *v = &_##v

#define node_to_cpumask_ptr_next(v, node)				\
			  _##v = node_to_cpumask(node)
#endif

#endif /* _ASM_GENERIC_TOPOLOGY_H */
