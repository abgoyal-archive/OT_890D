

#ifndef _ASM_X86_XEN_HYPERVISOR_H
#define _ASM_X86_XEN_HYPERVISOR_H

/* arch/i386/kernel/setup.c */
extern struct shared_info *HYPERVISOR_shared_info;
extern struct start_info *xen_start_info;

enum xen_domain_type {
	XEN_NATIVE,
	XEN_PV_DOMAIN,
	XEN_HVM_DOMAIN,
};

extern enum xen_domain_type xen_domain_type;

#ifdef CONFIG_XEN
#define xen_domain()		(xen_domain_type != XEN_NATIVE)
#else
#define xen_domain()		(0)
#endif

#define xen_pv_domain()		(xen_domain() && xen_domain_type == XEN_PV_DOMAIN)
#define xen_hvm_domain()	(xen_domain() && xen_domain_type == XEN_HVM_DOMAIN)

#define xen_initial_domain()	(xen_pv_domain() && xen_start_info->flags & SIF_INITDOMAIN)

#endif /* _ASM_X86_XEN_HYPERVISOR_H */
