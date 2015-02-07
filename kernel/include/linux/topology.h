
#ifndef _LINUX_TOPOLOGY_H
#define _LINUX_TOPOLOGY_H

#include <linux/cpumask.h>
#include <linux/bitops.h>
#include <linux/mmzone.h>
#include <linux/smp.h>
#include <asm/topology.h>

#ifndef node_has_online_mem
#define node_has_online_mem(nid) (1)
#endif

#ifndef nr_cpus_node
#define nr_cpus_node(node)				\
	({						\
		node_to_cpumask_ptr(__tmp__, node);	\
		cpus_weight(*__tmp__);			\
	})
#endif

#define for_each_node_with_cpus(node)			\
	for_each_online_node(node)			\
		if (nr_cpus_node(node))

int arch_update_cpu_topology(void);

/* Conform to ACPI 2.0 SLIT distance definitions */
#define LOCAL_DISTANCE		10
#define REMOTE_DISTANCE		20
#ifndef node_distance
#define node_distance(from,to)	((from) == (to) ? LOCAL_DISTANCE : REMOTE_DISTANCE)
#endif
#ifndef RECLAIM_DISTANCE
#define RECLAIM_DISTANCE 20
#endif
#ifndef PENALTY_FOR_NODE_WITH_CPUS
#define PENALTY_FOR_NODE_WITH_CPUS	(1)
#endif


#ifdef CONFIG_SCHED_SMT
#define ARCH_HAS_SCHED_WAKE_IDLE
/* Common values for SMT siblings */
#ifndef SD_SIBLING_INIT
#define SD_SIBLING_INIT (struct sched_domain) {		\
	.min_interval		= 1,			\
	.max_interval		= 2,			\
	.busy_factor		= 64,			\
	.imbalance_pct		= 110,			\
	.flags			= SD_LOAD_BALANCE	\
				| SD_BALANCE_NEWIDLE	\
				| SD_BALANCE_FORK	\
				| SD_BALANCE_EXEC	\
				| SD_WAKE_AFFINE	\
				| SD_WAKE_BALANCE	\
				| SD_SHARE_CPUPOWER,	\
	.last_balance		= jiffies,		\
	.balance_interval	= 1,			\
}
#endif
#endif /* CONFIG_SCHED_SMT */

#ifdef CONFIG_SCHED_MC
/* Common values for MC siblings. for now mostly derived from SD_CPU_INIT */
#ifndef SD_MC_INIT
#define SD_MC_INIT (struct sched_domain) {		\
	.min_interval		= 1,			\
	.max_interval		= 4,			\
	.busy_factor		= 64,			\
	.imbalance_pct		= 125,			\
	.cache_nice_tries	= 1,			\
	.busy_idx		= 2,			\
	.wake_idx		= 1,			\
	.forkexec_idx		= 1,			\
	.flags			= SD_LOAD_BALANCE	\
				| SD_BALANCE_FORK	\
				| SD_BALANCE_EXEC	\
				| SD_WAKE_AFFINE	\
				| SD_WAKE_BALANCE	\
				| SD_SHARE_PKG_RESOURCES\
				| sd_balance_for_mc_power()\
				| sd_power_saving_flags(),\
	.last_balance		= jiffies,		\
	.balance_interval	= 1,			\
}
#endif
#endif /* CONFIG_SCHED_MC */

/* Common values for CPUs */
#ifndef SD_CPU_INIT
#define SD_CPU_INIT (struct sched_domain) {		\
	.min_interval		= 1,			\
	.max_interval		= 4,			\
	.busy_factor		= 64,			\
	.imbalance_pct		= 125,			\
	.cache_nice_tries	= 1,			\
	.busy_idx		= 2,			\
	.idle_idx		= 1,			\
	.newidle_idx		= 2,			\
	.wake_idx		= 1,			\
	.forkexec_idx		= 1,			\
	.flags			= SD_LOAD_BALANCE	\
				| SD_BALANCE_EXEC	\
				| SD_BALANCE_FORK	\
				| SD_WAKE_AFFINE	\
				| SD_WAKE_BALANCE	\
				| sd_balance_for_package_power()\
				| sd_power_saving_flags(),\
	.last_balance		= jiffies,		\
	.balance_interval	= 1,			\
}
#endif

/* sched_domains SD_ALLNODES_INIT for NUMA machines */
#define SD_ALLNODES_INIT (struct sched_domain) {	\
	.min_interval		= 64,			\
	.max_interval		= 64*num_online_cpus(),	\
	.busy_factor		= 128,			\
	.imbalance_pct		= 133,			\
	.cache_nice_tries	= 1,			\
	.busy_idx		= 3,			\
	.idle_idx		= 3,			\
	.flags			= SD_LOAD_BALANCE	\
				| SD_BALANCE_NEWIDLE	\
				| SD_WAKE_AFFINE	\
				| SD_SERIALIZE,		\
	.last_balance		= jiffies,		\
	.balance_interval	= 64,			\
}

#ifdef CONFIG_NUMA
#ifndef SD_NODE_INIT
#error Please define an appropriate SD_NODE_INIT in include/asm/topology.h!!!
#endif
#endif /* CONFIG_NUMA */

#ifndef topology_physical_package_id
#define topology_physical_package_id(cpu)	((void)(cpu), -1)
#endif
#ifndef topology_core_id
#define topology_core_id(cpu)			((void)(cpu), 0)
#endif
#ifndef topology_thread_siblings
#define topology_thread_siblings(cpu)		cpumask_of_cpu(cpu)
#endif
#ifndef topology_core_siblings
#define topology_core_siblings(cpu)		cpumask_of_cpu(cpu)
#endif

#endif /* _LINUX_TOPOLOGY_H */
