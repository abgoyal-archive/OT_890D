
#ifndef _ASM_POWERPC_OF_PLATFORM_H
#define _ASM_POWERPC_OF_PLATFORM_H

/* Platform drivers register/unregister */
static inline int of_register_platform_driver(struct of_platform_driver *drv)
{
	return of_register_driver(drv, &of_platform_bus_type);
}
static inline void of_unregister_platform_driver(struct of_platform_driver *drv)
{
	of_unregister_driver(drv);
}

/* Platform devices and busses creation */
extern struct of_device *of_platform_device_create(struct device_node *np,
						   const char *bus_id,
						   struct device *parent);
/* pseudo "matches" value to not do deep probe */
#define OF_NO_DEEP_PROBE ((struct of_device_id *)-1)

extern int of_platform_bus_probe(struct device_node *root,
				 const struct of_device_id *matches,
				 struct device *parent);

extern struct of_device *of_find_device_by_phandle(phandle ph);

extern void of_instantiate_rtc(void);

#endif	/* _ASM_POWERPC_OF_PLATFORM_H */
