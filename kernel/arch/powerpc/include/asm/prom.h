
#ifndef _POWERPC_PROM_H
#define _POWERPC_PROM_H
#ifdef __KERNEL__

#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/platform_device.h>
#include <asm/irq.h>
#include <asm/atomic.h>

#define OF_ROOT_NODE_ADDR_CELLS_DEFAULT	1
#define OF_ROOT_NODE_SIZE_CELLS_DEFAULT	1

#define of_compat_cmp(s1, s2, l)	strcasecmp((s1), (s2))
#define of_prop_cmp(s1, s2)		strcmp((s1), (s2))
#define of_node_cmp(s1, s2)		strcasecmp((s1), (s2))

/* Definitions used by the flattened device tree */
#define OF_DT_HEADER		0xd00dfeed	/* marker */
#define OF_DT_BEGIN_NODE	0x1		/* Start of node, full name */
#define OF_DT_END_NODE		0x2		/* End node */
#define OF_DT_PROP		0x3		/* Property: name off, size,
						 * content */
#define OF_DT_NOP		0x4		/* nop */
#define OF_DT_END		0x9

#define OF_DT_VERSION		0x10

struct boot_param_header
{
	u32	magic;			/* magic word OF_DT_HEADER */
	u32	totalsize;		/* total size of DT block */
	u32	off_dt_struct;		/* offset to structure */
	u32	off_dt_strings;		/* offset to strings */
	u32	off_mem_rsvmap;		/* offset to memory reserve map */
	u32	version;		/* format version */
	u32	last_comp_version;	/* last compatible version */
	/* version 2 fields below */
	u32	boot_cpuid_phys;	/* Physical CPU id we're booting on */
	/* version 3 fields below */
	u32	dt_strings_size;	/* size of the DT strings block */
	/* version 17 fields below */
	u32	dt_struct_size;		/* size of the DT structure block */
};



typedef u32 phandle;
typedef u32 ihandle;

struct property {
	char	*name;
	int	length;
	void	*value;
	struct property *next;
};

struct device_node {
	const char *name;
	const char *type;
	phandle	node;
	phandle linux_phandle;
	char	*full_name;

	struct	property *properties;
	struct  property *deadprops; /* removed properties */
	struct	device_node *parent;
	struct	device_node *child;
	struct	device_node *sibling;
	struct	device_node *next;	/* next device of same type */
	struct	device_node *allnext;	/* next in list of all nodes */
	struct  proc_dir_entry *pde;	/* this node's proc directory */
	struct  kref kref;
	unsigned long _flags;
	void	*data;
};

extern struct device_node *of_chosen;

static inline int of_node_check_flag(struct device_node *n, unsigned long flag)
{
	return test_bit(flag, &n->_flags);
}

static inline void of_node_set_flag(struct device_node *n, unsigned long flag)
{
	set_bit(flag, &n->_flags);
}


#define HAVE_ARCH_DEVTREE_FIXUPS

static inline void set_node_proc_entry(struct device_node *dn, struct proc_dir_entry *de)
{
	dn->pde = de;
}


extern struct device_node *of_find_all_nodes(struct device_node *prev);
extern struct device_node *of_node_get(struct device_node *node);
extern void of_node_put(struct device_node *node);

/* For scanning the flat device-tree at boot time */
extern int __init of_scan_flat_dt(int (*it)(unsigned long node,
					    const char *uname, int depth,
					    void *data),
				  void *data);
extern void* __init of_get_flat_dt_prop(unsigned long node, const char *name,
					unsigned long *size);
extern int __init of_flat_dt_is_compatible(unsigned long node, const char *name);
extern unsigned long __init of_get_flat_dt_root(void);

/* For updating the device tree at runtime */
extern void of_attach_node(struct device_node *);
extern void of_detach_node(struct device_node *);

/* Other Prototypes */
extern void finish_device_tree(void);
extern void unflatten_device_tree(void);
extern void early_init_devtree(void *);
extern int machine_is_compatible(const char *compat);
extern void print_properties(struct device_node *node);
extern int prom_n_intr_cells(struct device_node* np);
extern void prom_get_irq_senses(unsigned char *senses, int off, int max);
extern int prom_add_property(struct device_node* np, struct property* prop);
extern int prom_remove_property(struct device_node *np, struct property *prop);
extern int prom_update_property(struct device_node *np,
				struct property *newprop,
				struct property *oldprop);

#ifdef CONFIG_PPC32
struct pci_bus;
struct pci_dev;
extern int pci_device_from_OF_node(struct device_node *node,
				   u8* bus, u8* devfn);
extern struct device_node* pci_busdev_to_OF_node(struct pci_bus *, int);
extern struct device_node* pci_device_to_OF_node(struct pci_dev *);
extern void pci_create_OF_bus_map(void);
#endif

extern struct resource *request_OF_resource(struct device_node* node,
				int index, const char* name_postfix);
extern int release_OF_resource(struct device_node* node, int index);




/* Helper to read a big number; size is in cells (not bytes) */
static inline u64 of_read_number(const u32 *cell, int size)
{
	u64 r = 0;
	while (size--)
		r = (r << 32) | *(cell++);
	return r;
}

/* Like of_read_number, but we want an unsigned long result */
#ifdef CONFIG_PPC32
static inline unsigned long of_read_ulong(const u32 *cell, int size)
{
	return cell[size-1];
}
#else
#define of_read_ulong(cell, size)	of_read_number(cell, size)
#endif

extern u64 of_translate_address(struct device_node *np, const u32 *addr);

/* Translate a DMA address from device space to CPU space */
extern u64 of_translate_dma_address(struct device_node *dev,
				    const u32 *in_addr);

extern const u32 *of_get_address(struct device_node *dev, int index,
			   u64 *size, unsigned int *flags);
#ifdef CONFIG_PCI
extern const u32 *of_get_pci_address(struct device_node *dev, int bar_no,
			       u64 *size, unsigned int *flags);
#else
static inline const u32 *of_get_pci_address(struct device_node *dev,
		int bar_no, u64 *size, unsigned int *flags)
{
	return NULL;
}
#endif /* CONFIG_PCI */

extern int of_address_to_resource(struct device_node *dev, int index,
				  struct resource *r);
#ifdef CONFIG_PCI
extern int of_pci_address_to_resource(struct device_node *dev, int bar,
				      struct resource *r);
#else
static inline int of_pci_address_to_resource(struct device_node *dev, int bar,
		struct resource *r)
{
	return -ENOSYS;
}
#endif /* CONFIG_PCI */

void of_parse_dma_window(struct device_node *dn, const void *dma_window_prop,
		unsigned long *busno, unsigned long *phys, unsigned long *size);

extern void kdump_move_device_tree(void);

/* CPU OF node matching */
struct device_node *of_get_cpu_node(int cpu, unsigned int *thread);

/* cache lookup */
struct device_node *of_find_next_cache_node(struct device_node *np);

/* Get the MAC address */
extern const void *of_get_mac_address(struct device_node *np);



#define OF_MAX_IRQ_SPEC		 4 /* We handle specifiers of at most 4 cells */

struct of_irq {
	struct device_node *controller;	/* Interrupt controller node */
	u32 size;			/* Specifier size */
	u32 specifier[OF_MAX_IRQ_SPEC];	/* Specifier copy */
};

#define OF_IMAP_OLDWORLD_MAC	0x00000001
#define OF_IMAP_NO_PHANDLE	0x00000002

extern void of_irq_map_init(unsigned int flags);


extern int of_irq_map_raw(struct device_node *parent, const u32 *intspec,
			  u32 ointsize, const u32 *addr,
			  struct of_irq *out_irq);


extern int of_irq_map_one(struct device_node *device, int index,
			  struct of_irq *out_irq);

struct pci_dev;
extern int of_irq_map_pci(struct pci_dev *pdev, struct of_irq *out_irq);

extern int of_irq_to_resource(struct device_node *dev, int index,
			struct resource *r);

extern void __iomem *of_iomap(struct device_node *device, int index);

#include <linux/of.h>

#endif /* __KERNEL__ */
#endif /* _POWERPC_PROM_H */
